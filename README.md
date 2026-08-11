## Input Controller (RP2040 front panel)

Firmware for the **front-panel / input board**: an RP2040 Arduino sketch that scans **faders, potentiometers, encoders, and buttons**, drives **status LEDs**, caches the **DCO's preset directory in RAM** (no LittleFS on this board — the DCO's LittleFS is the single preset store), and acts as the **serial hub** between the synth's voice side (two-way) and the **screen** (TX only).

Dual-core: Core 0 scans the panel; Core 1 maps manual controls, talks serial, refreshes LEDs, and fetches the preset directory at boot.

All detailed documentation lives under **[`docs/`](docs/)**. This README is the entry point.

---

## One sketch, two instruments

This is one repository, checked out into both synths, and the two working trees
are **byte-identical** — there is no model line to keep straight and nothing to
set before building. The instrument comes from the superproject:
[`project_config.h`](project_config.h) here is a symlink to `../project_config.h`,
so the same committed symlink resolves to `PROJECT_INSTRUMENT 3` in
DCO3-MONOSYNTH and `4` in DCO4-REBORN. [`board_model.h`](board_model.h) reads it
and derives everything that differs:

| | DCO3-MONOSYNTH | DCO4-REBORN |
|---|---|---|
| Voices / oscillators | 1 voice, 3 oscillators + sub | 4 voices of 2 oscillators (8 total) |
| Voice-side peer | the DCO, directly | the STM32 Mainboard, which relays to the DCO |
| `DCO_PORT` | `Serial1` (RX GP1 / TX GP0) | `Serial2` (RX GP5 / TX GP4) |
| `SCREEN_PORT` | `Serial2` (TX GP4) | `Serial1` (TX GP0) |
| GP5 | free, reused as LED PWM | **Mainboard RX — never drive it** |
| OSC3 encoder actions | live | `ACTION_NONE` |
| ADSR3 destination cycle | 0..4 (OSC1/2/3, all) | 0..2 (A, B, A+B) |
| Calibration stages | 2 per oscillator (saw, pulse) = 6 | 1 per oscillator = 8 |
| Wave keys 0..4 | OSC1 saw, OSC2 pulse, OSC1 tri, OSC1 pulse, OSC3 pulse | OSC A saw/pulse/tri, OSC B saw/pulse |
| Default voice mode | 0 (mono) | 1 (poly) |

Both instruments build with the same command, from either tree:

```bash
arduino-cli compile --fqbn rp2040:rp2040:rpipico --libraries ./_build_libs .
```

Building outside a superproject, where `project_config.h` does not resolve, is a
compile error rather than a silent monosynth default — that default is what used
to flash 3-oscillator firmware onto a 4-voice panel. Pass the model explicitly to
compile-check the other instrument from this tree:

```bash
arduino-cli compile --fqbn rp2040:rp2040:rpipico --libraries ./_build_libs \
  --build-property "compiler.cpp.extra_flags=-DINPUT_BOARD_MODEL=4" .
```

---

## Documentation

| Doc | Contents |
|-----|----------|
| [`docs/SYSTEM_OVERVIEW.md`](docs/SYSTEM_OVERVIEW.md) | Where this board sits, per-model UART table |
| [`docs/CONTROL_PIPELINE.md`](docs/CONTROL_PIPELINE.md) | Dual-core scan, outbound `'a'`–`'d'`/`'p'`, inbound relays |
| [`docs/PANEL_AND_PINS.md`](docs/PANEL_AND_PINS.md) | Mux, fader/pot indices, LED/UART pins per model |
| [`docs/PRESETS.md`](docs/PRESETS.md) | Preset ownership (DCO store, Input RAM cache), directory sync, UI FSM |
| [`docs/FILE_INDEX.md`](docs/FILE_INDEX.md) | Every file + functions + call sites |
| [`docs/REFERENCE_AI.md`](docs/REFERENCE_AI.md) | Deep semantic map for developers / AI |
| [`docs/README_serial_and_params.md`](docs/README_serial_and_params.md) | Shared serial / ParamId how-to |

**Suggested reading:** this README → system overview → control pipeline / panel pins → presets → FILE_INDEX or REFERENCE_AI.

---

## Features

