# Architecture

DISLED has three parts that share one protocol:

```
 ┌────────────── lamp (CH32V203) ──────────────┐        ┌──────── desktop app (Tauri) ────────┐
 │ touch ─┐                                    │  USB   │ Rust: USB worker thread, codec      │
 │ light ─┼─> lamp state ─> LED output (PWM)   │<──────>│ Vue: controls, telemetry, CSV       │
 │ host ──┘        └─> notifications           │  HID   │                                     │
 └─────────────────────────────────────────────┘        └─────────────────────────────────────┘
                      protocol/README.md + protocol/vectors/v1.json
```

The lamp owns its state ([ADR 0007](decisions/0007-device-behavior.md)):
the touch changes it locally, the host reads and changes it, and every
change is notified. The behavior itself is specified in
[behavior.md](behavior.md).

## Firmware

### Layers

| Layer | Path | Depends on | Role |
|---|---|---|---|
| Application | `src/app/` | everything below | wiring, scheduling, host protocol, debug commands, tunables |
| Diagnostics | `src/diag/` | usb, core | logs and line console over CDC (debug builds) |
| USB | `src/usb/` | bsp, TinyUSB, core | descriptors, link state, HID queues, CDC stream |
| Drivers | `src/drivers/` | bsp, SDK | LED PWM, TouchKey, I2C, VEML7700 |
| Board support | `src/bsp/` | SDK | pin map, clocks, time base, watchdog, unique ID, faults |
| Core | `lib/disled_core/` | C library only | all decisions: state machine, touch detection, brightness, color, power cap, PWM dithering, self-test, protocol codec, scheduler, formatter, assertions |

Only `src/bsp/ch32.h` includes the vendor device header. Includes are
relative to `src/` (`#include "drivers/led_pwm.h"`), so every include shows
the layer it reaches into. The core never includes an SDK header: it
builds natively and runs under Unity in CI.

### Data flow

```
TIM3 CC4 interrupt (1 kHz, fixed PWM phase) ─> TouchKey sample ─> queue
                                                                    │
main loop ── touch task ─> touch_detector ─> short/long press ─> lamp_fsm
          ── ALS task (100 ms) ─> VEML7700 ─> lux ─> brightness
          ── output task (10 ms) ─> led_output: palette, level, fade,
                                     power cap, pulse loss ─> duties in
                                     1/16 tick ─> led_pwm
          ── usb task ─> TinyUSB, link state (unconfigured/configured/suspended)
          ── host task ─> commands, STATE notifications, telemetry
          ── console task (debug) ─> text commands, watch
          └─ watchdog feed, then the next round at once (no WFI, ADR 0011)

TIM3 update interrupt (20 kHz, every PWM period) ─> dithers the duties
                                     into whole ticks for the next period
```

### Concurrency

Bare metal, one cooperative loop driven by a table-based scheduler
(`lib/disled_core/scheduler`). Interrupt handlers only capture data:

| Interrupt | Work |
|---|---|
| SysTick, 1 kHz | millisecond counter, arms the next TouchKey sample |
| TIM3 update, 20 kHz | dithers the three LED duties into whole ticks for the next period |
| TIM3 channel 4 | one TouchKey measurement (about 5 µs), pushed to a lock-free queue |
| USB (three vectors, one handler) | TinyUSB event capture; during suspend, first restores the full-speed clock (EXTI line 18, USB wake-up) |
| HardFault, NMI | saves `mcause`, `mepc`, `mtval` in backup registers, resets |

All interrupts share one priority, so the QingKe hardware stack never
nests ([ADR 0008](decisions/0008-toolchain-baseline.md)).

### Safety rules

- **One path to the LED.** Only `led_output` computes duties, and it
  always applies the current cap of the USB state: 70 mA without a host,
  125 mA with one, 0 while the host sleeps. The cap uses worst-case
  currents and scales the three channels together, which keeps the hue.
- **Shared pins.** PA10 (bonded to USB D−) and PB8 (bonded to BOOT0) are
  never configured; a static assertion guards the unused-pin masks.
- **No dynamic allocation, no floating point, no 64-bit division.** Fixed
  point everywhere, with static assertions on the intermediate ranges.
- **Every wait is bounded.** I2C waits time out after 3 ms; a missing
  light sensor falls back to a fixed brightness and is probed again every
  5 s; a TouchKey conversion gives up after about 100 µs.
- **Watchdog.** The loop feeds the independent watchdog (1 s) only after
  every task ran. The scheduler records the longest run of each task
  (console `tasks`): 0.5 ms at most, 1.7 ms on the suspend clock.
- **Faults are reported.** A crash or a failed `DISLED_ASSERT` leaves its
  cause in the backup registers (an assertion: `mcause` 0x0A55 followed by
  the line), then the chip resets; the next boot reports it in `INFO`, on
  the console and in `DIAGNOSTICS`, which also counts boots, watchdog
  resets and faults since power-up, the smallest stack margin (the stack is
  filled with a pattern at boot) and error counters.
- **Crystal failure.** The firmware detects it, keeps USB off (the clock
  would be out of tolerance) and still runs the lamp locally.

### Power

