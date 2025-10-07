import serial
from datetime import datetime
import math
import numpy as np
import time
import atexit
import json
import subprocess
import os
import debugpy
from threading import Thread
from alarmconstants import *

# DEBUGGER debugpy
# debugpy.listen(("0.0.0.0", 5678))
# print("Waiting for debugger attach...")
# debugpy.trace_this_thread(True)

debug = False
LISTEN_PORT = 8080

armed = False  # initial condition
alarmed = False
pastEvents = []
prev_pastEvents = [];
prev_status = {};
memberDevices = {}  # map of {string hex id:{properties}}
exceptMissingDevices = {hex(DENON_ID): True}
triggeredDevicesInCurrentArmCycle = {}
missingDevicesInCurrentArmCycle = {}
everTriggeredWithinAlarmCycle = {}  # map of {string hex id:int alarmTimeSec}
everTriggered = {}
currentlyTriggeredDevices = {}  # map of {string hex id:int alarmTimeSec}
currentlyMissingDevices = []
everMissingDevices = {}
denonPlayThread = 0
lastSentMessageTimeMsec = 0
lastAlarmTime = 0
armSetTimeSec = 0  # movement detection devices tend to send an alarm signal shortly after being powered on. This is used to ignore such signals if they occur within a few seconds of arming.
armTimeoutBeforeTriggeringAlarm = 2  # seconds
armPerDeviceTimeoutBeforeTriggeringAlarm = 2  # seconds
lastArmedTogglePressed = 0
deviceAbsenceThresholdSec = 3  # seconds before a device is considered missing
firstPowerCommandNeedsToBeSent = True
timeAllottedToBuildOutMembersSec = 2
initWaitSeconds = 5
sendTimeoutMsec = 500
lastCheckedMissingDevicesMsec = 0
checkForMissingDevicesEveryMsec = 1500
currentAlarmProfile = 0  # 0 = default
threadShouldTerminate = False
canDebugMessage = []
shouldSendDebugRepeatedly = False
shouldSendDebugMessage = False
alwaysKeepOnSet = {
    "0x30",
    "0x31",
    "0x40",
    "0x50",
}  # set of devices to always keep powered on (active). This should be limited to non-emitting sensors. #TODO: removing this logic inhibited the intended operation of the garage door sensor when disarmed. It makes sense to always have non-relay devices transmitting and not respond to base power commands, with base filtering the.
avrSoundChannel = "SAT/CBL"
quickSetAlarmProfiles = [
    0,
    26,
    1,
    3,
    15,
    2,
    7,
]  # quick set alarm profile buttons in UI (subset of all profiles, indexed by respective array index)


def getThisDirAddress():
    return os.path.dirname(__file__)


with open(getThisDirAddress() + "/alarmProfiles.json", "r") as file:
    alarmProfiles = json.loads(file.read())


# serial message format:
#   {sender id hex}-{receiver id hex}-{message hex}-{devicetype hex}\n
# when sending to 0xFF (home base arduino)
#   {HOME_BASE_ID}-0xFF-{message hex}-{message 2 hex}\n




# name of ARDUINO tty device
# on mac: /dev/tty.usbserial-10
# on linux: /dev/ttyUSB0

ser = serial.Serial(
    "/dev/ttyUSB0",
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=0.25,
)  # quarter second timeout so that Serial.readLine() doesn't block if no message(s) on CAN
print("Arduino: serial connection with PI established")
np.set_printoptions(formatter={"int": hex})

#deviceId: hex str OR int
def setDevicePower(deviceId):
    if isinstance(deviceId, int):
        deviceId = hex(deviceId);

    intendedPowerState = (
        armed
        and deviceId in getExplicitAlarmProfileTriggerDevices().union(alwaysKeepOnSet) #TODO: race condition possible -> if not all devices yet added to member list, you're screwed
    )
     # devicesOverrideArray, shouldBroadcast, powerState
    sendPowerCommand([deviceId], False, intendedPowerState)


def setDevicesPower():
    offDevices = []
    onDevices = []
    # if not armed, send OFF to all
    # if armed, send OFF or ON to all depending on whether the device is in the profile's sensorsthattrigger
    if not armed:
        sendPowerCommand(
            [], True, False
        )  # devicesOverrideArray, shouldBroadcast, powerState
        sendPowerCommand(list(alwaysKeepOnSet), False, True)
    else:
        offDevices, onDevices = getDevicesPowerStateLists()
        
        if (len(offDevices) > 0):
            sendPowerCommand(offDevices, False, False)
        if (len(onDevices) > 0):
            sendPowerCommand(onDevices, False, True)


def getExplicitAlarmProfileTriggerDevices():
    return (
        set(alarmProfiles[currentAlarmProfile]["sensorsThatTriggerAlarm"])
        if "sensorsThatTriggerAlarm" in alarmProfiles[currentAlarmProfile]
        else set(memberDevices)
    )


def getDevicesPowerStateLists():  # devices per current profile
    oldProfileDeviceSet = set(memberDevices)
    newProfileDeviceSet = getExplicitAlarmProfileTriggerDevices().union(alwaysKeepOnSet)

    offDevices = list(oldProfileDeviceSet - newProfileDeviceSet)
    onDevices = list(newProfileDeviceSet)

    return offDevices, onDevices


