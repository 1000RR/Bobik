import React, {
  useCallback,
  useEffect,
  useImperativeHandle,
  useRef,
  useState,
  forwardRef
} from "react";

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
}

// eslint-disable-next-line react/display-name
export const UIControls = forwardRef<NotificationController>((_, ref) => {
  const [wakeLockSupported, setWakeLockSupported] = useState(false);
  const [wakeLockActive, setWakeLockActive] = useState(false);
  const wakeLockRef = useRef<WakeLockSentinelLike | null>(null);
  const [notifSupported, setNotifSupported] = useState(false);
  const [notifEnabled, setNotifEnabled] = useState(false);

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

  useImperativeHandle(ref, () => ({
    sendNotification: (title: string, body: string, tag: string, renotify: boolean) => {
      if (!notifEnabled || Notification.permission !== "granted") return;
      //@ts-expect-error TS2345
      new Notification(title, { body, tag, renotify});
    },
  }));

  return (
    <div
      style={{
        display: "flex",
        flexDirection: "row",
        gap: "2rem",
        alignItems: "center",
        justifyContent: "center",
        fontFamily: "sans-serif",
        padding: "1rem",
      }}
    >
      <label style={{ display: "flex", alignItems: "center", gap: "0.5rem" }}>
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

      <label style={{ display: "flex", alignItems: "center", gap: "0.5rem" }}>
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

      <style>{`
        .ios-switch {
          position: relative;
          width: 50px;
          height: 28px;
          border-radius: 14px;
          background: #ccc;
          transition: background 0.2s;
          cursor: pointer;
        }
        .ios-switch.checked {
          background: #34c759; /* iOS green */
        }
        .ios-switch.disabled {
          opacity: 0.5;
          cursor: not-allowed;
        }
        .ios-knob {
          position: absolute;
          top: 2px;
          left: 2px;
          width: 24px;
          height: 24px;
          background: #fff;
          border-radius: 50%;
          box-shadow: 0 1px 3px rgba(0,0,0,0.3);
          transition: left 0.2s;
        }
        .ios-switch.checked .ios-knob {
          left: 24px;
        }
      `}</style>
    </div>
  );
});