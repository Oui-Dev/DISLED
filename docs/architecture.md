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
| Core | `lib/disled_core/` | C library only | all decisions: state machine, touch detection, brightness, color, power cap, protocol codec, scheduler, formatter |

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
                                     power cap ─> led_pwm duties
          ── usb task ─> TinyUSB, link state (unconfigured/configured/suspended)
          ── host task ─> commands, STATE notifications, telemetry
          ── console task (debug) ─> text commands, watch
          └─ watchdog feed, then WFI until the next interrupt
```

### Concurrency

Bare metal, one cooperative loop driven by a table-based scheduler
(`lib/disled_core/scheduler`). Interrupt handlers only capture data:

| Interrupt | Work |
|---|---|
| SysTick, 1 kHz | millisecond counter, arms the next TouchKey sample |
| TIM3 channel 4 | one TouchKey measurement (about 5 µs), pushed to a lock-free queue |
| USB (three vectors, one handler) | TinyUSB event capture |
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
  every task ran.
- **Faults are reported.** A crash leaves its cause in the backup
  registers; the next boot reports it in `INFO` and on the console.
- **Crystal failure.** The firmware detects it, keeps USB off (the clock
  would be out of tolerance) and still runs the lamp locally.

### Memory

32 KB flash, 10 KB RAM. `scripts/size_budget.py` prints usage after each
build, warns above 90 % and fails above 100 %. Link-time optimization is on.

| Build | Flash | RAM (2 KB stack included) |
|---|---|---|
| debug (HID + CDC console) | 24.3 KB | 4.9 KB |
| release (HID) | 17.8 KB | 4.1 KB |

### Configuration

Every tunable lives in `src/app/app_config.c`, with the reason for its
value. Values marked "tune" are first guesses that the bring-up campaigns
replace ([bringup.md](bringup.md)).

### Testing

- `pio test -e native` runs the core on the build machine (Unity): lamp
  rules, touch detection on synthetic signals (mains hum, drift, objects,
  finger at power-up), brightness, color, power cap, output fades,
  scheduler, formatter and the protocol codec.
- The protocol codec is checked byte for byte against
  `protocol/vectors/v1.json`, the same file the app tests use.
- `pio check` runs cppcheck on `src/` and the core at every severity.
- Hardware behavior is verified on the board with [bringup.md](bringup.md).

## Desktop app

- **Rust backend** (`app/src-tauri`). A worker thread owns the USB
  connection through `hidapi`: it finds the lamp, reconnects after an
  unplug, writes commands and decodes messages. Results reach the web view
  as Tauri events (`device-connection`, `device-message`). The codec is
  tested against the shared vectors.
- **Vue front end** (`app/src`). One reactive store (`lib/device.ts`)
  holds the connection, the lamp state and the telemetry history; the
  components only render it and send commands.
- USB access stays in Rust: the macOS web view has no WebHID, and a
  backend-owned connection keeps working once the app runs in the
  background (phase 2).
