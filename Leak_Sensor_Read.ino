const int leakSensor1 = 8; // pin number for leak sensor 1


void setup() {
  // put your setup code here, to run once:
  pinMode(leakSensor1, INPUT); // set leak sensor 1 to be a an input
  Serial.begin(9600); // begin serial reading for output display
  Serial.println(digitalRead(leakSensor1)); 
}
void loop() {
  // put your main code here, to run repeatedly:
  int sensorValue = digitalRead(leakSensor1); 
  Serial.print("The sensor reads: "); 
  Serial.println(sensorValue); // 0 = dry, 1 = wet 
}
