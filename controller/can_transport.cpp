#include "can_transport.h"

namespace bobik {
namespace {

bool hasElapsed(uint32_t now, uint32_t then, uint32_t interval) {
  return static_cast<uint32_t>(now - then) >= interval;
}

}  // namespace

CanTransport::CanTransport()
    : mcp2515_(MCP2515_CS_PIN),
      txQueue_{},
      txQueueHead_(0),
      txQueueTail_(0),
      txQueueCount_(0),
      ready_(false),
      lastInitAttemptMillis_(0),
      lastErrorPollMillis_(0),
      diagnosticHandler_(nullptr) {}

void CanTransport::notify(CanDiagnostic diagnostic, int detail) const {
  if (diagnosticHandler_ != nullptr) {
    diagnosticHandler_(diagnostic, detail);
  }
}

bool CanTransport::begin(uint32_t now, CanDiagnosticHandler diagnosticHandler) {
  diagnosticHandler_ = diagnosticHandler;
  return initialize(now);
}

bool CanTransport::initialize(uint32_t now) {
  lastInitAttemptMillis_ = now;
  bool initialized = true;

  MCP2515::ERROR error = mcp2515_.reset();
  if (error != MCP2515::ERROR_OK) {
    initialized = false;
    notify(CanDiagnostic::Reset, static_cast<int>(error));
  }

  error = mcp2515_.setBitrate(CAN_125KBPS);
  if (error != MCP2515::ERROR_OK) {
    initialized = false;
    notify(CanDiagnostic::Bitrate, static_cast<int>(error));
  }

  error = mcp2515_.setNormalMode();
  if (error != MCP2515::ERROR_OK) {
    initialized = false;
    notify(CanDiagnostic::Mode, static_cast<int>(error));
  }

  ready_ = initialized;
  return initialized;
}

void CanTransport::serviceRecovery(uint32_t now) {
  if (!ready_ && hasElapsed(now, lastInitAttemptMillis_, CAN_RECOVERY_INTERVAL_MS)) {
    initialize(now);
  }
}

bool CanTransport::queue(const ProtocolMessage &message) {
  if (!ready_) {
    notify(CanDiagnostic::NotReady);
    return false;
  }
  if (txQueueCount_ >= CAN_TX_QUEUE_CAPACITY) {
    notify(CanDiagnostic::TxQueueFull);
    return false;
  }

  TxItem &item = txQueue_[txQueueTail_];
  item.frame.can_id = message.senderId;
  item.frame.can_dlc = EXPECTED_CAN_DLC;
  item.frame.data[0] = message.addressee;
  item.frame.data[1] = message.command;
  item.frame.data[2] = message.payload;
  for (uint8_t index = EXPECTED_CAN_DLC; index < CAN_MAX_DLEN; index++) {
    item.frame.data[index] = 0;
  }
  item.busyRetries = 0;
  item.lastAttemptMillis = 0;

  txQueueTail_ = (txQueueTail_ + 1) % CAN_TX_QUEUE_CAPACITY;
  txQueueCount_++;
  return true;
}

void CanTransport::popTxQueue() {
  if (txQueueCount_ == 0) {
    return;
  }
  txQueueHead_ = (txQueueHead_ + 1) % CAN_TX_QUEUE_CAPACITY;
  txQueueCount_--;
}

void CanTransport::serviceTransmit(uint32_t now) {
  if (!ready_ || txQueueCount_ == 0) {
    return;
  }

  TxItem &item = txQueue_[txQueueHead_];
  if (item.busyRetries > 0 &&
      !hasElapsed(now, item.lastAttemptMillis, CAN_TX_BUSY_RETRY_INTERVAL_MS)) {
    return;
  }

  const MCP2515::ERROR error = mcp2515_.sendMessage(&item.frame);
  item.lastAttemptMillis = now;

  if (error == MCP2515::ERROR_OK) {
    popTxQueue();
    return;
  }

  if (error == MCP2515::ERROR_ALLTXBUSY) {
    item.busyRetries++;
    notify(CanDiagnostic::TxBusy, static_cast<int>(error));
    if (item.busyRetries < MAX_CAN_TX_BUSY_RETRIES) {
      return;
    }
  }

  // ERROR_FAILTX can be ambiguous, so do not blindly retry non-idempotent commands.
  notify(CanDiagnostic::TxFailed, static_cast<int>(error));
  popTxQueue();
}

bool CanTransport::isValidFrame(const struct can_frame &frame) const {
  const canid_t unsupportedFlags = frame.can_id & (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_ERR_FLAG);
  return unsupportedFlags == 0 && frame.can_dlc == EXPECTED_CAN_DLC &&
         frame.can_id <= MAX_STANDARD_CAN_ID;
}

CanReceiveResult CanTransport::receive(ProtocolMessage &message) {
  if (!ready_) {
    return CanReceiveResult::NoMessage;
  }

  struct can_frame frame = {};
  const MCP2515::ERROR error = mcp2515_.readMessage(&frame);
  if (error == MCP2515::ERROR_NOMSG) {
    return CanReceiveResult::NoMessage;
  }
  if (error != MCP2515::ERROR_OK) {
    notify(CanDiagnostic::RxFailed, static_cast<int>(error));
    return CanReceiveResult::Error;
  }
  if (!isValidFrame(frame)) {
    notify(CanDiagnostic::InvalidFrame, frame.can_dlc);
    return CanReceiveResult::ConsumedInvalid;
  }

  message.senderId = static_cast<uint16_t>(frame.can_id);
  message.addressee = frame.data[0];
  message.command = frame.data[1];
  message.payload = frame.data[2];
  return CanReceiveResult::Message;
}

void CanTransport::serviceControllerErrors(uint32_t now) {
  if (!ready_ || !hasElapsed(now, lastErrorPollMillis_, CAN_ERROR_POLL_INTERVAL_MS)) {
    return;
  }
  lastErrorPollMillis_ = now;

  const uint8_t flags = mcp2515_.getErrorFlags();
  const uint8_t receiveOverflowFlags = MCP2515::EFLG_RX0OVR | MCP2515::EFLG_RX1OVR;
  if ((flags & receiveOverflowFlags) != 0) {
    notify(CanDiagnostic::RxOverflow, flags);
    mcp2515_.clearRXnOVRFlags();
  }

  if ((flags & MCP2515::EFLG_TXBO) != 0) {
    notify(CanDiagnostic::BusOff, flags);
    ready_ = false;
    lastInitAttemptMillis_ = now;
  }
}

}  // namespace bobik
