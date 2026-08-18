#include "controller_state.h"

namespace bobik {

void clearDeviceIdSet(DeviceIdSet &deviceIds) {
  for (size_t index = 0; index < DEVICE_ID_SET_BYTES; index++) {
    deviceIds.bytes[index] = 0;
  }
}

void addDeviceId(DeviceIdSet &deviceIds, uint8_t deviceId) {
  const uint8_t mask = static_cast<uint8_t>(1U << (deviceId & 0x07));
  deviceIds.bytes[deviceId >> 3] |= mask;
}

void removeDeviceId(DeviceIdSet &deviceIds, uint8_t deviceId) {
  const uint8_t mask = static_cast<uint8_t>(1U << (deviceId & 0x07));
  deviceIds.bytes[deviceId >> 3] &= static_cast<uint8_t>(~mask);
}

bool containsDeviceId(const DeviceIdSet &deviceIds, uint8_t deviceId) {
  const uint8_t mask = static_cast<uint8_t>(1U << (deviceId & 0x07));
  return (deviceIds.bytes[deviceId >> 3] & mask) != 0;
}

void clearAlarmState(AppState &state) {
  state.alarmed = false;
  clearDeviceIdSet(state.activeAlarmDeviceIds);
  clearDeviceIdSet(state.allAlarmDeviceIds);
}

StateTransitionResult processBaseStationMessage(const ProtocolMessage &message, AppState &state) {
  if (message.senderId != HOME_BASE_CAN_ID || message.addressee != BASE_STATION_ADDRESS) {
    return {false, false};
  }

  switch (message.command) {
    case COMMAND_ARM:
      if (message.payload != DEVICE_TYPE_HOMEBASE) {
        return {false, false};
      }
      state.armed = true;
      return {true, false};

    case COMMAND_DISARM:
      if (message.payload != DEVICE_TYPE_HOMEBASE) {
        return {false, false};
      }
      state.armed = false;
      clearAlarmState(state);
      return {true, false};

    case COMMAND_ALARMED_DEVICE: {
      if (!isValidAlarmDeviceId(message.payload)) {
        return {false, false};
      }
      const bool alarmStarted = !state.alarmed;
      state.alarmed = true;
      addDeviceId(state.activeAlarmDeviceIds, message.payload);
      addDeviceId(state.allAlarmDeviceIds, message.payload);
      return {true, alarmStarted};
    }

    case COMMAND_DEVICE_CLEAR:
      if (!isValidAlarmDeviceId(message.payload)) {
        return {false, false};
      }
      removeDeviceId(state.activeAlarmDeviceIds, message.payload);
      return {true, false};

    case COMMAND_STOP_ALARM:
      if (message.payload != DEVICE_TYPE_HOMEBASE) {
        return {false, false};
      }
      clearAlarmState(state);
      return {true, false};

    default:
      return {false, false};
  }
}

}  // namespace bobik
