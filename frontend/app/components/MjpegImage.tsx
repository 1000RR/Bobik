/* eslint-disable @next/next/no-img-element */
import React, { useCallback, useEffect, useRef, useState } from "react";
import PowerIndicator from "@components/PowerIndicator";
import { useSelector } from "react-redux";
import { AppStateSlice } from "@components/AppStateSlice";

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
  videoSize?: VIDEO_SIZE;
};

export enum VIDEO_SIZE {
  SMALL = "small",
  MEDIUM = "medium",
  LARGE = "large"
}

const MjpegImage: React.FC<MjpegImageProps> = ({
  src,
  refreshMs = 60_000,
  initialBackoff = 1_000,
  maxBackoff = 15_000,
  imgProps = {},
  videoSize: VideoSize
}) => {
  const [bustedSrc, setBustedSrc] = useState<string>("");
  const backoffRef = useRef<number>(initialBackoff);
  const timerRef = useRef<number | null>(null);
  const mountedRef = useRef<boolean>(false);
  const [videoSize, setVideoSize] = useState<VIDEO_SIZE>(VideoSize ?? VIDEO_SIZE.LARGE);
  const overlayRef = useRef<HTMLDivElement>(null);
  const videoElementRef = useRef<HTMLImageElement>(null);
  const videoEnclosureRef = useRef<HTMLDivElement>(null);
  const alarmStatusIndicatorRef = useRef<HTMLDivElement>(null);
  const fullscreenButtonRef = useRef<HTMLDivElement>(null);
  const isArmed = useSelector(function (state: AppStateSlice) { 
        return state.appState.status?.armStatus === "ARMED"
    });

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

  const handleClick = useCallback(() => {
    if (videoElementRef?.current?.classList.contains('full')) {
      return;
    }
    if (videoSize == VIDEO_SIZE.LARGE) {
      setVideoSize(VIDEO_SIZE.MEDIUM);
    } else if (videoSize == VIDEO_SIZE.MEDIUM) {
      setVideoSize(VIDEO_SIZE.SMALL);
    } else {
      setVideoSize(VIDEO_SIZE.LARGE);
    }

    
    if (overlayRef?.current?.classList.contains('flash')) {
      overlayRef?.current.classList.remove('flash')
      setTimeout(()=> overlayRef?.current?.classList.add('flash'), 50);
    }
  }, [videoSize, overlayRef]);

   const handleToggleFullscreenClick = useCallback((event: React.MouseEvent<HTMLDivElement, MouseEvent>) => {
    videoEnclosureRef.current?.classList.toggle('video-maximized');
    videoElementRef.current?.classList.toggle('full');
    alarmStatusIndicatorRef.current?.classList.toggle('displaynone');
  }, [videoElementRef, videoEnclosureRef, alarmStatusIndicatorRef]);

  useEffect(() => {
    mountedRef.current = true;
    bust(); // kick off

    const vis = () => {
      if (!document.hidden) bust();
    };
    const online = () => bust();

    document.addEventListener("visibilitychange", vis);
    window.addEventListener("online", online);

    return () => {``
      mountedRef.current = false;
      stopTimer();
      document.removeEventListener("visibilitychange", vis);
      window.removeEventListener("online", online);
    };
  }, [bust, stopTimer]);

  return (
    // eslint-disable-next-line @next/next/no-img-element
    bustedSrc.length ? 
    <div style={{width: "fit-content", height: "fit-content", position: "relative"}} ref={videoEnclosureRef}>
      <div ref={fullscreenButtonRef} className="video-fullscreen-button" onClick={handleToggleFullscreenClick}>
        <img src={"/assets/fullscreen.svg"} height="50" width="50" alt="Toggle Fullscreen" />
      </div>
      <div ref={alarmStatusIndicatorRef} className="video-fullscreen-alarm-status-indicator displaynone">
        <PowerIndicator secondsPerRotation={isArmed ? 1 : 0} color={isArmed ? "cyan" : "maroon"} dotColor={isArmed ? "#215dbe" : "red"}></PowerIndicator>
      </div>
      <div
        ref={overlayRef}
        className="video-overlay flash">{videoSize[0].toUpperCase()}
      </div>
      <img
          {...imgProps}
          className={`video roundedCorners ${videoSize}`}
          id={"securityVideo"}
          src={bustedSrc}
          onLoad={handleLoad}
          onError={handleError}
          onClick={handleClick}
          alt=""
          ref={videoElementRef}
      />
      <style>{`
        
      `}</style>
    </div> : <></>
  );
};

export default MjpegImage;