The core never sleeps: WFI in the main loop hangs on this chip
([ADR 0011](decisions/0011-low-power.md)). While the host sleeps (USB
suspend), the firmware stops the LED timer, the touch and the light
sensor, and runs the core at 1 MHz from the internal oscillator, crystal
and PLL off. The USB wake-up interrupt restores the full clock before
TinyUSB handles the resume.

### Memory

32 KB flash, 10 KB RAM. `scripts/size_budget.py` prints usage after each
build, warns above 90 % and fails above 100 %. Link-time optimization is on.

| Build | Flash | RAM (2 KB stack included) |
|---|---|---|
| debug (HID + CDC console) | 30.3 KB | 5.1 KB |
| release (HID) | 20.8 KB | 4.3 KB |

The debug build is above the 90 % warning: new debug-only features
should replace bring-up console commands rather than add to them.

### Configuration

Every tunable lives in `src/app/app_config.c`, with the reason for its
value. Values marked "tune" are first guesses that the bring-up campaigns
replace ([bringup.md](bringup.md)); the others were measured or tuned on
the first board ([ADR 0009](decisions/0009-bring-up-corrections.md)).

### LED output

The PWM runs at 20 kHz with 3600 ticks of 14 ns. Duties are computed in
sixteenths of a tick: at the bottom of the range a channel lasts only a
few ticks, so whole ticks would make dim fades step and the color drift.
The TIM3 update interrupt spreads the fraction over 16 periods
(`pwm_dither`). Each lit channel also gets a fixed pulse loss added, the
part of every pulse that the driver edges turn into no light; without it,
the 1-tick blue of warm white at 10 % vanished and the white turned yellow.

### Touch detection

`touch_detector` averages the TouchKey samples over 20 ms (one mains
period) and measures the drop below a slow baseline, in per mille. A press
must reach an absolute threshold **and** rise above the floor, the lowest
value of the last 320 ms; it ends halfway back down, or under the absolute
release threshold. A hand resting near the lid lifts the signal to the
level of a light touch, and only this rise separates the taps on top of
it. The baseline freezes while a hand is near (until it stays for the
object time), so it cannot drift under the hand and blunt the next touch.

### Testing

- `pio test -e native` runs the core on the build machine (Unity): lamp
  rules, touch detection on synthetic signals (mains hum, drift, objects,
  finger at power-up, hand resting near the lid), brightness, color, power
  cap, output fades and pulse loss, PWM dithering, self-test, scheduler
  and its task timing, formatter and the protocol codec.
- The protocol codec is checked byte for byte against
  `protocol/vectors/v1.json`, the same file the app tests use.
- `pio check` runs cppcheck on `src/` and the core at every severity.
- Hardware behavior is verified on the board with [bringup.md](bringup.md).

## Desktop app

- **Rust backend** (`app/src-tauri`). A worker thread owns the USB
  connection through `hidapi`: it finds the lamp, reconnects after an
  unplug, writes commands and decodes messages. Results reach the web view
  as Tauri events (`device-connection`, `device-message`). The codec
  lives in its own crate, `crates/disled-protocol`, shared with the
  production tool and tested against the shared vectors. The Rust code is
  one Cargo workspace at the repository root.
- **Tray and windows** ([ADR 0013](decisions/0013-tray-app.md)). The app
  lives in the menu bar or notification area; its icon is drawn at run
  time from the lamp state. Three windows share one page, chosen by the
  window label: the popup (daily use), the Diagnostics window (tuning)
  and the lamp update window.
- **Settings** (brightness mode, fixed level, correction) are saved with
  the store plugin; the USB worker applies them as soon as the lamp
  answers `INFO`, so they hold across lamp restarts. Changes are broadcast
  to every window (`settings-changed`).
- **Updates** ([ADR 0014](decisions/0014-app-updates.md)). A thread checks
  the `latest.json` of the latest GitHub release at start-up and once a
  day (`updates.rs`, updater plugin); the popup shows the result
  (`update-status`) and asks the backend to install it.
- **Lamp updates** ([ADR 0016](decisions/0016-lamp-updates.md)). The same
  thread looks for the newest signed `fw-v*` release (`firmware.rs`). An
  update runs on its own thread: download, signature check against the
  update key, then `wchisp probe` every second until the ROM bootloader
  shows up, `wchisp flash`, and the `INFO` of the restarted lamp. wchisp
  is a separate program next to the app, driven through its log lines
  (`crates/disled-flash`, shared with the production tool); the lamp
  update window follows `firmware-status`.
- **Vue front end** (`app/src`). One reactive store per window
  (`lib/device.ts`) holds the connection, the lamp state, the settings and
  the telemetry history; the components only render it and send commands.
- USB access stays in Rust: the macOS web view has no WebHID, and a
  backend-owned connection keeps working while every window is hidden.

## Production tool

`tools/disled-factory` is a command-line program for batches of lamps
([production.md](production.md)). Board after board, it waits for the ROM
bootloader, flashes the signed release build with wchisp, checks the
`INFO` of the restarted lamp, runs `SELF_TEST` while the operator watches
the colors and touches the lid, and appends a row to a CSV log. It shares
the protocol codec and the USB IDs (`crates/disled-protocol`) and the
flashing code, wchisp and the firmware signatures (`crates/disled-flash`),
with the app.
