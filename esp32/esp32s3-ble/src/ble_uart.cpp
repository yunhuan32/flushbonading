#include "ble_uart.h"
#include "config.h"
#include "led_ctrl.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Nordic UART Service 的两个特征 UUID（业界通用，手机 APP 都认这套）
#define NUS_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // 手机 -> ESP32（写入）
#define NUS_TX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP32 -> 手机（通知）

static BLEServer*         s_server    = nullptr;
static BLECharacteristic* s_txChar    = nullptr;
static volatile bool      s_connected = false;

// ---------- 收到手机命令 ----------
static void handleCommand(const String& cmd) {
  Serial.printf("[BLE] 收到命令: %s\n", cmd.c_str());

  if (cmd == "ON") {
    ledSet(true);
    Serial.println("[LED] 已开灯");
    bleNotifyText("OK LED=ON");
  } else if (cmd == "OFF") {
    ledSet(false);
    Serial.println("[LED] 已关灯");
    bleNotifyText("OK LED=OFF");
  } else if (cmd == "ID") {
    bleNotifyId();
  } else {
    bleNotifyText("ERR 可用命令: ON / OFF / ID");
  }
}

// ---------- 手机写入 RX 特征时触发 ----------
class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String v = c->getValue().c_str();
    v.trim();
    v.toUpperCase();
    if (v.isEmpty()) return;
    handleCommand(v);
  }
};

// ---------- 连接 / 断开 ----------
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    s_connected = true;
    Serial.println("[BLE] 手机已连接");
  }

  void onDisconnect(BLEServer*) override {
    s_connected = false;
    Serial.println("[BLE] 手机已断开，重新开始广播，等待下次连接");
    // 不重新广播的话，手机断开后就再也搜不到这台设备了
    BLEDevice::startAdvertising();
  }
};

void bleInit() {
  BLEDevice::init(BLE_DEVICE_NAME);

  s_server = BLEDevice::createServer();
  s_server->setCallbacks(new ServerCallbacks());

  BLEService* svc = s_server->createService(NUS_SERVICE_UUID);

  // TX：ESP32 发给手机。同时给 READ 属性，这样手机不订阅通知也能"读"到学号
  s_txChar = svc->createCharacteristic(
      NUS_TX_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  s_txChar->addDescriptor(new BLE2902());  // 通知功能必需
  s_txChar->setValue((String("STUDENT_ID=") + STUDENT_ID).c_str());

  // RX：手机写给 ESP32
  BLECharacteristic* rxChar = svc->createCharacteristic(
      NUS_RX_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rxChar->setCallbacks(new RxCallbacks());

  svc->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE_UUID);
  adv->setScanResponse(true);  // 设备名放在扫描响应包里，避免广播包放不下
  adv->setMinPreferred(0x06);
  adv->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.printf("[BLE] 已开始广播，设备名: %s\n", BLE_DEVICE_NAME);
}

bool bleConnected() {
  return s_connected;
}

void bleNotifyText(const String& s) {
  if (!s_txChar) return;
  s_txChar->setValue(s.c_str());
  if (s_connected) {
    s_txChar->notify();
  }
  Serial.printf("[BLE] 发送: %s\n", s.c_str());
}

void bleNotifyId() {
  bleNotifyText(String("STUDENT_ID=") + STUDENT_ID);
}
