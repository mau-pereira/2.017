/*
  Arduino RedBoard (Uno) + SparkFun Wireless Motor Driver Shield
  + ESC (propeller) + rudder servo

  Pump on Channel B:
    pwm_b  = 6
    dir_b0 = 7
    dir_b1 = 8

  Propeller ESC signal pin: 9
  Rudder servo signal pin: 10

  Serial commands:
    1            -> Pump ON
    2            -> Pump OFF
    P <1000-2000> -> Set ESC microseconds (propeller)
                     Example: P 1500
    R <0-180>     -> Set rudder angle
                     Example: R 90
*/

#include <Servo.h>

// ---------- Pump (Shield Channel B) ----------
const int pwm_b  = 6;
const int dir_b0 = 7;
const int dir_b1 = 8;

// ---------- ESC + Rudder ----------
const int ESC_PIN    = 9;   // propeller ESC signal
const int RUDDER_PIN = 10;  // rudder servo signal

Servo esc;
Servo rudder;

int esc_us = 1000;      // safe startup throttle
int rudder_deg = 90;    // center rudder

void pumpOn() {
  // Fixed direction for pumping out
  digitalWrite(dir_b0, LOW);
  digitalWrite(dir_b1, HIGH);
  analogWrite(pwm_b, 255);   // full power
}

void pumpOff() {
  // Coast/off
  digitalWrite(dir_b0, LOW);
  digitalWrite(dir_b1, LOW);
  analogWrite(pwm_b, 0);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Pump pins
  pinMode(pwm_b, OUTPUT);
  pinMode(dir_b0, OUTPUT);
  pinMode(dir_b1, OUTPUT);
  pumpOff();

  // ESC + rudder
  esc.attach(ESC_PIN, 1000, 2000);
  rudder.attach(RUDDER_PIN);

  esc.writeMicroseconds(esc_us);
  rudder.write(rudder_deg);

  Serial.println("Commands:");
  Serial.println("  1           -> Pump ON");
  Serial.println("  2           -> Pump OFF");
  Serial.println("  P 1500      -> ESC throttle (1000..2000 us)");
  Serial.println("  R 90        -> Rudder angle (0..180 deg)");
}

void loop() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  // Pump shortcuts
  if (line == "1") {
    pumpOn();
    Serial.println("Pump ON");
    return;
  }
  if (line == "2") {
    pumpOff();
    Serial.println("Pump OFF");
    return;
  }

  // Command format: "<letter> <value>"
  char cmd = toupper(line.charAt(0));
  int spaceIdx = line.indexOf(' ');
  if (spaceIdx < 0) {
    Serial.println("Invalid. Use: P <1000-2000> or R <0-180>");
    return;
  }

  int value = line.substring(spaceIdx + 1).toInt();

  if (cmd == 'P') {
    if (value < 1000 || value > 2000) {
      Serial.println("ESC out of range (1000..2000)");
      return;
    }
    esc_us = value;
    esc.writeMicroseconds(esc_us);
    Serial.print("ESC us = ");
    Serial.println(esc_us);
    return;
  }

  if (cmd == 'R') {
    if (value < 0 || value > 180) {
      Serial.println("Rudder out of range (0..180)");
      return;
    }
    rudder_deg = value;
    rudder.write(rudder_deg);
    Serial.print("Rudder deg = ");
    Serial.println(rudder_deg);
    return;
  }

  Serial.println("Unknown command. Use 1, 2, P <value>, R <value>");
}