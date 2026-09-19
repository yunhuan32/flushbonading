#include <BluetoothSerial.h>

// LED
const int ledPin = 5;

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  if (SerialBT.begin("ESP32_LED")) {
    Serial.println("BT started, waiting for connection...");
  } else {
    Serial.println("BT init failed!");
  }
}

void loop() {
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "on") {
      digitalWrite(ledPin, HIGH);
      SerialBT.println("LED ON");
      SerialBT.println("852403532游铠嘉");
    } else if (cmd == "off") {
      digitalWrite(ledPin, LOW);
      SerialBT.println("LED OFF");
      Serial.println("OFF");
    } else {
      SerialBT.println("unknown, send on/off");
    }
  }
  delay(50);
}
