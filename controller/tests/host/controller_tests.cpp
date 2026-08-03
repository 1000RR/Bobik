#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

#include "mocks/mcp2515.h"
#include "mocks/ssd1306.h"

HardwareSerial Serial;
MockArduinoState mockArduino = {};
MockMcp2515State mockMcp2515;
MockSsd1306State mockSsd1306 = {};

#include "../../controller.ino"

namespace {

using namespace bobik;

int failures = 0;
unsigned int diagnosticCounts[static_cast<size_t>(CanDiagnostic::Count)] = {};
int lastDiagnosticDetail = -1;

#define CHECK(condition)                                                                  \
  do {                                                                                    \
    if (!(condition)) {                                                                   \
      std::cerr << __FILE__ << ':' << __LINE__ << ": check failed: " #condition << '\n'; \
      failures++;                                                                         \
    }                                                                                     \
  } while (false)

void resetArduino() {
  mockArduino = MockArduinoState{};
  for (size_t pin = 0; pin < 32; pin++) {
    mockArduino.digitalInputs[pin] = HIGH;
  }
  mockSsd1306 = MockSsd1306State{};
}

void resetMcp() {
  mockMcp2515 = MockMcp2515State{};
  for (size_t index = 0; index < static_cast<size_t>(CanDiagnostic::Count); index++) {
    diagnosticCounts[index] = 0;
  }
  lastDiagnosticDetail = -1;
}

void recordDiagnostic(CanDiagnostic diagnostic, int detail) {
  diagnosticCounts[static_cast<size_t>(diagnostic)]++;
  lastDiagnosticDetail = detail;
}

unsigned int diagnosticCount(CanDiagnostic diagnostic) {
  return diagnosticCounts[static_cast<size_t>(diagnostic)];
}

void expectValid(const char *line, uint16_t sender, uint8_t addressee, uint8_t command,
                 uint8_t payload) {
  ProtocolMessage message = {};
  CHECK(parseSerialMessage(line, std::char_traits<char>::length(line), message));
  CHECK(message.senderId == sender);
  CHECK(message.addressee == addressee);
  CHECK(message.command == command);
  CHECK(message.payload == payload);
}

void expectInvalid(const char *line) {
  ProtocolMessage message = {};
  CHECK(!parseSerialMessage(line, std::char_traits<char>::length(line), message));
}

SerialPollResult pollUntilResult(SerialTransport &transport, ProtocolMessage &message) {
  for (uint8_t attempt = 0; attempt < 16; attempt++) {
    const SerialPollResult result = transport.poll(message);
    if (result != SerialPollResult::None) {
      return result;
    }
  }
  return SerialPollResult::None;
}

can_frame makeFrame(canid_t id, uint8_t dlc, uint8_t addressee = 0x10,
                    uint8_t command = COMMAND_ARM, uint8_t payload = DEVICE_TYPE_HOMEBASE) {
  can_frame frame = {};
  frame.can_id = id;
  frame.can_dlc = dlc;
  frame.data[0] = addressee;
  frame.data[1] = command;
  frame.data[2] = payload;
  return frame;
}

void testProtocolParsing() {
  expectValid("0x14-0xff-0xd1-0x1-", 0x14, 0xff, 0xd1, 0x01);
  expectValid("14-ff-d1-01", 0x14, 0xff, 0xd1, 0x01);
  expectValid("0X7FF-0XFF-0XA0-0XFE", 0x7ff, 0xff, 0xa0, 0xfe);

  expectInvalid("");
  expectInvalid("0x14-0xff-0xd1");
  expectInvalid("0x14-0xff-0xd1-0x1-0x2");
  expectInvalid("0x14--0xd1-0x1");
  expectInvalid("0x14-0xff-0xzz-0x1");
  expectInvalid("-1-0xff-0xd1-0x1");
  expectInvalid("0x800-0xff-0xd1-0x1");
  expectInvalid("0x14-0x100-0xd1-0x1");
  expectInvalid("0x14-0xff-0xd1-0x1--");
  expectInvalid("0x-0xff-0xd1-0x1");
}

void testStateTransitionsAndDeviceIdentity() {
  AppState state = {};
  ProtocolMessage message = {HOME_BASE_CAN_ID, BASE_STATION_ADDRESS, COMMAND_ARM,
                             DEVICE_TYPE_HOMEBASE};

  StateTransitionResult result = processBaseStationMessage(message, state);
  CHECK(result.accepted);
  CHECK(!result.alarmStarted);
  CHECK(state.armed);

  const AppState armedState = state;
  message.senderId = HOME_BASE_CAN_ID + 1;
  CHECK(!processBaseStationMessage(message, state).accepted);
  CHECK(state.armed == armedState.armed);
  message.senderId = HOME_BASE_CAN_ID;
  message.addressee = 0x10;
  CHECK(!processBaseStationMessage(message, state).accepted);
  message.addressee = BASE_STATION_ADDRESS;
  message.payload = 0x02;
  CHECK(!processBaseStationMessage(message, state).accepted);

  message.command = COMMAND_ALARMED_DEVICE;
  message.payload = BROADCAST_ADDRESS;
  CHECK(!processBaseStationMessage(message, state).accepted);
  message.payload = BASE_STATION_ADDRESS;
  CHECK(!processBaseStationMessage(message, state).accepted);

  message.payload = 0x01;
  result = processBaseStationMessage(message, state);
  CHECK(result.accepted && result.alarmStarted);
  message.payload = 0x10;
  result = processBaseStationMessage(message, state);
  CHECK(result.accepted && !result.alarmStarted);
  CHECK(containsDeviceId(state.activeAlarmDeviceIds, 0x01));
  CHECK(containsDeviceId(state.activeAlarmDeviceIds, 0x10));

  message.command = COMMAND_DEVICE_CLEAR;
  message.payload = 0x01;
  CHECK(processBaseStationMessage(message, state).accepted);
  CHECK(!containsDeviceId(state.activeAlarmDeviceIds, 0x01));
  CHECK(containsDeviceId(state.activeAlarmDeviceIds, 0x10));
  CHECK(containsDeviceId(state.allAlarmDeviceIds, 0x01));
  CHECK(containsDeviceId(state.allAlarmDeviceIds, 0x10));

  message.command = COMMAND_STOP_ALARM;
  message.payload = DEVICE_TYPE_HOMEBASE;
  CHECK(processBaseStationMessage(message, state).accepted);
  CHECK(!state.alarmed);
  CHECK(!containsDeviceId(state.allAlarmDeviceIds, 0x01));

  message.command = COMMAND_DISARM;
  CHECK(processBaseStationMessage(message, state).accepted);
  CHECK(!state.armed);
}

void testRendering() {
  AppState state = {};
  DisplayRows rows = {};
  renderDisplayRows(state, rows);
  CHECK(std::string(rows.armed) == "DISABLED             ");
  CHECK(std::string(rows.alarm) == "      NO ALARM       ");
  CHECK(std::string(rows.activeDevices) == "                     ");

  state.armed = true;
  state.alarmed = true;
  addDeviceId(state.activeAlarmDeviceIds, 0xfe);
  addDeviceId(state.activeAlarmDeviceIds, 0x10);
  addDeviceId(state.activeAlarmDeviceIds, 0x01);
  addDeviceId(state.activeAlarmDeviceIds, 0xab);
  renderDisplayRows(state, rows);
  CHECK(std::string(rows.armed) == "ENABLED              ");
  CHECK(std::string(rows.alarm) == "        ALARM        ");
  CHECK(std::string(rows.activeDevices) == "0x1 0x10 0xab 0xfe   ");
}

void testSerialFramingAndBackpressure() {
  HardwareSerial serial;
  SerialTransport transport(serial);
  ProtocolMessage message = {};

  serial.setInput("0x14-0xff-0xd1-0x1-\ninvalid\n");
  CHECK(pollUntilResult(transport, message) == SerialPollResult::Message);
  CHECK(message.command == COMMAND_ARM);
  CHECK(pollUntilResult(transport, message) == SerialPollResult::ParseError);

  serial.setInput("0x14-0xff-0xd1");
  CHECK(transport.poll(message) == SerialPollResult::None);
  serial.setInput("0x14-0xff-0xd1\n");
  CHECK(pollUntilResult(transport, message) == SerialPollResult::ParseError);

  serial.setInput(std::string(SERIAL_LINE_CAPACITY, 'a') + "\n");
  CHECK(pollUntilResult(transport, message) == SerialPollResult::LineOverflow);

  serial.setInput(std::string("0x14-0xff") + '\0' + "-0xd1-0x1\n");
  CHECK(pollUntilResult(transport, message) == SerialPollResult::LineOverflow);

  serial.setAvailableForWrite(MIN_SERIAL_BYTES_FOR_PROTOCOL_MESSAGE - 1);
  CHECK(!transport.canWriteProtocolMessage());
  serial.setAvailableForWrite(MIN_SERIAL_BYTES_FOR_PROTOCOL_MESSAGE);
  CHECK(transport.canWriteProtocolMessage());

  serial.clearOutput();
  transport.writeProtocolMessage({0x14, 0xff, 0xd1, 0x01});
  CHECK(serial.output() == "0x14-0xff-0xd1-0x1\n");

  HardwareSerial rebootedSerial;
  SerialTransport beforeReboot(rebootedSerial);
  rebootedSerial.setInput("0x14-");
  CHECK(beforeReboot.poll(message) == SerialPollResult::None);
  SerialTransport afterReboot(rebootedSerial);
  rebootedSerial.setInput("0x14-0xff-0xd1-0x1\n");
  CHECK(pollUntilResult(afterReboot, message) == SerialPollResult::Message);
}

void testCanQueueAndTransmitFailures() {
  resetMcp();
  CanTransport transport;
  CHECK(transport.begin(0, recordDiagnostic));
  CHECK(mockMcp2515.chipSelectPin == MCP2515_CS_PIN);

  const ProtocolMessage message = {HOME_BASE_CAN_ID, 0x10, COMMAND_ARM,
                                   DEVICE_TYPE_HOMEBASE};
  for (uint8_t index = 0; index < CAN_TX_QUEUE_CAPACITY; index++) {
    CHECK(transport.queue(message));
  }
  CHECK(!transport.queue(message));
  CHECK(diagnosticCount(CanDiagnostic::TxQueueFull) == 1);

  transport.serviceTransmit(0);
  CHECK(mockMcp2515.sentFrameCount == 1);
  CHECK(mockMcp2515.sentFrames[0].can_id == HOME_BASE_CAN_ID);
  CHECK(mockMcp2515.sentFrames[0].can_dlc == EXPECTED_CAN_DLC);
  CHECK(mockMcp2515.sentFrames[0].data[0] == 0x10);

  resetMcp();
  CanTransport busyTransport;
  CHECK(busyTransport.begin(0, recordDiagnostic));
  for (uint8_t index = 0; index < MAX_CAN_TX_BUSY_RETRIES; index++) {
    mockMcp2515.sendResults[index] = MCP2515::ERROR_ALLTXBUSY;
  }
  mockMcp2515.sendResultCount = MAX_CAN_TX_BUSY_RETRIES;
  CHECK(busyTransport.queue(message));
  for (uint8_t attempt = 0; attempt < MAX_CAN_TX_BUSY_RETRIES; attempt++) {
    busyTransport.serviceTransmit(attempt * CAN_TX_BUSY_RETRY_INTERVAL_MS);
  }
  CHECK(mockMcp2515.sentFrameCount == MAX_CAN_TX_BUSY_RETRIES);
  CHECK(diagnosticCount(CanDiagnostic::TxBusy) == MAX_CAN_TX_BUSY_RETRIES);
  CHECK(diagnosticCount(CanDiagnostic::TxFailed) == 1);
  busyTransport.serviceTransmit(1000);
  CHECK(mockMcp2515.sentFrameCount == MAX_CAN_TX_BUSY_RETRIES);
}

void testCanReceiveValidationAndBursts() {
  resetMcp();
  CanTransport transport;
  CHECK(transport.begin(0, recordDiagnostic));

  const can_frame frames[] = {
      makeFrame(0x10, 0),
      makeFrame(0x10, 1),
      makeFrame(0x10, 2),
      makeFrame(0x10, 4),
      makeFrame(CAN_EFF_FLAG | 0x10, EXPECTED_CAN_DLC),
      makeFrame(CAN_RTR_FLAG | 0x10, EXPECTED_CAN_DLC),
      makeFrame(CAN_ERR_FLAG | 0x10, EXPECTED_CAN_DLC),
      makeFrame(MAX_STANDARD_CAN_ID + 1UL, EXPECTED_CAN_DLC),
      makeFrame(0x01, EXPECTED_CAN_DLC, 0xff, COMMAND_ALARMED_DEVICE, 0x01),
      makeFrame(0x10, EXPECTED_CAN_DLC),
      makeFrame(0x11, EXPECTED_CAN_DLC),
  };
  const size_t frameCount = sizeof(frames) / sizeof(frames[0]);
  for (size_t index = 0; index < frameCount; index++) {
    mockMcp2515.receiveResults[index] = MCP2515::ERROR_OK;
    mockMcp2515.receiveFrames[index] = frames[index];
  }
  mockMcp2515.receiveCount = frameCount;

  ProtocolMessage message = {};
  for (uint8_t index = 0; index < 8; index++) {
    CHECK(transport.receive(message) == CanReceiveResult::ConsumedInvalid);
  }
  CHECK(diagnosticCount(CanDiagnostic::InvalidFrame) == 8);

  CHECK(transport.receive(message) == CanReceiveResult::Message);
  CHECK(message.senderId == 0x01);
  CHECK(message.payload == 0x01);
  CHECK(transport.receive(message) == CanReceiveResult::Message);
  CHECK(message.senderId == 0x10);
  CHECK(transport.receive(message) == CanReceiveResult::Message);
  CHECK(message.senderId == 0x11);
  CHECK(transport.receive(message) == CanReceiveResult::NoMessage);

  mockMcp2515.receiveCount++;
  mockMcp2515.receiveResults[mockMcp2515.receiveIndex] = MCP2515::ERROR_FAIL;
  CHECK(transport.receive(message) == CanReceiveResult::Error);
  CHECK(diagnosticCount(CanDiagnostic::RxFailed) == 1);
}

void testCanControllerErrorsAndRolloverRecovery() {
  resetMcp();
  CanTransport transport;
  CHECK(transport.begin(0, recordDiagnostic));
  mockMcp2515.errorFlags = MCP2515::EFLG_RX0OVR | MCP2515::EFLG_TXBO;
  transport.serviceControllerErrors(CAN_ERROR_POLL_INTERVAL_MS);
  CHECK(diagnosticCount(CanDiagnostic::RxOverflow) == 1);
  CHECK(diagnosticCount(CanDiagnostic::BusOff) == 1);
  CHECK(mockMcp2515.clearOverflowCalls == 1);
  CHECK(!transport.queue({HOME_BASE_CAN_ID, 0x10, COMMAND_ARM, DEVICE_TYPE_HOMEBASE}));
  CHECK(diagnosticCount(CanDiagnostic::NotReady) == 1);

  transport.serviceRecovery(CAN_ERROR_POLL_INTERVAL_MS + CAN_RECOVERY_INTERVAL_MS - 1);
  CHECK(mockMcp2515.resetCalls == 1);
  transport.serviceRecovery(CAN_ERROR_POLL_INTERVAL_MS + CAN_RECOVERY_INTERVAL_MS);
  CHECK(mockMcp2515.resetCalls == 2);
  CHECK(transport.queue({HOME_BASE_CAN_ID, 0x10, COMMAND_ARM, DEVICE_TYPE_HOMEBASE}));

  resetMcp();
  mockMcp2515.resetResult = MCP2515::ERROR_FAILINIT;
  CanTransport rolloverTransport;
  const uint32_t nearRollover = std::numeric_limits<uint32_t>::max() - 15U;
  CHECK(!rolloverTransport.begin(nearRollover, recordDiagnostic));
  mockMcp2515.resetResult = MCP2515::ERROR_OK;
  rolloverTransport.serviceRecovery(nearRollover + CAN_RECOVERY_INTERVAL_MS - 1U);
  CHECK(mockMcp2515.resetCalls == 1);
  rolloverTransport.serviceRecovery(nearRollover + CAN_RECOVERY_INTERVAL_MS);
  CHECK(mockMcp2515.resetCalls == 2);
}

void testButtonBounceAndTimerRollover() {
  resetArduino();
  DebouncedButton button(ARM_BUTTON_PIN, BUTTON_DEBOUNCE_INTERVAL_MS);
  button.begin(0);

  mockArduino.digitalInputs[ARM_BUTTON_PIN] = LOW;
  CHECK(!button.pollPressed(5));
  mockArduino.digitalInputs[ARM_BUTTON_PIN] = HIGH;
  CHECK(!button.pollPressed(10));
  mockArduino.digitalInputs[ARM_BUTTON_PIN] = LOW;
  CHECK(!button.pollPressed(15));
  CHECK(!button.pollPressed(54));
  CHECK(button.pollPressed(55));
  CHECK(!button.pollPressed(56));

  mockArduino.digitalInputs[ARM_BUTTON_PIN] = HIGH;
  CHECK(!button.pollPressed(60));
  CHECK(!button.pollPressed(100));

  const uint32_t nearRollover = std::numeric_limits<uint32_t>::max() - 15U;
  DebouncedButton rolloverButton(ARM_BUTTON_PIN, BUTTON_DEBOUNCE_INTERVAL_MS);
  mockArduino.digitalInputs[ARM_BUTTON_PIN] = HIGH;
  rolloverButton.begin(nearRollover);
  mockArduino.digitalInputs[ARM_BUTTON_PIN] = LOW;
  CHECK(!rolloverButton.pollPressed(nearRollover));
  CHECK(!rolloverButton.pollPressed(nearRollover + BUTTON_DEBOUNCE_INTERVAL_MS - 1U));
  CHECK(rolloverButton.pollPressed(nearRollover + BUTTON_DEBOUNCE_INTERVAL_MS));
}

void testUiTimersAcrossRollover() {
  resetArduino();
  const uint32_t nearRollover = std::numeric_limits<uint32_t>::max() - 15U;
  AppState state = {};
  ControllerUi ui;
  ui.begin(nearRollover, state);
  CHECK(mockSsd1306.printCalls == 1);
  ui.service(nearRollover + SPLASH_DURATION_MS - 1U, state);
  CHECK(mockSsd1306.normalCalls == 0);
  ui.service(nearRollover + SPLASH_DURATION_MS, state);
  CHECK(mockSsd1306.normalCalls == 1);
  CHECK(mockSsd1306.printCalls == 5);

  ControllerUi blinkUi;
  blinkUi.begin(nearRollover, state);
  state.alarmed = true;
  blinkUi.onStateChanged(nearRollover, state, true);
  CHECK(mockArduino.digitalOutputs[DISARMED_LED_PIN] == HIGH);
  blinkUi.service(nearRollover + ALARM_BLINK_INTERVAL_MS, state);
  CHECK(mockArduino.digitalOutputs[DISARMED_LED_PIN] == LOW);
}

void testSketchOrchestration() {
  resetArduino();
  resetMcp();
  Serial = HardwareSerial{};
  appState = AppState{};
  setup();
  CHECK(Serial.baud() == 115200);

  Serial.setInput("0x14-0xff-0xd1-0x1-\n");
  mockArduino.now = 1;
  loop();
  CHECK(appState.armed);

  Serial.setInput("0x14-0x10-0xd1-0x1-\n");
  mockArduino.now = 2;
  loop();
  CHECK(mockMcp2515.sentFrameCount == 1);

  mockMcp2515.receiveFrames[0] =
      makeFrame(0x10, EXPECTED_CAN_DLC, BASE_STATION_ADDRESS, COMMAND_ALARMED_DEVICE, 0x10);
  mockMcp2515.receiveResults[0] = MCP2515::ERROR_OK;
  mockMcp2515.receiveCount = 1;
  Serial.clearOutput();
  mockArduino.now = 3;
  loop();
  CHECK(Serial.output() == "0x10-0xff-0xa0-0x10\n");
}

}  // namespace

int main() {
  testProtocolParsing();
  testStateTransitionsAndDeviceIdentity();
  testRendering();
  testSerialFramingAndBackpressure();
  testCanQueueAndTransmitFailures();
  testCanReceiveValidationAndBursts();
  testCanControllerErrorsAndRolloverRecovery();
  testButtonBounceAndTimerRollover();
  testUiTimersAcrossRollover();
  testSketchOrchestration();

  if (failures != 0) {
    std::cerr << failures << " controller test(s) failed\n";
    return 1;
  }

  std::cout << "All controller host tests passed\n";
  return 0;
}
