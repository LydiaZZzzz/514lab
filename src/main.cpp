#include <Arduino.h>

const int vout1Pin = A0;
const int vout2Pin = A1;

const float refVoltage = 3.3;

void setup() {
  Serial.begin(115200);
  Serial.println("Voltage Measurement Initialized");
}

void loop() {
  int vout1Value = analogRead(vout1Pin);
  int vout2Value = analogRead(vout2Pin);

  float vout1Voltage = (vout1Value / 4096.0) * refVoltage;
  float vout2Voltage = (vout2Value / 4096.0) * refVoltage;

  Serial.print("VOUT1: ");
  Serial.print(vout1Value);
  Serial.print(" (Raw), ");
  Serial.print(vout1Voltage);
  Serial.println(" V");

  Serial.print("VOUT2: ");
  Serial.print(vout2Value);
  Serial.print(" (Raw), ");
  Serial.print(vout2Voltage);
  Serial.println(" V");

  delay(1000);
}