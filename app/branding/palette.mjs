// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// The two color palettes of the app, written to src/assets/css/palettes.css
// for Tailwind and Nuxt UI:
//
// - `ink`, the neutral palette (backgrounds, text, borders): from the brand
//   white (50, a warm off-white) to the brand black (900, a black tinted
//   with the green of the logo). Light mode sits on 50, dark mode on 900.
// - `brand`, the primary palette, on the green of the logo (400, the
//   primary of dark mode). Its 500, the primary of light mode, is a darker
//   green that keeps about 4:1 against the background and under white text,
//   the level of Nuxt UI's own palettes.
//
// The other shades follow the lightness curves of the Tailwind palettes, in
// the OKLCH space, at the hue of the logo. The script checks the contrast of
// the pairs Nuxt UI draws and fails below the thresholds.
//
// node branding/palette.mjs           writes the palettes
// node branding/palette.mjs --check   fails if the file is out of date

import { readFileSync, writeFileSync } from 'node:fs';
import process from 'node:process';
import { fileURLToPath } from 'node:url';

const OUTPUT = fileURLToPath(new URL('../src/assets/css/palettes.css', import.meta.url));

const WHITE = '#fefcf7';
const BLACK = '#020f05';
const LOGO_GREEN = '#2ecc71';

const SHADES = [50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 950];

// --- Color spaces -----------------------------------------------------------

const channels = (hex) => {
  const value = Number.parseInt(hex.slice(1), 16);
  return [(value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff];
};

const toLinear = (c) => (c <= 0.04045 ? c / 12.92 : ((c + 0.055) / 1.055) ** 2.4);
const toGamma = (c) => (c <= 0.0031308 ? 12.92 * c : 1.055 * c ** (1 / 2.4) - 0.055);

/** sRGB hex to OKLCH: [lightness 0-1, chroma, hue in degrees]. */
function toOklch(hex) {
  const [r, g, b] = channels(hex).map((c) => toLinear(c / 255));
  const l = Math.cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
  const m = Math.cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
  const s = Math.cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);
  const lightness = 0.2104542553 * l + 0.793617785 * m - 0.0040720468 * s;
  const a = 1.9779984951 * l - 2.428592205 * m + 0.4505937099 * s;
  const bb = 0.0259040371 * l + 0.7827717662 * m - 0.808675766 * s;
  return [lightness, Math.hypot(a, bb), ((Math.atan2(bb, a) * 180) / Math.PI + 360) % 360];
}

/** OKLCH to linear sRGB, possibly out of gamut. */
function toLinearRgb(lightness, chroma, hue) {
  const h = (hue * Math.PI) / 180;
  const a = chroma * Math.cos(h);
  const b = chroma * Math.sin(h);
  const l = (lightness + 0.3963377774 * a + 0.2158037573 * b) ** 3;
  const m = (lightness - 0.1055613458 * a - 0.0638541728 * b) ** 3;
  const s = (lightness - 0.0894841775 * a - 1.291485548 * b) ** 3;
  return [
    4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
    -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
    -0.0041960863 * l - 0.7034186147 * m + 1.707614701 * s,
  ];
}

const inGamut = (rgb) => rgb.every((c) => c >= -1e-7 && c <= 1 + 1e-7);

/** The largest chroma sRGB can show at this lightness and hue. */
function maxChroma(lightness, hue) {
  let low = 0;
  let high = 0.4;
  for (let i = 0; i < 40; i++) {
    const middle = (low + high) / 2;
    if (inGamut(toLinearRgb(lightness, middle, hue))) {
      low = middle;
    } else {
      high = middle;
    }
  }
  return low;
}

/** OKLCH (lightness in percent) to hex, the chroma reduced into the sRGB gamut. */
function hex(lightness, chroma, hue) {
  const l = lightness / 100;
  const rgb = toLinearRgb(l, Math.min(chroma, maxChroma(l, hue)), hue);
  return `#${rgb
    .map((c) =>
      Math.round(toGamma(Math.min(1, Math.max(0, c))) * 255)
        .toString(16)
        .padStart(2, '0'),
    )
    .join('')}`;
}

// --- Palettes ---------------------------------------------------------------

const [, , GREEN_HUE] = toOklch(LOGO_GREEN);

/** Shades from [lightness %, chroma, hue] triples; `null` keeps a source color. */
function palette(sources, curve) {
  return Object.fromEntries(SHADES.map((shade, i) => [shade, sources[shade] ?? hex(...curve[i])]));
}

