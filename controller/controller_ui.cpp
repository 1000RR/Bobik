#include "controller_ui.h"

#include <ssd1306.h>

namespace bobik {
namespace {

bool hasElapsed(uint32_t now, uint32_t then, uint32_t interval) {
  return static_cast<uint32_t>(now - then) >= interval;
}

void clearDisplayRow(char row[OLED_ROW_BUFFER_SIZE]) {
  for (size_t index = 0; index < OLED_ROW_CHARACTERS; index++) {
    row[index] = ' ';
  }
  row[OLED_ROW_CHARACTERS] = '\0';
}

void copyTextToDisplayRow(char row[OLED_ROW_BUFFER_SIZE], size_t offset, const char *text) {
  while (*text != '\0' && offset < OLED_ROW_CHARACTERS) {
    row[offset++] = *text++;
  }
}

char hexDigit(uint8_t value) {
  return value < 10 ? static_cast<char>('0' + value)
                    : static_cast<char>('a' + value - 10);
}

void renderDeviceIds(const DeviceIdSet &deviceIds, char row[OLED_ROW_BUFFER_SIZE]) {
  clearDisplayRow(row);
  size_t position = 0;

  for (uint16_t candidate = 1; candidate < BASE_STATION_ADDRESS; candidate++) {
    const uint8_t deviceId = static_cast<uint8_t>(candidate);
    if (!containsDeviceId(deviceIds, deviceId)) {
      continue;
    }

    const size_t tokenLength = deviceId < 0x10 ? 4 : 5;
    if (position + tokenLength > OLED_ROW_CHARACTERS) {
      break;
    }

    row[position++] = '0';
    row[position++] = 'x';
    if (deviceId >= 0x10) {
      row[position++] = hexDigit(deviceId >> 4);
    }
    row[position++] = hexDigit(deviceId & 0x0F);
    row[position++] = ' ';
  }
}

}  // namespace

void renderDisplayRows(const AppState &state, DisplayRows &rows) {
  clearDisplayRow(rows.armed);
  clearDisplayRow(rows.alarm);
  clearDisplayRow(rows.activeDevices);
  clearDisplayRow(rows.allDevices);

  copyTextToDisplayRow(rows.armed, 0, state.armed ? "ENABLED" : "DISABLED");
  if (state.alarmed) {
    copyTextToDisplayRow(rows.alarm, 8, "ALARM");
    renderDeviceIds(state.activeAlarmDeviceIds, rows.activeDevices);
    renderDeviceIds(state.allAlarmDeviceIds, rows.allDevices);
  } else {
    copyTextToDisplayRow(rows.alarm, 6, "NO ALARM");
  }
}

ControllerUi::ControllerUi()
    : alarmBlinkOn_(false),
      splashActive_(false),
      splashStartedMillis_(0),
      lastOledRefreshMillis_(0),
      lastAlarmBlinkMillis_(0),
      displayRows_{} {}

void ControllerUi::applyLedState(const AppState &state) {
  digitalWrite(ARMED_LED_PIN, state.armed ? HIGH : LOW);
  digitalWrite(DISARMED_LED_PIN,
               state.alarmed ? (alarmBlinkOn_ ? HIGH : LOW) : (!state.armed ? HIGH : LOW));
}

void ControllerUi::begin(uint32_t now, const AppState &state) {
  pinMode(ARMED_LED_PIN, OUTPUT);
  pinMode(DISARMED_LED_PIN, OUTPUT);
  applyLedState(state);

  ssd1306_setFixedFont(ssd1306xled_font6x8);
  ssd1306_128x32_i2c_init();
  ssd1306_clearScreen();

  splashActive_ = true;
  splashStartedMillis_ = now;
  ssd1306_invertMode();
  ssd1306_setFixedFont(ssd1306xled_font8x16);
  ssd1306_printFixed(0, 8, "     BOBIK", STYLE_BOLD);
}

void ControllerUi::onStateChanged(uint32_t now, const AppState &state, bool alarmStarted) {
  if (alarmStarted) {
    alarmBlinkOn_ = true;
    lastAlarmBlinkMillis_ = now;
  } else if (!state.alarmed) {
    alarmBlinkOn_ = false;
  }
  applyLedState(state);
}

void ControllerUi::outputToLcd(const AppState &state) {
  renderDisplayRows(state, displayRows_);
  ssd1306_setFixedFont(ssd1306xled_font6x8);

  if (state.armed) {
    ssd1306_negativeMode();
  }
  ssd1306_printFixed(0, 0, displayRows_.armed, STYLE_NORMAL);
  if (state.armed) {
    ssd1306_positiveMode();
  }

  if (state.alarmed && alarmBlinkOn_) {
    ssd1306_negativeMode();
  }
  ssd1306_printFixed(0, 8, displayRows_.alarm, STYLE_BOLD);
  if (state.alarmed && alarmBlinkOn_) {
    ssd1306_positiveMode();
  }

  ssd1306_printFixed(0, 16, displayRows_.activeDevices, STYLE_BOLD);
  ssd1306_printFixed(0, 24, displayRows_.allDevices, STYLE_BOLD);
}

void ControllerUi::service(uint32_t now, const AppState &state) {
  if (state.alarmed && hasElapsed(now, lastAlarmBlinkMillis_, ALARM_BLINK_INTERVAL_MS)) {
    lastAlarmBlinkMillis_ = now;
    alarmBlinkOn_ = !alarmBlinkOn_;
    applyLedState(state);
  }

  if (splashActive_) {
    if (!hasElapsed(now, splashStartedMillis_, SPLASH_DURATION_MS)) {
      return;
    }
    splashActive_ = false;
    ssd1306_normalMode();
    ssd1306_clearScreen();
    lastOledRefreshMillis_ = now - OLED_REFRESH_INTERVAL_MS;
  }

  if (!hasElapsed(now, lastOledRefreshMillis_, OLED_REFRESH_INTERVAL_MS)) {
    return;
  }
  lastOledRefreshMillis_ = now;
  outputToLcd(state);
}

}  // namespace bobik
