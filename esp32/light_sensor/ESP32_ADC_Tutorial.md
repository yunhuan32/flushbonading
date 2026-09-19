# ESP32 ADC 使用教学

本文只讲 ESP32 的 ADC 外设怎么用，不修改你的工程代码。

## 1. ADC 是什么

ADC 是模数转换器，英文是 Analog-to-Digital Converter。

它把外部输入的连续电压转换成数字值。例如：

- 输入 `0V`，12 位 ADC 读出 `0`
- 输入 `3.3V`，12 位 ADC 读出 `4095`

ESP32 的 ADC 是逐次逼近型 SAR ADC。Arduino 环境里最常用的函数是：

```cpp
int value = analogRead(pin);
```

## 2. ESP32 ADC 的重要特点

### 2.1 分辨率

ESP32 的 ADC 支持 9、10、11、12 位分辨率。

12 位时：

```text
0 ~ 4095
```

10 位时：

```text
0 ~ 1023
```

设置分辨率：

```cpp
analogReadResolution(12);
```

分辨率越高，数值越细，但噪声也会更明显。

### 2.2 ADC1 和 ADC2

ESP32 有两组 ADC：

#### ADC1

常见引脚：

```text
GPIO32 -> ADC1_CH4
GPIO33 -> ADC1_CH5
GPIO34 -> ADC1_CH6
GPIO35 -> ADC1_CH7
GPIO36 -> ADC1_CH0
GPIO37 -> ADC1_CH1
GPIO38 -> ADC1_CH2
GPIO39 -> ADC1_CH3
```

#### ADC2

常见引脚：

```text
GPIO0, GPIO2, GPIO4
GPIO12, GPIO13, GPIO14, GPIO15
GPIO25, GPIO26, GPIO27
```

注意：

> 如果你使用了 Wi-Fi，尽量使用 ADC1，不要使用 ADC2。  
> 因为 ESP32 的 Wi-Fi 功能会占用 ADC2。

对于光线传感器项目，最推荐使用：

```text
GPIO34
```

它是输入专用引脚，通常也很方便接线。

### 2.3 输入电压范围

ESP32 的 ADC 不能随便输入超过芯片允许范围的电压。

常用输入范围是：

```text
0V ~ 3.3V
```

不要直接输入 5V，否则可能损坏芯片。

如果传感器输出 0~5V，需要先做分压。

## 3. Attenuation 衰减

ESP32 的 ADC 使用衰减来调整量程。

Arduino ESP32 中常用参数：

| 参数 | 大致量程 | 适用场景 |
| --- | --- | --- |
| `ADC_0db` | 约 0~1.1V | 小信号，精度较高 |
| `ADC_2_5db` | 约 0~1.5V | 较小信号 |
| `ADC_6db` | 约 0~2.2V | 中等信号 |
| `ADC_11db` | 约 0~3.3V | 最常见，读取 3.3V 系统 |

设置所有 ADC 通道：

```cpp
analogSetAttenuation(ADC_11db);
```

只设置某个引脚：

```cpp
analogSetPinAttenuation(34, ADC_11db);
```

大多数情况下，读取 3.3V 传感器使用 `ADC_11db` 就可以了。

## 4. 最基本的 ADC 代码

下面是一个最小示例：

```cpp
void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(34, ADC_11db);
}

void loop() {
  int raw = analogRead(34);

  Serial.print("ADC raw: ");
  Serial.println(raw);

  delay(200);
}
```

效果：

```text
ADC raw: 0
ADC raw: 0
ADC raw: 1
ADC raw: 3
ADC raw: 1200
ADC raw: 3400
ADC raw: 4095
```

## 5. 如何把原始值换算成电压

12 位 ADC 的范围是 `0~4095`，对应 `0~3300mV`。

换算公式：

```cpp
float voltage = raw * 3.3 / 4095.0;
```

使用整数毫伏：

```cpp
uint32_t millivolts = (uint32_t)raw * 3300UL / 4095UL;
```

ESP32 Arduino Core 也提供校准后的电压读取：

```cpp
uint32_t millivolts = analogReadMilliVolts(34);
```

这个函数会尝试使用芯片内部的校准数据，通常比手动换算更准确。

## 6. 读取时做平均

ADC 会有噪声，尤其是光线传感器附近电源不稳定时，读数可能跳动。

推荐连续读取多次，再取平均：

```cpp
uint16_t readAverage(uint8_t pin, uint8_t samples) {
  uint32_t sum = 0;

  for (uint8_t i = 0; i < samples; ++i) {
    sum += analogRead(pin);
    delayMicroseconds(100);
  }

  return (uint16_t)(sum / samples);
}
```

使用：

```cpp
uint16_t value = readAverage(34, 32);
```

采样次数越多，读数越稳，但响应会稍慢。

光线检测通常取 16、32 或 64 次平均即可。

## 7. 光敏电阻接线

光敏电阻通常使用分压电路。

推荐接法：

```text
3V3
 |
光敏电阻
 |
 +------ GPIO34
 |
10k 电阻
 |
GND
```

这样：

- 光线越强，光敏电阻阻值越小
- GPIO34 电压越高
- ADC 读数越大

如果接法反过来，读数方向也会反过来，这在写代码时要注意。

## 8. 常见问题

### 8.1 读数一直接近 0

检查：

- 传感器是否接到正确的 ADC 引脚
- 是否已经设置 `ADC_11db`
- 分压电路是否接错
- 地线是否和 ESP32 共地

### 8.2 读数一直接近 4095

检查：

- 是否直接把 3.3V 接到了 ADC 引脚
- 分压电阻是否接反
- 传感器是否输出 3.3V 或更高

### 8.3 使用 Wi-Fi 后 ADC 不准

优先使用 ADC1：

```text
GPIO32 ~ GPIO39
```

不要使用 ADC2：

```text
GPIO0, GPIO2, GPIO4, GPIO12 ~ GPIO15, GPIO25 ~ GPIO27
```

### 8.4 读数跳动

先做多次采样平均。

如果仍然跳动，检查：

- 电源是否稳定
- 传感器是否接触不良
- 是否靠近强干扰源

## 9. 常用函数总结

| 函数 | 作用 |
| --- | --- |
| `analogReadResolution(bits)` | 设置 ADC 分辨率 |
| `analogSetAttenuation(atten)` | 设置所有通道衰减 |
| `analogSetPinAttenuation(pin, atten)` | 设置指定引脚衰减 |
| `analogRead(pin)` | 读取原始 ADC 值 |
| `analogReadMilliVolts(pin)` | 读取毫伏电压 |

## 10. 建议使用顺序

在 `setup()` 中：

```cpp
analogReadResolution(12);
analogSetPinAttenuation(34, ADC_11db);
```

在 `loop()` 中：

```cpp
int raw = analogRead(34);
uint32_t millivolts = (uint32_t)raw * 3300UL / 4095UL;
```

如果需要更稳：

```cpp
uint16_t raw = readAverage(34, 32);
```

这已经足够覆盖大多数 ESP32 ADC 和光线传感器实验。
