#include <SPI.h>
#include <mcp2515.h>
#include <ssd1306.h>

/* Hardware and protocol constants */
constexpr uint8_t ARMED_LED_PIN = 3;
constexpr uint8_t DISARMED_LED_PIN = 2;
constexpr uint8_t ARM_BUTTON_PIN = 9;
constexpr uint8_t MCP2515_CS_PIN = 10;

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

/* Scheduling and bounded-work constants */
constexpr uint32_t OLED_REFRESH_INTERVAL_MS = 100;
constexpr uint32_t ALARM_BLINK_INTERVAL_MS = 500;
constexpr uint32_t BUTTON_DEBOUNCE_INTERVAL_MS = 40;
constexpr uint32_t SPLASH_DURATION_MS = 3000;
constexpr uint32_t CAN_RECOVERY_INTERVAL_MS = 5000;
constexpr uint32_t CAN_TX_BUSY_RETRY_INTERVAL_MS = 5;
constexpr uint32_t CAN_ERROR_POLL_INTERVAL_MS = 100;

constexpr uint8_t MAX_SERIAL_BYTES_PER_LOOP = 32;
constexpr uint8_t MAX_CAN_RX_FRAMES_PER_LOOP = 3;
constexpr uint8_t MIN_SERIAL_BYTES_FOR_CAN_FRAME = 24;
constexpr uint8_t CAN_TX_QUEUE_CAPACITY = 8;
constexpr uint8_t MAX_CAN_TX_BUSY_RETRIES = 20;
constexpr size_t SERIAL_LINE_CAPACITY = 64;

/*
 * Serial envelope: <senderCanId>-<addressee>-<command>-<payload>\n
 *
 * The payload is the CAN device type for forwarded messages. For the private
 * A0/B0 base-station commands it is the alarmed device ID. The parser accepts
 * the legacy Python trailing '-' as well as the canonical four-field form.
 *
 * CAN payload: data[0] addressee, data[1] command, data[2] device type.
 */
struct ProtocolMessage {
  uint16_t senderId;
  uint8_t addressee;
  uint8_t command;
  uint8_t payload;
};

struct CanTxItem {
  struct can_frame frame;
  uint8_t busyRetries;
  uint32_t lastAttemptMillis;
};

MCP2515 mcp2515(MCP2515_CS_PIN);

char serialLine[SERIAL_LINE_CAPACITY];
size_t serialLineLength = 0;
bool discardingSerialLine = false;

CanTxItem canTxQueue[CAN_TX_QUEUE_CAPACITY];
uint8_t canTxQueueHead = 0;
uint8_t canTxQueueTail = 0;
uint8_t canTxQueueCount = 0;
bool canReady = false;

int stableArmButtonState = HIGH;
int lastRawArmButtonState = HIGH;
uint32_t rawButtonStateChangedMillis = 0;

bool armedStatus = false;
bool alarmedStatus = false;
bool alarmBlinkOn = false;
bool splashActive = false;

uint32_t splashStartedMillis = 0;
uint32_t lastOledRefreshMillis = 0;
uint32_t lastAlarmBlinkMillis = 0;
uint32_t lastCanInitAttemptMillis = 0;
uint32_t lastCanErrorPollMillis = 0;

String strActiveAlarmedDevicesIdList = "";
String strAllAlarmedDevicesIdList = "";

/* Diagnostic counters are intentionally retained even when output is rate-limited. */
unsigned long serialParseErrorCount = 0;
unsigned long serialOverflowCount = 0;
unsigned long rejectedLocalMessageCount = 0;
unsigned long invalidCanFrameCount = 0;
unsigned long canReadErrorCount = 0;
unsigned long canRxOverflowCount = 0;
unsigned long canControllerErrorCount = 0;
unsigned long canInitErrorCount = 0;
unsigned long canTxBusyCount = 0;
unsigned long canNotReadyCount = 0;
unsigned long canTxFailureCount = 0;
unsigned long canTxQueueOverflowCount = 0;

