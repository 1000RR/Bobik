#pragma once

#include <Arduino.h>

namespace bobik {

class DebouncedButton {
 public:
  DebouncedButton(uint8_t pin, uint32_t debounceIntervalMillis);

  void begin(uint32_t now);
  bool pollPressed(uint32_t now);

 private:
  uint8_t pin_;
  uint32_t debounceIntervalMillis_;
  int stableState_;
  int lastRawState_;
  uint32_t rawStateChangedMillis_;
};

}  // namespace bobik
