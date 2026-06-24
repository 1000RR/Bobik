"""
Denon AVR local device controller plugin.
Handles all interaction with the Denon AV receiver (power, channel, volume, audio playback).

Plugin interface contract:
  - register(callbacks_dict)   called by alarm.py to provide hook callbacks
  - device_id()                returns this device's CAN address (0x77)
  - get_device_entry()         returns dict entry for memberDevices registration
  - on_alarm_enabled(...)      called when ALARM_ENABLE_COMMAND is sent to this device

Callbacks expected from alarm.py:
  - alarm_get_profile_sound_byte_data  -> (playSound: str, playSoundVolume: int)
  - device_dictionary                  -> full DEVICE_DICTIONARY dict from alarmconstants
  - mp3_alarm_dictionary               -> full MP3_ALARM_DICTIONARY dict from alarmconstants
"""

import os
import subprocess
import time
from datetime import datetime
import math
from threading import Thread

# ------------------------------------------------------------------
# Constants (local to this device only — not derived from alarmconstants)
# ------------------------------------------------------------------

DEVICE_ID = 0x77          # CAN address of this local device; owned by this plugin
avrSoundChannel = "SAT/CBL"
MP3_PLAYER_PROGRAM = ["/usr/bin/mpg123", "-o", "alsa", "-a", "hw:2,0"]

# ------------------------------------------------------------------
# Paths (derived from this file's location)
# ------------------------------------------------------------------

_plugin_dir = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.join(_plugin_dir, "scripts")
SOUNDS_DIR = os.path.join(_plugin_dir, "sounds")

# ------------------------------------------------------------------
# Module-level state
# ------------------------------------------------------------------

_denon_play_thread = None   # owned by plugin; alarm.py no longer tracks this

_local_callbacks = {}       # populated via register()

# ------------------------------------------------------------------
# Helpers — reverse lookup ID constants from device_dictionary
#
# Alarm.py passes DEVICE_DICTIONARY which maps hex string keys (e.g. "0xDE")
# to human-readable names. We need the integer form to do hex comparisons.
# These helpers look up by value substring match and return int IDs at runtime.
# ------------------------------------------------------------------

def _get_id_from_dict(target_key_substring):
    """
    Reverse-lookup an ID constant from device_dictionary.

    The dictionary maps hex string keys ("0xDE", "0x17") to names.
    We find the key whose hex(int) form matches one of our known IDs
    by scanning for entries that end with the target hex string.

    Returns the int ID, or None if not found.
    """
    dev_dict = _local_callbacks.get("device_dictionary", {})
    for dict_key in dev_dict:
        if dict_key.endswith(target_key_substring) or dict_key == target_key_substring:
            return int(dict_key, 16)
    return None

def TEST_ALARM_ID():
    return _get_id_from_dict("0xDE")

def CHECK_PHONES_ID():
    return _get_id_from_dict("0x17")

def GARAGE_DOOR_SENSOR_ID():
    # Not directly used in this plugin but available for lookups
    return _get_id_from_dict("0x30")


# ------------------------------------------------------------------
# Public plugin interface
# ------------------------------------------------------------------

def register(callbacks):
    """
    Called by alarm.py to provide callbacks the plugin may invoke.

    Expected callback keys:
      - alarm_get_profile_sound_byte_data  -> (playSound: str, playSoundVolume: int)
      - device_dictionary                  -> full DEVICE_DICTIONARY dict from alarmconstants
      - mp3_alarm_dictionary               -> full MP3_ALARM_DICTIONARY dict from alarmconstants
    """
    global _local_callbacks
    _local_callbacks = callbacks


def device_id():
    return DEVICE_ID


def get_device_entry(now_override=None):
    """Returns the memberDevices dict entry for this local device."""
    if now_override is None:
        now_override = _time_sec()
    dev_dict = _local_callbacks.get("device_dictionary", {})
    friendly_name = dev_dict.get(hex(DEVICE_ID), "unlisted")
    return {
        "id": hex(DEVICE_ID),
        "firstSeen": now_override,
        "firstSeenReadable": _readable_time_from_timestamp(now_override),
        "deviceType": "0x10",
        "lastSeen": now_override,
        "lastSeenReadable": _readable_time(),
        "friendlyName": friendly_name,
        "lastArmedTimeSec": -1,
    }


def on_alarm_enabled(currently_triggered_devices, ever_alarmed_during_alarm):
    """
    Called by alarm.py when an ALARM_ENABLE_COMMAND is sent to this device or broadcast.
    Runs the Denon playback sequence in a background thread.

    Args:
        currently_triggered_devices: dict {hex_id_str: timestamp}
        ever_alarmed_during_alarm:   dict {hex_id_str: timestamp}
    """
    global _denon_play_thread

    if _denon_play_thread and _denon_play_thread.is_alive():
        return  # already playing

    _denon_play_thread = Thread(
        target=_play_denon_main,
        args=(currently_triggered_devices, ever_alarmed_during_alarm),
        daemon=True,
    )
    _denon_play_thread.start()


# ------------------------------------------------------------------
# Private — playback thread main
# ------------------------------------------------------------------

