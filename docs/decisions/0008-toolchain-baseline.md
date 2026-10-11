# 0008 — Toolchain baseline from the spike

- Status: Accepted
- Date: 2026-10-12

## Context

TinyUSB has no ready-made PlatformIO integration for the CH32V203, so a
spike built a minimal vendor HID device before any feature work. It checked
compilation, linking, interrupt code generation and memory use. Behavior on
real hardware is checked during bring-up.

## Decision

- **Pinned versions:** `platform-ch32v` at commit `afe523b`, framework
  `framework-wch-noneos-sdk` at commit `4ba4a26` (SDK 2.30), TinyUSB tag
  `0.21.0`. The platform picks the compiler per host OS (WCH build of
  GCC 12.2.0, `riscv-wch-elf-gcc`, branch `gcc12`); that choice cannot be
  pinned in `platformio.ini`, so CI prints the compiler version.
- **Interrupts:** the startup code enables the QingKe hardware
  prologue/epilogue and nesting (`INTSYSCR = 0x3`). Handlers use the
  `WCH-Interrupt-fast` attribute (`BSP_ISR`), which emits no software
  prologue. All interrupts keep the same priority, so they never nest.
  Vector entries that share a handler are aliases, because the compiler
  folds identical interrupt functions into calls to each other.
- **Clocks:** `board_build.clock_source = hse+pll` gives 144 MHz from the
  8 MHz crystal. The SDK sets HCLK = PCLK2 = 144 MHz and PCLK1 = 72 MHz.
  The BSP then lowers PCLK2 to 72 MHz so that the ADC clock (PCLK2 / 6 =
  12 MHz) stays within its 14 MHz limit. TIM3 runs at 144 MHz (APB1 × 2),
  and USB at 48 MHz (PLL / 3). TinyUSB drives the internal D+ pull-up
  (`EXTEN_USBD_PU_EN`).
- **USB IDs:** release builds use `1209:0001` and debug builds `1209:0002`
  (both pid.codes test PIDs), because their interface sets differ and hosts
  cache descriptors per VID:PID.
- **Includes:** paths are relative to `src/` (`#include "bsp/ch32.h"`), so
  every include shows which layer it depends on. Only `bsp/ch32.h` includes
  the vendor device header, with its warnings silenced.

## Consequences

Baseline with the USB stack only:

| Build | Flash | RAM (stack included) |
|---|---|---|
| debug (HID + CDC) | 12.1 KB / 32 KB | 3.6 KB / 10 KB |
| release (HID) | 10.1 KB / 32 KB | 3.0 KB / 10 KB |

About 20 KB of flash remain for the application. `scripts/size_budget.py`
reports usage after every build, warns above 90 % and fails above 100 %.

Update after the first complete MVP build (same day):

| Build | Flash | RAM (stack included) |
|---|---|---|
| debug, no LTO | 28.3 KB | 4.9 KB |
| debug, LTO | 24.2 KB | 4.9 KB |
| release, LTO | 17.7 KB | 4.1 KB |

- **Link-time optimization is on** (`board_build.use_lto = yes`). The
  disassembly checks that interrupt handlers keep no software prologue for
  caller-saved registers, restore what they use and end with `mret`, and
  that vector aliases survive.
- **No 64-bit division:** libgcc's `__divdi3` and `__udivdi3` cost 2 KB.
  The fixed-point code keeps every intermediate within 32 bits, with
  static assertions on the bounds.