bool hasElapsed(uint32_t now, uint32_t then, uint32_t interval) {
  return static_cast<uint32_t>(now - then) >= interval;
}

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

void setLedPin(bool value, uint8_t pin) {
  digitalWrite(pin, value ? HIGH : LOW);
}

void applyLedState() {
  setLedPin(armedStatus, ARMED_LED_PIN);
  setLedPin(alarmedStatus ? alarmBlinkOn : !armedStatus, DISARMED_LED_PIN);
}

void setupOled() {
  ssd1306_setFixedFont(ssd1306xled_font6x8);
  ssd1306_128x32_i2c_init();
  ssd1306_clearScreen();
}

void setupArmedLeds() {
  pinMode(ARMED_LED_PIN, OUTPUT);
  pinMode(DISARMED_LED_PIN, OUTPUT);
  applyLedState();
}

void setupArmButtonPin() {
  pinMode(ARM_BUTTON_PIN, INPUT_PULLUP);
  stableArmButtonState = digitalRead(ARM_BUTTON_PIN);
  lastRawArmButtonState = stableArmButtonState;
  rawButtonStateChangedMillis = millis();
}

void startLcdHello(uint32_t now) {
  splashActive = true;
  splashStartedMillis = now;
  ssd1306_invertMode();
  ssd1306_setFixedFont(ssd1306xled_font8x16);
  ssd1306_printFixed(0, 8, "     BOBIK", STYLE_BOLD);
}

bool initializeCan() {
  lastCanInitAttemptMillis = millis();
  bool initialized = true;

  MCP2515::ERROR error = mcp2515.reset();
  if (error != MCP2515::ERROR_OK) {
    initialized = false;
    canInitErrorCount++;
    reportDiagnostic(F("CAN_RESET"), canInitErrorCount, static_cast<int>(error));
  }

  error = mcp2515.setBitrate(CAN_125KBPS);
  if (error != MCP2515::ERROR_OK) {
    initialized = false;
    canInitErrorCount++;
    reportDiagnostic(F("CAN_BITRATE"), canInitErrorCount, static_cast<int>(error));
  }

  error = mcp2515.setNormalMode();
  if (error != MCP2515::ERROR_OK) {
    initialized = false;
    canInitErrorCount++;
    reportDiagnostic(F("CAN_MODE"), canInitErrorCount, static_cast<int>(error));
  }

  canReady = initialized;
  return initialized;
}

void serviceCanRecovery(uint32_t now) {
  if (!canReady && hasElapsed(now, lastCanInitAttemptMillis, CAN_RECOVERY_INTERVAL_MS)) {
    initializeCan();
  }
}

