#pragma once

#include <Arduino.h>

#include "protocol.h"

namespace bobik {

constexpr size_t DEVICE_ID_SET_BYTES = 32;

struct DeviceIdSet {
  uint8_t bytes[DEVICE_ID_SET_BYTES];
};

struct AppState {
  bool armed;
  bool alarmed;
  DeviceIdSet activeAlarmDeviceIds;
  DeviceIdSet allAlarmDeviceIds;
};

struct StateTransitionResult {
  bool accepted;
  bool alarmStarted;
};

void clearDeviceIdSet(DeviceIdSet &deviceIds);
void addDeviceId(DeviceIdSet &deviceIds, uint8_t deviceId);
void removeDeviceId(DeviceIdSet &deviceIds, uint8_t deviceId);
bool containsDeviceId(const DeviceIdSet &deviceIds, uint8_t deviceId);
void clearAlarmState(AppState &state);
StateTransitionResult processBaseStationMessage(const ProtocolMessage &message, AppState &state);

}  // namespace bobik
