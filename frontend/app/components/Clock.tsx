import React, { useEffect, useState } from "react";

const Clock: React.FC = () => {
  const [now, setNow] = useState<Date>(new Date());

  useEffect(() => {
    // Function to update time
    const updateTime = () => {
      setNow(new Date());
    };

    // Calculate delay until the next full second
    const msUntilNextSecond = 1000 - new Date().getMilliseconds();

    // First sync at the exact next second boundary
    const timeout = setTimeout(() => {
      updateTime();

      // Then tick every 1 second exactly
      const interval = setInterval(updateTime, 1000);

      return () => clearInterval(interval);
    }, msUntilNextSecond);

    return () => clearTimeout(timeout);
  }, []);

  return (
    <div className={"clock"}>
      {now.toLocaleString("en-US", {
        month: "numeric",   // "1" … "12"
        day: "numeric",     // "1" … "31"
        hour: "2-digit",    // "00" … "23" with hour12:false
        minute: "2-digit",  // "00" … "59"
        second: "2-digit",  // "00" … "59"
        hour12: false       // force 24-hour clock
      })} {/* localized date + time */}
    </div>
  );
};

export default Clock;