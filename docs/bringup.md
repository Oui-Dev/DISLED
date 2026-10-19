# Board bring-up and tuning campaigns

Run these steps on each new board, in order: each one relies on the ones
before. The first board's results and the lessons drawn from them are in
[measurements](measurements/F1B9ABCD5A9BBD15E339E339.md) and
[ADR 0009](decisions/0009-bring-up-corrections.md). Write the results in `docs/measurements/` (one file per board,
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
      appears as `4348:55E0` (first board).
- [ ] Windows only, once: WinUSB driver with Zadig.

## 3. Debug firmware

- [ ] `pio run -e debug -t upload`, then RESET.
- [ ] The lamp fades in green.
- [ ] Device Manager or System Information shows `1209:0002` with a HID
      interface and a serial port.
- [ ] Console `info`: crystal **ok**, a 24-digit UID, reset cause 2 (pin)
      after RESET or 3 (software) right after a flash, light sensor
      **present**, 0 touch timeouts.
- [ ] Desktop app (`npm run tauri dev` in `app/`): connected, firmware
      version shown.

## 4. LED

- [ ] `set color 0..3`: green, orange, red and warm white look right; no
      flicker, smooth 300 ms fades. Judge the colors through the closed
      enclosure: the PLA shifts them. `palette <i> <r> <g> <b>` tunes a
      color live (RAM only); copy the result into `app_config.c`.
- [ ] Warm white at 3, 10, 30 and 100 % (`set level`): the same hue at
      every level. If the low levels drift (yellow: blue missing; pink:
      green missing), tune the pulse loss live with
      `pulse <r> <g> <b>` (sixteenths of a tick), at 3 % first.
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
with the desktop app (telemetry 50 ms, **Record**, then **Export CSV**) or
with `watch 20` on the console (every 20 ms window), and note the drop
below the baseline in per mille.

Test with the enclosure **closed** and one hand only: holding the board,
the enclosure or the USB plug with the other hand ties the body to the
board ground and doubles the drop (first board: 45 → 90 per mille), which
hides a weak signal.

- [ ] Idle: baseline stable, noise under a few per mille.
- [ ] Finger on the lid, on the electrode ring: ____ ‰; in its center:
      ____ ‰. Hand 1 cm above: ____ ‰. Hand resting near the lid between
      taps: ____ ‰. Finger on a side wall near the wire: ____ ‰.
- [ ] Short press cycles the colors, a 2 s press turns the lamp off then on,
      a phone left on the lid fires once and is then ignored.
- [ ] Twenty light, quick taps with the hand resting near the lid: one
      color step per tap, no skipped color, no long press.

Matrix, for each setup with the LED at 0, 50 and 100 % (`set level`):

| Setup | LED | Idle noise (‰ peak to peak) | Finger drop (‰) | False triggers? |
|---|---|---|---|---|
| Windows PC, monitor hub (earthed) | 0 / 50 / 100 % | | | |
| MacBook, monitor hub (earthed) | 0 / 50 / 100 % | | | |
| MacBook on battery, charger unplugged, direct (no earth) | 0 / 50 / 100 % | | | |

Experiments on the PWM coupling (review N1), at 100 % LED:
`touch phase <ticks>` (sample at another point of the 3600-tick PWM
period) and `touch dither` (random phase: what unsynchronized sampling
would give). Keep the phase with the lowest noise.

Then check the thresholds in `app_config.c` against the matrix: the press
threshold about halfway between the hand 1 cm above and the weakest
finger drop, the rise above the floor below the weakest tap on top of a
resting hand, and the release threshold above the idle noise. Recorded
taps can be replayed through `touch_detector` on the build machine before
flashing (see the first board's measurements).

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
