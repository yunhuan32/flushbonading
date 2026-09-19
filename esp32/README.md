## 目录

1. [GPIO 引脚配置](#1-gpio-引脚配置)
2. [硬件定时器（hw_timer_t）](#2-硬件定时器hw_timer_t)
3. [PWM —— LEDC 外设](#3-pwm--ledc-外设)
4. [DMA —— I2S 并行模式驱动 GPIO](#4-dma--i2s-并行模式驱动-gpio)

---

## 1. GPIO 引脚配置

### 1.1 引脚速查

ESP32 的 GPIO 并非全部等价，不同引脚在电气特性与功能复用上存在差异：

| 引脚 | 可用模式 | 注意事项 |
|---|---|---|
| GPIO 0, 2, 4, 12–15, 25–27, 32–33 | 全部（输入/输出/上下拉/中断） | GPIO 0 为 Strapping Pin，上电电平影响启动模式 |
| GPIO 34, 35, 36, 39 | 仅输入，无内部上下拉 | RTC_IO，适合接外部传感器或按键（需外接上下拉电阻） |
| GPIO 1, 3 | UART0 默认占用（TX/RX） | 烧录串口，谨慎复用 |
| GPIO 6–11 | 连接板载 SPI Flash | **不可用作通用 GPIO** |

### 1.2 `pinMode()` —— Arduino 标准接口

这是最接近 Arduino 原生的 API，ESP32 在此基础上做了开漏输出的扩展：

```cpp
pinMode(2, OUTPUT);              // 推挽输出
pinMode(2, INPUT);               // 高阻输入
pinMode(2, INPUT_PULLUP);        // 内部上拉（~45 kΩ）
pinMode(2, INPUT_PULLDOWN);      // 内部下拉（~45 kΩ）
pinMode(2, OUTPUT_OPEN_DRAIN);   // 开漏输出 —— 只能拉低，需外部上拉才能输出高电平
```

> **注意：** GPIO 34–39 不支持内部上下拉，强行配置 `INPUT_PULLUP` / `INPUT_PULLDOWN` 不生效。

读写引脚：

```cpp
digitalWrite(2, HIGH);
digitalWrite(2, LOW);
int val = digitalRead(4);
```

### 1.3 GPIO34–39（纯输入引脚）的使用

这四个引脚只能配置为输入，且没有内部上/下拉电阻。典型用法是接外部按键或传感器：

```cpp
void setup() {
  Serial.begin(115200);
  pinMode(34, INPUT);   // 只能 INPUT
}

void loop() {
  int val = digitalRead(34);
  Serial.println(val);
  delay(100);
}
```

若按键接 GPIO34 → GND（按下为 LOW），需在 GPIO34 与 3.3 V 之间串接 10 kΩ 上拉电阻，或使用外部下拉电阻配合 `INPUT` 模式。

### 1.4 `gpio_config_t` —— ESP-IDF 原生接口

当 `pinMode()` 无法满足需求时（如同时配置多引脚、指定驱动强度、禁用中断、引脚保持），使用 ESP-IDF 原生的 `gpio_config_t`：

```cpp
#include <driver/gpio.h>

gpio_config_t ioConf = {
  .pin_bit_mask = (1ULL << 2),          // 位掩码：可同时配置多引脚
  .mode         = GPIO_MODE_OUTPUT,     // 见下方模式表
  .pull_up_en   = GPIO_PULLUP_DISABLE,
  .pull_down_en = GPIO_PULLDOWN_DISABLE,
  .intr_type    = GPIO_INTR_DISABLE
};
gpio_config(&ioConf);

gpio_set_level(GPIO_NUM_2, 1);   // HIGH
gpio_set_level(GPIO_NUM_2, 0);   // LOW
int val = gpio_get_level(GPIO_NUM_4);
```

**`gpio_mode_t` 全部取值：**

| 模式 | `pinMode()` 等效 | 说明 |
|---|---|---|
| `GPIO_MODE_DISABLE` | — | 引脚禁用（输入/输出均断开） |
| `GPIO_MODE_INPUT` | `INPUT` | 纯输入 |
| `GPIO_MODE_OUTPUT` | `OUTPUT` | 推挽输出 |
| `GPIO_MODE_OUTPUT_OD` | `OUTPUT_OPEN_DRAIN` | 开漏输出（只能拉低） |
| `GPIO_MODE_INPUT_OUTPUT_OD` | — | 输入 + 开漏输出（如 I2C） |
| `GPIO_MODE_INPUT_OUTPUT` | — | 双向 |

### 1.5 开漏输出（Open-Drain）

开漏输出只能主动拉低线路（吸收电流），无法输出高电平——高电平由外部上拉电阻提供。适合 I2C 总线、多设备共享信号线、电平转换等场景：

```cpp
gpio_config_t ioConf = {
  .pin_bit_mask = (1ULL << 2),
  .mode         = GPIO_MODE_OUTPUT_OD,
  .pull_up_en   = GPIO_PULLUP_ENABLE,   // 开启内部上拉
  .pull_down_en = GPIO_PULLDOWN_DISABLE,
  .intr_type    = GPIO_INTR_DISABLE
};
gpio_config(&ioConf);

gpio_set_level(GPIO_NUM_2, 0);   // 拉低（吸收电流）
gpio_set_level(GPIO_NUM_2, 1);   // 高阻态 —— 外部/内部上拉将总线拉高
```

### 1.6 引脚保持（Pin Hold）与深睡眠

深睡眠期间 GPIO 通常会被复位。若希望某引脚保持当前电平（如对一个常供电的外设维持使能信号），使用 hold 功能：

```cpp
#include <driver/gpio.h>
#include <driver/rtc_io.h>

gpio_config_t conf = {
  .pin_bit_mask = (1ULL << 2),
  .mode         = GPIO_MODE_OUTPUT,
  .pull_up_en   = GPIO_PULLDOWN_DISABLE,
  .pull_down_en = GPIO_PULLDOWN_DISABLE,
  .intr_type    = GPIO_INTR_DISABLE
};
gpio_config(&conf);
gpio_set_level(GPIO_NUM_2, 1);

gpio_hold_en(GPIO_NUM_2);        // 使能 hold
gpio_deep_sleep_hold_en();       // 深睡眠期间 hold 生效
esp_deep_sleep_start();
```

### 1.7 外部中断

ESP32 支持在任意 GPIO 上配置中断，触发类型涵盖边沿和电平：

```cpp
volatile bool btnPressed = false;

void IRAM_ATTR btnISR() {
  btnPressed = true;   // ISR 体中仅置标志，不做耗时操作
}

void setup() {
  Serial.begin(115200);

  // 方式 A: Arduino API
  pinMode(4, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(4), btnISR, FALLING);

  // 方式 B: ESP-IDF 原生（可指定 CPU 核心）
  gpio_config_t conf = {
    .pin_bit_mask = (1ULL << 4),
    .mode         = GPIO_MODE_INPUT,
    .pull_up_en   = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_NEGEDGE
  };
  gpio_config(&conf);
  gpio_install_isr_service(0);             // ESP_INTR_FLAG_DEFAULT
  gpio_isr_handler_add(GPIO_NUM_4, btnISR, NULL);
}

void loop() {
  if (btnPressed) {
    btnPressed = false;
    Serial.println("Button pressed!");
  }
  delay(10);
}
```

**中断触发类型：**

| `gpio_int_type_t` | 触发条件 |
|---|---|
| `GPIO_INTR_DISABLE` | 禁止中断 |
| `GPIO_INTR_POSEDGE` | 上升沿 |
| `GPIO_INTR_NEGEDGE` | 下降沿 |
| `GPIO_INTR_ANYEDGE` | 双边沿 |
| `GPIO_INTR_LOW_LEVEL` | 低电平 |
| `GPIO_INTR_HIGH_LEVEL` | 高电平 |

### 1.8 配置方式选型

| 场景 | 推荐方式 |
|---|---|
| 简单 LED 开关、读按键 | `pinMode()` + `digitalWrite/Read` |
| 同时配置多引脚、精细模式 | `gpio_config_t` |
| 开漏输出（I2C、共享总线） | `GPIO_MODE_OUTPUT_OD` |
| 深睡眠保持引脚电平 | `gpio_hold_en` + `gpio_deep_sleep_hold_en` |
| 外部中断 | `attachInterrupt` 或 `gpio_isr_handler_add` |
| GPIO 34–39 | 仅 `INPUT`，**外接**上下拉电阻 |

---

## 2. 硬件定时器（hw_timer_t）

### 2.1 概述

ESP32 内置 4 个 64 位硬件定时器组（timer 0–3），每个定时器配备 16 位预分频器与报警比较器。与 Arduino 的 `millis()` / `delay()`（基于软件定时器）不同，硬件定时器直接由 APB 时钟（80 MHz）驱动，具备微秒级精度且不受中断屏蔽影响。

### 2.2 基本用法：定时中断

以下示例每 500 ms 翻转一次 LED（GPIO 2）：

```cpp
hw_timer_t *timer = NULL;
volatile bool ledState = false;

// ISR 必须标注 IRAM_ATTR，确保代码驻留在 IRAM 中
void IRAM_ATTR onTimer() {
  ledState = !ledState;
  digitalWrite(2, ledState);
}

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);

  // timer 0, 预分频 80 → 1 tick = 1 µs
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 500000, true);   // 500,000 µs = 500 ms，自动重载
  timerAlarmEnable(timer);
}

void loop() {
  delay(1000);
  Serial.println("main loop running...");
}
```

**API 说明：**

| 函数 | 参数 | 说明 |
|---|---|---|
| `timerBegin(id, prescaler, countUp)` | `id`: 定时器编号 (0–3)<br>`prescaler`: 时钟分频<br>`countUp`: `true` 向上计数 | APB 时钟 80 MHz 经分频得到 tick 时钟；`countUp = true` 时计数器从 0 开始递增 |
| `timerAttachInterrupt(timer, fn, edge)` | `fn`: ISR 函数<br>`edge`: `true` = 边沿触发 | 绑定 ISR；`fn` 必须带 `IRAM_ATTR` |
| `timerAlarmWrite(timer, val, reload)` | `val`: 报警阈值 (tick)<br>`reload`: `true` = 周期性 | 计数器到达 `val` 时触发报警 |
| `timerAlarmEnable(timer)` | — | 启动报警 |

### 2.3 轮询方式（不使用 ISR）

若不需要中断的实时性，可以在 `loop()` 中轮询定时器计数值：

```cpp
hw_timer_t *timer = timerBegin(0, 80, true);

void setup() {
  Serial.begin(115200);
  timerAlarmWrite(timer, 1000000, true);   // 1 秒
  timerAlarmEnable(timer);
}

void loop() {
  if (timerAlarmRead(timer) >= 1000000) {
    timerWrite(timer, 0);     // 复位计数器
    timerAlarm(timer);        // 手动触发报警
    Serial.println("1s tick (polled)");
  }
}
```

### 2.4 注意事项

- ISR 必须标注 `IRAM_ATTR`，否则触发中断时会因 Flash Cache 不可用而 panic。
- ISR 体内不得调用 `delay()`、`Serial.print()` 等阻塞函数，也不得调用 FreeRTOS 可能阻塞的 API。复杂逻辑通过置标志位交由 `loop()` 处理。
- 定时器 0 和 1 属于 Timer Group 0，定时器 2 和 3 属于 Timer Group 1，两组硬件完全独立。

---

## 3. PWM —— LEDC 外设

### 3.1 概述

ESP32 的 LEDC（LED PWM Controller）提供 8 个高速通道和 8 个低速通道，支持任意频率和最高 16 位占空比精度。Arduino 的 `analogWrite()` 使用软件 PWM，频率固定约 1 kHz、精度仅 8 位，不适合需要精确波形控制的场景。

### 3.2 基本用法：呼吸灯

以下示例在 GPIO 2 上输出 5 kHz、8 位分辨率的 PWM，实现呼吸灯效果：

```cpp
#include <BluetoothSerial.h>
BluetoothSerial SerialBT;

const int pwmPin  = 2;
const int pwmCh   = 0;
const int freq    = 5000;
const int resBits = 8;          // 占空比范围: 0–255

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_LED");

  ledcSetup(pwmCh, freq, resBits);
  ledcAttachPin(pwmPin, pwmCh);
}

void loop() {
  for (int duty = 0; duty <= 255; duty++) {
    ledcWrite(pwmCh, duty);
    delay(5);
  }
  for (int duty = 255; duty >= 0; duty--) {
    ledcWrite(pwmCh, duty);
    delay(5);
  }
}
```

**API 说明：**

| 函数 | 参数 | 说明 |
|---|---|---|
| `ledcSetup(ch, freq, resBits)` | `ch`: 通道 0–15<br>`freq`: 频率 (Hz)<br>`resBits`: 分辨率 (1–16) | 配置 PWM 通道参数 |
| `ledcAttachPin(pin, ch)` | 将 GPIO 绑定到指定通道 | 一个通道可绑定多引脚，输出相同波形 |
| `ledcWrite(ch, duty)` | `duty`: 0 – (2^resBits − 1) | 设置占空比 |
| `ledcWriteTone(ch, freq)` | `freq`: 频率 (Hz) | 输出指定频率的 50% 方波（用于蜂鸣器） |
| `ledcWriteNote(ch, note, octave)` | `note`: NOTE_C–NOTE_B<br>`octave`: 0–8 | 输出指定音高（用于蜂鸣器） |

### 3.3 频率与分辨率的关系

LEDC 的基准时钟为 80 MHz（APB_CLK）或 8 MHz（REF_TICK）。输出频率、分辨率与基准时钟满足：

> `freq = f_clk / (2^resBits)`

| 分辨率 (bits) | 最大频率 | 占空比精度 |
|---|---|---|
| 1 | 40 MHz（理论） | 2 级 |
| 8 | 312.5 kHz | 256 级 |
| 12 | ~19.5 kHz | 4096 级 |
| 16 | ~1.22 kHz | 65536 级 |

实际可達频率受 GPIO 驱动能力和 PCB 走线限制，通常在数 MHz 以内。

### 3.4 多通道同步输出

LEDC 通道 0–7 共享高速定时器组 0，通道 8–15 共享低速定时器组 1。同一定时器组内的通道在硬件层面相位对齐，无需软件同步：

```cpp
ledcSetup(0, 5000, 8);
ledcSetup(1, 5000, 8);
ledcAttachPin(2, 0);
ledcAttachPin(4, 1);

ledcWrite(0, 128);
ledcWrite(1, 128);
// 通道 0 和 1 自动相位对齐（共享高速定时器 0）
```

### 3.5 渐变控制

LEDC 内置硬件渐变（fading）功能，可在 ISR 中触发或由软件启动，实现完全无 CPU 干预的占空比渐变：

```cpp
ledcSetup(0, 5000, 8);
ledcAttachPin(2, 0);

// 安装渐变功能（无需 ISR 时第二个参数为 0）
ledc_fade_func_install(0);

// 在 1 秒内渐变至 255，完成后等待下一次渐变
ledc_set_fade_with_time(ledc_channel_t(0), 0, 255, 1000);
ledc_fade_start(ledc_channel_t(0), 0, LEDC_FADE_WAIT_DONE);

// 再在 1 秒内渐变回 0
ledc_set_fade_with_time(ledc_channel_t(0), 0, 0, 1000);
ledc_fade_start(ledc_channel_t(0), 0, LEDC_FADE_WAIT_DONE);
```

---

## 4. DMA —— I2S 并行模式驱动 GPIO

### 4.1 概述

ESP32 没有独立的 GPIO DMA 引擎，但 I2S 外设的 LCD 模式可将 I2S 的时钟/数据线复用为并行 GPIO 总线，利用 I2S 内部的 DMA 引擎从内存搬运数据到 GPIO 输出寄存器。典型应用包括 LED 矩阵刷新、高速并行 DAC、自定义通信协议等。

> **注意：** Arduino-ESP32 的版本（core 2.x vs 3.x）对应不同的 ESP-IDF 底层（v4.x vs v5.x），I2S 结构体字段名称有差异。以下示例基于 ESP-IDF v4.x（Arduino core 2.x）。若使用 Arduino core 3.x，请优先采用 `I2S_CONFIG_DEFAULT()` 宏初始化配置结构体。

### 4.2 基本用法：8 位并行输出

以下示例配置 I2S0 为 8 位并行输出模式，以 1 MSPS 刷新速率将 DMA 缓冲区的数据输出到 GPIO：

```cpp
#include <driver/i2s.h>

static i2s_config_t conf = {
  .mode            = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
  .sample_rate     = 1000000,                   // 1 MSPS → 1 µs/样本
  .bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT,
  .channel_format  = I2S_CHANNEL_FMT_RIGHT_LEFT, // 双声道 = 16 位并行
  .communication_format = I2S_COMM_FORMAT_STAND_I2S,
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count   = 8,
  .dma_buf_len     = 1024,
  .use_apll        = false,
  .tx_desc_auto_clear = true,
  .fixed_mclk      = 0
};

static i2s_pin_config_t pins = {
  .bck_io_num   = I2S_PIN_NO_CHANGE,
  .ws_io_num    = I2S_PIN_NO_CHANGE,
  .data_out_num = I2S_PIN_NO_CHANGE,   // 并行模式下无效
  .data_in_num  = I2S_PIN_NO_CHANGE
};

#define DMA_BUF_SIZE 2048
uint8_t dmaBuf[DMA_BUF_SIZE];

void setup() {
  Serial.begin(115200);

  i2s_driver_install(I2S_NUM_0, &conf, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_set_dac_mode(I2S_DAC_CHANNEL_DISABLE);   // 关闭内部 DAC
}

void loop() {
  // 填充 DMA buffer：生成 8 位计数波形
  for (int i = 0; i < DMA_BUF_SIZE; i++) {
    dmaBuf[i] = (uint8_t)(i & 0xFF);
  }

  size_t written;
  i2s_write(I2S_NUM_0, dmaBuf, DMA_BUF_SIZE, &written, portMAX_DELAY);
  Serial.printf("Wrote %d bytes via DMA\n", written);
}
```

**API 说明：**

| 函数 | 说明 |
|---|---|
| `i2s_driver_install(port, conf, queue, i2sQueue)` | 安装 I2S 驱动；`queue = 0` 时创建 DMA 缓冲 |
| `i2s_set_pin(port, pins)` | 绑定 BCK / WS / DATA 到物理引脚 |
| `i2s_set_dac_mode(...)` | 并行模式下需显式关闭内部 DAC |
| `i2s_write(port, src, size, written, timeout)` | 通过 DMA 发送数据；阻塞直到 DMA 搬运完成或超时 |

### 4.3 替代方案：RMT + DMA（适用于 LED 灯带）

对于 WS2812 / SK6812 等可寻址 LED，RMT 外设内置 DMA 支持，比 I2S 更简便：

```cpp
#include <driver/rmt.h>

rmt_config_t config = RMT_DEFAULT_CONFIG_TX(GPIO_NUM_2, RMT_CHANNEL_0);
config.clk_div = 4;          // 80 MHz / 4 = 20 MHz 基准

rmt_config(&config);
rmt_driver_install(config.channel, 0, 0);
```

### 4.4 三者组合场景

在复杂应用中，三种外设可以互不冲突地协同工作，因为它们使用独立的硬件资源：

| 模块 | 硬件资源 | 典型角色 |
|---|---|---|
| `hw_timer_t` | Timer Group | 精确定时触发、采样/输出节拍控制 |
| `ledc` | LEDC 外设 | 模拟控制（背光、电机、音频功放偏置） |
| I2S DMA | I2S 外设 + DMA 引擎 | 高速批量 GPIO 输出（LED 矩阵、并行 DAC） |

---

## 附录：本工程文件说明

- `bluetooth.ino` — 当前主 sketch：通过 BLE 串口接收 `"on"` / `"off"` 命令控制 GPIO 2 上的 LED 开关。
- 本 `README.md` — 供参考的外设编程手册，涵盖 GPIO 配置、硬件定时器、PWM 与 DMA 的完整用法与示例代码。