def setCurrentAlarmProfile(
    profileNumber, requestMethod
):  # -1 means no profile set. All devices trigger. Alarms are broadcast to all devices.
    global currentAlarmProfile
    global currentlyTriggeredDevices
    global alarmed

    if profileNumber >= -1 and profileNumber < len(alarmProfiles):
        currentAlarmProfile = profileNumber
        currentlyTriggeredDevices = {}
        stopAlarm(trigger = "alarm profile changed to " + str(profileNumber) + " via " + requestMethod)

        setDevicesPower()
        print(
            "SETTING ALARM PROFILE "
            + str(profileNumber)
            + " - "
            + getProfileName(profileNumber)
        )
        addEvent(
            {
                "event": "SET PROFILE : "
                + str(profileNumber)
                + " - "
                + getProfileName(profileNumber),
                "time": getReadableTimeFromTimestamp(getTimeSec()),
                "trigger": requestMethod,
            }
        )
    else:
        print(
            ">>>> PROFILE NUMBER OUT OF RANGE [0,"
            + str(len(alarmProfiles) - 1)
            + "] : "
            + str(profileNumber)
        )


def addEvent(event):
    global pastEvents
    pastEvents.append(event);


def getArmedStatus():
    return armed


def toggleArmed(now, strActionOrigin):
    global lastArmedTogglePressed
    global alarmed
    global armed
    global everTriggeredWithinAlarmCycle
    global triggeredDevicesInCurrentArmCycle
    global missingDevicesInCurrentArmCycle
    global currentlyTriggeredDevices
    global armSetTimeSec

    lastArmedTogglePressed = now
    if armed == True:
        print(
            f">>>>>>>>TURNING OFF ALARM AT {getReadableTimeFromTimestamp(now)} PER {strActionOrigin}<<<<<<<<<"
        )
        addEvent(
            {
                "event": "DISARMED",
                "time": getReadableTimeFromTimestamp(now),
                "trigger": strActionOrigin,
            }
        )
        armed = False
        stopAlarm(trigger = "Disarming via " + strActionOrigin) # reset alarmed state
        triggeredDevicesInCurrentArmCycle = {}
        missingDevicesInCurrentArmCycle = {}
    else:
        print(
            f">>>>>>>>TURNING ON ALARM AT {getReadableTimeFromTimestamp(now)} PER {strActionOrigin}<<<<<<<<<"
        )
        addEvent(
            {
                "event": "ARMED",
                "time": getReadableTimeFromTimestamp(now),
                "trigger": strActionOrigin,
            }
        )
        armed = True
        armSetTimeSec = now + 5 #overshoots first so that any new messages being handled are not considered within the arm timeout period
        stopAlarm(trigger = "Arming via " + strActionOrigin) # reset alarmed state

    # turn on or off devices depending on armed state
    setDevicesPower()
    if (armed): #this is intentionally done after setting all the devices' power state, so that an alarm is considered on only after all power messages are sent
        armSetTimeSec = now
    sendArmedLedSignal()
    print("Clearing member devices list")
    resetMemberDevices()  # reset all members on the bus when turning on/off


def getMemberDeviceDictEntry(
    id,
    firstSeen,
    firstSeenReadable,
    deviceType,
    lastSeen,
    lastSeenReadable,
    friendlyName,
    lastArmedTimeSec,
):
    return {
        "id": id,
        "firstSeen": firstSeen,
        "firstSeenReadable": firstSeenReadable,
        "deviceType": deviceType,
        "lastSeen": lastSeen,
        "lastSeenReadable": lastSeenReadable,
        "friendlyName": lastArmedTimeSec,
        "lastArmedTimeSec": lastArmedTimeSec,
    }


def resetMemberDevices():
    global memberDevices
    memberDevices = {
        hex(DENON_ID): getMemberDeviceDictEntry(
            id=hex(DENON_ID),
            firstSeen=getTimeSec(),
            firstSeenReadable=getReadableTime(),
            deviceType="0x10",
            lastSeen=getTimeSec(),
            lastSeenReadable=getTimeSec(),
            friendlyName=getFriendlyDeviceName(DENON_ID),
            lastArmedTimeSec=-1,
        )
    }


def decodeLine(line):
    try:
        msg = line.split("-")
        msg[3] = msg[3].rstrip("\n")
        msg = [int(i, 16) for i in msg]
    except:
        print(f">>>>ERROR DECODING UTF8 LINE {msg}<<<<<")
        raise ("PARSE-ERROR")
    return msg


def encodeLine(message):  # [myCanId, addressee, message, myDeviceType]
    printableArr = message.copy()
    printableArr.append(getTimeSec())
    # print("SENDING ", np.array(printableArr));
    return (
        hex(message[0])
        + "-"
        + hex(message[1])
        + "-"
        + hex(message[2])
        + "-"
        + hex(message[3])
        + "-\n"
    )


def sendMessage(messageArray):
    global lastSentMessageTimeMsec
    global denonPlayThread

    outgoing = encodeLine(messageArray)
    ser.write(bytearray(outgoing, "ascii"))
    ser.flushOutput()
    lastSentMessageTimeMsec = getTimeMsec()
    if messageArray[1] == DENON_ID or messageArray[1] == BROADCAST_ID:
        if messageArray[2] == ALARM_ENABLE_COMMAND and not (
            denonPlayThread and denonPlayThread.is_alive()
        ):
            denonPlayThread = Thread(
                target=playDenonThreadMain,
                args=(currentlyTriggeredDevices, everTriggeredWithinAlarmCycle),
            )
            denonPlayThread.start()


def getCurrentProfileSoundByteData():
    playSoundVolume = -1
    playSound = ""

    for index, profile in enumerate(alarmProfiles):
        #print(">>>>>> PROFILE INDEX " + str(index))
        if index != currentAlarmProfile:
            continue
        if "playSound" in profile and profile["playSound"]:
            playSound = profile["playSound"]
        if "playSoundVolume" in profile and profile["playSoundVolume"]:
            playSoundVolume = profile["playSoundVolume"]
    print(">>>>>>> PLAYSOUND " + playSound)
    print(">>>>>>> PLAYSOUNDVOLUME " + str(playSoundVolume))
    return playSound, playSoundVolume