bool parseHexField(const char *field, size_t length, unsigned long maximum, unsigned long &value) {
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

String alarmDeviceToken(uint8_t deviceId) {
  return "0x" + String(deviceId, HEX) + " ";
}

bool isValidAlarmDeviceId(uint8_t deviceId) {
  return deviceId != BROADCAST_ADDRESS && deviceId != BASE_STATION_ADDRESS;
}

bool processBaseStationMessage(const ProtocolMessage &message) {
  if (message.senderId != HOME_BASE_CAN_ID || message.addressee != BASE_STATION_ADDRESS) {
    return false;
  }

  switch (message.command) {
    case COMMAND_ARM:
      if (message.payload != DEVICE_TYPE_HOMEBASE) {
        return false;
      }
      armedStatus = true;
      applyLedState();
      return true;

    case COMMAND_DISARM:
      if (message.payload != DEVICE_TYPE_HOMEBASE) {
        return false;
      }
      armedStatus = false;
      alarmedStatus = false;
      alarmBlinkOn = false;
      strActiveAlarmedDevicesIdList = "";
      strAllAlarmedDevicesIdList = "";
      applyLedState();
      return true;

    case COMMAND_ALARMED_DEVICE: {
      if (!isValidAlarmDeviceId(message.payload)) {
        return false;
      }
      if (!alarmedStatus) {
        alarmedStatus = true;
        alarmBlinkOn = true;
        lastAlarmBlinkMillis = millis();
        applyLedState();
      }

      const String token = alarmDeviceToken(message.payload);
      if (strActiveAlarmedDevicesIdList.indexOf(token) == -1) {
        strActiveAlarmedDevicesIdList += token;
      }
      if (strAllAlarmedDevicesIdList.indexOf(token) == -1) {
        strAllAlarmedDevicesIdList += token;
      }
      return true;
    }

    case COMMAND_DEVICE_CLEAR: {
      if (!isValidAlarmDeviceId(message.payload)) {
        return false;
      }
      const String token = alarmDeviceToken(message.payload);
      const int tokenIndex = strActiveAlarmedDevicesIdList.indexOf(token);
      if (tokenIndex >= 0) {
        strActiveAlarmedDevicesIdList.remove(tokenIndex, token.length());
      }
      return true;
    }

    case COMMAND_STOP_ALARM:
      if (message.payload != DEVICE_TYPE_HOMEBASE) {
        return false;
      }
      alarmedStatus = false;
      alarmBlinkOn = false;
      strActiveAlarmedDevicesIdList = "";
      strAllAlarmedDevicesIdList = "";
      applyLedState();
      return true;

    default:
      return false;
  }
}

void popCanTxQueue() {
  if (canTxQueueCount == 0) {
    return;
  }
  canTxQueueHead = (canTxQueueHead + 1) % CAN_TX_QUEUE_CAPACITY;
  canTxQueueCount--;
}

bool queueCanMessage(const ProtocolMessage &message) {
  if (!canReady) {
    canNotReadyCount++;
    reportDiagnostic(F("CAN_NOT_READY"), canNotReadyCount, -1);
    return false;
  }
  if (canTxQueueCount >= CAN_TX_QUEUE_CAPACITY) {
    canTxQueueOverflowCount++;
    reportDiagnostic(F("CAN_TX_QUEUE_FULL"), canTxQueueOverflowCount, -1);
    return false;
  }

  CanTxItem &item = canTxQueue[canTxQueueTail];
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

  canTxQueueTail = (canTxQueueTail + 1) % CAN_TX_QUEUE_CAPACITY;
  canTxQueueCount++;
  return true;
}

void handleSerialMessage(const ProtocolMessage &message) {
  if (message.addressee == BASE_STATION_ADDRESS) {
    if (!processBaseStationMessage(message)) {
      rejectedLocalMessageCount++;
      reportDiagnostic(F("LOCAL_COMMAND"), rejectedLocalMessageCount, message.command);
    }
    return;
  }

  queueCanMessage(message);
}

void finishSerialLine() {
  if (discardingSerialLine) {
    serialOverflowCount++;
    reportDiagnostic(F("SERIAL_OVERFLOW"), serialOverflowCount, -1);
  } else if (serialLineLength > 0) {
    ProtocolMessage message = {};
    if (parseSerialMessage(serialLine, serialLineLength, message)) {
      handleSerialMessage(message);
    } else {
      serialParseErrorCount++;
      reportDiagnostic(F("SERIAL_PARSE"), serialParseErrorCount, -1);
    }
  }

  serialLineLength = 0;
  discardingSerialLine = false;
}

void serviceSerialInput() {
  uint8_t processed = 0;
  while (Serial.available() > 0 && processed < MAX_SERIAL_BYTES_PER_LOOP) {
    const char character = static_cast<char>(Serial.read());
    processed++;

    if (character == '\n') {
      finishSerialLine();
    } else if (character == '\r') {
      continue;
    } else if (character == '\0') {
      discardingSerialLine = true;
    } else if (!discardingSerialLine && serialLineLength < SERIAL_LINE_CAPACITY - 1) {
      serialLine[serialLineLength++] = character;
    } else {
      discardingSerialLine = true;
    }
  }
}

void serviceCanTransmit(uint32_t now) {
  if (!canReady || canTxQueueCount == 0) {
    return;
  }

  CanTxItem &item = canTxQueue[canTxQueueHead];
  if (item.busyRetries > 0 &&
      !hasElapsed(now, item.lastAttemptMillis, CAN_TX_BUSY_RETRY_INTERVAL_MS)) {
    return;
  }

  const MCP2515::ERROR error = mcp2515.sendMessage(&item.frame);
  item.lastAttemptMillis = now;

  if (error == MCP2515::ERROR_OK) {
    popCanTxQueue();
    return;
  }

  if (error == MCP2515::ERROR_ALLTXBUSY) {
    canTxBusyCount++;
    item.busyRetries++;
    reportDiagnostic(F("CAN_TX_BUSY"), canTxBusyCount, static_cast<int>(error));
    if (item.busyRetries < MAX_CAN_TX_BUSY_RETRIES) {
      return;
    }
  }

  // ERROR_FAILTX can be ambiguous, so do not blindly retry non-idempotent commands.
  canTxFailureCount++;
  reportDiagnostic(F("CAN_TX_FAILED"), canTxFailureCount, static_cast<int>(error));
  popCanTxQueue();
}

bool isValidCanFrame(const struct can_frame &frame) {
  const canid_t unsupportedFlags = frame.can_id & (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_ERR_FLAG);
  return unsupportedFlags == 0 && frame.can_dlc == EXPECTED_CAN_DLC &&
         frame.can_id <= MAX_STANDARD_CAN_ID;
}

void writeCanFrameToSerial(const struct can_frame &frame) {
  Serial.print(F("0x"));
  Serial.print(frame.can_id & CAN_SFF_MASK, HEX);
  Serial.print(F("-0x"));
  Serial.print(frame.data[0], HEX);
  Serial.print(F("-0x"));
  Serial.print(frame.data[1], HEX);
  Serial.print(F("-0x"));
  Serial.print(frame.data[2], HEX);
  Serial.println();
}

void serviceCanReceive() {
  if (!canReady) {
    return;
  }

  for (uint8_t count = 0; count < MAX_CAN_RX_FRAMES_PER_LOOP; count++) {
    if (Serial.availableForWrite() < MIN_SERIAL_BYTES_FOR_CAN_FRAME) {
      return;
    }

    struct can_frame frame = {};
    const MCP2515::ERROR error = mcp2515.readMessage(&frame);
    if (error == MCP2515::ERROR_NOMSG) {
      return;
    }
    if (error != MCP2515::ERROR_OK) {
      canReadErrorCount++;
      reportDiagnostic(F("CAN_RX"), canReadErrorCount, static_cast<int>(error));
      return;
    }
    if (!isValidCanFrame(frame)) {
      invalidCanFrameCount++;
      reportDiagnostic(F("CAN_FRAME"), invalidCanFrameCount, frame.can_dlc);
      continue;
    }

    writeCanFrameToSerial(frame);
  }
}

void serviceCanControllerErrors(uint32_t now) {
  if (!canReady || !hasElapsed(now, lastCanErrorPollMillis, CAN_ERROR_POLL_INTERVAL_MS)) {
    return;
  }
  lastCanErrorPollMillis = now;

  const uint8_t flags = mcp2515.getErrorFlags();
  const uint8_t receiveOverflowFlags = MCP2515::EFLG_RX0OVR | MCP2515::EFLG_RX1OVR;
  if ((flags & receiveOverflowFlags) != 0) {
    canRxOverflowCount++;
    reportDiagnostic(F("CAN_RX_OVERFLOW"), canRxOverflowCount, flags);
    mcp2515.clearRXnOVRFlags();
  }

  if ((flags & MCP2515::EFLG_TXBO) != 0) {
    canControllerErrorCount++;
    reportDiagnostic(F("CAN_BUS_OFF"), canControllerErrorCount, flags);
    canReady = false;
    lastCanInitAttemptMillis = now;
  }
}

void sendArmToggleToHost() {
  Serial.print(F("0x"));
  Serial.print(HOME_BASE_CAN_ID, HEX);
  Serial.print(F("-0x"));
  Serial.print(HOME_BASE_CAN_ID, HEX);
  Serial.print(F("-0x"));
  Serial.print(COMMAND_ARM_TOGGLE, HEX);
  Serial.print(F("-0x"));
  Serial.print(DEVICE_TYPE_HOMEBASE, HEX);
  Serial.println();
}

void serviceArmButton(uint32_t now) {
  const int rawState = digitalRead(ARM_BUTTON_PIN);
  if (rawState != lastRawArmButtonState) {
    lastRawArmButtonState = rawState;
    rawButtonStateChangedMillis = now;
  }

  if (rawState != stableArmButtonState &&
      hasElapsed(now, rawButtonStateChangedMillis, BUTTON_DEBOUNCE_INTERVAL_MS)) {
    stableArmButtonState = rawState;
    if (stableArmButtonState == LOW) {
      sendArmToggleToHost();
    }
  }
}

void serviceAlarmLed(uint32_t now) {
  if (!alarmedStatus || !hasElapsed(now, lastAlarmBlinkMillis, ALARM_BLINK_INTERVAL_MS)) {
    return;
  }

  lastAlarmBlinkMillis = now;
  alarmBlinkOn = !alarmBlinkOn;
  applyLedState();
}

void outputToLcd() {
  ssd1306_setFixedFont(ssd1306xled_font6x8);

  if (armedStatus) {
    ssd1306_negativeMode();
  }
  ssd1306_printFixed(0, 0, armedStatus ? "ENABLED              " : "DISABLED             ", STYLE_NORMAL);
  if (armedStatus) {
    ssd1306_positiveMode();
  }

  if (alarmedStatus) {
    if (alarmBlinkOn) {
      ssd1306_negativeMode();
    }
    ssd1306_printFixed(0, 8, "        ALARM        ", STYLE_BOLD);
    if (alarmBlinkOn) {
      ssd1306_positiveMode();
    }

    String output = strActiveAlarmedDevicesIdList + "                     ";
    ssd1306_printFixed(0, 16, &output[0], STYLE_BOLD);
    output = strAllAlarmedDevicesIdList + "                     ";
    ssd1306_printFixed(0, 24, &output[0], STYLE_BOLD);
  } else {
    ssd1306_printFixed(0, 8, "      NO ALARM       ", STYLE_BOLD);
    ssd1306_printFixed(0, 16, "                     ", STYLE_BOLD);
    ssd1306_printFixed(0, 24, "                     ", STYLE_BOLD);
  }
}

void serviceDisplay(uint32_t now) {
  if (splashActive) {
    if (!hasElapsed(now, splashStartedMillis, SPLASH_DURATION_MS)) {
      return;
    }
    splashActive = false;
    ssd1306_normalMode();
    ssd1306_clearScreen();
    lastOledRefreshMillis = now - OLED_REFRESH_INTERVAL_MS;
  }

  if (!hasElapsed(now, lastOledRefreshMillis, OLED_REFRESH_INTERVAL_MS)) {
    return;
  }
  lastOledRefreshMillis = now;
  outputToLcd();
}

void setup() {
  Serial.begin(115200);

  setupArmedLeds();
  setupArmButtonPin();
  setupOled();
  initializeCan();
  startLcdHello(millis());
}

void loop() {
  const uint32_t now = millis();

  serviceSerialInput();
  serviceArmButton(now);
  serviceCanRecovery(now);
  serviceCanTransmit(now);
  serviceCanReceive();
  serviceCanControllerErrors(now);
  serviceAlarmLed(now);
  serviceDisplay(now);
}