- **Panel:** CD74HC4067 muxed analog (8 faders + pots) and digital (encoders + 16 buttons).
- **Encoders:** 11× `MD_REncoder` with layered actions (normal / alt / preset / cal / menu).
- **Buttons:** Wave toggles, LFO, voice mode, manual fader/pot enable, preset load/save, function key, calibration UI.
- **LEDs:** Dual 74HC595 status indicators (+ PWM brightness on DCO3 only).
- **Presets:** the DCO's 256-slot LittleFS store is the single source of truth; Input keeps a RAM-only 256-name directory cache (`presetDir[256][16]`), synced via `'N'`/`'O'`/`'L'`.
- **Hub relay:** inbound `'x'` gap (154) forwarded verbatim to the Screen; cal offset (155) stored locally; `'d'` filter block tracked locally and forwarded.
- **Serial:** 2.5 Mbaud on both links; USB Serial @ 2 Mbaud debug (off by default).

**Inactive today:** local `params.ino` apply router (commented); `sendSerial()` is an empty stub; USB MIDI stack commented out.

---

## High-level architecture

| Subsystem | Files | Role |
|-----------|-------|------|
| Model config | `project_config.h` (symlink), `board_model.h` | Which instrument, then voice count, UART wiring, panel layout |
| Entry | `INPUT-CONTROLLER.ino` | Dual-core setup/loop |
| Scan | `Controls.*`, `encoders.*`, `buttons.*` | Mux + actions |
| LEDs | `LED_control.*` | 595 mux |
| Serial | `Serial.*`, `Serial2.ino`, `serial_*.h` | TX blocks/params; RX relays |
| Presets | `presetStorage.ino` | RAM directory cache + save/load/loaded protocol |
| Timing | `Timers_millis.*` | Soft timers both cores |

Details: [`docs/CONTROL_PIPELINE.md`](docs/CONTROL_PIPELINE.md).

---

## Building

- **Toolchain:** Arduino IDE / CLI with Earle Philhower RP2040 core (or compatible).
- **Sketch:** `INPUT-CONTROLLER.ino` (matches the folder name in both projects).
- **Libraries:** `SimpleKalmanFilter`. `DCO-PROTOCOL` is `_build_libs/DCO-PROTOCOL` → `../../DCO-PROTOCOL`; sketch-root shims (`params_def.h`, etc.) forward into it so Arduino IDE finds the headers without `--libraries`. `RoxMux_fela` is `_build_libs/RoxMux_fela` → `../../RoxMux_FELA`. `MD_REncoder_fela` is `_build_libs/MD_REncoder_fela` → `../../MD_REncoder_FELA`. `CD74HC4067` is `_build_libs/CD74HC4067` → `../../CD74HC4067_FELA` (not sketchbook). LittleFS is **no longer** a dependency.

```bash
arduino-cli compile --fqbn rp2040:rp2040:rpipico --libraries ./_build_libs .
```

### Feature flags

| Flag | Default | Effect |
|------|---------|--------|
| `INPUT_BOARD_MODEL` | `PROJECT_INSTRUMENT` from the superproject's `project_config.h` | Selects DCO3 vs DCO4; derives voice count, UARTs and panel layout. Set it on the build line only to cross-check the other instrument |
| `ENABLE_SERIAL` | **off** (commented) | USB debug printing |
| `ENABLE_DCO_LINK` | on | Voice-side UART (`DCO_PORT`) |
| `ENABLE_SCREEN_LINK` | on | Screen UART (`SCREEN_PORT`) |
| `SERIAL_FRAMING_COBS` | **off** (commented) | Must match the DCO/Mainboard/Screen; host: `dco_control --cobs` |
| `SERIAL_INNER_MAX_PAYLOAD` | 17 | Largest inner frame (Screen `'q'` scroll and inbound `'O'`) |

`NUM_VOICES` and `NUM_OSCILLATORS` are no longer set here — they come from `board_model.h`.

---

## Contributing / hacking

- Start with [`docs/REFERENCE_AI.md`](docs/REFERENCE_AI.md) and [`docs/FILE_INDEX.md`](docs/FILE_INDEX.md).
- `params_def.h` is the canonical superset enum shared byte-for-byte by the DCO, Mainboard, Input and Screen of both instruments. Edit it in `DCO3-MONOSYNTH/DCO/` and copy it out; never renumber an existing ID.
- Put anything that differs between the two instruments in `board_model.h` rather than an `#ifdef` at the use site, and keep every other file identical across the two trees.
- When adding continuous controls, respect manual flags + `serial_send_manual_controls`.
- Preset **storage** layout lives on the DCO (`preset_store.ino`); this board only needs to keep the 16-byte name width and the `'N'`/`'O'`/`'L'` directory protocol in sync with it. On DCO4, a new command byte also has to be registered in the Mainboard's relay tables.