def getTime():
    return datetime.now().timestamp()
    # return math.floor(datetime.now(timezone('US/Pacific')).timestamp())


def getTimeSec():
    return math.floor(getTime())


def getTimeMsec():
    return math.floor(getTime() * 1000)


def getReadableTime():
    return getReadableTimeFromTimestamp(getTimeSec())


def getReadableTimeFromTimestamp(timestamp):
    return f"{datetime.fromtimestamp(timestamp).strftime('%c')} LOCAL TIME"


def possiblyAddMember(msg):
    global memberDevices
    now = getTimeSec()
    senderId = msg[0]
    if senderId != HOME_BASE_ID:
        readableTimestamp = getReadableTime()

        if hex(senderId) not in memberDevices:  # new device sending a signal
            print(
                f"Adding new device to members list {hex(senderId)} at {readableTimestamp}"
            )
            addEvent(
                {
                    "event": "NEW MEMBER",
                    "trigger": hex(senderId),
                    "time": readableTimestamp,
                }
            )
            memberDevices[hex(senderId)] = getMemberDeviceDictEntry(
                id=hex(senderId),
                firstSeen=now,
                firstSeenReadable=readableTimestamp,
                deviceType=msg[3],
                lastSeen=now,
                lastSeenReadable=readableTimestamp,
                friendlyName=getFriendlyDeviceName(senderId),
                lastArmedTimeSec=(
                    -1
                    if not (
                        armed and isDeviceInActiveProfileTriggersList(hex(senderId))
                    )
                    else now + armPerDeviceTimeoutBeforeTriggeringAlarm
                ),
            )

            # if a new device is added while armed, set its power state according to the current profile
            if (
                armed
                and isDeviceInActiveProfileTriggersList(hex(senderId))
            ):
                setDevicePower(senderId)

        else:  # existing device sending a signal
            memberDevices[hex(senderId)]["lastSeen"] = now
            memberDevices[hex(senderId)]["lastSeenReadable"] = readableTimestamp
            if hex(senderId) in currentlyMissingDevices and debug:
                print(
                    f"Removing missing device {hex(senderId)} at {getReadableTime()}."
                )


def playDenonThreadMain(currentlyTriggeredDevices, everAlarmedDuringAlarm):
    cwd = getThisDirAddress()
    playCommandArray = ["/usr/bin/mpg123", "-o", "alsa", "-a", "hw:2,0"] #specific to raspberry pi 4b / Raspberry Pi OS 64
    volume = "55"  # defaul
    ####types of sounds####
    # test sound from TEST_ALARM_ID
    # pick up your phones from CHECK_PHONES_ID
    # sound byte override
    # fall back to saying the sensors that are activated

    playCommandArray, volume = determineStuffToPlay(
        playCommandArray, volume, everAlarmedDuringAlarm, currentlyTriggeredDevices
    )
    startPowerStatus, startChannelStatus, startVolume = getDenonInitialState(cwd)
    if (
        startPowerStatus == False
        and startChannelStatus == False
        and startVolume == False
    ):
        return
    setDenonPlayState(startPowerStatus, startChannelStatus, volume, cwd)
    playDenonSounds(playCommandArray, cwd)
    setDenonOriginalState(startPowerStatus, startChannelStatus, startVolume, cwd)


def determineStuffToPlay(
    playCommandArray, volume, everAlarmedDuringAlarm, currentlyTriggeredDevices
):
    sound = ""
    # playCommandArray.append("./alert.mp3")

    if hex(TEST_ALARM_ID) in currentlyTriggeredDevices:
        sound = "thisisatest.mp3"
        currentlyTriggeredDevices.pop(hex(TEST_ALARM_ID))
    elif hex(CHECK_PHONES_ID) in currentlyTriggeredDevices:
        sound = "checkyourphones.mp3"
        volume = "79"
        currentlyTriggeredDevices.pop(hex(CHECK_PHONES_ID))
    else:
        playCommandArray.append("./alert.mp3")
        soundByteOverride, volumeOverride = getCurrentProfileSoundByteData()
        if soundByteOverride and volumeOverride:
            volume = volumeOverride
            sound = soundByteOverride

    # if special case sound found above, use it
    if sound:
        playCommandArray.append(sound)
    # otherwise, play names of sensors active
    else:
        for device in everAlarmedDuringAlarm:
            resolvedMp3 = MP3_ALARM_DICTIONARY[device]
            if resolvedMp3:
                playCommandArray.append(resolvedMp3)
        # playCommandArray.append('compromised.mp3')

    return playCommandArray, volume


def playDenonSounds(playCommandArray, cwd):
    # play sound(s)
    subprocess.run(playCommandArray, cwd=cwd)


def setDenonPlayState(startPowerStatus, startChannelStatus, volume, cwd):
    # turn on and switch to $avrSoundChannel if previously off OR previously channel isn't $avrSoundChannel;
    # then sleep the appropriate number of seconds to let denon get ready
    if startPowerStatus != "ON" or startChannelStatus != avrSoundChannel:
        subprocess.run("./denonon.sh", cwd=cwd)
        time.sleep(8 if startPowerStatus != "ON" else 3)

    # set volume
    subprocess.run(["./denonvol.sh", str(volume)], cwd=str(cwd))


def setDenonOriginalState(startPowerStatus, startChannelStatus, startVolume, cwd):
    # turn off if was off before
    if startPowerStatus != "ON":  # TODO: add condition: and the alarm has been canceled
        subprocess.run(getThisDirAddress() + "/denonoff.sh", cwd=cwd)
    # otherwise, set volume to old volume
    else:
        subprocess.run(["./denonvol.sh", startVolume], cwd=cwd)
        if startChannelStatus != avrSoundChannel:
            subprocess.run(["./denonchannel.sh", startChannelStatus], cwd=cwd)


