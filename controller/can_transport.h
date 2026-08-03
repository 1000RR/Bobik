#pragma once

#include <Arduino.h>
#include <mcp2515.h>

#include "controller_config.h"
#include "protocol.h"

namespace bobik {

enum class CanDiagnostic : uint8_t {
  Reset,
  Bitrate,
  Mode,
  NotReady,
  TxQueueFull,
  TxBusy,
  TxFailed,
  RxFailed,
  InvalidFrame,
  RxOverflow,
  BusOff,
  Count,
};

using CanDiagnosticHandler = void (*)(CanDiagnostic diagnostic, int detail);

enum class CanReceiveResult : uint8_t {
  NoMessage,
  Message,
  ConsumedInvalid,
  Error,
};

class CanTransport {
 public:
  CanTransport();

  bool begin(uint32_t now, CanDiagnosticHandler diagnosticHandler);
  bool queue(const ProtocolMessage &message);
  void serviceRecovery(uint32_t now);
  void serviceTransmit(uint32_t now);
  CanReceiveResult receive(ProtocolMessage &message);
  void serviceControllerErrors(uint32_t now);

 private:
  struct TxItem {
    struct can_frame frame;
    uint8_t busyRetries;
    uint32_t lastAttemptMillis;
  };

  bool initialize(uint32_t now);
  void notify(CanDiagnostic diagnostic, int detail = -1) const;
  void popTxQueue();
  bool isValidFrame(const struct can_frame &frame) const;

  MCP2515 mcp2515_;
  TxItem txQueue_[CAN_TX_QUEUE_CAPACITY];
  uint8_t txQueueHead_;
  uint8_t txQueueTail_;
  uint8_t txQueueCount_;
  bool ready_;
  uint32_t lastInitAttemptMillis_;
  uint32_t lastErrorPollMillis_;
  CanDiagnosticHandler diagnosticHandler_;
};

}  // namespace bobik
