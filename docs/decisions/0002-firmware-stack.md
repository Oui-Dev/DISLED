# 0002 — PlatformIO, WCH NoneOS SDK, TinyUSB, C11, bare metal

- Status: Accepted
- Date: 2026-10-12

## Context

The CH32V203G6U6 has 32 KB of flash and 10 KB of RAM. The firmware needs
USB device support, a PWM LED driver, an I²C light sensor and the TouchKey
peripheral. The developer wants PlatformIO and code that a senior engineer
would consider clean and maintainable.

Options considered: WCH NoneOS SDK with TinyUSB, FreeRTOS, the openwch
Arduino core, ch32fun, and Rust (ch32-hal with Embassy).

## Decision

- **Build system:** PlatformIO with the community `platform-ch32v`, pinned
  to a commit, board `genericCH32V203G6U6`, framework `noneos-sdk`.
- **USB stack:** TinyUSB 0.21.0 as a git submodule, using its FSDEV (USBD)
  port for the CH32V20x.
- **Language:** C11. The WCH SDK and TinyUSB are written in C, every vendor
  example is in C, and C gives a predictable binary size.
- **Concurrency:** bare metal with a table-driven cooperative scheduler.
  Interrupt handlers only capture data; the main loop does the work.
- **Testability:** all decision logic lives in `firmware/lib/disled_core`,
  which includes no SDK header and runs natively under Unity in CI.

Rust was rejected for now: it does not fit PlatformIO, and the CH32 Rust HAL
is still experimental. Arduino and FreeRTOS cost too much of the 32 KB / 10 KB
budget for no real gain.

## Consequences

- TinyUSB has no ready-made PlatformIO integration for this chip. A
  toolchain spike settles the interrupt convention, the clock tree and the
  linker setup before any feature work.
- No dynamic allocation and no floating point in the firmware; values use
  fixed point.
- Flash and RAM usage are checked in CI against a budget.
