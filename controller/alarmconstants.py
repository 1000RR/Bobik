HOME_BASE_ID = 0x14 #interdependent with deviceDictionary
BROADCAST_ID = 0x00
BASE_STATION_ID = 0xFF

DENON_ID = 0x77
TEST_ALARM_ID = 0xDE
CHECK_PHONES_ID = 0x17
GARAGE_DOOR_OPENER_ID = 0xD0
GARAGE_DOOR_SENSOR_ID = 0x30

DEVICE_DICTIONARY = {
    "0x80": "SENSOR | GARAGE MOVEMENT | 0x80",
    "0x75": "SENSOR | KITCHEN MOVEMENT | 0x75",
    hex(GARAGE_DOOR_SENSOR_ID): "SENSOR | GARAGE CAR DOOR | " + hex(GARAGE_DOOR_SENSOR_ID),
    "0x31": "SENSOR | GARAGE SIDE DOOR | 0x31",
    "0x40": "SENSOR | KITCHEN BACK DOOR | 0x40",
    "0x50": "SENSOR | FRONT DOOR | 0x50",
    hex(HOME_BASE_ID): "HOMEBASE | HOME BASE | 0x14",
    hex(BASE_STATION_ID): "HOMEBASE | HOME BASE communicating to its arduino | " +  hex(BASE_STATION_ID),
    "0x10": "ALARM | LAUNDRY FIRE ALARM BELL | 0x10",
    "0x15": "ALARM | GARAGE PIEZO LOUD ALARM | 0x15",
    "0x99": "ALARM | OFFICE BUZZER ALARM | 0x99",
    hex(DENON_ID): "ALARM | OFFICE SPEAKERS | " + hex(DENON_ID),
    hex(CHECK_PHONES_ID): "SENSOR | VIRTUAL sensor for getting attention | " + hex(CHECK_PHONES_ID),
    hex(TEST_ALARM_ID): "SENSOR | VIRTUAL sensor for triggering a test alarm | " + hex(TEST_ALARM_ID),
    hex(GARAGE_DOOR_OPENER_ID): "OPENER | GARAGE DOOR OPENER | " + hex(GARAGE_DOOR_OPENER_ID),
    "0x60": "SENSOR | MOVEMENT 60 | 0x60",
    "0x61": "SENSOR | MOVEMENT 61 | 0x61",
    "0x62": "SENSOR | MOVEMENT 62 | 0x62"
}

###################### ADDRESSES ######################
#0x00 - broadcast
#0xFF - code for home base's arduino "base station". Messages addressed to this device are not forwarded by arduino to CANBUS.
#0x80 - garage, commercial type (high emmissions, long range)
#0x75 - inside, consumer type (short range)
#0x14 - home base
#0x10 - fire alarm bell
#0x15 - siren alarm
#0x99 - indoor siren with led
#0x30 - door sensor
#0x31 - door sensor
#0xD0 - garage door opener
#0x17 - virtual sensor device that is used to alert people to pick up their phones
#0xDE - virtual test device (sensor)
#0x51 - virtual silence device (alarm, used alone)
#0x60 - PIR sensor on multidevice node
#0x61 - PIR sensor on multidevice node
#0x62 - PIR sensor on multidevice node

MP3_ALARM_DICTIONARY = {
    "0x80": "garagemovement.mp3",
    "0x75": "kitchenmovement.mp3",
    hex(GARAGE_DOOR_SENSOR_ID): "garagedoor.mp3",
    "0x31": "garagesidedoor.mp3",
    hex(CHECK_PHONES_ID): "checkyourphones.mp3",
    "0x50": "frontdoor.mp3",
    "0x40": "kitchenbackdoor.mp3"
}

###################### MESSAGES #######################
# 0xAA - alarm triggered signal
# 0xBB - alarm device enabled signal
# 0xCC - alarm device disabled signal
# 0xD1 - sent to home base arduino - arm 
# 0xD0 - sent to home base arduino - disarm
# 0x0F - power off sensor
# 0x01 - power on sensor
# 0xA0 - sending over to home base arduino (address BASE_STATION_ID) the address of the alarmed device
# 0xB0 - sending over to home base arduino (address BASE_STATION_ID) the address of the no longer alarmed device
# 0xC0 - sending over to home base arduino (address BASE_STATION_ID) stop alarm signal
# 0xEE - arm toggle button on unit pressed
# 0x0D - momentary switch trigger

ALARM_TRIGGERED_COMMAND = 0xAA
ALARM_ENABLE_COMMAND = 0xBB
ALARM_DISABLE_COMMAND = 0xCC
ALARM_ARM_COMMAND = 0xD1
ALARM_DISARM_COMMAND = 0xD0
SENSOR_POWER_OFF_COMMAND = 0x0F
SENSOR_POWER_ON_COMMAND = 0x01
ALARMED_DEVICE_ID_COMMAND = 0xA0
NO_LONGER_ALARMED_DEVICE_ID_COMMAND = 0xB0
STOP_ALARM_COMMAND = 0xC0
ARM_TOGGLE_COMMAND = 0xEE
MOMENTARY_SWITCH_TRIGGER_COMMAND = 0x0D

#DEVICE TYPE DICTIONARY:
#01 home base
#02 pir/microwave sensor
#03 bell alarm
#04 visual alarm
#05 door open sensor
#06 device controller
#07 temperature/humidity sensor

DEVICE_TYPE_HOMEBASE = 0x01
DEVICE_TYPE_PIR_SENSOR = 0x02
DEVICE_TYPE_BELL_ALARM = 0x03
DEVICE_TYPE_VISUAL_ALARM = 0x04
DEVICE_TYPE_DOOR_SENSOR = 0x05
DEVICE_TYPE_DEVICE_CONTROLLER = 0x06
DEVICE_TYPE_TEMP_HUMIDITY_SENSOR = 0x07


FLOAT_DELAY_BETWEEN_POWER_ON_COMMANDS_SEC = 0.15
FLOAT_ALARM_TEST_LENGTH_TIME_SEC = 0.05

MP3_PLAYER_PROGRAM = ["/usr/bin/mpg123", "-o", "alsa", "-a", "hw:2,0"] #specific to raspberry pi 4b / Raspberry Pi OS 64