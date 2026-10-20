# 0011 — Low power without WFI

- Status: Accepted
- Date: 2026-10-21

## Context

USB allows a suspended device about 2.5 mA. The CH32V203 at 144 MHz draws
far more, so the firmware must save power while the host sleeps. The usual
tool is WFI (sleep until the next interrupt), in the main loop and during
suspend. In phase 1, WFI in the main loop made the board reset every 2 s
and was removed ([ADR 0009](0009-bring-up-corrections.md)); phase 2 went
back to it on the first board (2026-10-21).

What was measured:

- **WFI works in a tight loop.** Run from a console command, 2000 WFIs
  woke on the 20 kHz PWM interrupt in 91 ms, and 200 WFIs woke on the 1 ms
  tick alone in 200 ms, in user mode (the mode `main()` runs in, see
  [ADR 0010](0010-firmware-updates-from-the-app.md)).
- **WFI in the main loop never returns.** With one WFI per pass of the
  loop, the board stopped within milliseconds, the watchdog reset it, and
  USB never enumerated (the new diagnostics counted 89 resets in a row).
  A marker written to a backup register before and after the WFI showed
  the loop stopped inside it. Four variants behaved the same: the SDK
  `__WFI()`, interrupts masked around `wfi` (the RISC-V pattern),
  `__WFE()`, and `wfi` in machine mode through an `ecall`.
- No fault was recorded: the core just never came back to the loop. The
  root cause is not known; the difference with the tight loop is that USB,
  touch and LED interrupts are all active.

## Decision

- **The core never sleeps.** No WFI, neither in the main loop nor during
  suspend.
- **USB suspend slows the clock instead.** The firmware stops the LED
  timer (outputs forced low), the touch sampling and the light sensor
  (VEML7700 shutdown), switches to the internal 8 MHz oscillator divided by
  8 (1 MHz), and stops the crystal and the PLL. The main loop keeps running
  slowly and keeps feeding the watchdog.
- **USB wake-up restores the clock first.** During suspend only, EXTI line
  18 (USB wake-up) is enabled; the USB interrupt handler brings back the
  internal oscillator at full speed, the crystal and the PLL before
  TinyUSB sees the resume.
- When awake and powered by USB, the board does not try to save power:
  the LED takes up to 125 mA, and the microcontroller a few more.

## Consequences

- Suspend and resume work on Windows (PC sleep, then wake): the LED turns
  off at once, the lamp comes back in its previous state, and nothing
  resets. The tasks then run 144 times slower; the longest took 1.7 ms,
  far within the 1 s watchdog.
- The suspend current is not measured yet (phase 3, with a multimeter). If
  it exceeds 2.5 mA, the next steps are the deeper clock dividers and a
  new look at the WFI hang, with a debug probe if one is available.