def getDenonInitialState(cwd):
    # store original power status
    startPowerStatus = str(
        subprocess.run(
            "./denonpowerstatus.sh", cwd=cwd, stderr=None, capture_output=True
        ).stdout
    ).translate({ord(c): None for c in "b\\n'"})

    # if cannot find denon, cannot play -> exit thread
    if startPowerStatus == "":
        print(">>>>DENON NOT FOUND")
        return False, False, False  # signals quit now

    # store original channel
    startChannelStatus = str(
        subprocess.run(
            "./denonchannelstatus.sh", cwd=cwd, stderr=None, capture_output=True
        ).stdout
    ).translate({ord(c): None for c in "b\\n'"})

    # store original volume
    tempvol = str(
        subprocess.run(
            "./denonvolumestatus.sh", cwd=cwd, stderr=None, capture_output=True
        ).stdout
    ).translate({ord(c): None for c in "b\\n'"})
    if tempvol == "--":
        tempvol = "0"

    startVolume = str(int(float(tempvol) + 81))

    return startPowerStatus, startChannelStatus, startVolume


def getFriendlyDeviceName(address):
    strAddress = hex(address)
    return (
        DEVICE_DICTIONARY[strAddress] if strAddress in DEVICE_DICTIONARY else "unlisted"
    )


def getFriendlyDeviceNamesFromDeviceDictionary(dict):
    friendlyDeviceNames = []
    for key in dict:
        if key in DEVICE_DICTIONARY:
            friendlyDeviceNames.append(DEVICE_DICTIONARY[key])
        else:
            friendlyDeviceNames.append("UNKNOWN | UNKNOWN DEVICE | " + key)
    return friendlyDeviceNames


def checkMembersOnline():
    now = getTimeSec()
    global lastCheckedMissingDevicesMsec
    global missingDevicesInCurrentArmCycle
    global everMissingDevices
    lastCheckedMissingDevicesMsec = getTimeMsec()
    missingMembers = []
    for memberId in memberDevices:
        if (
            not memberId in exceptMissingDevices
            and memberDevices[memberId]["lastSeen"] + deviceAbsenceThresholdSec < now
        ):
            print(
                f"Adding missing device {memberId} at {getReadableTime()}. missing for {(getTimeSec()-memberDevices[memberId]['lastSeen'])} seconds"
            )
            missingMembers.append(memberId)
            everMissingDevices[memberId] = True
            missingDevicesInCurrentArmCycle[memberId] = now
    return missingMembers


def sendArmedLedSignal():
    if armed == True:
        messageToSend = [
            HOME_BASE_ID,
            BASE_STATION_ID,
            ALARM_ARM_COMMAND,
            DEVICE_TYPE_HOMEBASE,
        ]
        print(f">>>> SENDING ARM SIGNAL TO ARDUINO {np.array(messageToSend)}")
    else:
        messageToSend = [
            HOME_BASE_ID,
            BASE_STATION_ID,
            ALARM_DISARM_COMMAND,
            DEVICE_TYPE_HOMEBASE,
        ]
        print(f">>>> SENDING DISARM SIGNAL TO ARDUINO {np.array(messageToSend)}")
    sendMessage(messageToSend)


# by default, sends to all members of current profile, unless overridden with at most 1 of the first 2 params
# devicesOverrideArray: list of hex strings, not ints
def sendPowerCommand(devicesOverrideArray, shouldBroadcast, powerState):  # two op
    devicesToSendTo = (
        devicesOverrideArray
        if devicesOverrideArray
        else (
            memberDevices
            if shouldBroadcast
            else (
                alarmProfiles[currentAlarmProfile]["sensorsThatTriggerAlarm"]
                if "sensorsThatTriggerAlarm" in alarmProfiles[currentAlarmProfile]
                else memberDevices
            )
        )
    )

    if shouldBroadcast:
        messageToSend = [
            HOME_BASE_ID,
            BROADCAST_ID,
            SENSOR_POWER_OFF_COMMAND if powerState else SENSOR_POWER_ON_COMMAND,
            DEVICE_TYPE_HOMEBASE,
        ]
        sendMessage(messageToSend)
        print(
            f">>>> SENDING POWER {'ON' if powerState else 'OFF'} SIGNAL TO ALL {np.array(messageToSend)}"
        )
    else:
        for member in devicesToSendTo:
            intMemberId = int(member, 16) if isinstance(member, str) else member
            messageToSend = [
                HOME_BASE_ID,
                intMemberId,
                SENSOR_POWER_OFF_COMMAND if powerState else SENSOR_POWER_ON_COMMAND,
                DEVICE_TYPE_HOMEBASE,
            ]
            sendMessage(
                messageToSend
            )  # stand up power - SENSOR_POWER_OFF_COMMAND enabled / 0x01 disabled
            print(
                f">>>> SENDING POWER {'ON' if powerState else 'OFF'} SIGNAL {np.array(messageToSend)}"
            )
            time.sleep(
                FLOAT_DELAY_BETWEEN_POWER_ON_COMMANDS_SEC
            )  # in seconds represented as double - to not have a voltage drop from multiple relay-gated PIR/mwave devices powering on (and charging capacitor) simultaneously


def exitSteps():
    print(f"\n\nEXITING AT {getReadableTime()}")
    print("BROADCASTING QUIET-ALL-ALARMS SIGNAL")
    sendMessage(
        [HOME_BASE_ID, BROADCAST_ID, ALARM_DISABLE_COMMAND, DEVICE_TYPE_HOMEBASE]
    )  # reset all devices (broadcast)
    print("BROADCASTING ALL-SENSOR-DEVICES-OFF SIGNAL")
    sendMessage(
        [HOME_BASE_ID, BROADCAST_ID, SENSOR_POWER_OFF_COMMAND, DEVICE_TYPE_HOMEBASE]
    )  # all devices off (broadcast)
    print("\nPAST EVENTS LIST FOLLOWS:")
    for line in pastEvents:
        print(f"\t{line}")


