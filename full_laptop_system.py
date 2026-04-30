from pynput import keyboard
# import cv2  # ArUco/camera tracking disabled for serial-only testing
import time
import sys
import os
import threading
import numpy as np
import serial
import serial.tools.list_ports
import struct

# ── Arduino Serial ────────────────────────────────────────────────────────────
SERIAL_PORT_INDEX = 0
SERIAL_BAUD = 115200
SEND_PERIOD_S = 0.2

SPEED = 1620

LATCH_PRESETS = {
    '1': (SPEED, 1650, "RUDDER 1"),
    '2': (SPEED, 1700, "RUDDER 2"),
    '3': (SPEED, 1750, "RUDDER 3"),
    '4': (SPEED, 1800, "RUDDER 4"),
    '5': (SPEED, 1850, "RUDDER 5"),
    'f': (2000, 1500, "FORWARD"),
    'r': (1000, 1500, "REVERSE"),
    'n': (1500, 1500, "NEUTRAL"),
}

ARROW_PRESETS = {
    keyboard.Key.up:    (1650, 1500, "UP"),
    keyboard.Key.down:  (1450, 1500, "DOWN"),
    keyboard.Key.right: (1600, 1700, "RIGHT"),
    keyboard.Key.left:  (1600, 1300, "LEFT"),
}

IDLE = (1500, 1500, "IDLE")

current_state = IDLE
latched_state = IDLE
serial_conn = None
pump_on = False
state_lock = threading.Lock()

# ── Camera / ArUco config (DISABLED for serial-only testing) ────────────────
# CAMERA_INDEX = 1
# TRACK_TAG_ID = None  # None = track first detected tag; set to int for a specific ID

# ── Recording ────────────────────────────────────────────────────────────────
RAWDATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "rawdata")
recording = False
record_t0 = 0.0  # set via time.perf_counter() when recording starts
current_rot = None
record_data = []
record_lock = threading.Lock()

# x/y kept only as placeholders so logging/recording paths stay simple without tracker.
x = np.nan
y = np.nan


# ── ArUco tracker thread (DISABLED for serial-only testing) ─────────────────
# def tracker_loop():
#     """Background thread: reads camera, detects ArUco markers, updates x/y."""
#     ...


# ── Recorder thread ─────────────────────────────────────────────────────────
def recorder_loop():
    """Background thread that samples data at 30 Hz while recording."""
    while True:
        time.sleep(1 / 30)
        with record_lock:
            if recording:
                prop, rud, _ = current_state
                cur_x, cur_y = x, y
                record_data.append([round(time.perf_counter() - record_t0, 3),
                                    cur_x, cur_y, prop, rud])


# ── Serial / command helpers ─────────────────────────────────────────────────
def resolve_serial_port(index):
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        raise RuntimeError("No serial ports found.")
    if index < 0 or index >= len(ports):
        raise RuntimeError(f"Serial port index {index} out of range. Found {len(ports)} port(s).")
    return ports[index].device


def send_serial_vector(propeller, rudder, waterpump):
    payload = struct.pack('<fff', float(propeller), float(rudder), float(waterpump))
    try:
        serial_conn.write(payload)
    except Exception as e:
        print(f"[SERIAL ERROR] {e}")


def serial_sender_loop():
    while True:
        time.sleep(SEND_PERIOD_S)
        with state_lock:
            prop, rud, _ = current_state
            pump = 1 if pump_on else 0
        send_serial_vector(prop, rud, pump)


def apply_state(state):
    global current_state
    if state != current_state:
        with state_lock:
            current_state = state
            prop, rud, label = state
            pump = 1 if pump_on else 0
        print(f"[{time.time():.3f}] propeller={prop}  rudder={rud}  "
              f"pump={pump}  ({label})")


