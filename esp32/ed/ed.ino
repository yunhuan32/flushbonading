//流水灯
#define button 23
#define led1 5
#define led2 18
#define led3 19
#define led4 21

const int ledPins[] = {led1, led2, led3, led4};
const int ledCount = sizeof(ledPins) / sizeof(ledPins[0]);

void setup() {
  for (int i = 0; i < ledCount; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
}

void loop() {
  for (int i = 0; i < ledCount; i++) {
    digitalWrite(ledPins[i], HIGH);
    delay(150);
    digitalWrite(ledPins[i], LOW);
  }
}