def arrayToString(array):
    string = ""
    for i in array:
        string += "" + i + " "
    return string


def isDeviceInActiveProfileTriggersList(deviceId):
    if "sensorsThatTriggerAlarm" in alarmProfiles[currentAlarmProfile]:
        return deviceId in alarmProfiles[currentAlarmProfile]["sensorsThatTriggerAlarm"]
    return True  # if no sensorsThatTriggerAlarm list, all devices trigger


def hasMissingDevicesThatTriggerAlarm():
    if "missingDevicesThatTriggerAlarm" in alarmProfiles[currentAlarmProfile]:
        for missingDevice in currentlyMissingDevices:
            if (
                missingDevice
                in alarmProfiles[currentAlarmProfile]["missingDevicesThatTriggerAlarm"]
            ):
                return True
        return False
    return False  # if no missingDevicesThatTriggerAlarm list, no missing devices trigger
    


def hasTriggeredDevicesThatTriggerAlarm():
    if "sensorsThatTriggerAlarm" in alarmProfiles[currentAlarmProfile]:
        for triggeredDevice in currentlyTriggeredDevices:
            if (
                triggeredDevice
                in alarmProfiles[currentAlarmProfile]["sensorsThatTriggerAlarm"]
            ):
                return True
        return False
    return len(currentlyTriggeredDevices) > 0  # if no sensorsThatTriggerAlarm list, all triggered devices trigger


def handleMessage(msg):
    global alarmed
    global lastAlarmTime
    global armed
    global lastArmedTogglePressed
    global currentlyTriggeredDevices
    global everTriggeredWithinAlarmCycle
    global triggeredDevicesInCurrentArmCycle
    global currentAlarmProfile
    global canDebugMessage

    senderId = msg[0]
    receiverId = msg[1]
    message = msg[2]
    deviceType = msg[3]

    if debug:
        print(
            f"SENDER {senderId} RECEIVER {receiverId} MESSAGE {message} DEVICE-TYPE {deviceType}"
        )

    possiblyAddMember(msg)
    now = getTimeSec()

    # for some messages - handle special cases intended for this unit from arduino, and return; if not, drop down to handle general case logic block
    if (
        senderId == HOME_BASE_ID
        and receiverId == HOME_BASE_ID
        and message == ARM_TOGGLE_COMMAND
        and lastArmedTogglePressed < now
    ):
        toggleArmed(now, "ARDUINO")
        return

    # alarm message coming in from a device that isn't in the currentlyTriggeredDevices list
    if (
        (receiverId == HOME_BASE_ID or receiverId == BROADCAST_ID)
        and message == ALARM_TRIGGERED_COMMAND
        and hex(senderId) not in currentlyTriggeredDevices
    ):
        
        if isDeviceInActiveProfileTriggersList(hex(senderId)):
            print(
                f">>>>>>>>>>>>>>>>>RECEIVED TRIGGER SIGNAL FROM {hex(senderId)} AT {getReadableTime()}<<<<<<<<<<<<<<<<<<"
            )
            if (
                armed
                and (now - armSetTimeSec >= armTimeoutBeforeTriggeringAlarm)
                and (
                    now - memberDevices[hex(senderId)]["lastArmedTimeSec"]
                    >= armPerDeviceTimeoutBeforeTriggeringAlarm
                )
            ):  # if armed, and not within arm timeout period AND not within per-device arm timeout period
                currentlyTriggeredDevices[hex(senderId)] = now
                alarmed = True
                lastAlarmTime = now
                triggeredDevicesInCurrentArmCycle[hex(senderId)] = now
                everTriggeredWithinAlarmCycle[hex(senderId)] = now
                everTriggered[hex(senderId)] = now
                addEvent(
                    {
                        "event": "SENSOR TRIGGERED WITH ALARM",
                        "trigger": hex(senderId),
                        "time": getReadableTimeFromTimestamp(lastAlarmTime),
                    }
                )
                print(f">>>>>currentAlarmProfile {currentAlarmProfile}")
                sendMessage(
                    [HOME_BASE_ID, BASE_STATION_ID, ALARMED_DEVICE_ID_COMMAND, senderId]
                )  # send to the home base's arduino a non-forwardable message with the ID of the alarm-generating device to be added to the list
            elif not armed:
                currentlyTriggeredDevices[hex(senderId)] = now
                addEvent(
                    {
                        "event": "SENSOR TRIGGERED WITH NO ALARM",
                        "trigger": hex(senderId),
                        "time": getReadableTimeFromTimestamp(now),
                    }
                )
                everTriggered[hex(senderId)] = now
        else:
            currentlyTriggeredDevices[hex(senderId)] = now
            addEvent(
                {
                    "event": "SENSOR TRIGGERED WITH NO ALARM",
                    "trigger": hex(senderId),
                    "time": getReadableTimeFromTimestamp(now),
                }
            )
            everTriggered[hex(senderId)] = now

    # a no-alarm message is coming in from a device that is in the alarmed device list
    elif (
        (receiverId == HOME_BASE_ID or receiverId == BROADCAST_ID)
        and message != ALARM_TRIGGERED_COMMAND
        and hex(senderId) in currentlyTriggeredDevices
    ):
        print(
            f"DEVICE {hex(senderId)} NO LONGER IN currentlyTriggeredDevices - MESSAGE TO REMOVE FROM OLED"
        )
        # home base's arduino should not show this device's ID as one that is currently alarmed
        currentlyTriggeredDevices.pop(hex(senderId), None)
        addEvent(
            {
                "event": "SENSOR TRIGGER STOPPED",
                "trigger": hex(senderId),
                "time": getReadableTimeFromTimestamp(now),
            }
        )
        sendMessage(
            [
                HOME_BASE_ID,
                BASE_STATION_ID,
                NO_LONGER_ALARMED_DEVICE_ID_COMMAND,
                senderId,
            ]
        )

