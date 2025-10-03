import React, { useEffect, useState } from "react";

const Clock: React.FC = () => {
  const [now, setNow] = useState<Date>(new Date());

  useEffect(() => {
    const updateTime = () => setNow(new Date());

    // Calculate delay until the next full second
    const msUntilNextSecond = 1000 - new Date().getMilliseconds();

    // Timeout to sync at next second boundary
    const secondTimeout = setTimeout(() => {
      updateTime();

      // Tick every 1 second exactly
      const secondInterval = setInterval(updateTime, 1000);

      // Also set up a resync at the next top of the hour
      const d = new Date();
      const msUntilNextHour =
        (60 - d.getMinutes()) * 60 * 1000 -
        d.getSeconds() * 1000 -
        d.getMilliseconds();

      const hourTimeout = setTimeout(() => {
        // Clear the second interval and restart sync logic
        clearInterval(secondInterval);
        updateTime(); // force update at hour boundary
      }, msUntilNextHour);

      // Cleanup
      return () => {
        clearInterval(secondInterval);
        clearTimeout(hourTimeout);
      };
    }, msUntilNextSecond);

    return () => clearTimeout(secondTimeout);
  }, []);

  return (
    <div className="clock">
      {now.toLocaleString(navigator.language, {
        month: "numeric",   // "1" … "12"
        day: "numeric",     // "1" … "31"
        hour: "numeric",    // "00" … "23"
        minute: "2-digit",  // "00" … "59"
        second: "2-digit",  // "00" … "59"
        hour12: false,       // force 24-hour clock
        timeZoneName: "short" // "GMT", "PST", "PDT", etc.
      })}
    </div>
  );
};

export default Clock;