import React, { useCallback, useEffect, useRef, useState } from "react";

type MjpegImageProps = {
  /** Your MJPEG endpoint, e.g. "/video/" */
  src: string;
  /** Periodic refresh to unstick silent stalls (ms). Set 0 to disable. */
  refreshMs?: number;
  /** First retry delay after an error (ms). */
  initialBackoff?: number;
  /** Max retry delay after exponential backoff (ms). */
  maxBackoff?: number;
  /** Pass through to the underlying <img> (className, style, alt, etc.). */
  imgProps?: Omit<React.ImgHTMLAttributes<HTMLImageElement>, "src" | "onLoad" | "onError">;
};

const MjpegImage: React.FC<MjpegImageProps> = ({
  src,
  refreshMs = 60_000,
  initialBackoff = 1_000,
  maxBackoff = 15_000,
  imgProps = {},
}) => {
  const [bustedSrc, setBustedSrc] = useState<string>("");
  const backoffRef = useRef<number>(initialBackoff);
  const timerRef = useRef<number | null>(null);
  const mountedRef = useRef<boolean>(false);

  const bust = useCallback(() => {
    const sep = src.includes("#") ? "&" : "#";
    setBustedSrc(`${src}${sep}t=${Date.now()}`);
  }, [src]);

  const stopTimer = useCallback(() => {
    if (timerRef.current) {
      window.clearInterval(timerRef.current);
      timerRef.current = null;
    }
  }, []);

  const startTimer = useCallback(() => {
    stopTimer();
    if (refreshMs > 0) {
      timerRef.current = window.setInterval(bust, refreshMs);
    }
  }, [refreshMs, bust, stopTimer]);

  const restartWithBackoff = useCallback(() => {
    stopTimer();
    const delay = backoffRef.current;
    backoffRef.current = Math.min(Math.ceil(backoffRef.current * 1.6), maxBackoff);
    window.setTimeout(() => {
      if (!mountedRef.current) return;
      bust();
    }, delay);
  }, [maxBackoff, bust, stopTimer]);

  const handleLoad = useCallback(() => {
    backoffRef.current = initialBackoff; // reset on success
    startTimer();
  }, [initialBackoff, startTimer]);

  const handleError = useCallback(() => {
    restartWithBackoff();
  }, [restartWithBackoff]);

  useEffect(() => {
    mountedRef.current = true;
    bust(); // kick off

    const vis = () => {
      if (!document.hidden) bust();
    };
    const online = () => bust();

    document.addEventListener("visibilitychange", vis);
    window.addEventListener("online", online);

    return () => {
      mountedRef.current = false;
      stopTimer();
      document.removeEventListener("visibilitychange", vis);
      window.removeEventListener("online", online);
    };
  }, [bust, stopTimer]);

  return (
    // eslint-disable-next-line @next/next/no-img-element
    <img
      {...imgProps}
      className={"roundedCorners"}
      id={"securityVideo"}
      src={bustedSrc}
      onLoad={handleLoad}
      onError={handleError}
      alt=""
    />
  );
};

export default MjpegImage;