#missingDevicesThatTriggerAlarm is optional and additive - if not present, no missing devices trigger the alarm
#sensorsThatTriggerAlarm is optional - if not present, all devices trigger the alarm
def getProfilesJsonString():
    profilesJSON = ""
    for profile in alarmProfiles:
        profilesJSON += json.dumps(profile) + ","
    profilesJSON = profilesJSON[:-1]

    strReturn = '{"profiles": [' + profilesJSON + "]}"
    return strReturn


def getStatusJsonString():
    outgoingMessageDict = {
        "armStatus": "ARMED" if armed else "DISARMED",
        "alarmStatus": "ALARM" if alarmed else "NORMAL",
        "garageOpen": hex(GARAGE_DOOR_SENSOR_ID) in currentlyTriggeredDevices,
        "profile": alarmProfiles[currentAlarmProfile]["name"],
        "profileNumber": str(currentAlarmProfile),
        "currentTriggeredDevices": list(currentlyTriggeredDevices.keys()),
        "currentMissingDevices": list(currentlyMissingDevices),
        "everTriggered": list(everTriggered.keys()),
        "everTriggeredWithinAlarmCycle": list(everTriggeredWithinAlarmCycle.keys()),
        "everTriggeredWithinArmCycle": list(triggeredDevicesInCurrentArmCycle.keys()),
        "everMissingWithinArmCycle": list(missingDevicesInCurrentArmCycle.keys()),
        "everMissingDevices": list(everMissingDevices.keys()),
        "memberCount": len(memberDevices),
        "memberDevices": list(memberDevices.keys()),
        "memberDevicesReadable": getFriendlyDeviceNamesFromDeviceDictionary(
            list(memberDevices.keys())
        ),
        "quickSetAlarmProfiles": quickSetAlarmProfiles,
        "profileDefinition": alarmProfiles[currentAlarmProfile],
    }
    return json.dumps(outgoingMessageDict)


def getPastEventsJsonString():
    outgoingMessage = '{"pastEvents": ' + str(pastEvents).replace("'", '"')
    outgoingMessage += "}"
    return outgoingMessage


def stopAlarm(trigger = "unspecified"):
    global alarmed
    global lastAlarmTime
    global everTriggeredWithinAlarmCycle
    global currentlyTriggeredDevices

    if (alarmed):
        addEvent(
            {"event": "FINISHED ALARM", "time": getReadableTimeFromTimestamp(lastAlarmTime), "trigger": trigger}
        )
    alarmed = False
    everTriggeredWithinAlarmCycle = {}
    currentlyTriggeredDevices = {}
    sendMessage(
        [HOME_BASE_ID, BASE_STATION_ID, STOP_ALARM_COMMAND, DEVICE_TYPE_HOMEBASE]
    )
    sendMessage( #turn off existing alarms blaring, not sensors
        [HOME_BASE_ID, BROADCAST_ID, ALARM_DISABLE_COMMAND, DEVICE_TYPE_HOMEBASE]
    )


