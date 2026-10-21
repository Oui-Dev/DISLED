# 0017 — Automatic brightness: sensor ranging and a logarithmic filter

- Status: Accepted
- Date: 2026-10-21

## Context

Until firmware 0.2.1, automatic brightness read the VEML7700 at one
setting: gain 1/8, 100 ms, so 0.54 lx per count and up to 35 klx. A curve
went from 3 % at 1 lx to 100 % at 3000 lx. A linear filter followed it with
a time constant of 3.2 s, and a dead band ignored changes of less than 10
per mille.

On the first board, the sensor looks through a notch in the side wall and
reads a fraction of the light of the room. After a few days of use
(2026-10-21):

- the minimum suited the night, with the bias at −50 % (1.5 %);
- full brightness took a phone flashlight held in the notch, so the level
  hardly followed the room;
- changes came slowly.

Three causes:

- The curve was a guess for an open sensor. A lit room gives a few to a
  few tens of lux at the sensor, where that setting counts 10 to 100.
- The filter took about 10 s to settle.
- The dead band froze the low levels: ±1 point at 1.5 to 5 %.

## Decision

**Sensor ranging** (`lib/disled_core/als_range`), as the Vishay
application note 84323 describes:

- Six steps: gain 1/8, 1/4, 1 and 2 at 100 ms, then gain 2 at 200 and
  400 ms. In the dark that is 0.0084 lx per count (datasheet rev. 1.8),
  64 times finer than before; in bright light the range still reaches
  35 klx.
- After each reading, the step is kept while it reads more than 50 counts
  and stays under its upper limit. Otherwise the light just measured picks
  the least sensitive step that gives more than 100 counts.
- Gains 1 and 2 stay below 100 lx, where they are linear. Integrations
  longer than 100 ms serve only below about 3 lx, so a lit room still gets
  ten readings a second.
- The sensor is reconfigured as the note does (shutdown, new setting,
  power on), and the integration under way is skipped.
- Left out, because they only matter above 1000 lx, beyond full
  brightness: 25 and 50 ms integrations, and the correction of the
  nonlinearity at high light.
- Left out as well, the 800 ms integration. Its counts are twice as fine,
  but far below the floor of the curve, and on the first board it made the
  lamp see a light switched on in the dark up to 0.4 s later.

**Filter on a logarithmic scale** (`lib/disled_core/brightness`), in log2
of the lux, the way the eye adapts:

- A change larger than a factor of 2 that lasts 0.4 s is then followed with
  a time constant of 0.25 s. A light switched on or off is followed in
  about 1.5 s, the speed the user chose.
- A shorter change (a hand or a shadow in front of the sensor) moves
  nothing.
- Smaller changes are followed with a time constant of 0.75 s, and
  differences under about 2 % never move the lamp.
- The filter counts the time elapsed between readings, so the slower
  readings in the dark keep the same speed.
- Back in automatic mode, the level is the automatic one at once; the LED
  output fades to it.

**Curve** in lux as the sensor reads them inside the enclosure:

- The floor is 1.5 % in the dark, so the bias can go back to 0.
- The first points were tuned live on the first board, with the console
  command `curve` of debug builds (RAM only), the user judging by eye with
  a phone flashlight at several distances: 1.5 % up to 0.1 lx, 28.5 % at
  2 lx, 55 % at 8 lx, 75 % at 25 lx, full brightness from 100 lx. Daily use
  will refine them.

**The protocol does not change.** `TELEMETRY` keeps lux × 100;
`als_raw` is now the count at the range in use.

## Consequences

- The level follows the room over the whole range the sensor sees, with
  no visible steps at low light.
- With the sensor that sensitive, light from the lamp itself may come back
  into the notch, from the wall or the desk. Check in the dark, lamp off,
  then at several levels: if the reading moves, the firmware has to take
  the lamp's own light out of it.
- The curve belongs to this enclosure and this notch: another window needs
  its own measurements.
- The debug build fills 95 % of the flash (31.2 of 32 KB).
