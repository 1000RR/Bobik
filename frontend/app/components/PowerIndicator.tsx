import React from "react";
import ParseUtils from "@src/ColorUtils";

interface PowerIndicatorProps {
  color?: string;       // main circle color (gradient)
  size?: number;        // svg width/height in px (default 40)
  dotColor?: string;    // orbiting dot color (defaults to color)
  dotRadius?: number;   // radius of the orbiting dot
  secondsPerRotation?: number;       // seconds per full rotation
  rimInset?: number;    // how far inside the rim the dot orbits
}

const PowerIndicator: React.FC<PowerIndicatorProps> = ({
  color = "#999999",        // main circle (default medium grey)
  size = 40,
  dotColor,                 // orbiting dot
  dotRadius = 5,
  secondsPerRotation = 1,
  rimInset = 3,
}) => {
  const r = size / 2;
  const cx = r, cy = r;
  const orbitR = Math.max(0, r - rimInset - dotRadius);

  // Fallback dot color if none provided
  const effectiveDotColor = dotColor ?? ParseUtils.getOffColor(color);

  return (
    <svg
      width={size}
      height={size}
      viewBox={`0 0 ${size} ${size}`}
      xmlns="http://www.w3.org/2000/svg"
    >
      <defs>
        <radialGradient id="power-indicator" cx={cx} cy={cy} r={r} gradientUnits="userSpaceOnUse">
          <stop offset="0" stopColor={color} stopOpacity="1" />
          <stop offset={((r - 5) / r).toString()} stopColor={color} stopOpacity="1" />
          <stop offset="1" stopColor={color} stopOpacity="0" />
        </radialGradient>
      </defs>

      <circle cx={cx} cy={cy} r={r} fill="url(#power-indicator)" />

      <g>
        <circle
          cx={cx + orbitR}
          cy={cy}
          r={dotRadius}
          fill={effectiveDotColor}
        />
        <animateTransform
          attributeName="transform"
          attributeType="XML"
          type="rotate"
          from={`0 ${cx} ${cy}`}
          to={`360 ${cx} ${cy}`}
          dur={`${secondsPerRotation}s`}
          repeatCount="indefinite"
        />
      </g>
    </svg>
  );
};

export default PowerIndicator;