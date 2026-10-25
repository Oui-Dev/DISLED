# touch_replay

Replays a touch recording through `touch_detector`, on the build machine,
so that detector settings can be compared before they are flashed.

## Recording

On the debug console, `log 2` (to keep the touch events for comparison)
then `watch 20`: one line per 20 ms window, as the detector sees it. Save
the console output to a file.

The replay feeds each window value to the detector 20 times, as 1 kHz
samples. The console may skip a window now and then; the timing is close
enough to compare settings, not to time a long press to the millisecond.

## Build and run

Any C11 compiler for the build machine:

```sh
gcc -std=c11 -O2 -I../../lib/disled_core/include \
    touch_replay.c ../../lib/disled_core/src/touch_detector.c -o touch_replay
./touch_replay recording.log
./touch_replay recording.log rise=15 debounce=60
```

Settings: `press`, `release`, `rise`, `proximity` (per mille), `debounce`,
`rearm`, `long`, `object` (ms). The defaults copy `app_touch_config` from
`src/app/app_config.c`; check that they still match.
