#pragma once

#include <Arduino.h>

namespace bobik {

constexpr uint16_t HOME_BASE_CAN_ID = 0x14;
constexpr uint8_t BROADCAST_ADDRESS = 0x00;
constexpr uint8_t BASE_STATION_ADDRESS = 0xFF;
constexpr uint8_t DEVICE_TYPE_HOMEBASE = 0x01;

constexpr uint8_t COMMAND_ARM = 0xD1;
constexpr uint8_t COMMAND_DISARM = 0xD0;
constexpr uint8_t COMMAND_ALARMED_DEVICE = 0xA0;
constexpr uint8_t COMMAND_DEVICE_CLEAR = 0xB0;
constexpr uint8_t COMMAND_STOP_ALARM = 0xC0;
constexpr uint8_t COMMAND_ARM_TOGGLE = 0xEE;

constexpr uint16_t MAX_STANDARD_CAN_ID = 0x7FF;
constexpr uint8_t EXPECTED_CAN_DLC = 3;

/*
 * Serial envelope: <senderCanId>-<addressee>-<command>-<payload>\n
 *
 * Payload is the CAN device type for forwarded messages. For private A0/B0
 * base-station commands it is the alarmed device ID.
 */
struct ProtocolMessage {
  uint16_t senderId;
  uint8_t addressee;
  uint8_t command;
  uint8_t payload;
};

bool parseSerialMessage(const char *line, size_t length, ProtocolMessage &message);
bool isValidAlarmDeviceId(uint8_t deviceId);

}  // namespace bobik
