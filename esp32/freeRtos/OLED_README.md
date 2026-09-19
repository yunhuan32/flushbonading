# OLED SSD1306 驱动使用说明

本工程新增了一个独立的 SSD1306 OLED 驱动，不依赖 Adafruit GFX 或 Adafruit SSD1306 库。

文件：

```text
oled_ssd1306.h
oled_ssd1306.cpp
```

## 1. 支持内容

- I2C 接口的 SSD1306 OLED
- 常见分辨率：`128x64` 和 `128x32`
- 英文、数字和常用 ASCII 符号：字符范围 `32..126`
- 使用 Arduino 的 `Wire` 库通信
- 继承自 Arduino `Print`，可以直接使用 `print`、`println`
- 提供基础绘图接口：点、线、矩形、填充矩形
- 内部使用显存缓冲，调用 `display()` 后统一刷新到屏幕

## 2. 接线示例

下面以 ESP32 为例：

| OLED 引脚 | ESP32 引脚 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

如果开发板引脚不同，只需在初始化时传入实际的 SDA、SCL 引脚。

## 3. 初始化

在 `.ino` 文件中包含头文件，并创建一个全局对象：

```cpp
#include "oled_ssd1306.h"

OledSsd1306 oled;
```

在 `setup()` 中初始化：

```cpp
void setup() {
  Serial.begin(115200);

  // 参数：I2C 实例、OLED 地址、SDA、SCL、I2C 频率
  if (!oled.begin(Wire, 0x3C, 21, 22)) {
    Serial.println("OLED init failed");
    while (true) {
      delay(100);
    }
  }

  oled.clear();
  oled.display();
}
```

说明：

- 默认 I2C 地址是 `0x3C`
- 如果模块地址是 `0x3D`，改成 `oled.begin(Wire, 0x3D, 21, 22)`
- 如果不想指定引脚，可以使用 `oled.begin(Wire, 0x3C)`
- `begin()` 返回 `true` 表示内存初始化成功

## 4. 显示英文文本

最常用的方式是 `print` 和 `println`：

```cpp
oled.clear();
oled.setCursor(0, 0);
oled.print("Hello ESP32");
oled.println("FreeRTOS");
oled.print("1234567890");
oled.display();
```

也可以使用 `drawString()` 直接指定坐标：

```cpp
oled.clear();
oled.drawString(0, 0, "Hello");
oled.drawString(0, 16, "ESP32");
oled.display();
```

注意：

- 每次修改画面后，必须调用 `oled.display()`
- 行间距建议使用 `8` 的倍数，例如 `0`、`8`、`16`、`24`
- 一个 5x7 字符加一个像素间距，约占 6 像素宽
- `128` 像素宽度下，每行最多约 `21` 个英文或数字
- 默认开启自动换行

## 5. 常用接口

```cpp
void clear();
void display();

void setContrast(uint8_t contrast);
void setInverted(bool inverted);
void setCursor(uint8_t x, uint8_t y);
void setTextWrap(bool wrap);

void drawPixel(int16_t x, int16_t y, bool color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color);
void drawRect(int16_t x, int16_t y, int16_t width, int16_t height, bool color);
void fillRect(int16_t x, int16_t y, int16_t width, int16_t height, bool color);
void drawString(uint8_t x, uint8_t y, const char *text);

size_t write(uint8_t character);
```

简单示例：

```cpp
oled.clear();
oled.fillRect(0, 0, 128, 16, true);
oled.drawLine(0, 20, 127, 63, true);
oled.drawRect(40, 24, 48, 24, true);
oled.drawString(4, 5, "Hello");
oled.display();
```

## 6. 移植到其他工程

1. 复制 `oled_ssd1306.h` 和 `oled_ssd1306.cpp` 到目标工程目录。
2. 在 Arduino 工程中 `#include "oled_ssd1306.h"`。
3. 根据目标开发板修改 SDA、SCL 引脚和 I2C 地址。
4. 保留原来的 I2C 上拉电阻或 OLED 模块自带的上拉。

如果目标工程不是 ESP32：

```cpp
oled.begin(Wire, 0x3C);
```

这样会使用目标开发板默认的 I2C 引脚。

## 7. 注意事项

- 内置字体只支持英文、数字和常用 ASCII 符号，不支持中文。
- 如果 OLED 没有显示，请先确认 I2C 地址是否为 `0x3C` 或 `0x3D`。
- 如果字体方向上下颠倒，通常需要修改 SSD1306 初始化中的 `SEG remap` 或 `COM scan` 命令。
- 驱动使用动态内存保存显存，`128x64` 需要约 `1024` 字节，`128x32` 需要约 `512` 字节。
