#pragma once

#include <Arduino.h>
#include <Print.h>
#include <Wire.h>

// Self-contained SSD1306 OLED driver for I2C displays.
// Only ASCII characters 32..126 are supported by the built-in font.
class OledSsd1306 : public Print {
public:
  static constexpr uint8_t WIDTH = 128;
  static constexpr uint8_t HEIGHT = 64;
  static constexpr uint8_t DEFAULT_ADDRESS = 0x3C;
  static constexpr uint8_t CHAR_WIDTH = 5;
  static constexpr uint8_t CHAR_HEIGHT = 8;

  OledSsd1306(uint8_t width = WIDTH, uint8_t height = HEIGHT);
  ~OledSsd1306();

  bool begin(TwoWire &wire = Wire,
             uint8_t address = DEFAULT_ADDRESS,
             int8_t sda = -1,
             int8_t scl = -1,
             uint32_t clockHz = 400000);

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

  size_t write(uint8_t character) override;
  using Print::write;

private:
  void sendCommand(uint8_t command);
  void sendDataPage(uint8_t page);
  void drawChar(uint8_t x, uint8_t y, char character);
  bool isInside(int16_t x, int16_t y) const;

  TwoWire *_wire;
  uint8_t _address;
  uint8_t _width;
  uint8_t _height;
  uint8_t _pages;
  uint8_t *_buffer;
  uint8_t _cursorX;
  uint8_t _cursorY;
  bool _textWrap;
  bool _initialized;
};
