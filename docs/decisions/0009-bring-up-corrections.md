# 0009 — Corrections from the first board bring-up

- Status: Accepted
- Date: 2026-10-20
- Amends: [0008](0008-toolchain-baseline.md) (clock tree)

## Context

The first v4 board (serial `F1B9ABCD5A9BBD15E339E339`, results in
[measurements](../measurements/F1B9ABCD5A9BBD15E339E339.md)) showed that
several choices made before any hardware existed did not hold. This record
lists the ones that change the design; the tuned values themselves live in
`firmware/src/app/app_config.c`, each with its reason.

## Decision

- **PCLK1 = HCLK / 4 = 36 MHz.** `I2C_Init` of the SDK writes PCLK1 in MHz
  into the 6-bit `FREQ` field before clamping it to 60, so 72 MHz became 8
  and no byte transfer completed. With PCLK1 at 36 MHz, TIM3 runs at
  72 MHz (APB1 × 2): the 20 kHz LED PWM has **3600 ticks of 14 ns**
  instead of 7200. PCLK2 stays at 72 MHz and the ADC at PCLK2 / 8 = 9 MHz.
- **No WFI in the main loop.** The CPU never woke from it, not even on the
  1 ms tick; the watchdog reset the chip every 2 s. The loop now spins.
  Low-power idle is a phase 2 item and starts from this finding.
- **I2C reads byte by byte**, as in the WCH examples: ACK off and STOP
  queued after the first byte. The STM32 two-byte method (POS, BTF) never
  completed. The bus is released (9 clocks, then a clean STOP) at start-up,
  after any failure and before each new probe of the sensor: a STOP made
  with SDA falling while SCL was high was a START, and lost arbitration.
- **LED duties in 1/16 of a tick.** At low levels a channel lasts only a
  few ticks. The TIM3 update interrupt (every 50 µs) dithers each duty over
  16 periods, and each lit channel gets a fixed pulse loss added (the
  driver edges eat part of every pulse; warm white turned yellow at 10 %).
  Both were tuned by eye on warm white from 3 to 100 %.
- **Touch thresholds relative to the recent floor.** Through the closed
  enclosure a finger lowers the reading by about 50 per mille and a hand
  resting near the lid by 20 to 30. A press must also rise 18 per mille
  above the lowest value of the last 320 ms and ends halfway back; the
  baseline freezes while a hand is near. Fixed thresholds alone merged
  light taps into one press or held a false long press.

## Consequences

- The TIM3 interrupt now fires 20000 times a second (about 1 % of the CPU).
  The touch noise did not change.
- Every duty in `led_output` and `led_pwm` is in sub-ticks; the telemetry
  still reports whole ticks.
- Memory after bring-up: debug 26.7 KB flash, 5.0 KB RAM; release 18.2 KB
  flash, 4.2 KB RAM.
- Still open, waiting for the enclosure V2: touch on the Mac (with and
  without earth), LED current with a multimeter, thermal test V2, and the
  automatic brightness curve.
