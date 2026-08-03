#pragma once

#include <Arduino.h>

namespace bobik {

constexpr uint8_t ARMED_LED_PIN = 3;
constexpr uint8_t DISARMED_LED_PIN = 2;
constexpr uint8_t ARM_BUTTON_PIN = 9;
constexpr uint8_t MCP2515_CS_PIN = 10;

constexpr uint32_t OLED_REFRESH_INTERVAL_MS = 100;
constexpr uint32_t ALARM_BLINK_INTERVAL_MS = 500;
constexpr uint32_t BUTTON_DEBOUNCE_INTERVAL_MS = 40;
constexpr uint32_t SPLASH_DURATION_MS = 3000;
constexpr uint32_t CAN_RECOVERY_INTERVAL_MS = 5000;
constexpr uint32_t CAN_TX_BUSY_RETRY_INTERVAL_MS = 5;
constexpr uint32_t CAN_ERROR_POLL_INTERVAL_MS = 100;

constexpr uint8_t MAX_SERIAL_BYTES_PER_LOOP = 32;
constexpr uint8_t MAX_CAN_RX_FRAMES_PER_LOOP = 3;
constexpr uint8_t MIN_SERIAL_BYTES_FOR_PROTOCOL_MESSAGE = 24;
constexpr uint8_t CAN_TX_QUEUE_CAPACITY = 8;
constexpr uint8_t MAX_CAN_TX_BUSY_RETRIES = 20;
constexpr size_t SERIAL_LINE_CAPACITY = 64;
constexpr size_t OLED_ROW_CHARACTERS = 21;
constexpr size_t OLED_ROW_BUFFER_SIZE = OLED_ROW_CHARACTERS + 1;

}  // namespace bobik
