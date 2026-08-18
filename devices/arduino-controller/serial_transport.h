#pragma once

#include <Arduino.h>

#include "controller_config.h"
#include "protocol.h"

namespace bobik {

enum class SerialPollResult : uint8_t {
  None,
  Message,
  ParseError,
  LineOverflow,
};

class SerialTransport {
 public:
  explicit SerialTransport(HardwareSerial &serial);

  SerialPollResult poll(ProtocolMessage &message);
  bool canWriteProtocolMessage() const;
  void writeProtocolMessage(const ProtocolMessage &message);

 private:
  void resetLine();
  SerialPollResult finishLine(ProtocolMessage &message);

  HardwareSerial &serial_;
  char line_[SERIAL_LINE_CAPACITY];
  size_t lineLength_;
  bool discardingLine_;
};

}  // namespace bobik
