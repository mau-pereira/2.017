#include <Servo.h>

Servo esc;

const int ESC_PIN = 9;    // 
int throttle_us = 1500;   // neutral by standard, 2000 full forward, 1000 full backward

void setup() {
  Serial.begin(115200);
  esc.attach(ESC_PIN, 1000, 2000);
  esc.writeMicroseconds(throttle_us);

  Serial.println("Calibration commands (follow pdf instructions):");
  Serial.println("n = neutral (1500 us)");
  Serial.println("f = full throttle (2000 us)");
  Serial.println("r = full reverse (1000 us)");
}

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();

    if (c == 'n') {
      throttle_us = 1500;
      Serial.println("Neutral: 1500 us");
    }
    else if (c == 'f') {
      throttle_us = 2000;
      Serial.println("Full throttle: 2000 us");
    }
    else if (c == 'r') {
      throttle_us = 1000;
      Serial.println("Full reverse: 1000 us");
    }

    esc.writeMicroseconds(throttle_us);
  }

  esc.writeMicroseconds(throttle_us);
  delay(20);
}