# ── Keyboard handlers ───────────────────────────────────────────────────────
def on_press(key):
    global latched_state, recording, record_t0, record_data, current_rot, pump_on

    try:
        ch = key.char
    except AttributeError:
        ch = None

    if ch == 's':
        with record_lock:
            if recording:
                print("Already recording!")
                return
            if current_rot is None:
                print("Select a rotation first (1-5)!")
                return
            record_data = []
            record_t0 = time.perf_counter()
            recording = True
        print(f"[0.000] >>> RECORDING STARTED (rot{current_rot})")
        return

    if ch == 'e':
        with record_lock:
            if not recording:
                print("Not recording.")
                return
            recording = False
            trial = get_next_trial(current_rot)
            fname = os.path.join(RAWDATA_DIR, f"rot{current_rot}_trial{trial}.csv")
            data = np.array(record_data)
            np.savetxt(fname, data, delimiter=',',
                       header='timestamp,x,y,pwm_prop,pwm_rudder', comments='')
            n = len(record_data)
            record_data = []
        print(f"[{time.time():.3f}] >>> SAVED {n} samples -> {fname}")
        return

    if ch == 'x':
        with record_lock:
            if not recording:
                print("Not recording.")
                return
            recording = False
            n = len(record_data)
            record_data = []
        print(f"[{time.time():.3f}] >>> DISCARDED {n} samples")
        return

    if ch and ch in LATCH_PRESETS:
        if ch in '12345':
            current_rot = int(ch)
        latched_state = LATCH_PRESETS[ch]
        apply_state(latched_state)
        return
    if ch == 'w':
        with state_lock:
            pump_on = True
            prop, rud, label = current_state
            pump = 1
        print(f"[{time.time():.3f}] propeller={prop}  rudder={rud}  "
              f"pump={pump}  ({label})")
        return

    state = ARROW_PRESETS.get(key)
    if state:
        latched_state = IDLE
        apply_state(state)


def on_release(key):
    global pump_on
    try:
        ch = key.char
    except AttributeError:
        ch = None

    if key == keyboard.Key.esc:
        with state_lock:
            pump_on = False
        send_serial_vector(1500, 1500, 0)
        print("Sending IDLE vector and exiting...")
        return False
    if ch == 'w':
        with state_lock:
            pump_on = False
            prop, rud, label = current_state
            pump = 0
        print(f"[{time.time():.3f}] propeller={prop}  rudder={rud}  "
              f"pump={pump}  ({label})")
        return
    if key in ARROW_PRESETS:
        apply_state(latched_state)


# ── Utility ──────────────────────────────────────────────────────────────────
def get_next_trial(rot_num):
    os.makedirs(RAWDATA_DIR, exist_ok=True)
    trial = 1
    while os.path.exists(os.path.join(RAWDATA_DIR, f"rot{rot_num}_trial{trial}.csv")):
        trial += 1
    return trial


# ── Main ─────────────────────────────────────────────────────────────────────
def main():
    global serial_conn

    print("Resolving serial port ...")

    try:
        port_name = resolve_serial_port(SERIAL_PORT_INDEX)
        serial_conn = serial.Serial(port_name, SERIAL_BAUD, timeout=0)
    except Exception as e:
        print(f"Could not open serial port: {e}")
        sys.exit(1)

    print(f"Connected on {port_name} @ {SERIAL_BAUD} baud\n")
    print("=== Boat Remote (Arduino Serial Test Mode) ===")
    print(f"Speed (propeller) fixed at {SPEED} for number keys")
    print("Arrow keys: UP/DOWN = throttle, LEFT/RIGHT = steer")
    print("Number keys 1-5: preset rudder angles [1650..1850] (latched) + set rotation")
    print("F = forward (2000), R = reverse (1000), N = neutral (1500) (latched)")
    print("W (hold) = water pump ON, release W = OFF")
    print("S = start recording, E = save recording, X = discard recording")
    print(f"Data saved to: {RAWDATA_DIR}")
    print(f"Serial vector format: [propeller_pwm, rudder_pwm, waterpump] as 3 float32 every {SEND_PERIOD_S}s")
    print("ESC to quit\n")

    # threading.Thread(target=tracker_loop, daemon=True).start()  # ArUco disabled
    threading.Thread(target=recorder_loop, daemon=True).start()
    threading.Thread(target=serial_sender_loop, daemon=True).start()

    try:
        with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
            listener.join()
    finally:
        if serial_conn and serial_conn.is_open:
            serial_conn.close()
        print("Serial connection closed.")


if __name__ == "__main__":
    main()
