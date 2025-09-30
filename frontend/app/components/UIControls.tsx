import React, {
  useCallback,
  useEffect,
  useImperativeHandle,
  useRef,
  useState,
  forwardRef
} from "react";
import styled from "styled-components";
import AlarmAudio, { AlarmAudioRef } from "@components/AlarmAudio";

const Container = styled.div`
  display: flex;
  flex-direction: row;
  flex-flow: row wrap;
  gap: 2rem;
  align-items: center;
  justify-content: center;
  font-family: sans-serif;
  padding: 1rem;
  width: 100%;
`;

// --- WakeLock types ---
type WakeLockType = "screen";
interface WakeLockSentinelLike extends EventTarget {
  released: boolean;
  release(): Promise<void>;
}
interface NavigatorWithWakeLock extends Navigator {
  WakeLock?: {
    request(type: WakeLockType): Promise<WakeLockSentinelLike>;
  };
}

export interface NotificationController {
  sendNotification: (title: string, body: string, tag: string, renotify: boolean) => void;
  playAlarmSound: () => void;
  stopAlarmSound: () => void;
}

// eslint-disable-next-line react/display-name
export const UIControls = forwardRef<NotificationController>((_, ref) => {
  const [wakeLockSupported, setWakeLockSupported] = useState(false);
  const [wakeLockActive, setWakeLockActive] = useState(false);
  const wakeLockRef = useRef<WakeLockSentinelLike | null>(null);
  const [notifSupported, setNotifSupported] = useState(false);
  const [notifEnabled, setNotifEnabled] = useState(false);
  const [beepOnAlarmEnabled, setBeepOnAlarmEnabled] = useState(false);
  const [loudAlarmEnabled, setLoudAlarmEnabled] = useState(false); 

  const alarmRef = useRef<AlarmAudioRef>(null);

  useEffect(() => {
    setWakeLockSupported(Boolean((navigator as NavigatorWithWakeLock).wakeLock));
  }, []);

  const acquireWakeLock = useCallback(async () => {
    const nav = navigator as NavigatorWithWakeLock;
    if (!nav.wakeLock) return;
    try {
      wakeLockRef.current = await nav.wakeLock.request("screen");
      setWakeLockActive(true);
      wakeLockRef.current.addEventListener("release", () => {
        setWakeLockActive(false);
      });
    } catch {
      setWakeLockActive(false);
    }
  }, []);

  const releaseWakeLock = useCallback(async () => {
    try {
      await wakeLockRef.current?.release();
    } catch {}
    wakeLockRef.current = null;
    setWakeLockActive(false);
  }, []);

  const toggleWakeLock = () => {
    wakeLockActive ? releaseWakeLock() : acquireWakeLock();
  };

  useEffect(() => {
    const onVis = () => {
      if (document.visibilityState === "visible" && wakeLockRef.current?.released) {
        acquireWakeLock();
      }
    };
    document.addEventListener("visibilitychange", onVis);
    return () => document.removeEventListener("visibilitychange", onVis);
  }, [acquireWakeLock]);

  useEffect(() => {
    "Notification" in window && Notification.permission === "granted" && setNotifSupported(true);
  }, []);

  const enableNotifications = async () => {
    if (!("Notification" in window)) return;
    setNotifEnabled(true);
  };

  const disableNotifications = () => setNotifEnabled(false);

  const toggleNotifications = () => {
    if ("Notification" in window && Notification.permission !== "granted") {
      Notification.requestPermission().then((permission) => {
        permission === "granted" ? enableNotifications() : disableNotifications();
      });
    } else {
      notifEnabled ? disableNotifications() : enableNotifications();
    }
  };

  const toggleBeepOnAlarm = async () => {
    beepOnAlarmEnabled && alarmRef.current?.stop();
    beepOnAlarmEnabled ? setBeepOnAlarmEnabled(false) : setBeepOnAlarmEnabled(true);
    try {
      await alarmRef.current?.unlock();
    } catch (e) {
      console.error(e);
    }
  };

  const toggleLoudAlarm = () => {
    if (!beepOnAlarmEnabled)
      return;
    alarmRef.current?.stop();
    setLoudAlarmEnabled(!loudAlarmEnabled);
  };

  useImperativeHandle(ref, () => ({
    sendNotification: (title: string, body: string, tag: string, renotify: boolean) => {
      if (!notifEnabled || Notification.permission !== "granted") return;
      //@ts-expect-error TS2345
      new Notification(title, { body, tag, renotify});
    },
    playAlarmSound: () => {
      if (beepOnAlarmEnabled) {
        alarmRef.current?.play();
      }
    },
    stopAlarmSound: () => {
      alarmRef.current?.stop();
    }
  }));

  return (
    <>
    <Container>
      <label className="toggleSwitch" style={{ display: "flex", alignItems: "center", gap: "0.5rem",  flexDirection: "column" }}>
        <span>Inhibit Screen Sleep</span>
        <div
          className={`ios-switch ${wakeLockActive ? "checked" : ""} ${
            !wakeLockSupported ? "disabled" : ""
          }`}
          onClick={() => wakeLockSupported && toggleWakeLock()}
        >
          <div className="ios-knob" />
        </div>
      </label>

      <label className="toggleSwitch" style={{ display: "flex", alignItems: "center", gap: "0.5rem", flexDirection: "column" }}>
        <span>Notifications</span>
        <div
          className={`ios-switch ${notifEnabled ? "checked" : ""} ${
            !notifSupported ? "disabled" : ""
          }`}
          onClick={() => notifSupported && toggleNotifications()}
        >
          <div className="ios-knob" />
        </div>
      </label>

      <label className="toggleSwitch" style={{ display: "flex", alignItems: "center", gap: "0.5rem", flexDirection: "column" }}>
        <span>Sound on alarm</span>
        <div
          className={`ios-switch ${beepOnAlarmEnabled ? "checked" : ""}`}
          onClick={() => toggleBeepOnAlarm()}
        >
          <div className="ios-knob" />
        </div>
      </label>

      <label className="toggleSwitch" style={{ display: "flex", alignItems: "center", gap: "0.5rem", flexDirection: "column" }}>
        <span>Soft beep / Loud sound</span>
        <div
          className={`ios-switch ${loudAlarmEnabled ? "checked" : ""} ${beepOnAlarmEnabled ? "" : "disabled"}`}
          onClick={() => toggleLoudAlarm()}
        >
          <div className="ios-knob knob-equal-choices" />
        </div>
      </label>
    </Container>

      <AlarmAudio srcDataUri={loudAlarmEnabled ? '/alarm-loud.mp3' : '/alarm-beep-short.mp3'} ref={alarmRef} loop={true} volume={1.0} />
    </>
  );
});