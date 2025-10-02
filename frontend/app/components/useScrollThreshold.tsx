import { useSyncExternalStore } from "react";

let isAboveThreshold = false;
const listeners = new Set<() => void>();
let thresholdValue = 10; // default

function getSnapshot() {
  return isAboveThreshold;
}

function subscribe(callback: () => void) {
  listeners.add(callback);

  if (listeners.size === 1) {
    // First subscriber → attach global listener
    window.addEventListener("scroll", handleScroll);
    // Initialize state
    handleScroll();
  }

  return () => {
    listeners.delete(callback);
    if (listeners.size === 0) {
      // No subscribers → detach global listener
      window.removeEventListener("scroll", handleScroll);
    }
  };
}

function handleScroll() {
  const newState = window.scrollY > thresholdValue;
  if (newState !== isAboveThreshold) {
    isAboveThreshold = newState;
    listeners.forEach((cb) => cb());
  }
}

export function useScrollThreshold(threshold: number) {
  thresholdValue = threshold;
  return useSyncExternalStore(subscribe, getSnapshot);
}