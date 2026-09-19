#include <Arduino.h>

// ========== 引脚定义（按实际接线修改） ==========
// 左电机
#define LEFT_EN   26   // 使能/PWM（需接 PWM 引脚）
#define LEFT_IN1  25
#define LEFT_IN2  33

// 右电机
#define RIGHT_EN  14   // 使能/PWM（需接 PWM 引脚）
#define RIGHT_IN1 27
#define RIGHT_IN2 12

// ========== PWM 参数 ==========
const int pwmFreq = 5000;    // PWM 频率 5kHz
const int pwmResolution = 8; // 8 位分辨率（0-255）
const int pwmChannelL = 0;
const int pwmChannelR = 1;

void motorLeft(int speed) {
  // speed: -255 ~ 255，正数前进，负数后退
  if (speed > 0) {
    digitalWrite(LEFT_IN1, HIGH);
    digitalWrite(LEFT_IN2, LOW);
    ledcWrite(pwmChannelL, speed);
  } else if (speed < 0) {
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, HIGH);
    ledcWrite(pwmChannelL, -speed);
  } else {
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, LOW);
    ledcWrite(pwmChannelL, 0);
  }
}

void motorRight(int speed) {
  if (speed > 0) {
    digitalWrite(RIGHT_IN1, HIGH);
    digitalWrite(RIGHT_IN2, LOW);
    ledcWrite(pwmChannelR, speed);
  } else if (speed < 0) {
    digitalWrite(RIGHT_IN1, LOW);
    digitalWrite(RIGHT_IN2, HIGH);
    ledcWrite(pwmChannelR, -speed);
  } else {
    digitalWrite(RIGHT_IN1, LOW);
    digitalWrite(RIGHT_IN2, LOW);
    ledcWrite(pwmChannelR, 0);
  }
}

// ========== 小车动作 ==========
void carForward(int speed = 200)  { motorLeft(speed);  motorRight(speed);  }
void carBackward(int speed = 200) { motorLeft(-speed); motorRight(-speed); }
void carTurnLeft(int speed = 200) { motorLeft(-speed); motorRight(speed);  }
void carTurnRight(int speed = 200){ motorLeft(speed);  motorRight(-speed); }
void carStop()                    { motorLeft(0);      motorRight(0);      }

void setup() {
  Serial.begin(115200);

  // GPIO 初始化
  pinMode(LEFT_IN1, OUTPUT);  pinMode(LEFT_IN2, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT); pinMode(RIGHT_IN2, OUTPUT);

  // PWM 通道绑定
  ledcSetup(pwmChannelL, pwmFreq, pwmResolution);
  ledcAttachPin(LEFT_EN, pwmChannelL);
  ledcSetup(pwmChannelR, pwmFreq, pwmResolution);
  ledcAttachPin(RIGHT_EN, pwmChannelR);

  carStop();
  Serial.println("Motor driver ready.");
  Serial.println("Commands: w=forward  s=backward  a=left  d=right  space=stop");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'w': carForward();   Serial.println("forward");  break;
      case 's': carBackward();  Serial.println("backward"); break;
      case 'a': carTurnLeft();  Serial.println("left");     break;
      case 'd': carTurnRight(); Serial.println("right");    break;
      case ' ': carStop();      Serial.println("stop");     break;
    }
  }
  delay(20);
}
