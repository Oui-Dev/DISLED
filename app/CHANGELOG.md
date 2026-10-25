# Changelog

## [1.0.0](https://github.com/Oui-Dev/DISLED/compare/app-v0.5.1...app-v1.0.0) (2026-10-25)


### 🚀 Enhancements

* **app:** brightness on one slider, as on a phone ([74d8f0b](https://github.com/Oui-Dev/DISLED/commit/74d8f0b25693a72ee9cfeefd315aedbea99c3471))
* **app:** color mode and language settings, translated tray and window titles ([5401de9](https://github.com/Oui-Dev/DISLED/commit/5401de98ee5863df8d5ae7b9ae5028694d979086))
* **app:** diagnostics window with tabs on the left, telemetry on the right ([c4805ed](https://github.com/Oui-Dev/DISLED/commit/c4805edda3f0295b91ebae54333d458e613d9851))
* **app:** popup rebuilt with Nuxt UI and sized to its content ([ac52053](https://github.com/Oui-Dev/DISLED/commit/ac52053ba07639508ef9a27e2d95eabc12c642e7))
* **app:** popup settings with the rare actions in a "more" menu ([ded65fd](https://github.com/Oui-Dev/DISLED/commit/ded65fdf18dc9f1750aff39cb334f38771d8ca0f))
* **app:** the brightness slider sets the correction in automatic mode ([b525caa](https://github.com/Oui-Dev/DISLED/commit/b525caa0343bc48b713a8bb2caed575c85bf169e))


### 🩹 Fixes

* **app:** keep the last error on show while the app polls the lamp ([800018c](https://github.com/Oui-Dev/DISLED/commit/800018ce1148bb2c0afccb997435d743f05fc07a))
* **app:** no scrollbar flash while the popup grows ([e47819c](https://github.com/Oui-Dev/DISLED/commit/e47819c4c5439ee9c8cf0445f781236eee0730ad))
* **app:** reinstall the lamp firmware from the settings menu at any time ([2fc9d86](https://github.com/Oui-Dev/DISLED/commit/2fc9d86c051b8350ba963eecb0c08c1a0bdeb206))
* **app:** stop the lamp telemetry when the Diagnostics window closes ([4ca422e](https://github.com/Oui-Dev/DISLED/commit/4ca422e50958eab102594fd31c10e5699937f756))
* **app:** the brightness slider stays put when the mode changes ([4bfdb20](https://github.com/Oui-Dev/DISLED/commit/4bfdb20b1ebf554779630f4892ae494493bc02cc))
* **app:** the popup opens again on the lamp page ([4d4cec3](https://github.com/Oui-Dev/DISLED/commit/4d4cec38db6e4dae097cb8ccde5574b51bb0374a))
* **hid:** Limit device scan to lamp vendor to prevent stalls ([4531fa4](https://github.com/Oui-Dev/DISLED/commit/4531fa43ef9c899a956127841a64f527e22510d1))


### 💅 Refactors

* **app:** accessibility fixes from an axe audit ([298beb8](https://github.com/Oui-Dev/DISLED/commit/298beb8a2b0e4de47bb04822bb9eb52934738d06))
* **app:** diagnostics window and charts with Nuxt UI ([e87f0be](https://github.com/Oui-Dev/DISLED/commit/e87f0be0d6929e2ff3f59523517dbc19b5686c97))
* **app:** fainter tint for the neutral palette, grey off swatch ([9013f7a](https://github.com/Oui-Dev/DISLED/commit/9013f7adb55364fa9e014c90eca3ca07c6c0a182))
* **app:** improve CI, documentation, and app clarity ([1cbe034](https://github.com/Oui-Dev/DISLED/commit/1cbe0342aa6beb3162097a0a6e8ce847be90b943))
* **app:** keep module helpers private ([e6c15a4](https://github.com/Oui-Dev/DISLED/commit/e6c15a4ad411e9dad739d4ec19b26bd1d6664a8a))
* **app:** lamp update window with Nuxt UI ([5d35178](https://github.com/Oui-Dev/DISLED/commit/5d3517881bb390a198f28ee23586f94f30bb708e))
* **app:** Pinia stores over a typed IPC layer ([ebc4344](https://github.com/Oui-Dev/DISLED/commit/ebc4344606c3740af82a3c12ef0f939e81e02aa6))
* **app:** popup footer at the bottom and smaller, compact alert, cards for the settings ([123a0b9](https://github.com/Oui-Dev/DISLED/commit/123a0b96a8e9f82374c70c3ec92b68c6fe9de2d7))
* **app:** smaller popup controls, a 360 px popup, tidier diagnostics header ([5d5565e](https://github.com/Oui-Dev/DISLED/commit/5d5565eb85e3e322cba15b18e28ad3a3a9380c1e))
* **app:** telemetry at 250 ms, recording at the bottom, a legend per chart ([53f0f6d](https://github.com/Oui-Dev/DISLED/commit/53f0f6d31b133e99c73fd79e6cfd7e74f0f1d768))
* **app:** TypeScript types generated from the Rust types ([3c6192b](https://github.com/Oui-Dev/DISLED/commit/3c6192b415d6458805215cae452245cd854bcef4))
* global cleanup & some fix ([5c8ce22](https://github.com/Oui-Dev/DISLED/commit/5c8ce22ec7076839bf598693b397fc92db1bbf03))

## [0.5.1](https://github.com/Oui-Dev/DISLED/compare/app-v0.5.0...app-v0.5.1) (2026-10-24)


### Bug Fixes

* **app:** rolldown to tauri 2.11 to prevent update issue ([b3a22af](https://github.com/Oui-Dev/DISLED/commit/b3a22afc5d4497cc8beb9538678c99f2aa908247))

## [0.5.0](https://github.com/Oui-Dev/DISLED/compare/app-v0.4.0...app-v0.5.0) (2026-10-24)


### Features

* **app:** logo of the lamp on the app and its installers ([95b1244](https://github.com/Oui-Dev/DISLED/commit/95b124405ff349f470dd356b6d9a145634246b81))
* **app:** rounded squares for the tray icon and swatches, and an off swatch ([64b92f5](https://github.com/Oui-Dev/DISLED/commit/64b92f58dba1b57e5b277a918e793dc490082d31))


### Bug Fixes

* **app:** ensure exported CSV ends with newline ([6650786](https://github.com/Oui-Dev/DISLED/commit/6650786ca43e8b4960111b30931f79798ba36723))
* **app:** sign the macOS app ad hoc so that it opens on Apple Silicon ([74ba1b4](https://github.com/Oui-Dev/DISLED/commit/74ba1b47e7bb89746912503c918719d593f1a4d4)), closes [#19](https://github.com/Oui-Dev/DISLED/issues/19)

## [0.4.0](https://github.com/Oui-Dev/DISLED/compare/app-v0.3.0...app-v0.4.0) (2026-10-21)


### Features

* **app:** update the lamp firmware from the app ([4036ce7](https://github.com/Oui-Dev/DISLED/commit/4036ce7319b2b1e73ed96e3bc98e5aa9bfaca32a))


### Bug Fixes

* **deps:** bump the production group across 1 directory with 3 updates ([4746a45](https://github.com/Oui-Dev/DISLED/commit/4746a45daea3a3cbeddf4b569e0fdfc1f544c6a9))

## [0.3.0](https://github.com/Oui-Dev/DISLED/compare/app-v0.2.0...app-v0.3.0) (2026-10-20)


### Features

* **app:** live in the tray with a popup, settings applied at every connection ([45294be](https://github.com/Oui-Dev/DISLED/commit/45294bec26cbddc953472cca6c18ad5726dcab03))
* **app:** updates from GitHub Releases, offered in the popup ([1c16054](https://github.com/Oui-Dev/DISLED/commit/1c160546f981723c3c8b34a3b4c4cf1afa298ba2))

## [0.2.0](https://github.com/Oui-Dev/DISLED/compare/app-v0.1.0...app-v0.2.0) (2026-10-20)


### Features

* **protocol:** protocol 1.1 draft and a shared Rust codec ([fe8cd0b](https://github.com/Oui-Dev/DISLED/commit/fe8cd0befb2c218c26578ec54271b1fb7fae4e72))


### Bug Fixes

* **app:** let the logged-in user reach the lamp on Linux ([5eabaf4](https://github.com/Oui-Dev/DISLED/commit/5eabaf42945191df8ddf31852413a51e98097edf))

## 0.1.0 (2026-10-19)


### Features

* **app:** Tauri 2 test and tuning app ([ce35758](https://github.com/Oui-Dev/DISLED/commit/ce357583239df80283a08d9e839a23703b87d173))
