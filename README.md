## DCO4 – Input Controller (RP2040 front panel)

Firmware for the **DCO4 front-panel / input board**: an RP2040 Arduino sketch that scans **faders, potentiometers, encoders, and buttons**, drives **status LEDs**, stores **presets in LittleFS**, and acts as the **serial hub** between the **DCO** (`Serial1`, two-way) and the **screen** (`Serial2`, TX only).

Dual-core: Core 0 scans the panel; Core 1 maps manual controls, talks serial, refreshes LEDs, and runs filesystem init.

How this board fits the instrument: [`docs/SYSTEM_OVERVIEW.md`](docs/SYSTEM_OVERVIEW.md) → canonical overview in sibling **DCO4_DCO**.

All detailed documentation lives under **[`docs/`](docs/)**. This README is the entry point.

---

## Documentation

| Doc | Status | Contents |
|-----|--------|----------|
| [`docs/SYSTEM_OVERVIEW.md`](docs/SYSTEM_OVERVIEW.md) | Current | Stub → canonical three-board overview + local UART table |
| [`docs/CONTROL_PIPELINE.md`](docs/CONTROL_PIPELINE.md) | Current | Dual-core scan → slim LE `'a'`–`'d'` / `'p'` → DCO & Screen; DCO `'x'` relay |
| [`docs/PANEL_AND_PINS.md`](docs/PANEL_AND_PINS.md) | Current | Mux, fader/pot indices, LED/UART pins |
| [`docs/PRESETS.md`](docs/PRESETS.md) | Current | Preset ownership, UI FSM, load/save, 180-byte v1 layout |
| [`docs/FILE_INDEX.md`](docs/FILE_INDEX.md) | Current | Every file + functions + call sites |
| [`docs/REFERENCE_AI.md`](docs/REFERENCE_AI.md) | Current | Deep semantic map for developers / AI |
| [`docs/README_serial_and_params.md`](docs/README_serial_and_params.md) | Current | Shared serial / ParamId how-to |

**Suggested reading:** this README → system stub → control pipeline / panel pins → presets → FILE_INDEX or REFERENCE_AI.

---

## Features

- **Panel:** CD74HC4067 muxed analog (8 faders + pots) and digital (encoders + 16 buttons).
- **Encoders:** 11× `MD_REncoder` with layered actions (normal / alt / preset / cal / menu).
- **Buttons:** Wave toggles, LFO, voice mode, manual fader/pot enable, preset load/save, function key, calibration UI.
- **LEDs:** Dual 74HC595 status indicators + PWM brightness.
- **Presets:** 256 × 180-byte LittleFS bank (`presetBank1`; migrates legacy 140-byte slots).
- **Hub relay:** inbound DCO `'x'` gap (154) forwarded verbatim to the Screen; cal offset (155) stored locally.
- **Serial:** 2.5 Mbaud to the DCO (`Serial1` / `DCO_PORT`, bidirectional) and Screen (`Serial2` / `SCREEN_PORT`, TX only); USB Serial @ 2 Mbaud debug.

**Inactive today:** local `params.ino` apply router (commented); legacy `sendSerial()`; USB MIDI stack commented out.

---

## High-level architecture

| Subsystem | Files | Role |
|-----------|-------|------|
| Entry | `DCO4_Input_Controller.ino` | Dual-core setup/loop |
| Scan | `Controls.*`, `encoders.*`, `buttons.*` | Mux + actions |
| LEDs | `LED_control.*` | 595 mux |
| Serial | `Serial.*`, `Serial2.ino`, `serial_*.h` | TX blocks/params; RX DCO `'x'` + Screen relay |
| Presets | `FS.h`, `presetStorage.ino` | LittleFS |
| Timing | `Timers_millis.*` | Soft timers both cores |

Details: [`docs/CONTROL_PIPELINE.md`](docs/CONTROL_PIPELINE.md).

---

## Hardware / UART summary

| Port | Pins | Baud | Peer |
|------|------|------|------|
| Serial | USB | 2 000 000 | Debug |
| Serial1 (`DCO_PORT`) | RX1 / TX0 | 2 500 000 | DCO (TX panel data on GP0, RX `'x'` 154/155 on GP1) |
| Serial2 (`SCREEN_PORT`) | RX5 / TX4 | 2 500 000 | Screen (TX only on GP4; RX5 unwired) |

Mux select GP18–21; analog SIG GP27; digital SIG GP2/16/17. Full map: [`docs/PANEL_AND_PINS.md`](docs/PANEL_AND_PINS.md).

---

## Building

- **Toolchain:** Arduino IDE / CLI with Earle Philhower RP2040 core (or compatible).
- **Sketch:** `DCO4_Input_Controller.ino`.
- **Libraries:** `RoxMux` / `RoxMux_fela`, `CD74HC4067`, `SimpleKalmanFilter`, LittleFS (core). `MD_REncoder_fela` is `_build_libs/MD_REncoder_fela` → `../../MD_REncoder_FELA` (not sketchbook).

### Feature flags

| Flag | Default | Effect |
|------|---------|--------|
| `ENABLE_SERIAL` | on | USB debug |
| `ENABLE_DCO_LINK` | on | DCO UART (`DCO_PORT` = `Serial1`) |
| `ENABLE_SCREEN_LINK` | on | Screen UART (`SCREEN_PORT` = `Serial2`) |
| `SERIAL_FRAMING_COBS` | **off** (commented) | Must match DCO/Screen; host: `dco_control --cobs` |
| `NUM_VOICES` | **1** | Array sizing (monosynth) |
| `NUM_OSCILLATORS` | **3** | Cal offset array / stage → osc index |

No float/fixed engine forks (no `ENGINE_OPTIONS.md`).

OSC3 ParamIds **33–35** and ADSR3→osc **0–4** match DCO. Encoder: enc3 action3 = OSC3 interval; enc5 action3 = OSC3 detune; enc5 alt3 = LFO2→OSC3.

---

## Contributing / hacking

- Start with [`docs/REFERENCE_AI.md`](docs/REFERENCE_AI.md) and [`docs/FILE_INDEX.md`](docs/FILE_INDEX.md).
- Keep ParamIds aligned with DCO `params_def.h` (the DCO is the protocol reference; Mainboard archived).
- When adding continuous controls, respect manual flags + `serial_send_manual_controls`.
- When extending presets, update both `loadPreset` and `writePreset` layouts.
