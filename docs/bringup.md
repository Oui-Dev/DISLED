# Board bring-up

Run these steps in order on each new board, since each one relies on the
ones before. The settings they check live in
`firmware/src/app/app_config.c`. After changing one, flash the debug
firmware again.

Equipment:

- a multimeter, with a temperature probe if available. It reads the LED
  current across the series resistors (step 4), with no cable to open;
- optional, for the current of the whole board (steps 2, 4 and 7),
  something that puts the multimeter in series on VBUS: a USB-A to USB-C
  cable with only its red wire, VBUS, cut, or a USB-C passthrough board
  whose VBUS runs through a jumper. A USB tester can do the same if it is
  accurate to about 0.1 mA at low current: many are about 1 mA off, too
  much for the 2.5 mA of USB suspend;
- a Windows PC and a MacBook, both with an earthed monitor that has a USB
  hub;
- a phone charger, a phone with a light-meter app, and a flashlight.

## 1. Before power

- [ ] Visual check against the JLC assembly preview: dot of U1, "+" of
      LED1, pin 1 of D1, clear face of U3 (light sensor) toward the left
      edge of the board.
- [ ] The VBUS to GND resistance on the USB connector shows no short.
- [ ] The 3.3 V to GND resistance on the test pads shows no short.

## 2. First power and bootloader

- [ ] Plug in, through the passthrough board and the multimeter if you
      have them. The blank chip draws a few mA, and nothing heats.