// Warm at the white end, green from the middle greys to the black.
const ink = palette({ 50: WHITE, 900: BLACK }, [
  null,
  [96.9, 0.006, 100],
  [92.3, 0.007, 125],
  [87.1, 0.01, 145],
  [71, 0.017, GREEN_HUE],
  [53.6, 0.021, GREEN_HUE],
  [44, 0.02, GREEN_HUE],
  [36.5, 0.019, GREEN_HUE],
  [25.5, 0.018, GREEN_HUE],
  null,
  [8.3, 0.012, GREEN_HUE],
]);

const brand = palette({ 400: LOGO_GREEN }, [
  [98.2, 0.018, GREEN_HUE],
  [95.8, 0.04, GREEN_HUE],
  [91.4, 0.075, GREEN_HUE],
  [84.6, 0.13, GREEN_HUE],
  null,
  [57, 0.17, GREEN_HUE],
  [50.4, 0.135, GREEN_HUE],
  [44, 0.115, GREEN_HUE],
  [37.6, 0.097, GREEN_HUE],
  [31.6, 0.08, GREEN_HUE],
  [21.2, 0.055, GREEN_HUE],
]);

// --- Contrast ---------------------------------------------------------------

function luminance(color) {
  const [r, g, b] = channels(color).map((c) => toLinear(c / 255));
  return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

function contrast(a, b) {
  const [light, dark] = [luminance(a), luminance(b)].sort((x, y) => y - x);
  return (light + 0.05) / (dark + 0.05);
}

/** `color` at `alpha` over `background`, as Tailwind's `bg-primary/10`. */
function over(color, alpha, background) {
  const front = channels(color);
  return `#${channels(background)
    .map((c, i) =>
      Math.round(front[i] * alpha + c * (1 - alpha))
        .toString(16)
        .padStart(2, '0'),
    )
    .join('')}`;
}

const TEXT = 4.5;
const PRIMARY = 3.8;

// The pairs of the Nuxt UI tokens as main.css maps them. A threshold of 0
// only reports the ratio.
const pairs = [
  ['light: text on the background', ink[700], ink[50], TEXT],
  ['light: muted text on the background', ink[500], ink[50], TEXT],
  ['light: muted text on an elevated surface', ink[500], ink[100], TEXT],
  ['light: primary text on the background', brand[500], ink[50], PRIMARY],
  ['light: primary text on an elevated surface', brand[500], ink[100], PRIMARY],
  ['light: text on a primary button', ink[50], brand[500], PRIMARY],
  ['light: primary text on a soft background', brand[500], over(brand[500], 0.1, ink[50]), 0],
  ['dark: text on the background', ink[200], ink[900], TEXT],
  ['dark: muted text on the background', ink[400], ink[900], TEXT],
  ['dark: muted text on an elevated surface', ink[400], ink[800], TEXT],
  ['dark: primary text on the background', brand[400], ink[900], PRIMARY],
  ['dark: primary text on an elevated surface', brand[400], ink[800], PRIMARY],
  ['dark: text on a primary button', ink[900], brand[400], PRIMARY],
  ['dark: primary text on a soft background', brand[400], over(brand[400], 0.1, ink[900]), 0],
];

// --- Output -----------------------------------------------------------------

function declarations(name, colors, notes) {
  return SHADES.map((shade) => {
    const [l, c, h] = toOklch(colors[shade]);
    const note = notes[shade] ? `, ${notes[shade]}` : '';
    const oklch = `oklch(${(l * 100).toFixed(1)}% ${c.toFixed(3)} ${h.toFixed(0)})`;
    return `  --color-${name}-${shade}: ${colors[shade]}; /* ${oklch}${note} */`;
  }).join('\n');
}

const css = `/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 *
 * Generated by branding/palette.mjs: edit the script, not this file.
 */

/* Static: Nuxt UI reads these variables at run time. */
@theme static {
${declarations('ink', ink, { 50: 'brand white', 900: 'brand black' })}

${declarations('brand', brand, { 400: 'logo green', 500: 'light-mode primary' })}
}
`;

let failed = false;
for (const [label, foreground, background, minimum] of pairs) {
  const ratio = contrast(foreground, background);
  const low = ratio < minimum;
  failed ||= low;
  const limit = minimum ? `min ${minimum}` : 'reported';
  console.log(`${low ? 'LOW ' : 'ok  '} ${ratio.toFixed(2)}:1 (${limit})  ${label}`);
}
if (failed) {
  console.error('A contrast is below its threshold: palettes.css was not written.');
  process.exit(1);
}

if (process.argv.includes('--check')) {
  if (readFileSync(OUTPUT, 'utf8') !== css) {
    console.error('palettes.css is out of date: run node branding/palette.mjs');
    process.exit(1);
  }
} else {
  writeFileSync(OUTPUT, css);
  console.log(`Wrote ${OUTPUT}`);
}
