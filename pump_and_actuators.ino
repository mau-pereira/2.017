/*
  Demo of working actuation (propeller and rudder) with water pump.


  Arduino RedBoard (Uno) + SparkFun Wireless Motor Driver Shield
  + ESC (propeller) + rudder servo

  Pump on Channel B:
    pwm_b  = 6
    dir_b0 = 7
    dir_b1 = 8

  Propeller ESC signal pin: 9
  Rudder servo signal pin: 10

  Serial commands:
    1              -> Pump ON
    2              -> Pump OFF
    n/f/r          -> ESC neutral/forward/reverse
    P <1000-2000>  -> Set ESC microseconds (propeller)
                       Example: P 1500
    R <1000-2000>  -> Set rudder microseconds
                       Example: R 1700
*/

#include <Servo.h>

// ---------- Pump (Shield Channel B) ----------
const int pwm_b  = 6;
const int dir_b0 = 7;
const int dir_b1 = 8;

// ---------- ESC + Rudder ----------
const int ESC_PIN    = 10;   // propeller ESC signal
const int RUDDER_PIN = 9;  // rudder servo signal

Servo esc;
Servo rudder;

int esc_us = 1500;      // neutral startup throttle (matches ESC calibration)
int rudder_us = 1500;   // center rudder (microseconds)

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
  rudder.attach(RUDDER_PIN, 1000, 2000);

  esc.writeMicroseconds(esc_us);
  rudder.writeMicroseconds(rudder_us);

  Serial.println("Commands:");
  Serial.println("  1           -> Pump ON");
  Serial.println("  2           -> Pump OFF");
  Serial.println("  n           -> ESC neutral (1500 us)");
  Serial.println("  f           -> ESC full forward (2000 us)");
  Serial.println("  r           -> ESC full reverse (1000 us)");
  Serial.println("  P 1500      -> ESC throttle (1000..2000 us)");
  Serial.println("  R 1700      -> Rudder pulse (1000..2000 us)");
}

void loop() {
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      esc.writeMicroseconds(esc_us);
      delay(20);
      return;
    }

    // Pump shortcuts
    if (line == "1") {
      pumpOn();
      Serial.println("Pump ON");
      esc.writeMicroseconds(esc_us);
      delay(20);
      return;
    }
    if (line == "2") {
      pumpOff();
      Serial.println("Pump OFF");
      esc.writeMicroseconds(esc_us);
      delay(20);
      return;
    }

    // ESC calibration-style single-letter shortcuts
    char first = tolower(line.charAt(0));
    if (line.length() == 1) {
      if (first == 'n') {
        esc_us = 1500;
        Serial.println("Neutral: 1500 us");
      } else if (first == 'f') {
        esc_us = 2000;
        Serial.println("Full throttle: 2000 us");
      } else if (first == 'r') {
        esc_us = 1000;
        Serial.println("Full reverse: 1000 us");
      } else {
        Serial.println("Unknown command. Use 1, 2, n, f, r, P <value>, R <value>");
      }
      esc.writeMicroseconds(esc_us);
      delay(20);
      return;
    }

    // Command format: "<letter> <value>"
    char cmd = toupper(line.charAt(0));
    int spaceIdx = line.indexOf(' ');
    if (spaceIdx < 0) {
      Serial.println("Invalid. Use: n, f, r, P <1000-2000>, or R <1000-2000>");
      esc.writeMicroseconds(esc_us);
      delay(20);
      return;
    }

    int value = line.substring(spaceIdx + 1).toInt();

    if (cmd == 'P') {
      if (value < 1000 || value > 2000) {
        Serial.println("ESC out of range (1000..2000)");
        esc.writeMicroseconds(esc_us);
        delay(20);
        return;
      }
      esc_us = value;
      Serial.print("ESC us = ");
      Serial.println(esc_us);
      esc.writeMicroseconds(esc_us);
      delay(20);
      return;
    }

    if (cmd == 'R') {
      if (value < 1000 || value > 2000) {
        Serial.println("Rudder out of range (1000..2000 us)");
        esc.writeMicroseconds(esc_us);
        delay(20);
        return;
      }
      rudder_us = value;
      rudder.writeMicroseconds(rudder_us);
      Serial.print("Rudder us = ");
      Serial.println(rudder_us);
      esc.writeMicroseconds(esc_us);
      delay(20);
      return;
    }

    Serial.println("Unknown command. Use 1, 2, n, f, r, P <value>, R <value>");
  }

  // Keep ESC command refreshed at servo frame rate.
  esc.writeMicroseconds(esc_us);
  delay(20);
}
