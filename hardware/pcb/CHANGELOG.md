# Changelog

The versions of the DISLED board, written by hand
([docs/releasing.md](../../docs/releasing.md#hardware-releases)). The
dates are those of the designs.

## 1.0.0 (2026-10-10)

PCB v4, silkscreen "DISLED – Rev 1.0": the board in production. A 35 mm
square.

### 🚀 Enhancements

* The light sensor moves to the left edge and looks left, away from the
  cable and the monitor, through a window of its own, isolated from the
  LED.
* LED currents balanced for luminance: red 75, green 52 and blue 98 mA
  typical (series resistance 40, 40 and 20 Ω).
* A locking JST-XH connector for the touch electrode at the back edge,
  instead of the 2.54 mm header, with no copper pour along the touch
  trace.
* Teardrops are back, the tracks on the bottom side are 19 % shorter, and
  the buttons and the LDO moved.

### ⚠ Trade-offs

* The touch path is almost twice as long (11.9 to 20.9 mm), with a pass
  on the bottom side next to the LED PWM (constraint N1 in the hardware
  README): the cost of the connector at the back edge.
* U1 and C4 sit at 45°: three parts off the right angle, up from one.
* The 3.3 V tracks are longer (50 to 86 mm), with no electrical
  consequence.

## 0.3.0 (2026-10-07)

> **Never manufactured**: a design step on the way to 1.0.0.

PCB v3, a 34 mm square.

### 🚀 Enhancements

* Every part on the top side, so no SMD soldering by hand; the touch
  header stays out of the assembly order.
* An SMD RGB 5050 power LED (1.5 W) instead of the 5 mm through-hole one.
* A VEML7700 light sensor instead of the BH1730FVC.
* A horizontal USB-C receptacle (TYPE-C-31-M-12) at the back edge.
* Ground planes on both sides, with 5 V islands.
* One part off the right angle, down from 27; the five SWD pads in a line
  at a 2.54 mm pitch; two diagonal mounting holes.

### 🩹 Fixes

* Back to an 8 MHz crystal (12 pF load) with 18 pF capacitors: the
  12 MHz one with 33 pF was overloaded.

### ⚠ Known issues

* Unbalanced LED currents: red about 94, green 51 and blue 68 mA.
* The light sensor looks backward, at the cable and the monitor.

## 0.2.0 (2026-10-06)

> **Never manufactured**: a design step on the way to 1.0.0.

PCB v2, round, Ø35 mm.

### 🚀 Enhancements

* Touch: a 2.54 mm header for the electrode, through a 2.2 kΩ resistor.
* Five SWD test pads (3.3 V, GND, RST, SWCLK, SWDIO) instead of the 5-pin
  header.
* Pull-down resistors on the MOSFET gates.

### 🩹 Fixes

* The LED gets its current, about 20 mA per color (100, 100 and 150 Ω):
  the 10 kΩ resistors in the MOSFET sources are gone.

### ⚠ Known issues

* The 12 MHz crystal with 33 pF capacitors is overloaded.
* The LED and the touch header are soldered by hand on the bottom side.

## 0.1.0 (2026-10-03)

> **Never manufactured**, and the LED barely lights: the 10 kΩ resistors
> in the MOSFET sources leave it about 0.3 mA.

PCB v1, round, Ø40 mm: a CH32V203G6U6 microcontroller with an 8 MHz
crystal, an ME6231A33 LDO, USBLC6-2 ESD protection, a vertical USB-C
receptacle, a 5 mm through-hole RGB LED (common anode) soldered by hand on
the bottom side and driven by three AO3400A MOSFETs, a BH1730FVC light
sensor, BOOT and RESET buttons, a 5-pin SWD header and four mounting
holes.
