class ColorUtils {
  parseHex(hex: string) {
    const s = hex.toLowerCase();
    if (!/^#([0-9a-f]{3}|[0-9a-f]{6}|[0-9a-f]{8})$/.test(s)) return null;

    let r = 0, g = 0, b = 0, a = 255;
    if (s.length === 4) { r = parseInt(s[1] + s[1], 16); g = parseInt(s[2] + s[2], 16); b = parseInt(s[3] + s[3], 16); }
    else if (s.length === 7) { r = parseInt(s.slice(1, 3), 16); g = parseInt(s.slice(3, 5), 16); b = parseInt(s.slice(5, 7), 16); }
    else if (s.length === 9) { r = parseInt(s.slice(1, 3), 16); g = parseInt(s.slice(3, 5), 16); b = parseInt(s.slice(5, 7), 16); a = parseInt(s.slice(7, 9), 16); }
    return { r, g, b, a };
  }

  parseRgbFunc(rgbStr: string) {
    const m = rgbStr.replace(/\s+/g, "")
      .match(/^rgba?\((\d{1,3}),(\d{1,3}),(\d{1,3})(?:,((?:\d*\.?\d+)))?\)$/i);
    if (!m) return null;
    const r = Math.min(255, parseInt(m[1], 10));
    const g = Math.min(255, parseInt(m[2], 10));
    const b = Math.min(255, parseInt(m[3], 10));
    const a = m[4] == null ? 1 : Math.max(0, Math.min(1, parseFloat(m[4])));
    return { r, g, b, a: Math.round(a * 255) };
  }

  parseCssColor(input: string) {
    const trimmed = input.trim();
    const hexDirect = this.parseHex(trimmed);
    if (hexDirect) return hexDirect;

    const rgbDirect = this.parseRgbFunc(trimmed);
    if (rgbDirect) return rgbDirect;

    if (typeof document === "undefined") throw new Error("parseCssColor requires a browser environment");
    const ctx = document.createElement("canvas").getContext("2d");
    if (!ctx) throw new Error("Canvas not supported");

    ctx.fillStyle = "#000";
    ctx.fillStyle = trimmed;
    const computed = ctx.fillStyle;

    const hex = this.parseHex(computed);
    if (hex) return hex;

    const rgb = this.parseRgbFunc(computed);
    if (rgb) return rgb;

    throw new Error(`Could not parse color: ${input} (normalized: ${computed})`);
  }

  rgbToHsl(r: number, g: number, b: number) {
    r /= 255; g /= 255; b /= 255;
    const max = Math.max(r, g, b), min = Math.min(r, g, b);
    let h = 0, s = 0;
    const l = (max + min) / 2;

    if (max !== min) {
      const d = max - min;
      s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
      switch (max) {
        case r: h = (g - b) / d + (g < b ? 6 : 0); break;
        case g: h = (b - r) / d + 2; break;
        case b: h = (r - g) / d + 4; break;
      }
      h *= 60;
    }
    return { h, s, l };
  }

  hslToRgb(h: number, s: number, l: number) {
    const c = (1 - Math.abs(2 * l - 1)) * s;
    const hp = (h % 360 + 360) % 360 / 60;
    const x = c * (1 - Math.abs((hp % 2) - 1));
    let r1 = 0, g1 = 0, b1 = 0;
    if (0 <= hp && hp < 1) { r1 = c; g1 = x; }
    else if (1 <= hp && hp < 2) { r1 = x; g1 = c; }
    else if (2 <= hp && hp < 3) { g1 = c; b1 = x; }
    else if (3 <= hp && hp < 4) { g1 = x; b1 = c; }
    else if (4 <= hp && hp < 5) { r1 = x; b1 = c; }
    else { r1 = c; b1 = x; }
    const m = l - c / 2;
    return {
      r: Math.round((r1 + m) * 255),
      g: Math.round((g1 + m) * 255),
      b: Math.round((b1 + m) * 255),
    };
  }

  toHex2(n: number) { return n.toString(16).padStart(2, "0"); }
  toHex({ r, g, b }: { r: number; g: number; b: number }) {
    return `#${this.toHex2(r)}${this.toHex2(g)}${this.toHex2(b)}`;
  }

  // --- new helpers for perceived brightness / blending ---

  // sRGB -> linear
  srgbToLinear(c255: number) {
    const c = c255 / 255;
    return c <= 0.04045 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4);
  }

  // WCAG relative luminance (0..1)
  relLuminanceRGB(r: number, g: number, b: number) {
    const R = this.srgbToLinear(r);
    const G = this.srgbToLinear(g);
    const B = this.srgbToLinear(b);
    return 0.2126 * R + 0.7152 * G + 0.0722 * B;
  }

  // Blend original towards white by factor t in [0..1]
  blendTowardWhite(rgb: { r: number; g: number; b: number }, t: number) {
    const r = Math.round(rgb.r + t * (255 - rgb.r));
    const g = Math.round(rgb.g + t * (255 - rgb.g));
    const b = Math.round(rgb.b + t * (255 - rgb.b));
    return { r, g, b };
  }

  /**
   * Always return a color that is *perceptibly brighter* than input.
   * Uses WCAG relative luminance; blends towards white until luminance
   * increases by at least +0.08 absolute OR +25% relative (whichever larger).
   * Falls back to white with a tiny hue tweak if the input is already near-white.
   */
  getOffColor(input: string): string {
    const { r, g, b } = this.parseCssColor(input);

    const baseLum = this.relLuminanceRGB(r, g, b);
    const targetLum = Math.min(1, Math.max(baseLum + 0.08, baseLum * 1.25)); // stricter of +0.08 or +25%

    // If already extremely bright, return white; also shift hue a touch so it's not identical to pure white in some UIs
    if (baseLum > 0.97) {
      const { h, s, l } = this.rgbToHsl(r, g, b);
      const bumped = this.hslToRgb(h + 8, s * 0.9, Math.min(1, l + 0.01));
      return this.toHex({ r: Math.min(255, bumped.r), g: Math.min(255, bumped.g), b: Math.min(255, bumped.b) });
    }

    // Binary search t in [0..1] to reach target luminance by blending towards white
    let lo = 0, hi = 1, best = { r, g, b }, iterations = 12;
    while (iterations--) {
      const mid = (lo + hi) / 2;
      const cand = this.blendTowardWhite({ r, g, b }, mid);
      const lum = this.relLuminanceRGB(cand.r, cand.g, cand.b);
      if (lum >= targetLum) {
        best = cand;
        hi = mid;
      } else {
        lo = mid;
      }
    }

    // If the increase is still tiny (extreme edge cases), nudge hue/lightness a bit too
    const finalLum = this.relLuminanceRGB(best.r, best.g, best.b);
    if (finalLum - baseLum < 0.02) {
      const { h, s, l } = this.rgbToHsl(best.r, best.g, best.b);
      const tweaked = this.hslToRgb(h + 6, s * 0.95, Math.min(1, l + 0.03));
      return this.toHex(tweaked);
    }

    return this.toHex(best);
  }

}

export default new ColorUtils() as ColorUtils;