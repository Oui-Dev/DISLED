# DISLED hardware

Board `connected_status_light`, silkscreen "DISLED – Rev 1.0" (PCB v4):
35 × 35 mm, 2 layers, assembled by JLCPCB (Economic PCBA, top side only).

| Path | Content |
|---|---|
| `easyeda/` | EasyEDA Pro projects v1 to v4. `v4_…_2026-10-10.epro2` is the board in production. |
| `exports/v4/` | Schematic, PCB and 3D renders of v4. The STEP model is not versioned; export it again from EasyEDA. |
| `review/` | Pre-production design review of v4 (in French) and the brief it answered |

## v4 pin map (CH32V203G6U6, QFN28)

| Signal | MCU pin | Function | Notes |
|---|---|---|---|
| LED_R | PA6 (12) | TIM3_CH1 | Gate of Q2, active high, 10 kΩ pull-down |
| LED_G | PA7 (13) | TIM3_CH2 | Gate of Q3, active high, 10 kΩ pull-down |
| LED_B | PB0 (14) | TIM3_CH3 | Gate of Q1, active high, 10 kΩ pull-down |
| TOUCH | PB1 (15) | ADC_IN9 / TouchKey 9 | CN1.2 through R6 (2.2 kΩ) |
| SCL / SDA | PB6 (27) / PB7 (28) | I2C1 | VEML7700 at 0x10, 2.2 kΩ pull-ups |
| USB D− / D+ | PA11 (19) / PA12 (20) | USBD | Pin 19 is also bonded to PA10 |
| SWDIO / SWCLK | PA13 (21) / PA14 (22) | SDI debug | Back-side test pads |
| OSC_IN / OSC_OUT | PD0 (2) / PD1 (3) | HSE | 8 MHz crystal, 18 pF |
| BOOT0 | PB8 / BOOT0 (1) | Boot select | 4.7 kΩ pull-down, BOOT button to 3.3 V |
| NRST | 4 | Reset | RESET button, 100 nF |
| Unused | PA0–PA5, PA9, PA15, PB3–PB5 | — | Configure as analog inputs |

## Constraints the firmware must respect (from the review)

- **N3, shared pins:** pin 19 bonds PA10 to PA11 (USB D−). Never drive PA10, never enable a pull-up on it and never route USART1 RX to it. Pin 1 bonds BOOT0 to PB8, so never drive PB8 at boot.
- **N2, LED power:** typical currents at 100 % are R 75, G 52 and B 98 mA (worst case 86, 63 and 125 mA). White at 100 % draws about 225 mA and 1.1 W inside a closed PLA cube, so the firmware caps the total LED current. Declare `bMaxPower` = 300 mA.
- **N1, touch:** the touch trace runs next to LED_B. Synchronize TouchKey sampling with the LED PWM.

## License

CERN-OHL-S-2.0, see [LICENSE](LICENSE).
