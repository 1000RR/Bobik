#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define HEX 16

class __FlashStringHelper {};
#define F(value) reinterpret_cast<const __FlashStringHelper *>(value)

class HardwareSerial {
 public:
  void begin(unsigned long baud) { baud_ = baud; }

  void setInput(const std::string &input) {
    input_ = input;
    inputPosition_ = 0;
  }

  int available() const { return static_cast<int>(input_.size() - inputPosition_); }
  int availableForWrite() const { return availableForWrite_; }

  int read() {
    if (inputPosition_ >= input_.size()) {
      return -1;
    }
    return static_cast<unsigned char>(input_[inputPosition_++]);
  }

  void print(const __FlashStringHelper *value) {
    output_ += reinterpret_cast<const char *>(value);
  }

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, void>::type print(T value) {
    output_ += std::to_string(static_cast<unsigned long>(value));
  }

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, void>::type print(T value, int base) {
    if (base != HEX) {
      print(value);
      return;
    }

    const char digits[] = "0123456789abcdef";
    char buffer[2 * sizeof(T) + 1] = {};
    size_t position = sizeof(buffer) - 1;
    unsigned long remaining = static_cast<unsigned long>(value);
    do {
      buffer[--position] = digits[remaining & 0x0fU];
      remaining >>= 4;
    } while (remaining != 0);
    output_ += buffer + position;
  }

  void println() { output_ += '\n'; }

  void setAvailableForWrite(int bytes) { availableForWrite_ = bytes; }
  const std::string &output() const { return output_; }
  unsigned long baud() const { return baud_; }
  void clearOutput() { output_.clear(); }

 private:
  std::string input_;
  size_t inputPosition_ = 0;
  std::string output_;
  int availableForWrite_ = 64;
  unsigned long baud_ = 0;
};

struct MockArduinoState {
  int digitalInputs[32];
  int digitalOutputs[32];
  int pinModes[32];
  unsigned long now;
};

extern HardwareSerial Serial;
extern MockArduinoState mockArduino;

inline unsigned long millis() { return mockArduino.now; }
inline void pinMode(uint8_t pin, int mode) { mockArduino.pinModes[pin] = mode; }
inline int digitalRead(uint8_t pin) { return mockArduino.digitalInputs[pin]; }
inline void digitalWrite(uint8_t pin, int value) { mockArduino.digitalOutputs[pin] = value; }