def _play_denon_main(currently_triggered_devices, ever_alarmed_during_alarm):
    play_command_array = list(MP3_PLAYER_PROGRAM)
    volume = "55"

    test_id = TEST_ALARM_ID()
    check_phones_id = CHECK_PHONES_ID()

    play_command_array, volume = _determine_stuff_to_play(
        play_command_array,
        volume,
        ever_alarmed_during_alarm,
        currently_triggered_devices,
        test_id,
        check_phones_id,
    )

    start_power, start_channel, start_volume = _get_denon_initial_state()
    if start_power is False:
        return  # Denon not found / unreachable

    _set_denon_play_state(start_power, start_channel, volume)
    _play_sounds(play_command_array)
    _restore_denon_original_state(start_power, start_channel, start_volume)


# ------------------------------------------------------------------
# Private — sound selection
# ------------------------------------------------------------------

def _determine_stuff_to_play(
    play_command_array,
    volume,
    ever_alarmed_during_alarm,
    currently_triggered_devices,
    test_id,
    check_phones_id,
):
    """Decide what audio to play and at what volume."""
    sound = ""

    if hex(test_id) in currently_triggered_devices:
        sound = "thisisatest.mp3"
        tmp = dict(currently_triggered_devices)
        tmp.pop(hex(test_id), None)
        currently_triggered_devices = tmp
    elif hex(check_phones_id) in currently_triggered_devices:
        sound = "checkyourphones.mp3"
        volume = "79"
        tmp = dict(currently_triggered_devices)
        tmp.pop(hex(check_phones_id), None)
        currently_triggered_devices = tmp
    else:
        play_command_array.append("alert.mp3")
        profile_cb = _local_callbacks.get("alarm_get_profile_sound_byte_data")
        if profile_cb:
            sound_override, volume_override = profile_cb()
            if sound_override and volume_override:
                volume = str(volume_override)
                sound = sound_override

    if sound:
        play_command_array.append(sound)
    else:
        mp3_dict = _local_callbacks.get("mp3_alarm_dictionary", {})
        for device_hex in ever_alarmed_during_alarm:
            resolved_mp3 = mp3_dict.get(device_hex)
            if resolved_mp3:
                play_command_array.append(resolved_mp3)

    return play_command_array, volume


# ------------------------------------------------------------------
# Private — Denon hardware interaction helpers
# ------------------------------------------------------------------

def _play_sounds(play_command_array):
    subprocess.run(play_command_array, cwd=SOUNDS_DIR)


def _set_denon_play_state(start_power_status, start_channel_status, volume):
    """Power on (if needed), switch to avrSoundChannel, set volume."""
    if start_power_status != "ON" or start_channel_status != avrSoundChannel:
        subprocess.run([os.path.join(SCRIPTS_DIR, "denonon.sh")], cwd=SCRIPTS_DIR)
        time.sleep(8 if start_power_status != "ON" else 3)

    subprocess.run(
        [os.path.join(SCRIPTS_DIR, "denonvol.sh"), str(volume)],
        cwd=str(SCRIPTS_DIR),
    )


def _restore_denon_original_state(start_power_status, start_channel_status, start_volume):
    """Restore original power and channel state after playback."""
    if start_power_status != "ON":
        subprocess.run([os.path.join(SCRIPTS_DIR, "denonoff.sh")], cwd=SCRIPTS_DIR)
    else:
        subprocess.run(
            [os.path.join(SCRIPTS_DIR, "denonvol.sh"), str(start_volume)],
            cwd=SCRIPTS_DIR,
        )
        if start_channel_status != avrSoundChannel:
            subprocess.run(
                [os.path.join(SCRIPTS_DIR, "denonchannel.sh"), str(start_channel_status)],
                cwd=SCRIPTS_DIR,
            )


def _get_denon_initial_state():
    """
    Query the Denon's current power, channel, and volume.
    Returns (power_status, channel_status, volume) or (False, False, False) if unreachable.
    """
    # Power
    start_power = str(
        subprocess.run(
            [os.path.join(SCRIPTS_DIR, "denonpowerstatus.sh")],
            cwd=SCRIPTS_DIR,
            stderr=None,
            capture_output=True,
        ).stdout
    ).translate({ord(c): None for c in "b'\\n"})

    if start_power == "":
        print(">>>>DENON NOT FOUND")
        return False, False, False

    # Channel
    start_channel = str(
        subprocess.run(
            [os.path.join(SCRIPTS_DIR, "denonchannelstatus.sh")],
            cwd=SCRIPTS_DIR,
            stderr=None,
            capture_output=True,
        ).stdout
    ).translate({ord(c): None for c in "b'\\n"})

    # Volume
    tempvol = str(
        subprocess.run(
            [os.path.join(SCRIPTS_DIR, "denonvolumestatus.sh")],
            cwd=SCRIPTS_DIR,
            stderr=None,
            capture_output=True,
        ).stdout
    ).translate({ord(c): None for c in "b'\\n"})
    if tempvol == "--":
        tempvol = "0"
    start_volume = str(int(float(tempvol) + 81))

    return start_power, start_channel, start_volume


# ------------------------------------------------------------------
# Private — time / formatting utilities
# ------------------------------------------------------------------

def _time_sec():
    return math.floor(datetime.now().timestamp())


def _readable_time():
    return _readable_time_from_timestamp(_time_sec())


def _readable_time_from_timestamp(timestamp):
    return f"{datetime.fromtimestamp(timestamp).strftime('%c')} LOCAL TIME"