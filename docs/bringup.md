# Board bring-up and tuning campaigns

Run these steps on each new board, in order: each one relies on the ones
before. Write the results in `docs/measurements/` (one file per board,
named after its serial number). The measurements of the tuning campaigns
(V1, V2, V4, named after the design review) set the values marked "tune" in
`firmware/src/app/app_config.c` for phase 2.

Equipment: multimeter (current in series through a USB breakout or an
opened cable, temperature probe if available), a Windows PC and a MacBook,
both with an earthed monitor that has a USB hub, a phone charger, a phone
with a light-meter app, a flashlight.

## 1. Before power

- [ ] Visual check against the JLC assembly preview: dot of U1, "+" of
      LED1, pin 1 of D1, clear face of U3 (light sensor) toward the left
      edge of the board.
- [ ] VBUS to GND resistance on the USB connector: not a short.
- [ ] 3.3 V to GND resistance (test pads): not a short.

## 2. First power and bootloader

- [ ] Plug in through the current meter: the blank chip draws a few mA, no
      heat anywhere.
- [ ] Enter the bootloader ([flashing.md](flashing.md)); the WCH device
      appears. Record its USB ID: `____:____`.
- [ ] Windows only, once: WinUSB driver with Zadig.

## 3. Debug firmware

- [ ] `pio run -e debug -t upload`, then RESET.
- [ ] The lamp fades in green.
- [ ] Device Manager or System Information shows `1209:0002` with a HID
      interface and a serial port.
- [ ] Console `info`: crystal **ok**, a 24-digit UID, reset cause 2 (pin),
      light sensor **present**, 0 touch timeouts.
- [ ] Desktop app (`npm run tauri dev` in `app/`): connected, firmware
      version shown.

## 4. LED

- [ ] `set color 0..3`: green, orange, red and warm white look right; no
      flicker, smooth 300 ms fades.
- [ ] `set level 1`: still visible in a dark room. `set level 100`: bright.
- [ ] `state` shows sensible duties and the estimated current.
- [ ] Plugged into a **phone charger** (no host): it works, and the measured
      total current stays under 100 mA with warm white at 100 % (the LED
      cap is 70 mA without a host). Measured: ____ mA.
- [ ] Plugged into a **computer**: warm white at 100 % stays capped
      (125 mA LED estimate). Measured total: ____ mA.

## 5. Light sensor (V1)

- [ ] `watch 500`: lux values change with the room light.
- [ ] Hand over the sensor window: the lux drops. Flashlight on the window:
      the lux jumps.
- [ ] Flashlight inside the closed enclosure, window covered: the lux must
      **not** move (the LED cannot leak into the sensor).
- [ ] Compare with the phone light meter held next to the window, at three
      light levels. Only the trend matters; the absolute values may differ.

| Situation | Phone (lx) | Lamp (lx) | Automatic level chosen (%) | Looks right? |
|---|---|---|---|---|
| Evening, lamps off | | | | |
| Office lighting | | | | |
| Daylight near a window | | | | |

Tune the brightness curve until the automatic level looks right in each
situation.

## 6. Touch (V4)

The firmware samples the touch at a fixed phase of the LED PWM. Record
with the desktop app (telemetry 50 ms, **Record**, then **Export CSV**) and
note the drop below the baseline in per mille.

- [ ] Idle: baseline stable, noise under a few per mille.
- [ ] Finger on the lid: drop of ____ ‰. Hand 1 cm above: ____ ‰.
      Finger on a side wall near the wire: ____ ‰.
- [ ] Short press cycles the colors, a 2 s press turns the lamp off then on,
      a phone left on the lid fires once and is then ignored.

Matrix, for each setup with the LED at 0, 50 and 100 % (`set level`):

| Setup | LED | Idle noise (‰ peak to peak) | Finger drop (‰) | False triggers? |
|---|---|---|---|---|
| Windows PC, monitor hub (earthed) | 0 / 50 / 100 % | | | |
| MacBook, monitor hub (earthed) | 0 / 50 / 100 % | | | |
| MacBook on battery, charger unplugged, direct (no earth) | 0 / 50 / 100 % | | | |

Experiments on the PWM coupling (review N1), at 100 % LED:
`touch phase <ticks>` (sample at another point of the 7200-tick PWM
period) and `touch dither` (random phase: what unsynchronized sampling
would give). Keep the phase with the lowest noise.

Then set the thresholds: press at about 40 to 50 % of the smallest finger
drop, release at about 60 % of the press threshold, and both well above
the worst idle noise.

## 7. USB power states

- [ ] Put the computer to sleep: the lamp turns off within a second and
      touches do nothing.
- [ ] Wake it up: the previous color and brightness come back; the app
      reconnects.

## 8. Thermal (V2)

Closed enclosure on the monitor, at room temperature.

- [ ] `cap 300` (debug only, lifts the current cap), `set color 3`,
      `set level 100`: warm white at full power, about 1.1 W.
- [ ] After one hour, measure: LED body ____ °C, R17/R18 ____ °C, inside
      of the lid ____ °C, room ____ °C.
- [ ] Criteria: LED under 70 °C, PLA under 45 °C. If they hold, the
      configured cap may rise; otherwise lower it.
- [ ] `cap off` at the end.

## 9. Release firmware

- [ ] `pio run -e release -t upload`: the lamp shows up as `1209:0001`,
      HID only, and the app works the same.
