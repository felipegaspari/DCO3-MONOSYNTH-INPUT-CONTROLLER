# Input Controller — Reference (AI / developers)

Semantic map of the RP2040 front-panel firmware. Prefer [`FILE_INDEX.md`](FILE_INDEX.md) for call sites.

- System: [`SYSTEM_OVERVIEW.md`](SYSTEM_OVERVIEW.md) → DCO board's overview is canonical
- Pipeline: [`CONTROL_PIPELINE.md`](CONTROL_PIPELINE.md)
- Pins: [`PANEL_AND_PINS.md`](PANEL_AND_PINS.md)
- Presets: [`PRESETS.md`](PRESETS.md)
- Serial how-to: [`README_serial_and_params.md`](README_serial_and_params.md)
- Entry: [`../README.md`](../README.md)

---

## One tree, two instruments

This sketch is shared verbatim between DCO3-MONOSYNTH and DCO4-REBORN. The only
line that differs is `INPUT_BOARD_MODEL` in [`../board_model.h`](../board_model.h),
which derives voice/oscillator counts, both UART assignments, LED PWM
availability, OSC3 encoder actions, the ADSR3 destination range, calibration
stage→oscillator mapping, the wave-key table and the default voice mode.

**Rule for every change:** if it differs per instrument, express it as a symbol
in `board_model.h` and consume that symbol at the use site. Do not add
`#if INPUT_BOARD_MODEL == ...` anywhere else, and keep every other file
byte-identical between the two project trees. See the README table for what
each model resolves to.

On DCO4 the panel link's peer is the **Mainboard**, not the DCO; it relays
preset traffic (`'q'`, `'N'`, ParamIds 170/171 outbound; `'O'`, `'L'`, the
persistable `'p'` mirror and `'a'`–`'d'` blocks inbound) and consumes the rest
itself. The relay is per-command-byte, so a new preset command needs registering
in the Mainboard's tables too.

---

## What this board owns

| Owns | Does not own |
|------|----------------|
| Panel scan (faders, pots, encoders, buttons) | Voice allocation / DCO pitch |
| RAM-only 256-slot preset **name cache** | Preset **storage** (the DCO's LittleFS is the only store) |
| UART fan-out of controls/params to the voice side + Screen | TFT UI (Screen) |
| Serial hub role: relay DCO gap `'x'` 154 + persistable `'p'` to the Screen | Gap measurement itself (DCO) |
| Manual-mode continuous control streaming | Amp/PW calibration measurement (DCO) |

Primarily a **protocol sender**, plus inbound `'x'` (gap/cal), the persistable
`'p'` mirror (USB/MIDI/preset-load → in-RAM locals + Screen toast), the `'d'`
filter block, and preset directory sync (`'O'`/`'L'` → RAM name cache). The live
inbound apply-router is unused (`params.ino` commented); the `'p'` handler
writes locals only and does not re-TX. Input has **no LittleFS**.

---

## Runtime model

**Core 0:** scan hardware (`readControls` → mux / encoders / buttons). Encoder/button handlers emit ParamIds, UI signals, preset ops.

**Core 1:** map filtered ADC when manual flags set; TX slim LE `'a'`–`'d'` / `'p'` on `DCO_PORT`; LED refresh; parse inbound `'x'` / persistable `'p'` / `'d'` / `'O'` / `'L'` on `DCO_PORT` (`serial_read_from_dco`) and relay gap 154, persistable `'p'` and `'d'` to the Screen on `SCREEN_PORT`. `setup1()` issues the `'N'` directory request.

---

## Modules

| Module | Role |
|--------|------|
| `board_model.h` | The one model switch; derives counts, ports, panel layout |
| `Controls.*` | Mux GPIO, ADC Kalman, `setControlValues` |
| `encoders.*` | 11 encoders + `EncoderAction` table (OSC3 slots gated per model) |
| `buttons.*` | 16 buttons + mode machine; `toggle_wave_key()` drives the per-model wave table |
| `LED_control.*` | Dual 595 status LEDs; first five follow `inputWaveKeys[]` |
| `Serial.ino` / `Serial2.ino` | TX helpers; manual blocks; inbound parser + Screen relay |
| `presetStorage.ino` | RAM-only 256-name directory cache + DCO save/load/loaded protocol |
| `params_def.h` | Canonical superset enum, byte-identical across all seven board copies |
| `Timers_millis.*` | Soft timers for both cores |
| `auxiliary.*` | Kalman + lin→exp table |

---

## Edit carefully

- **`params_def.h` is a shared superset**, not a fork: the same file is copied byte-for-byte to the DCO, Mainboard, Input and Screen of both instruments. Edit the DCO3 DCO copy and propagate; never renumber or reuse an existing ID, and expect any given board to implement only a subset.
- **`'a'`–`'d'` payload sizes + LE** — must match the DCO handlers. VCA/PW are `'p'` 222 / 210, not `'e'`/`'f'`.
- **Voice-side `'q'` is 16 chars; Screen `'q'` is preset# + 16 chars** — same length now, but still two separate frames/ports; do not conflate. `SERIAL_INNER_MAX_PAYLOAD` is 17 here, sized for the Screen's 17-byte `'q'`, which the 17-byte `'O'` directory entry also fits. The DCO4 Mainboard had to be raised to 17 for the same reason.
- **Framing** — default RAW; `#define SERIAL_FRAMING_COBS` must match every other board.
- **Never address `Serial1` / `Serial2` directly** — the numbers are swapped between the two PCBs and do not tell you the peer. Always use `DCO_PORT` / `SCREEN_PORT`.
- **TX waits** must use `availableForWrite() < 1` — RP2040 hardware UARTs report only 0 or 1 free.
- **GP5** is LED PWM on DCO3 (harmless: no conductor, and the Screen never transmits) but is the **Mainboard RX** line on DCO4, which is why the PWM setup is gated behind `INPUT_HAS_LED_PWM`.
- **Preset record layout lives on the DCO** (`preset_store.ino`, 598-byte record). Input has no layout to keep in sync — only the 16-byte name width (`'q'`, `presetDir[256][16]`) and the `'N'`/`'O'`/`'L'` protocol.

---

## Dead / inactive

- `sendSerial()` — retired; kept as an empty stub, not scheduled
- `params.ino` / most `formulas.ino` — commented
- TinyUSB MIDI — include commented
- Legacy preset-save encoder path in `Controls.ino` — superseded by main encoder/button modes
