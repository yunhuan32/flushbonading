#include <BluetoothSerial.h>   // 引入蓝牙串口库

const int motorPin      = 19;   // 电机 PWM 输出引脚，接驱动板 PWMA
const int ain1          = 5;    // 电机方向控制脚 AIN1，接驱动板 AIN1
const int ain2          = 18;   // 电机方向控制脚 AIN2，接驱动板 AIN2
const int pwmFreq       = 5000; // PWM 频率设为 5kHz
const int pwmResolution = 8;    // PWM 分辨率设为 8 位，占空比范围 0~255

int  currentDuty  = 255; // 当前 PWM 占空比，默认 255 表示全速
bool motorRunning = true; // 电机当前运行状态，默认运行

BluetoothSerial SerialBT; // 创建蓝牙串口对象

bool _isNumeric(const String& s) { // 判断字符串是否全部由数字组成
  if (s.length() == 0) return false; // 空字符串不是数字
  for (size_t i = 0; i < s.length(); i++) { // 遍历字符串中的每一个字符
    if (!isDigit(s[i])) return false; // 遇到非数字字符则返回 false
  }
  return true; // 所有字符都是数字，返回 true
}

void motorCtrl() // 读取并处理一条蓝牙指令
{
  if (!SerialBT.available()) { // 如果蓝牙串口没有收到数据
    delay(10); // 等待 10 毫秒
    return; // 本次处理结束
  }

  String input = SerialBT.readStringUntil('\n'); // 读取一行以换行符结尾的指令
  input.trim(); // 删除指令前后的空格和换行符
  if (input.length() == 0) return; // 忽略空指令

  if (input.equalsIgnoreCase("on")) { // 收到 on 指令
    currentDuty = 255; // 设置全速占空比
    motorRunning = true; // 标记电机正在运行
    ledcWrite(motorPin, currentDuty); // 输出 PWM，让电机全速转动
    SerialBT.println("Motor ON  duty=" + String(currentDuty)); // 回复当前运行状态

  } else if (input.equalsIgnoreCase("off")) { // 收到 off 指令
    motorRunning = false; // 标记电机停止
    ledcWrite(motorPin, 0); // 输出占空比 0，让电机停止
    SerialBT.println("Motor OFF"); // 回复停止状态

  } else if (_isNumeric(input)) { // 收到纯数字指令
    int value = constrain(input.toInt(), 0, 255); // 把输入数值限制在 0~255
    currentDuty = value; // 保存新的占空比
    motorRunning = (value > 0); // 占空比大于 0 时视为运行
    ledcWrite(motorPin, value); // 按输入数值设置 PWM 占空比
    SerialBT.println("PWM = " + String(value)); // 回复当前占空比

  } else { // 其他无法识别的指令
    SerialBT.println("? " + input); // 回显错误提示
  }
}

void setup() { // 初始化函数
  Serial.begin(115200); // 初始化 USB 串口，波特率 115200
  pinMode(ain1, OUTPUT); // 设置 AIN1 为输出模式
  pinMode(ain2, OUTPUT); // 设置 AIN2 为输出模式
  digitalWrite(ain1, HIGH); // AIN1 输出高电平
  digitalWrite(ain2, LOW); // AIN2 输出低电平，组合为电机正向旋转方向
  ledcAttach(motorPin, pwmFreq, pwmResolution); // 将电机引脚绑定为 PWM 输出
  ledcWrite(motorPin, currentDuty); // 输出默认占空比，让电机立即转动
  SerialBT.begin("ESP32_Motor"); // 启动蓝牙串口，设备名称为 ESP32_Motor
}

void loop() { // 主循环函数
  motorCtrl(); // 反复检查并处理蓝牙指令
}
