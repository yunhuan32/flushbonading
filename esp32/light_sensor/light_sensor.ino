#include <Arduino.h>
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(32, ADC_11db);

}

void loop() {
  // put your main code here, to run repeatedly:
  
  int lightValue = analogRead(32);
  Serial.println(lightValue);
  delay(100);
}
