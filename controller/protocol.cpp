#include "protocol.h"

namespace bobik {
namespace {

bool parseHexField(const char *field, size_t length, unsigned long maximum,
                   unsigned long &value) {
  if (length == 0) {
    return false;
  }

  size_t index = 0;
  if (length >= 2 && field[0] == '0' && (field[1] == 'x' || field[1] == 'X')) {
    index = 2;
  }
  if (index == length) {
    return false;
  }

  unsigned long parsed = 0;
  for (; index < length; index++) {
    const char character = field[index];
    uint8_t digit;
    if (character >= '0' && character <= '9') {
      digit = character - '0';
    } else if (character >= 'a' && character <= 'f') {
      digit = character - 'a' + 10;
    } else if (character >= 'A' && character <= 'F') {
      digit = character - 'A' + 10;
    } else {
      return false;
    }

    if (parsed > (maximum - digit) / 16) {
      return false;
    }
    parsed = parsed * 16 + digit;
  }

  value = parsed;
  return true;
}

}  // namespace

bool parseSerialMessage(const char *line, size_t length, ProtocolMessage &message) {
  if (length == 0) {
    return false;
  }

  // Keep compatibility with alarm.py's current "field-field-field-field-" output.
  if (line[length - 1] == '-') {
    length--;
  }
  if (length == 0) {
    return false;
  }

  unsigned long fields[4] = {0, 0, 0, 0};
  size_t fieldStart = 0;
  uint8_t fieldIndex = 0;

  for (size_t index = 0; index <= length; index++) {
    if (index != length && line[index] != '-') {
      continue;
    }
    if (fieldIndex >= 4 || index == fieldStart) {
      return false;
    }

    const unsigned long maximum = fieldIndex == 0 ? MAX_STANDARD_CAN_ID : 0xFF;
    if (!parseHexField(line + fieldStart, index - fieldStart, maximum, fields[fieldIndex])) {
      return false;
    }

    fieldIndex++;
    fieldStart = index + 1;
  }

  if (fieldIndex != 4) {
    return false;
  }

  message.senderId = static_cast<uint16_t>(fields[0]);
  message.addressee = static_cast<uint8_t>(fields[1]);
  message.command = static_cast<uint8_t>(fields[2]);
  message.payload = static_cast<uint8_t>(fields[3]);
  return true;
}

bool isValidAlarmDeviceId(uint8_t deviceId) {
  return deviceId != BROADCAST_ADDRESS && deviceId != BASE_STATION_ADDRESS;
}

}  // namespace bobik
