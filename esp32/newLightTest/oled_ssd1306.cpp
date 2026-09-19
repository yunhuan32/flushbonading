#include "oled_ssd1306.h"

namespace {

// Public domain style 5x7 ASCII font, covering characters 32 through 126.
const uint8_t FONT_5X7[95][5] = {
  {0x00, 0x00, 0x00, 0x00, 0x00}, // space
  {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
  {0x00, 0x07, 0x00, 0x07, 0x00}, // "
  {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
  {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
  {0x23, 0x13, 0x08, 0x64, 0x62}, // %
  {0x36, 0x49, 0x55, 0x22, 0x50}, // &
  {0x00, 0x05, 0x03, 0x00, 0x00}, // '
  {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
  {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
  {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
  {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
  {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
  {0x08, 0x08, 0x08, 0x08, 0x08}, // -
  {0x00, 0x60, 0x60, 0x00, 0x00}, // .
  {0x20, 0x10, 0x08, 0x04, 0x02}, // /
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
  {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
  {0x00, 0x36, 0x36, 0x00, 0x00}, // :
  {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
  {0x08, 0x14, 0x22, 0x41, 0x00}, // <
  {0x14, 0x14, 0x14, 0x14, 0x14}, // =
  {0x00, 0x41, 0x22, 0x14, 0x08}, // >
  {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
  {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
  {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
  {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
  {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
  {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
  {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
  {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
  {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
  {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
  {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
  {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
  {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
  {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
  {0x46, 0x49, 0x49, 0x49, 0x31}, // S
  {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
  {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
  {0x63, 0x14, 0x08, 0x14, 0x63}, // X
  {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
  {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
  {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
  {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
  {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
  {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
  {0x40, 0x40, 0x40, 0x40, 0x40}, // _
  {0x00, 0x01, 0x02, 0x04, 0x00}, // `
  {0x20, 0x54, 0x54, 0x54, 0x78}, // a
  {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
  {0x38, 0x44, 0x44, 0x44, 0x20}, // c
  {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
  {0x38, 0x54, 0x54, 0x54, 0x18}, // e
  {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
  {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
  {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
  {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
  {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
  {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
  {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
  {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
  {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
  {0x38, 0x44, 0x44, 0x44, 0x38}, // o
  {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
  {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
  {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
  {0x48, 0x54, 0x54, 0x54, 0x20}, // s
  {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
  {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
  {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
  {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
  {0x44, 0x28, 0x10, 0x28, 0x44}, // x
  {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
  {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
  {0x00, 0x08, 0x36, 0x41, 0x00}, // {
  {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
  {0x00, 0x41, 0x36, 0x08, 0x00}, // }
  {0x02, 0x01, 0x02, 0x04, 0x02}, // ~
};

}  // namespace

OledSsd1306::OledSsd1306(uint8_t width, uint8_t height)
    : _wire(&Wire),
      _address(DEFAULT_ADDRESS),
      _width(width),
      _height(height),
      _pages((height + 7) / 8),
      _buffer(nullptr),
      _cursorX(0),
      _cursorY(0),
      _textWrap(true),
      _initialized(false) {}

OledSsd1306::~OledSsd1306() {
  if (_buffer != nullptr) {
    free(_buffer);
  }
}

bool OledSsd1306::begin(TwoWire &wire,
                        uint8_t address,
                        int8_t sda,
                        int8_t scl,
                        uint32_t clockHz) {
  _wire = &wire;
  _address = address;

  if (sda >= 0 && scl >= 0) {
#if defined(ESP32) || defined(ESP8266)
    _wire->begin(sda, scl, clockHz);
#elif defined(ARDUINO_ARCH_RP2040)
    _wire->setSDA(sda);
    _wire->setSCL(scl);
    _wire->begin();
#else
    _wire->begin();
#endif
  } else {
    _wire->begin();
  }

  if (_buffer != nullptr) {
    free(_buffer);
    _buffer = nullptr;
  }

  _buffer = static_cast<uint8_t *>(malloc(static_cast<size_t>(_width) * _pages));
  if (_buffer == nullptr) {
    return false;
  }
  clear();

  sendCommand(0xAE);  // display off
  sendCommand(0xD5);  // clock divider
  sendCommand(0x80);
  sendCommand(0xA8);  // multiplex ratio
  sendCommand(_height - 1);
  sendCommand(0xD3);  // display offset
  sendCommand(0x00);
  sendCommand(0x40);  // start line 0
  sendCommand(0x8D);  // charge pump
  sendCommand(0x14);
  sendCommand(0x20);  // memory addressing mode
  sendCommand(0x00);  // horizontal
  sendCommand(0xA1);  // segment remap
  sendCommand(0xC8);  // COM scan direction
  sendCommand(0xDA);  // COM pins
  sendCommand(_height == 32 ? 0x02 : 0x12);
  sendCommand(0x81);  // contrast
  sendCommand(0xCF);
  sendCommand(0xD9);  // precharge
  sendCommand(0xF1);
  sendCommand(0xDB);  // VCOM detect
  sendCommand(0x40);
  sendCommand(0xA4);  // resume from RAM
  sendCommand(0xA6);  // normal display
  sendCommand(0xAF);  // display on

  _initialized = true;
  return true;
}

void OledSsd1306::clear() {
  if (_buffer == nullptr) {
    return;
  }
  memset(_buffer, 0, static_cast<size_t>(_width) * _pages);
  _cursorX = 0;
  _cursorY = 0;
}

void OledSsd1306::display() {
  if (!_initialized || _buffer == nullptr) {
    return;
  }

  for (uint8_t page = 0; page < _pages; ++page) {
    sendCommand(0xB0 + page);
    sendCommand(0x00);
    sendCommand(0x10);
    sendDataPage(page);
  }
}

void OledSsd1306::setContrast(uint8_t contrast) {
  sendCommand(0x81);
  sendCommand(contrast);
}

void OledSsd1306::setInverted(bool inverted) {
  sendCommand(inverted ? 0xA7 : 0xA6);
}

void OledSsd1306::setCursor(uint8_t x, uint8_t y) {
  _cursorX = x < _width ? x : _width - 1;
  _cursorY = y < _height ? y : _height - 1;
}

void OledSsd1306::setTextWrap(bool wrap) {
  _textWrap = wrap;
}

bool OledSsd1306::isInside(int16_t x, int16_t y) const {
  return x >= 0 && x < _width && y >= 0 && y < _height;
}

void OledSsd1306::drawPixel(int16_t x, int16_t y, bool color) {
  if (_buffer == nullptr || !isInside(x, y)) {
    return;
  }

  const uint8_t page = static_cast<uint8_t>(y) / 8;
  const uint8_t bit = static_cast<uint8_t>(y) % 8;
  const size_t index = static_cast<size_t>(page) * _width + static_cast<size_t>(x);

  if (color) {
    _buffer[index] |= (1U << bit);
  } else {
    _buffer[index] &= ~(1U << bit);
  }
}

void OledSsd1306::drawLine(int16_t x0,
                           int16_t y0,
                           int16_t x1,
                           int16_t y1,
                           bool color) {
  const int16_t deltaX = abs(x1 - x0);
  const int16_t stepX = x0 < x1 ? 1 : -1;
  const int16_t deltaY = -abs(y1 - y0);
  const int16_t stepY = y0 < y1 ? 1 : -1;
  int16_t error = deltaX + deltaY;

  while (true) {
    drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int16_t doubledError = error * 2;
    if (doubledError >= deltaY) {
      error += deltaY;
      x0 += stepX;
    }
    if (doubledError <= deltaX) {
      error += deltaX;
      y0 += stepY;
    }
  }
}

void OledSsd1306::drawRect(int16_t x,
                           int16_t y,
                           int16_t width,
                           int16_t height,
                           bool color) {
  drawLine(x, y, x + width - 1, y, color);
  drawLine(x, y + height - 1, x + width - 1, y + height - 1, color);
  drawLine(x, y, x, y + height - 1, color);
  drawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
}

void OledSsd1306::fillRect(int16_t x,
                           int16_t y,
                           int16_t width,
                           int16_t height,
                           bool color) {
  for (int16_t row = y; row < y + height; ++row) {
    drawLine(x, row, x + width - 1, row, color);
  }
}

void OledSsd1306::drawChar(uint8_t x, uint8_t y, char character) {
  if (_buffer == nullptr) {
    return;
  }

  if (character < 32 || character > 126) {
    character = ' ';
  }

  const uint8_t *glyph = FONT_5X7[static_cast<uint8_t>(character) - 32];
  const uint8_t shift = y & 0x07;
  const uint8_t page = y / 8;

  for (uint8_t column = 0; column < CHAR_WIDTH; ++column) {
    if (x + column >= _width) {
      break;
    }

    const uint16_t bits = static_cast<uint16_t>(glyph[column]) << shift;
    const size_t lowIndex = static_cast<size_t>(page) * _width + x + column;
    _buffer[lowIndex] |= static_cast<uint8_t>(bits & 0xFF);

    if (shift != 0 && page + 1 < _pages) {
      const size_t highIndex = static_cast<size_t>(page + 1) * _width + x + column;
      _buffer[highIndex] |= static_cast<uint8_t>((bits >> 8) & 0xFF);
    }
  }
}

void OledSsd1306::drawString(uint8_t x, uint8_t y, const char *text) {
  if (text == nullptr) {
    return;
  }

  const uint8_t previousX = _cursorX;
  const uint8_t previousY = _cursorY;
  setCursor(x, y);

  while (*text != '\0') {
    write(static_cast<uint8_t>(*text));
    ++text;
  }

  _cursorX = previousX;
  _cursorY = previousY;
}

size_t OledSsd1306::write(uint8_t character) {
  if (!_initialized || _buffer == nullptr) {
    return 0;
  }

  if (character == '\n') {
    _cursorX = 0;
    _cursorY += CHAR_HEIGHT;
    if (_cursorY >= _height) {
      _cursorY = 0;
    }
    return 1;
  }

  if (character == '\r') {
    _cursorX = 0;
    return 1;
  }

  if (_textWrap && _cursorX + CHAR_WIDTH + 1 > _width) {
    _cursorX = 0;
    _cursorY += CHAR_HEIGHT;
    if (_cursorY >= _height) {
      _cursorY = 0;
    }
  }

  drawChar(_cursorX, _cursorY, static_cast<char>(character));
  _cursorX += CHAR_WIDTH + 1;

  return 1;
}

void OledSsd1306::sendCommand(uint8_t command) {
  if (_wire == nullptr) {
    return;
  }

  _wire->beginTransmission(_address);
  _wire->write(0x00);
  _wire->write(command);
  _wire->endTransmission();
}

void OledSsd1306::sendDataPage(uint8_t page) {
  if (_wire == nullptr || _buffer == nullptr) {
    return;
  }

  const uint8_t chunkSize = 16;
  _wire->beginTransmission(_address);
  _wire->write(0x40);
  const size_t offset = static_cast<size_t>(page) * _width;
  for (uint8_t column = 0; column < _width; column += chunkSize) {
    const uint8_t remaining = _width - column;
    const uint8_t length = remaining < chunkSize ? remaining : chunkSize;
    _wire->write(_buffer + offset + column, length);
  }
  _wire->endTransmission();
}
