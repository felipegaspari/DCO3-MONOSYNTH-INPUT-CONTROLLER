# DCO4_Input_Controller — Reference (AI / developers)

Semantic map of the RP2040 front-panel firmware. Prefer [`FILE_INDEX.md`](FILE_INDEX.md) for call sites.

- System: [`SYSTEM_OVERVIEW.md`](SYSTEM_OVERVIEW.md) → DCO4_DCO canonical
- Pipeline: [`CONTROL_PIPELINE.md`](CONTROL_PIPELINE.md)
- Pins: [`PANEL_AND_PINS.md`](PANEL_AND_PINS.md)
- Presets: [`PRESETS.md`](PRESETS.md)
- Serial how-to: [`README_serial_and_params.md`](README_serial_and_params.md)
- Entry: [`../README.md`](../README.md)

---

## What this board owns

| Owns | Does not own |
|------|----------------|
| Panel scan (faders, pots, encoders, buttons) | Voice allocation / DCO pitch |
| LittleFS preset bank | ADSR/LFO CV generation (DCO) |
| UART fan-out of controls/params to DCO + Screen | TFT UI (Screen) |
| Serial hub role: relay DCO gap `'x'` 154 to the Screen | Gap measurement itself (DCO) |
| Manual-mode continuous control streaming | Amp/PW calibration measurement (DCO) |

Primarily a **protocol sender**, plus a thin relay of DCO `'x'` frames to the Screen. Live inbound apply-router is almost unused (`params.ino` commented).

---

## Runtime model

**Core 0:** scan hardware (`readControls` → mux / encoders / buttons). Encoder/button handlers emit ParamIds, UI signals, preset ops.

**Core 1:** map filtered ADC when manual flags set; TX slim LE `'a'`–`'d'` / `'p'` on `DCO_PORT`; LED refresh; parse inbound DCO `'x'` on `DCO_PORT` (`serial_read_from_dco`) and relay gap 154 to the Screen on `SCREEN_PORT`.

---

## Modules

| Module | Role |
|--------|------|
| `Controls.*` | Mux GPIO, ADC Kalman, `setControlValues` |
| `encoders.*` | 11 encoders + `EncoderAction` table |
| `buttons.*` | 16 buttons + mode machine |
| `LED_control.*` | Dual 595 status LEDs |
| `Serial.ino` / `Serial2.ino` | TX helpers; manual blocks; inbound DCO `'x'` parser + Screen relay |
| `presetStorage.ino` / `FS.h` | LittleFS bank |
| `params_def.h` | Shared IDs (fork may lag the DCO copy — sync carefully) |
| `Timers_millis.*` | Soft timers for both cores |
| `auxiliary.*` | Kalman + lin→exp table |

---

## Edit carefully

- **ParamId numbers** — keep aligned with the DCO `params_def.h` (coordination point).
- **`'a'`–`'d'` payload sizes + LE** — must match DCO handlers. VCA/PW are `'p'` 222 / 210, not `'e'`/`'f'`.
- **DCO `'q'` is 8 chars; Screen `'q'` is preset# + 16 chars** — do not conflate. `SERIAL_INNER_MAX_PAYLOAD` is 17 here for Screen TX.
- **Framing** — default RAW; `#define SERIAL_FRAMING_COBS` must match DCO/Screen.
- **`DCO_PORT` is `Serial1`** (TX GP0 → DCO GP21, RX GP1 ← DCO GP20); **`SCREEN_PORT` is `Serial2`** (TX GP4 → Screen GP13, TX-only — the Screen never transmits). Never address `Serial1` / `Serial2` directly; the numbers do not tell you the peer.
- **TX waits** must use `availableForWrite() < 1` — RP2040 hardware UARTs report only 0 or 1 free.
- LED PWM on **GP5** takes the pin back from `Serial2` RX, which is fine: no conductor, and the Screen never transmits.
- Preset **140-byte** layout — extend only by updating both load and write paths.

---

## Dead / inactive

- `sendSerial()` — not scheduled
- `params.ino` / most `formulas.ino` — commented
- TinyUSB MIDI — include commented
- Legacy preset-save encoder path in `Controls.ino` — superseded by main encoder/button modes
