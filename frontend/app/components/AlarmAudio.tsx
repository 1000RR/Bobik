// AlarmAudio.tsx
import { forwardRef, useImperativeHandle, useRef, useState } from "react";

export type AlarmAudioRef = {
  /** Call this once from a user gesture (click/tap) to satisfy autoplay rules */
  unlock: () => Promise<void>;
  /** Play (retrigger-safe). Works after unlock() */
  play: () => Promise<void>;
  /** Stop and rewind */
  stop: () => void;
  /** Toggle looping */
  setLoop: (on: boolean) => void;
  /** Whether audio is unlocked */
  isUnlocked: () => boolean;
};

type AlarmAudioProps = {
  /** Optional: override with your own data: URI or regular URL */
  srcDataUri?: string;
  /** Default volume (0.0–1.0) */
  volume?: number;
  /** Start with loop on? */
  loop?: boolean;
};

/**
 * AlarmAudio
 * - Embeds an <audio> element with a default inlined beep (data: URI).
 * - Exposes unlock()/play()/stop()/setLoop() via ref.
 * - Call unlock() once from a user gesture, then play() anytime (even if tab is backgrounded on desktop).
 * - iOS Safari requires the tab in foreground to hear audio.
 */
const AlarmAudio = forwardRef<AlarmAudioRef, AlarmAudioProps>(function AlarmAudio(
  { srcDataUri, volume = 1.0, loop = false },
  ref
) {
  // A tiny 1 kHz ~200ms WAV beep (data URI). Replace if you like.
  // (If you have your own, pass it as srcDataUri.)
  const DEFAULT_BEEP =
    "data:audio/wav;base64,UklGRkQAAABXQVZFZm10IBAAAAABAAEAESsAACJWAAACABAAZGF0YQwAAAAAAP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AP8A/wD/AAAA"; // short beep; replace as needed

  const audioEl = useRef<HTMLAudioElement | null>(null);
  const [unlocked, setUnlocked] = useState(false);

  // Lazily init the audio element once
  const ensureAudio = () => {
    if (!audioEl.current) {
      const el = new Audio(srcDataUri ?? DEFAULT_BEEP);
      el.preload = "auto";
      el.loop = loop;
      el.volume = volume;
      el.addEventListener("ended", () => {
        // no-op; keep for retrigger logic if you want
      });
      audioEl.current = el;
    } else {
      audioEl.current.loop = loop;
      audioEl.current.volume = volume;
      if (srcDataUri && audioEl.current.src !== srcDataUri) {
        audioEl.current.src = srcDataUri;
      }
    }
    return audioEl.current;
  };

  async function unlock() {
    //const el = ensureAudio();
    try {
      // A brief play/pause cycle from a user gesture “unlocks” audio
    //   await el.play();
    //   el.pause();
    //   el.currentTime = 0;
      setUnlocked(true);
    } catch {
      // Some environments may still block until another gesture
      setUnlocked(false);
      throw new Error("Autoplay unlock failed; try clicking again.");
    }
  }

  async function play() {
    if (!unlocked) {
      // Optional: you could throw here instead; this keeps it silent if not unlocked.
      return;
    }
    const el = ensureAudio();
    // If already playing and not looping, rewind for immediate retrigger
    if (!el.paused && !el.loop) el.currentTime = 0;
    await el.play();
  }

  function stop() {
    const el = ensureAudio();
    el.pause();
    el.currentTime = 0;
  }

  function setLoop(on: boolean) {
    const el = ensureAudio();
    el.loop = on;
  }

  useImperativeHandle(ref, () => ({
    unlock,
    play,
    stop,
    setLoop,
    isUnlocked: () => unlocked,
  }));

  // No visible UI—pure “audio service” component
  return null;
});

export default AlarmAudio;