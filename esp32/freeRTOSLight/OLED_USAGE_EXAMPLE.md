# OLED SSD1306 使用实例

本说明对应 `freeRTOSLight` 目录中自带的 SSD1306 OLED 驱动文件：

```text
oled_ssd1306.h
oled_ssd1306.cpp
```

该驱动不依赖 Adafruit GFX 或 Adafruit SSD1306 库，使用 Arduino `Wire` 库，通过 I2C 通信。

## 1. 硬件连接

以 ESP32 为例：

| OLED 引脚 | ESP32 引脚 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

如果开发板的 SDA/SCL 引脚不同，只需在 `begin()` 中传入实际引脚即可。

## 2. 可运行示例

可以直接把下面的代码放进 `freeRTOSLight.ino`：

```cpp
#include "oled_ssd1306.h"

OledSsd1306 oled;

void setup() {
  Serial.begin(115200);

  // 参数：Wire 实例、I2C 地址、SDA 引脚、SCL 引脚
  if (!oled.begin(Wire, 0x3C, 21, 22)) {
    Serial.println("OLED init failed");
    while (true) {
      delay(100);
    }
  }

  oled.clear();
  oled.setCursor(0, 0);
  oled.println("Hello ESP32");
  oled.println("FreeRTOS");
  oled.println("OLED SSD1306");
  oled.display();
}

void loop() {
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    oled.clear();
    oled.setCursor(0, 0);
    oled.println("Uptime:");
    oled.print(millis() / 1000);
    oled.println(" s");

    // 底部显示一个随时间变化的进度条
    int progress = map(millis() % 10000, 0, 9999, 0, 128);
    oled.fillRect(0, 40, progress, 24, true);

    oled.display();
  }
}
```

示例效果：

- 第一屏显示标题。
- 之后每秒刷新运行时间，并在屏幕底部显示一个进度条。

## 3. 初始化说明

```cpp
OledSsd1306 oled;

// 默认 I2C 地址是 0x3C
oled.begin(Wire, 0x3C, 21, 22);
```

- 如果模块地址是 `0x3D`，改成 `oled.begin(Wire, 0x3D, 21, 22)`。
- 如果不指定 SDA/SCL，可以使用开发板默认 I2C 引脚：

```cpp
oled.begin(Wire, 0x3C);
```

- `begin()` 返回 `true` 表示驱动初始化成功。

## 4. 显示文本

常用方式：

```cpp
oled.clear();
oled.setCursor(0, 0);
oled.print("Hello ");
oled.println("ESP32");
oled.print("1234567890");
oled.display();
```

也可以直接指定坐标：

```cpp
oled.clear();
oled.drawString(0, 0, "Hello");
oled.drawString(0, 16, "ESP32");
oled.display();
```

注意事项：

- 每次修改画面后，必须调用 `oled.display()` 才会刷新到屏幕。
- 建议使用 `8` 的倍数作为纵向坐标，例如 `0`、`8`、`16`、`24`。
- 内置字体仅支持 ASCII 字符 `32..126`，不支持中文。

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
```

图形绘制示例：

```cpp
oled.clear();
oled.fillRect(0, 0, 128, 16, true);
oled.drawLine(0, 20, 127, 63, true);
oled.drawRect(40, 24, 48, 24, true);
oled.drawString(4, 5, "Hello");
oled.display();
```

## 6. 排错提示

- 如果 OLED 没有显示，先确认 I2C 地址是 `0x3C` 还是 `0x3D`。
- 如果文字方向上下颠倒，通常需要调整 `oled_ssd1306.cpp` 中的 `SEG remap` 或 `COM scan` 初始化命令。
- 驱动内部会为显存动态分配内存，`128x64` 屏幕约需要 `1024` 字节，`128x32` 屏幕约需要 `512` 字节。
