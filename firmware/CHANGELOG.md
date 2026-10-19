# Changelog

## 0.1.0 (2026-10-19)


### Features

* **core:** lamp logic, touch detection, light pipeline and protocol codec ([ab5d6a2](https://github.com/Oui-Dev/DISLED/commit/ab5d6a2c416b5f20001ff10c132f09bf87a37d36))
* **firmware:** board support and peripheral drivers ([ec87d6b](https://github.com/Oui-Dev/DISLED/commit/ec87d6bd25280fe8c8192b9a3b3e5e4938e03b24))
* **firmware:** compensate the light lost per PWM pulse ([86e26b4](https://github.com/Oui-Dev/DISLED/commit/86e26b46b96e7c043362f89538240d72b5360fde))
* **firmware:** dither the LED PWM to 1/16 of a tick ([f3b5c55](https://github.com/Oui-Dev/DISLED/commit/f3b5c55f0305aa39c63254cc96ca96abc63429a2))
* **firmware:** pulse loss on red and green too ([b3fba4a](https://github.com/Oui-Dev/DISLED/commit/b3fba4abb0ee7941597f61e99ebd4d86d06359f6))
* **firmware:** shorter touch debounce ([8381c23](https://github.com/Oui-Dev/DISLED/commit/8381c2369f28d7afece859580f13e89d730e0bce))
* **firmware:** touch detection with a hand resting near the lid ([6b30667](https://github.com/Oui-Dev/DISLED/commit/6b30667cef9d912a9816cbf256168ee998f5c623))
* **firmware:** touch thresholds and orange for the closed enclosure ([3543e6d](https://github.com/Oui-Dev/DISLED/commit/3543e6d27106444ef1217fa40010a0f9a675c6f3))
* **firmware:** TouchKey timing measured on the board ([28b6913](https://github.com/Oui-Dev/DISLED/commit/28b6913d52b204747a4f94f914fb52e209cc04c5))
* **firmware:** tune the orange on the board ([1f9afb0](https://github.com/Oui-Dev/DISLED/commit/1f9afb0082d0f6029feffe54511a73d710ed4f55))
* **firmware:** tune warm white and the low brightness end on the board ([aec044b](https://github.com/Oui-Dev/DISLED/commit/aec044bb25a1758446914f71cb0803f4258557c3))
* **firmware:** USB transport, debug console and application MVP ([5628c64](https://github.com/Oui-Dev/DISLED/commit/5628c642fad21e73b69e98479e223d68d45aa029))
* **protocol:** HID protocol v1 specification and shared test vectors ([caf803d](https://github.com/Oui-Dev/DISLED/commit/caf803dc52bf977ceebe19668b43337e814592f1))


### Bug Fixes

* **firmware:** bring-up fixes on the first v4 board ([6a0b991](https://github.com/Oui-Dev/DISLED/commit/6a0b99198ebd4eae93e42c657c0e4f872151ab26))
* **firmware:** one state_seq step per STATE message with new content ([32d6d66](https://github.com/Oui-Dev/DISLED/commit/32d6d66597fd330f584795b871944f1faf73a642))
* **firmware:** skip light readings until the sensor settles ([c63b2a1](https://github.com/Oui-Dev/DISLED/commit/c63b2a1c85f0be7f8c5db7f86234cd26ea9d50c6))
