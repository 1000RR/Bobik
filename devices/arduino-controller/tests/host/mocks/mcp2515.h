#pragma once

#include "Arduino.h"

using canid_t = uint32_t;

#define CAN_EFF_FLAG 0x80000000UL
#define CAN_RTR_FLAG 0x40000000UL
#define CAN_ERR_FLAG 0x20000000UL
#define CAN_SFF_MASK 0x000007FFUL
#define CAN_MAX_DLEN 8

struct can_frame {
  canid_t can_id;
  uint8_t can_dlc;
  uint8_t data[CAN_MAX_DLEN];
};

enum CAN_SPEED { CAN_125KBPS };

struct MockMcp2515State;
extern MockMcp2515State mockMcp2515;

class MCP2515 {
 public:
  enum ERROR {
    ERROR_OK,
    ERROR_FAIL,
    ERROR_ALLTXBUSY,
    ERROR_FAILINIT,
    ERROR_FAILTX,
    ERROR_NOMSG,
  };

  enum EFLG : uint8_t {
    EFLG_TXBO = 1 << 5,
    EFLG_RX0OVR = 1 << 6,
    EFLG_RX1OVR = 1 << 7,
  };

  explicit MCP2515(uint8_t chipSelectPin);
  ERROR reset();
  ERROR setBitrate(CAN_SPEED speed);
  ERROR setNormalMode();
  ERROR sendMessage(const can_frame *frame);
  ERROR readMessage(can_frame *frame);
  uint8_t getErrorFlags();
  void clearRXnOVRFlags();
};

struct MockMcp2515State {
  MCP2515::ERROR resetResult = MCP2515::ERROR_OK;
  MCP2515::ERROR bitrateResult = MCP2515::ERROR_OK;
  MCP2515::ERROR modeResult = MCP2515::ERROR_OK;
  MCP2515::ERROR sendResults[64] = {};
  size_t sendResultCount = 0;
  size_t sendResultIndex = 0;
  can_frame sentFrames[64] = {};
  size_t sentFrameCount = 0;
  MCP2515::ERROR receiveResults[64] = {};
  can_frame receiveFrames[64] = {};
  size_t receiveCount = 0;
  size_t receiveIndex = 0;
  uint8_t errorFlags = 0;
  uint8_t chipSelectPin = 0;
  unsigned int resetCalls = 0;
  unsigned int bitrateCalls = 0;
  unsigned int modeCalls = 0;
  unsigned int clearOverflowCalls = 0;
};

inline MCP2515::MCP2515(uint8_t chipSelectPin) {
  mockMcp2515.chipSelectPin = chipSelectPin;
}

inline MCP2515::ERROR MCP2515::reset() {
  mockMcp2515.resetCalls++;
  return mockMcp2515.resetResult;
}

inline MCP2515::ERROR MCP2515::setBitrate(CAN_SPEED) {
  mockMcp2515.bitrateCalls++;
  return mockMcp2515.bitrateResult;
}

inline MCP2515::ERROR MCP2515::setNormalMode() {
  mockMcp2515.modeCalls++;
  return mockMcp2515.modeResult;
}

inline MCP2515::ERROR MCP2515::sendMessage(const can_frame *frame) {
  if (mockMcp2515.sentFrameCount < 64) {
    mockMcp2515.sentFrames[mockMcp2515.sentFrameCount++] = *frame;
  }
  if (mockMcp2515.sendResultIndex < mockMcp2515.sendResultCount) {
    return mockMcp2515.sendResults[mockMcp2515.sendResultIndex++];
  }
  return ERROR_OK;
}

inline MCP2515::ERROR MCP2515::readMessage(can_frame *frame) {
  if (mockMcp2515.receiveIndex >= mockMcp2515.receiveCount) {
    return ERROR_NOMSG;
  }
  const size_t index = mockMcp2515.receiveIndex++;
  const ERROR result = mockMcp2515.receiveResults[index];
  if (result == ERROR_OK) {
    *frame = mockMcp2515.receiveFrames[index];
  }
  return result;
}

inline uint8_t MCP2515::getErrorFlags() { return mockMcp2515.errorFlags; }

inline void MCP2515::clearRXnOVRFlags() {
  mockMcp2515.clearOverflowCalls++;
  mockMcp2515.errorFlags &= static_cast<uint8_t>(~(EFLG_RX0OVR | EFLG_RX1OVR));
}