def run(webserver_message_queue):
    global currentlyTriggeredDevices
    global everTriggeredWithinAlarmCycle
    global prev_pastEvents
    global alarmed
    global lastAlarmTime
    global shouldSendDebugMessage
    global shouldSendDebugRepeatedly
    global canDebugMessage
    global armed
    global lastArmedTogglePressed
    global deviceAbsenceThresholdSec
    global firstPowerCommandNeedsToBeSent
    global timeAllottedToBuildOutMembersSec
    global lastSentMessageTimeMsec
    global currentlyMissingDevices
    global lastCheckedMissingDevicesMsec
    global currentAlarmProfile
    global alwaysKeepOnSet  # TODO: LEGACY - for unpowered devices that listen to on/off commands. In new iteration, only powered devices should listen to this.

    resetMemberDevices()

    atexit.register(exitSteps)
    print(
        f"STARTING ALARM SCRIPT AT {getReadableTimeFromTimestamp(getTimeSec())}.\nWAITING {initWaitSeconds} SECONDS TO SET UP SERIAL BUS..."
    )
    time.sleep(initWaitSeconds)
    print(
        f"DONE WAITING, OPERATIONAL NOW AT {getReadableTimeFromTimestamp(getTimeSec())}. STATUS:\nARMED: {armed}\nALARMED: {alarmed}\n\n\n"
    )

    ser.flushOutput()
    ser.flushInput()  # this clears the input buffer, and should not be done routinely during the receiving loop - leads to dropped messages and thus missing devices
    sendMessage(
        [HOME_BASE_ID, BROADCAST_ID, ALARM_DISABLE_COMMAND, DEVICE_TYPE_HOMEBASE]
    )  # turn off all alarms (broadcast)
    sendArmedLedSignal()
    firstTurnedOnTimestamp = getTimeSec()

    while True:
        line = ser.readline()
        if not webserver_message_queue.empty():
            message = webserver_message_queue.get()
            # print(f"GOT MESSAGE: {message}")
            if message["request"] == "ENABLE-ALARM" and getArmedStatus() == False:
                toggleArmed(getTimeSec(), f"WEB API {message['ip']}")
            elif message["request"] == "DISABLE-ALARM" and getArmedStatus() == True:
                toggleArmed(getTimeSec(), f"WEB API {message['ip']}")
            elif message["request"] == "ALARM-STATUS":
                message["responseQueue"].put(
                    {"response": getStatusJsonString(), "web_request_id": message["web_request_id"]}
                )
            elif message["request"].startswith("SET-ALARM-PROFILE-"):
                profileNumber = int(
                    message["request"].split("SET-ALARM-PROFILE-", 1)[1]
                )
                setCurrentAlarmProfile(profileNumber, f"WEB API {message['ip']}")
            elif message["request"] == "GET-ALARM-PROFILES":
                prev_pastEvents = getProfilesJsonString()
                message["responseQueue"].put(
                    {"response": getProfilesJsonString(), "web_request_id": message["web_request_id"]}
                )
            elif message["request"] == "FORCE-ALARM-SOUND-ON":
                currentlyTriggeredDevices[hex(TEST_ALARM_ID)] = getTimeSec()
                sendAlarmMessage(True, True)
                time.sleep(FLOAT_ALARM_TEST_LENGTH_TIME_SEC) #how long to chirp for
                sendAlarmMessage(False, False)
            elif message["request"] == "TOGGLE-GARAGE-DOOR-STATE":
                sendMessage(
                    [
                        HOME_BASE_ID,
                        GARAGE_DOOR_OPENER_ID,
                        MOMENTARY_SWITCH_TRIGGER_COMMAND,
                        DEVICE_TYPE_HOMEBASE,
                    ]
                )
            elif message["request"] == "CLEAR-OLD-DATA":
                clearOldData()
            elif message["request"] == "ALERT-CHECK-PHONES":
                currentlyTriggeredDevices[hex(CHECK_PHONES_ID)] = getTimeSec()
                saveProfile = currentAlarmProfile
                currentAlarmProfile = 0
                sendAlarmMessage(True, True)
                time.sleep(0.1) #in seconds as float
                sendAlarmMessage(False, False)
                currentAlarmProfile = saveProfile
            elif message["request"].startswith("CAN-REPEATEDLY-SEND-"):
                sendcan(message["request"].split("CAN-REPEATEDLY-SEND-")[1], True)
            elif message["request"].startswith("CAN-SINGLE-SEND-"):
                sendcan(message["request"].split("CAN-SINGLE-SEND-")[1], False)
            elif message["request"] == "CAN-STOP-SENDING":
                stopsendingcan()
            elif message["request"] == "GET-PAST-EVENTS":
                message["responseQueue"].put(
                    {"response": getPastEventsJsonString(), "web_request_id": message["web_request_id"]}
                )

        if not line:
            continue  # nothing on CAN -> repeat while loop (since web server message is already taken care of above)

        if (
            firstPowerCommandNeedsToBeSent
            and getTimeSec() > firstTurnedOnTimestamp + timeAllottedToBuildOutMembersSec
        ):
            firstPowerCommandNeedsToBeSent = False
            print(
                f"Members array built at {getReadableTimeFromTimestamp(getTimeSec())} as:"
            )
            for member in memberDevices:
                print(f"{member} : {memberDevices[member]}")
            print("\n\n\n")
            setDevicesPower()
        try:
            decodedLine = line.decode("utf-8")
        except:
            print(
                f">>>>>ERROR ON BUS WHILE PARSING MESSAGE //// SKIPPING THIS MESSAGE<<<<<<"
            )
            continue
        if decodedLine.startswith(
            ">>>"
        ):  # handle debug lines over serial without crashing
            # print(line.decode('utf-8'))
            continue
        try:
            msg = decodeLine(decodedLine)
        except:
            print(f"ERROR WITH PARSING LINE, CONTINUING LOOP<<<<<")
            continue
        msg.append(getTimeSec())
        # print("GETTING", np.array(msg)) #DEBUG: uncomment

        handleMessage(msg)

         # if there is a ui-driven debug message to send, send it now
        if shouldSendDebugMessage:
            handleMessage(canDebugMessage)
            if not shouldSendDebugRepeatedly:
                shouldSendDebugMessage = False
                canDebugMessage = []

        if (
            lastCheckedMissingDevicesMsec + checkForMissingDevicesEveryMsec
            < getTimeMsec()
        ):  # do a check for missing devices
            if debug:
                print(f">>>Checking for missing devices at {getTimeMsec()}")
            previouslyMissingDevices = currentlyMissingDevices
            currentlyMissingDevices = checkMembersOnline()
            newMissingDevices = list(
                set(currentlyMissingDevices) - set(previouslyMissingDevices)
            )
            backOnlineDevices = list(
                set(previouslyMissingDevices) - set(currentlyMissingDevices)
            )

            for backOnlineDevice in backOnlineDevices: #hex string
                addEvent(
                    {
                        "event": "MISSING DEVICE IS BACK ONLINE",
                        "trigger": backOnlineDevice,
                        "time": getReadableTimeFromTimestamp(getTime()),
                    }
                )
                setDevicePower(backOnlineDevice)

            if len(newMissingDevices) > 0:
                print(
                    f">>>>>>>>>>>>>>>>>>>> ADDING MISSING DEVICES {arrayToString(currentlyMissingDevices)} at {getReadableTime()}<<<<<<<<<<<<<<<<<<<"
                )
                
                for (
                    missingDevice
                ) in newMissingDevices:  # each missingDevice is a hex string
                    currentlyTriggeredDevices.pop(
                        missingDevice, None
                    )  # if was triggered, remove from triggered list while adding to missing list
                    
                    if (
                        armed
                        and hasMissingDevicesThatTriggerAlarm()
                    ):
                        alarmed = True
                        lastAlarmTime = getTimeSec()
                        addEvent(
                            {
                                "event": "NEW MISSING DEVICE, ALARM",
                                "trigger": f"missing {missingDevice}",
                                "time": getReadableTimeFromTimestamp(lastAlarmTime),
                            }
                        )
                    else:
                        addEvent(
                            {
                                "event": "NEW MISSING DEVICE, NO ALARM",
                                "trigger": f"missing {missingDevice}",
                                "time": getReadableTimeFromTimestamp(getTime()),
                            }
                        )
            elif (
                alarmed
                and len(currentlyMissingDevices) > 0
                and len(
                    set(
                        alarmProfiles[currentAlarmProfile][
                            "missingDevicesThatTriggerAlarm"
                        ]
                    ).union(set(currentlyMissingDevices))
                )
                == 0
            ):
                stopAlarm(trigger = "all missing devices back online")
        # if currently alarmed and there are no profile-matching missing or alarmed devices and it's been long enough that alarmTimeLengthSec has run out, DISABLE ALARM FLAG
        if (
            alarmed
            and getCurrentProfileAlarmTime() > -1
            and lastAlarmTime + getCurrentProfileAlarmTime() < getTimeSec()
            and not hasMissingDevicesThatTriggerAlarm()
            and not hasTriggeredDevicesThatTriggerAlarm()
        ):
            stopAlarm(trigger = "alarm time elapsed and no more triggered or missing devices")

        # possibly send a message (if it's been sendTimeoutMsec)
        if getTimeMsec() > (lastSentMessageTimeMsec + sendTimeoutMsec):
            sendAlarmMessage(armed, alarmed)