- [ ] Enter the bootloader ([flashing.md](flashing.md#enter-the-bootloader)).
      The WCH device appears as `4348:55E0` or `1A86:55E0`.
- [ ] On Windows, once per computer, install the WinUSB driver with Zadig
      ([flashing.md](flashing.md#first-time-on-windows-install-the-winusb-driver)).

## 3. Debug firmware

- [ ] `pio run -e debug -t upload`, then RESET.
- [ ] The lamp fades in green.
- [ ] Device Manager or System Information shows `1209:0002` with a HID
      interface and a serial port.
- [ ] The desktop app (`npm run tauri dev` in `app/`) connects and shows the
      firmware version.
- [ ] Its Diagnostics window shows a 24-digit serial (the chip UID) and the
      reset cause "reset pin" after RESET, or "software" right after a
      flash. Under Health: crystal **Running**, light sensor **Present**,
      0 touch samples lost.

## 4. LED

- [ ] `set color 0` to `3` show green, orange, red and warm white, without
      flicker and with smooth 300 ms fades. Judge the colors through the
      closed enclosure, because the PLA shifts them. The palette is
      `app_color_config`, and the self-test repeats its warm white in
      `app_self_test_config.white`.
- [ ] Warm white keeps the same hue at 3, 10, 30 and 100 % (`set level`).
      If the low levels drift, yellow when blue is missing or pink when
      green is, raise the pulse loss of the missing channel,
      `app_led_pulse_loss_ns`, starting at 3 %.
- [ ] `set level 1` is still visible in a dark room, and `set level 100` is
      bright.
- [ ] `state` shows sensible duties and the estimated current.
- [ ] Measure the LED current with the lamp open (the LED is bright
      without the cube) and the multimeter on DC volts, across one series
      resistor per channel: R15 for red, R13 for green (20 Ω each) and R17
      for blue (10 Ω). The meter averages the PWM, so the voltage divided
      by the resistance gives the mean current of the channel. With the
      multimeter in series on VBUS instead, the LED current of all the
      channels is the total minus the total with the lamp off.
- [ ] On a **computer**, red then green at 100 % (`set color`, `set
      level`) stay within the worst-case currents that the cap counts,
      `app_power_config.full_scale_ma`: red ____ mA (at most 86), green
      ____ mA (at most 63).
- [ ] Warm white at 100 % stays under the 125 mA cap: red ____ + green
      ____ + blue ____ = ____ mA, against ____ mA estimated by `state`.
- [ ] On a **phone charger**, with no host, the lamp works. Two short
      touches give red, and the flashlight on the sensor window takes it to
      the top of the automatic range: red ____ mA, under the 70 mA cap.
      Through the passthrough board, the whole board draws ____ mA, under
      100 mA.

## 5. Light sensor

- [ ] `watch 500` shows the lux following the room light.
- [ ] A hand over the sensor window lowers the lux, and a flashlight on the
      window raises it.
- [ ] With a flashlight inside the closed enclosure and the window covered,
      the lux must **not** move, which shows that the LED cannot leak into
      the sensor.
- [ ] The lamp's own light comes back to its sensor from the desk and the
      walls. In a dark room, read the lux with the lamp off, then at a few
      levels (`set level`). What the lamp adds must stay well under the lux
      at which the curve chooses that level, or the lamp keeps itself lit
      in the dark.
- [ ] Compare with the phone light meter held next to the window, at three
      light levels. Only the trend matters, since the absolute values may
      differ.

| Situation | Phone (lx) | Lamp (lx) | Automatic level chosen (%) | Looks right? |
|---|---|---|---|---|
| Evening, lamps off | | | | |
| Office lighting | | | | |
| Daylight near a window | | | | |

Tune the brightness curve, `app_brightness_curve`, until the automatic
level looks right in each situation. Its points use the lux the lamp reads
behind its window, not those of the phone. A steeper low end leaves less
room for the lamp's own light, so check it again in the dark after a
change.

## 6. Touch

Record with the desktop app (telemetry at 50 ms, **Record**, then **Export
CSV…**) or with `watch 20` on the console, which prints every 20 ms window.
Note the drop below the baseline, in per mille.

Test with the enclosure **closed** and one hand only. Holding the board,
the enclosure or the USB plug with the other hand ties the body to the
board ground and doubles the drop, which hides a weak signal.

- [ ] At rest, the baseline is stable and the noise stays under a few per
      mille.
- [ ] Drop with a finger on the electrode ring: ____ ‰; in its center:
      ____ ‰. A hand 1 cm above: ____ ‰. A hand resting near the lid
      between taps: ____ ‰. A finger on a side wall near the wire: ____ ‰.
- [ ] A short press cycles the colors, a 2 s press turns the lamp off then
      on, and a phone left on the lid fires once, then the lamp ignores it.
- [ ] Twenty light, quick taps with the hand resting near the lid: one
      color step per tap, no skipped color, no long press.

Matrix, for each setup with the LED at 0, 50 and 100 % (`set level`):

| Setup | LED | Idle noise (‰ peak to peak) | Finger drop (‰) | False triggers? |
|---|---|---|---|---|
| Windows PC, monitor hub (earthed) | 0 / 50 / 100 % | | | |
| MacBook, monitor hub (earthed) | 0 / 50 / 100 % | | | |
| MacBook on battery, charger unplugged, direct (no earth) | 0 / 50 / 100 % | | | |

Then check the thresholds in `app_config.c` against the matrix. The press
threshold sits about halfway between the hand 1 cm above and the weakest
finger drop, the rise above the floor under the weakest tap on top of a
resting hand, and the release threshold above the idle noise.
[touch_replay](../firmware/tools/touch_replay/README.md) replays a
`watch 20` recording through `touch_detector` on the build machine, to try
thresholds before flashing them.

## 7. USB power states

- [ ] Put the computer to sleep: the lamp turns off as soon as the computer
      suspends its USB ports, within a second on Windows and a little later
      on a Mac, and touches do nothing.
- [ ] Optional: asleep, the board draws at most 2.5 mA, the most USB
      allows a suspended device. No host checks it, and it only matters to
      a laptop that powers its ports while asleep. Measure it through the
      passthrough board, with the multimeter on its mA range from the
      start: a change of range opens the circuit and restarts the board.
      Measured: ____ mA. Above that, a deeper clock divider in
      `firmware/src/bsp/bsp_power.c` is the first remedy.
- [ ] Wake it up: the previous color and brightness come back, and the app
      reconnects.

## 8. Heat

Close the enclosure and put the lamp where it gets warmest, on a monitor
for instance, at room temperature.

- [ ] Tape the probe, the thermocouple of the multimeter or any
      thermometer with a wire probe, inside the lid above the LED, and
      bring its wire out of the closed lamp. A second run with the probe on
      the LED body or on R15/R16, the red resistors, the warmest in warm
      white, gives their temperature.
- [ ] `cap 300` (debug builds only) lifts the current cap. Then
      `set color 3` and `set level 100` give warm white at full duty, the
      color that draws the most: about 140 mA for the LED at worst, under
      0.9 W for the whole board.
- [ ] After one hour, measure: inside of the lid ____ °C, LED body ____ °C,
      R15/R16 ____ °C, room ____ °C. Without a thermometer, a rough check:
      the cube is barely warm to the touch, and it has neither warped nor
      loosened on its base.
- [ ] The LED must stay under 70 °C and the PLA under 45 °C. If they hold,
      the configured cap may rise, so that warm white reaches 100 %.
      Otherwise, lower it.
- [ ] Run `cap off` at the end.

## 9. Diagnostics and self-test

- [ ] Diagnostics window, Health: no crashes, a stack margin above 1 KB, no
      I2C errors.
- [ ] Self-test tab, **Run the self-test**: check the four colors by eye and
      touch the lid while it glows blue. Every check passes, and the lamp is
      back in its previous state.

## 10. Release firmware

- [ ] `pio run -e release -t upload`: the lamp shows up as `1209:0001`,
      HID only, and the app works the same.
