#include "can_transport.h"
#include "controller_config.h"
#include "controller_state.h"
#include "controller_ui.h"
#include "debounced_button.h"
#include "protocol.h"
#include "serial_transport.h"

// Keep Arduino's prototype generator from placing these inside the anonymous namespace.
void setup();
void loop();

namespace {

using namespace bobik;

constexpr size_t CAN_DIAGNOSTIC_COUNT = static_cast<size_t>(CanDiagnostic::Count);

AppState appState = {};
SerialTransport serialTransport(Serial);
CanTransport canTransport;
DebouncedButton armButton(ARM_BUTTON_PIN, BUTTON_DEBOUNCE_INTERVAL_MS);
ControllerUi controllerUi;

unsigned long serialParseErrorCount = 0;
unsigned long serialOverflowCount = 0;
unsigned long rejectedLocalMessageCount = 0;
unsigned long canDiagnosticCounts[CAN_DIAGNOSTIC_COUNT] = {};

bool shouldReportDiagnostic(unsigned long count) {
  return count == 1 || (count % 16) == 0;
}

void reportDiagnostic(const __FlashStringHelper *category, unsigned long count, int detail) {
  if (!shouldReportDiagnostic(count)) {
    return;
  }

  // alarm.py deliberately ignores serial lines beginning with ">>>".
  Serial.print(F(">>>ERR "));
  Serial.print(category);
  Serial.print(F(" count="));
  Serial.print(count);
  if (detail >= 0) {
    Serial.print(F(" detail="));
    Serial.print(detail);
  }
  Serial.println();
}

const __FlashStringHelper *canDiagnosticName(CanDiagnostic diagnostic) {
  switch (diagnostic) {
    case CanDiagnostic::Reset:
      return F("CAN_RESET");
    case CanDiagnostic::Bitrate:
      return F("CAN_BITRATE");
    case CanDiagnostic::Mode:
      return F("CAN_MODE");
    case CanDiagnostic::NotReady:
      return F("CAN_NOT_READY");
    case CanDiagnostic::TxQueueFull:
      return F("CAN_TX_QUEUE_FULL");
    case CanDiagnostic::TxBusy:
      return F("CAN_TX_BUSY");
    case CanDiagnostic::TxFailed:
      return F("CAN_TX_FAILED");
    case CanDiagnostic::RxFailed:
      return F("CAN_RX");
    case CanDiagnostic::InvalidFrame:
      return F("CAN_FRAME");
    case CanDiagnostic::RxOverflow:
      return F("CAN_RX_OVERFLOW");
    case CanDiagnostic::BusOff:
      return F("CAN_BUS_OFF");
    case CanDiagnostic::Count:
      return F("CAN_UNKNOWN");
  }
  return F("CAN_UNKNOWN");
}

void handleCanDiagnostic(CanDiagnostic diagnostic, int detail) {
  const size_t index = static_cast<size_t>(diagnostic);
  if (index >= CAN_DIAGNOSTIC_COUNT) {
    return;
  }

  const unsigned long count = ++canDiagnosticCounts[index];
  reportDiagnostic(canDiagnosticName(diagnostic), count, detail);
}

void handleHostMessage(const ProtocolMessage &message, uint32_t now) {
  if (message.addressee != BASE_STATION_ADDRESS) {
    canTransport.queue(message);
    return;
  }

  const StateTransitionResult result = processBaseStationMessage(message, appState);
  if (!result.accepted) {
    rejectedLocalMessageCount++;
    reportDiagnostic(F("LOCAL_COMMAND"), rejectedLocalMessageCount, message.command);
    return;
  }

  controllerUi.onStateChanged(now, appState, result.alarmStarted);
}

void serviceSerialInput(uint32_t now) {
  ProtocolMessage message = {};
  switch (serialTransport.poll(message)) {
    case SerialPollResult::Message:
      handleHostMessage(message, now);
      break;
    case SerialPollResult::ParseError:
      serialParseErrorCount++;
      reportDiagnostic(F("SERIAL_PARSE"), serialParseErrorCount, -1);
      break;
    case SerialPollResult::LineOverflow:
      serialOverflowCount++;
      reportDiagnostic(F("SERIAL_OVERFLOW"), serialOverflowCount, -1);
      break;
    case SerialPollResult::None:
      break;
  }
}

void serviceArmButton(uint32_t now) {
  if (!armButton.pollPressed(now)) {
    return;
  }

  const ProtocolMessage message = {
      HOME_BASE_CAN_ID,
      static_cast<uint8_t>(HOME_BASE_CAN_ID),
      COMMAND_ARM_TOGGLE,
      DEVICE_TYPE_HOMEBASE,
  };
  serialTransport.writeProtocolMessage(message);
}

void serviceCanReceive() {
  for (uint8_t count = 0; count < MAX_CAN_RX_FRAMES_PER_LOOP; count++) {
    if (!serialTransport.canWriteProtocolMessage()) {
      return;
    }

    ProtocolMessage message = {};
    const CanReceiveResult result = canTransport.receive(message);
    if (result == CanReceiveResult::Message) {
      serialTransport.writeProtocolMessage(message);
    } else if (result == CanReceiveResult::ConsumedInvalid) {
      continue;
    } else {
      return;
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  const uint32_t now = millis();

  armButton.begin(now);
  controllerUi.begin(now, appState);
  canTransport.begin(now, handleCanDiagnostic);
}

void loop() {
  const uint32_t now = millis();

  serviceSerialInput(now);
  serviceArmButton(now);
  canTransport.serviceRecovery(now);
  canTransport.serviceTransmit(now);
  serviceCanReceive();
  canTransport.serviceControllerErrors(now);
  controllerUi.service(now, appState);
}