def sendAlarmMessage(armed, alarmed):
    if (
        "alarmOutputDevices" in alarmProfiles[currentAlarmProfile] and armed and alarmed
    ):  # send alarms to chosen devices under this profile (non-default profile)
        for deviceToBeAlarmed in alarmProfiles[currentAlarmProfile][
            "alarmOutputDevices"
        ]:
            sendMessage(
                [
                    HOME_BASE_ID,
                    int(deviceToBeAlarmed, 16),
                    ALARM_ENABLE_COMMAND,
                    DEVICE_TYPE_HOMEBASE,
                ]
            )
            time.sleep(
                5 / 100
            )  # bugfix - can't send in immediate rapid succession, or can fails
    elif (
        "alarmOutputDevices" in alarmProfiles[currentAlarmProfile]
    ):  # send cancel alarms to all devices under this profile (non-default profile)
        for deviceToBeAlarmed in alarmProfiles[currentAlarmProfile][
            "alarmOutputDevices"
        ]:
            sendMessage(
                [
                    HOME_BASE_ID,
                    int(deviceToBeAlarmed, 16),
                    ALARM_DISABLE_COMMAND,
                    DEVICE_TYPE_HOMEBASE,
                ]
            )
            time.sleep(
                5 / 100
            )  # bugfix - can't send in immediate rapid succession, or can fails
    else:  # for profiles missing alarmOutputDevices - broadcast alarm on or off
        sendMessage(
            [
                HOME_BASE_ID,
                BROADCAST_ID,
                ALARM_ENABLE_COMMAND if armed and alarmed else ALARM_DISABLE_COMMAND,
                DEVICE_TYPE_HOMEBASE,
            ]
        )


def getCurrentProfileAlarmTime():
    return alarmProfiles[currentAlarmProfile]["alarmTimeLengthSec"]


def getProfileName(profileNumber):
    return alarmProfiles[profileNumber]["name"]


def clearOldData():
    global everTriggered
    global everTriggeredWithinAlarmCycle
    global triggeredDevicesInCurrentArmCycle
    global missingDevicesInCurrentArmCycle
    global everMissingDevices
    global currentlyMissingDevices
    global pastEvents

    everTriggeredWithinAlarmCycle = {}
    everTriggered = {}
    triggeredDevicesInCurrentArmCycle = {}
    missingDevicesInCurrentArmCycle = {}
    everMissingDevices = {}
    currentlyMissingDevices = []
    pastEvents = []
    resetMemberDevices()


def stopsendingcan():
    global canDebugMessage
    global shouldSendDebugRepeatedly
    global shouldSendDebugMessage
    global currentlyTriggeredDevices

    canDebugMessage = []
    shouldSendDebugRepeatedly = False
    shouldSendDebugMessage = False
    currentlyTriggeredDevices = {}

    print("STOPPING SENDING FAKE MESSAGE FROM UI")
    addEvent(
        {
            "event": "STOPPING SENDING DEBUG CAN MESSAGE FROM UI",
            "time": getReadableTimeFromTimestamp(getTimeSec()),
            "trigger": "WEB API",
        }
    )


def sendcan(message, repeatedly):
    global canDebugMessage
    global shouldSendDebugRepeatedly
    global shouldSendDebugMessage
    messageConforms = True

    arrCanDebugMessage = message.split(":")
    if len(arrCanDebugMessage) == 4:
        for index, i in enumerate(arrCanDebugMessage):
            if not i.startswith("0x") or len(i) != 4:
                messageConforms = False
                break
            else:
                arrCanDebugMessage[index] = int(i, 16)
        if messageConforms:
            hexedMessage = "[" + ", ".join([hex(n) for n in arrCanDebugMessage]) + "]"
            print(
                "SENDING FAKE MESSAGE FROM UI "
                + hexedMessage
                + (" REPEATEDLY " if repeatedly else "")
            )
            addEvent(
                {
                    "event": "STARTING SENDING DEBUG CAN MESSAGE "
                    + hexedMessage
                    + " FROM UI"
                    + (" REPEATEDLY" if repeatedly else ""),
                    "time": getReadableTimeFromTimestamp(getTimeSec()),
                    "trigger": "WEB API",
                }
            )
            canDebugMessage = arrCanDebugMessage
            shouldSendDebugRepeatedly = True if repeatedly else False
            shouldSendDebugMessage = True


if __name__ == "__main__":
    run(None)  # For testing in standalone mode
