#include "serial_transport.h"

namespace bobik {

SerialTransport::SerialTransport(HardwareSerial &serial)
    : serial_(serial), line_{}, lineLength_(0), discardingLine_(false) {}

void SerialTransport::resetLine() {
  lineLength_ = 0;
  discardingLine_ = false;
}

SerialPollResult SerialTransport::finishLine(ProtocolMessage &message) {
  if (discardingLine_) {
    resetLine();
    return SerialPollResult::LineOverflow;
  }
  if (lineLength_ == 0) {
    resetLine();
    return SerialPollResult::None;
  }

  const bool parsed = parseSerialMessage(line_, lineLength_, message);
  resetLine();
  return parsed ? SerialPollResult::Message : SerialPollResult::ParseError;
}

SerialPollResult SerialTransport::poll(ProtocolMessage &message) {
  uint8_t processed = 0;
  while (serial_.available() > 0 && processed < MAX_SERIAL_BYTES_PER_LOOP) {
    const char character = static_cast<char>(serial_.read());
    processed++;

    if (character == '\n') {
      const SerialPollResult result = finishLine(message);
      if (result != SerialPollResult::None) {
        return result;
      }
    } else if (character == '\r') {
      continue;
    } else if (character == '\0') {
      discardingLine_ = true;
    } else if (!discardingLine_ && lineLength_ < SERIAL_LINE_CAPACITY - 1) {
      line_[lineLength_++] = character;
    } else {
      discardingLine_ = true;
    }
  }

  return SerialPollResult::None;
}

bool SerialTransport::canWriteProtocolMessage() const {
  return serial_.availableForWrite() >= MIN_SERIAL_BYTES_FOR_PROTOCOL_MESSAGE;
}

void SerialTransport::writeProtocolMessage(const ProtocolMessage &message) {
  serial_.print(F("0x"));
  serial_.print(message.senderId, HEX);
  serial_.print(F("-0x"));
  serial_.print(message.addressee, HEX);
  serial_.print(F("-0x"));
  serial_.print(message.command, HEX);
  serial_.print(F("-0x"));
  serial_.print(message.payload, HEX);
  serial_.println();
}

}  // namespace bobik
