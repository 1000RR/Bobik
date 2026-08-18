#pragma once

#include <Arduino.h>

#include "controller_config.h"
#include "controller_state.h"

namespace bobik {

struct DisplayRows {
  char armed[OLED_ROW_BUFFER_SIZE];
  char alarm[OLED_ROW_BUFFER_SIZE];
  char activeDevices[OLED_ROW_BUFFER_SIZE];
  char allDevices[OLED_ROW_BUFFER_SIZE];
};

void renderDisplayRows(const AppState &state, DisplayRows &rows);

class ControllerUi {
 public:
  ControllerUi();

  void begin(uint32_t now, const AppState &state);
  void onStateChanged(uint32_t now, const AppState &state, bool alarmStarted);
  void service(uint32_t now, const AppState &state);

 private:
  void applyLedState(const AppState &state);
  void outputToLcd(const AppState &state);

  bool alarmBlinkOn_;
  bool splashActive_;
  uint32_t splashStartedMillis_;
  uint32_t lastOledRefreshMillis_;
  uint32_t lastAlarmBlinkMillis_;
  DisplayRows displayRows_;
};

}  // namespace bobik
