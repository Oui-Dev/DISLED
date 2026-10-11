# Lamp behavior

This is the reference specification of what the lamp does. The firmware
state machine (`firmware/lib/disled_core/lamp_fsm`) and its unit tests
implement it, and the scenarios at the end verify it on a real board.

## State

| Item | Values |
|---|---|
| Power | `ON`, `OFF` |
| Color | `GREEN`, `ORANGE`, `RED`, `WARM_WHITE` |
| Brightness mode | `AUTO` (follows the room light) or `MANUAL` (1–100 %) |
| USB state | `UNCONFIGURED`, `CONFIGURED`, `SUSPENDED` |

At power-up the lamp is `ON`, `GREEN`, `AUTO`. Nothing is stored in flash;
the desktop app applies its saved settings again when it connects.

## Touch

| Gesture | Lamp on | Lamp off |
|---|---|---|
| Short press (released before 2 s) | Next color: green → orange → red → green. Warm white → green. | Ignored |
| Long press (held for 2 s) | Turns off at 2 s, while the finger is still down | Turns on at 2 s, with the last color |
| Contact longer than 10 s | Treated as an object on the lid: no further action, baseline recalibrated | Same |

Each contact triggers at most one action. Releasing the touch re-arms it.

## Host control (USB HID)

- The host reads the state and can set any subset of: power, color (one of
  the four), brightness mode, manual level (1–100 %).
- Every change, whatever its source (touch, host or automatic brightness),
  produces a state notification with an increasing sequence number.
- In `AUTO` mode the notification carries the level currently applied, so
  the app can show it on a disabled slider.

## Transitions

Color and power changes fade over about 300 ms. In `AUTO` mode the level
follows the room light through a slow filter (a few seconds) with
hysteresis, so it never flickers.

## USB states and LED power limits

| USB state | When | Lamp | Touch | LED current cap |
|---|---|---|---|---|
| `UNCONFIGURED` | Power-up, plain charger, hub without a host | Works locally | Active | 70 mA |
| `CONFIGURED` | Enumerated by a host | Normal | Active | 125 mA (tuned after thermal test V2) |
| `SUSPENDED` | Host asleep | Off | Ignored | 0 |

On resume, the lamp shows its previous state again.

The caps use the worst-case LED model: 86 mA (red), 63 mA (green) and
125 mA (blue) at 100 % duty. When a color would exceed the cap, the three
channels are scaled down together, which keeps the hue. A pure color fits
within 125 mA; warm white is limited to about half of its full power.

## Out of scope for now

Custom RGB colors (the internal color model is RGB so they can be added
later) and integrations with other software.

## Test scenarios

Run them on a board connected to a computer, with the desktop app open.

1. Plug in: the lamp shows green; the app shows `ON`, `GREEN`, `AUTO`.
2. Short press three times: orange, red, green.
3. Set warm white from the app, then short press: green.
4. Long press while on: the lamp turns off at 2 s with the finger still
   down. Releasing does nothing more.
5. Short press while off: nothing happens.
6. Long press while off: the lamp turns on at 2 s with the last color.
7. Leave a phone on the lid for 15 s: one action at 2 s, then the contact
   is ignored. Removing the phone does nothing, and the touch works again
   afterwards.
8. In the app, switch to manual at 30 %: the lamp dims. Switch back to auto:
   the slider shows the applied level and follows the room light.
9. Put the computer to sleep: the lamp turns off and the touch does
   nothing. Wake it up: the previous state comes back.
10. Plug the lamp into a phone charger: it works on its own and the LED
    current stays at or below 70 mA.
11. Every change, from the touch or from the app, shows up in the app with
    an increasing sequence number.
