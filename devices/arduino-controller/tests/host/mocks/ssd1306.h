#pragma once

#include "Arduino.h"

#define STYLE_NORMAL 0
#define STYLE_BOLD 1

static const uint8_t *ssd1306xled_font6x8 = nullptr;
static const uint8_t *ssd1306xled_font8x16 = nullptr;

struct MockSsd1306State {
  unsigned int clearCalls;
  unsigned int printCalls;
  unsigned int invertCalls;
  unsigned int normalCalls;
  unsigned int negativeCalls;
  unsigned int positiveCalls;
};

extern MockSsd1306State mockSsd1306;

inline void ssd1306_setFixedFont(const uint8_t *) {}
inline void ssd1306_128x32_i2c_init() {}
inline void ssd1306_clearScreen() { mockSsd1306.clearCalls++; }
inline void ssd1306_invertMode() { mockSsd1306.invertCalls++; }
inline void ssd1306_normalMode() { mockSsd1306.normalCalls++; }
inline void ssd1306_negativeMode() { mockSsd1306.negativeCalls++; }
inline void ssd1306_positiveMode() { mockSsd1306.positiveCalls++; }
inline void ssd1306_printFixed(int, int, const char *, int) { mockSsd1306.printCalls++; }

