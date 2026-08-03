#include "debounced_button.h"

namespace bobik {

DebouncedButton::DebouncedButton(uint8_t pin, uint32_t debounceIntervalMillis)
    : pin_(pin),
      debounceIntervalMillis_(debounceIntervalMillis),
      stableState_(HIGH),
      lastRawState_(HIGH),
      rawStateChangedMillis_(0) {}

void DebouncedButton::begin(uint32_t now) {
  pinMode(pin_, INPUT_PULLUP);
  stableState_ = digitalRead(pin_);
  lastRawState_ = stableState_;
  rawStateChangedMillis_ = now;
}

bool DebouncedButton::pollPressed(uint32_t now) {
  const int rawState = digitalRead(pin_);
  if (rawState != lastRawState_) {
    lastRawState_ = rawState;
    rawStateChangedMillis_ = now;
  }

  if (rawState == stableState_ ||
      static_cast<uint32_t>(now - rawStateChangedMillis_) < debounceIntervalMillis_) {
    return false;
  }

  stableState_ = rawState;
  return stableState_ == LOW;
}

}  // namespace bobik
