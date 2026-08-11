# Input Controller — panel and pin map

Hardware mapping for the **INPUT-CONTROLLER** sketch (RP2040 front panel). One source tree serves
both instruments: the mux, analog, digital and LED pins below are identical on the two PCBs, but
the **UART assignment**, the **GP5 LED-PWM question**, the **wave-key wiring** and the
**calibration staging** are per-model and come from `board_model.h`, selected by
`INPUT_BOARD_MODEL`.

---

## UART

The two PCBs swap which UART faces which peer, so the peer can **never** be inferred from the port
number. The code only ever names the `DCO_PORT` and `SCREEN_PORT` aliases from `Serial.h`, which
expand to `INPUT_DCO_PORT_OBJ` / `INPUT_SCREEN_PORT_OBJ` in `board_model.h`. `DCO_PORT` is the
panel's outbound data link on both models; on DCO4 it reaches the DCO through the Mainboard.

### DCO3-MONOSYNTH (`INPUT_BOARD_MODEL` = `INPUT_BOARD_DCO3`)

| Port | Alias | Pins | Baud | Peer |
|------|-------|------|------|------|
| `Serial` | — | USB | 2 000 000 | Debug (`ENABLE_SERIAL`, commented out by default) |
| `Serial1` | `DCO_PORT` | TX **GP0** → DCO GP21, RX **GP1** ← DCO GP20 | 2 500 000 | The DCO directly — slim LE `'a'`–`'d'` / `'p'` / `'q'` / `'N'` out; `'x'` 154/155, persistable `'p'` mirror, `'d'` filter block and `'O'`/`'L'` preset frames back in |
| `Serial2` | `SCREEN_PORT` | TX **GP4** → Screen GP13; RX **GP5** unwired | 2 500 000 | Screen — UI frames plus the relayed gap frames, TX only (the Screen never transmits) |

GP5 has no conductor on this PCB and the Screen never transmits, so the pin is reclaimed from
`SCREEN_PORT` RX to dim the panel LEDs (`INPUT_HAS_LED_PWM` = 1).

### DCO4-REBORN (`INPUT_BOARD_MODEL` = `INPUT_BOARD_DCO4`)

| Port | Alias | Pins | Baud | Peer |
|------|-------|------|------|------|
| `Serial` | — | USB | 2 000 000 | Debug (`ENABLE_SERIAL`, commented out by default) |
| `Serial2` | `DCO_PORT` | TX **GP4** → Mainboard PE0, RX **GP5** ← Mainboard PE1 | 2 500 000 | STM32 Mainboard, which relays these frames on to the DCO and the DCO's replies back |
| `Serial1` | `SCREEN_PORT` | TX **GP0** → Screen GP13; RX **GP1** unwired | 2 500 000 | Screen — the same UI frames as on DCO3, TX only |

**Warning: never drive GP5 on DCO4.** GP5 is the Mainboard RX pad here, so `INPUT_HAS_LED_PWM` is
0 and the `analogWrite(PIN_LED_PWM, …)` in `setup1()` is not compiled; LED brightness is fixed on
this instrument. Driving GP5 would kill everything inbound: the gap relay, the calibration
offsets, the persistable `'p'` mirror, the `'d'` filter block and the `'O'`/`'L'` preset directory
traffic.

Both links run at 2 500 000 baud on either model, with FIFO 512 and IRQ mode
(`setPollingMode(false)`). They are brought up in `setup1()` from `INPUT_DCO_RX_PIN`,
`INPUT_DCO_TX_PIN`, `INPUT_SCREEN_RX_PIN` and `INPUT_SCREEN_TX_PIN`.

---

## Analog / digital mux (CD74HC4067)

Same on both models. Shared select lines for analog + digital muxes:

| Function | GPIO |
|----------|------|
| S0 | GP21 |
| S1 | GP20 |
| S2 | GP19 |
| S3 | GP18 |
| Analog SIG | GP27 |
| Digital bank 1 SIG | GP2 |
| Digital bank 2 SIG | GP16 |
| Digital bank 3 SIG | GP17 |

Digital scan fills `valorMUX1[0..47]` (3 banks × 16 channels). Analog reads fill
`muxAnalogRaw[0..15]` then Kalman-filtered `muxAnalogData[]`.

### Fader / pot array positions (`Controls.h`)

| Control | `muxAnalogData` index |
|---------|----------------------|
| Fader 1–8 | 0–7 |
| Pot 4 | 12 |
| Pot 2 | 13 |
| Pot 1 | 14 |
| Pot 3 | 15 |
| Pot 5 | 11 |
| Pot 6 | 10 |
| Pot 7 / 8 | declared, not assigned defaults |

### Manual mapping (`setControlValues`)

| Flag | Controls → locals |
|------|-------------------|
| `faderRow1ControlManual` | Faders 1–4 → ADSR1 A/D/S/R |
| `faderRow2ControlManual` | Faders 5–8 → ADSR2 **or** ADSR3 (if `ADSR3Enabled`) |
| `VCFPotsControlManual` | Pot 2 → CUTOFF, Pot 3 → RESONANCE, Pot 4 → ADSR2toVCF, Pot 1 → LFO2toVCF |
| `VCAPotsControlManual` | Pot 5 → ADSR1toVCA |
| `PWMPotsControlManual` | Pot 6 → PW |

---

## Encoders (mux channels)

11× `MD_REncoder` via digital mux pin pairs — see `encoders[]` in `encoders.h` (`muxPin1`/`muxPin2`
into `valorMUX1`). Actions are layered (normal / alt / menu / cal) via `EncoderAction`.

Three positions are per-model, wired through the `ENC_OSC3_*` macros in `encoders.h`, which
resolve to the real action when `INPUT_HAS_OSC3_PANEL` and to `ACTION_NONE` otherwise:

| Slot | DCO3 | DCO4 |
|------|------|------|
| enc3 `action3` / `actionAlt2` | OSC3 interval | `ACTION_NONE` |
| enc5 `action3` | OSC3 detune | `ACTION_NONE` |
| enc5 `actionAlt3` | LFO2 → OSC3 | `ACTION_NONE` |

The monosynth exposes a third oscillator on the panel; the 4×2 panel has none to steer.

---

## Buttons

16× `RoxButton` on mux channels — see `buttons.h` / `buttons.ino`. Modes include wave toggles, LFO
select, preset load/save, manual fader/pot enable, function key, calibration UI.

### Wave keys (per model)

The five wave keys are physically the same on both panels and are listed in LED order in
`inputWaveKeys[]` (`board_model.h`), so a key index is also its LED index. They are wired to
different oscillators, which is why `toggle_wave_key()` and `set_LED_Status(16, …)` read the table
rather than naming oscillators:

| Key / LED | Button (mux ch.) | Action | DCO3 target | DCO4 target |
|-----------|------------------|--------|-------------|-------------|
| 0 | button1 (15) | `TG_SAW1` | OSC1 saw | OSC A saw |
| 1 | button2 (12) | `TG_SQR1` | OSC2 pulse | OSC A pulse |
| 2 | button3 (9) | `TG_TRI` | OSC1 triangle | OSC A triangle |
| 3 | button4 (6) | `TG_SAW2` | OSC1 pulse | OSC B saw |
| 4 | button5 (3) | `TG_SQR2` | OSC3 pulse | OSC B pulse |

On the monosynth each oscillator's pulse is switched independently through the DG411, which is why
three of its five keys land on OSC1. `TG_SIN` exists in the action enum but neither panel has a
sine key. The ParamId sent for a key comes from `input_wave_key_param_id(osc, wave)`.

`TG_ADSR3_TO_OSC_SELECT` also differs: the destination cycle wraps at
`INPUT_ADSR3_TO_OSC_SELECT_MAX`, which is 4 on DCO3 (OSC1 / OSC2 / OSC3 plus an "all" position)
and 2 on DCO4 (A / B / A+B).

---

## Manual calibration staging (per model)

Manual calibration walks the oscillators one stage at a time. `INPUT_CAL_STAGE_MAX` bounds the
stage encoder and `INPUT_CAL_STAGE_TO_OSC(stage)` turns a stage into the oscillator index used for
`manualCalibrationInitAmpCompOffset[]`:

| | DCO3 | DCO4 |
|---|------|------|
| Oscillators (`NUM_OSCILLATORS`) | 3 | 8 |
| `INPUT_CAL_STAGES_PER_OSC` | 2 (sawtooth, then pulse) | 1 |
| Stage count / `INPUT_CAL_STAGE_MAX` | 6 stages, max 5 | 8 stages, max 7 |
| `INPUT_CAL_STAGE_TO_OSC(stage)` | `stage / 2` | `stage` |

The monosynth splits each oscillator in two because its waveforms are switched independently in
the analog path. The stage encoder lives in `encoders.ino` (`ACTION_CALIBRATION_STAGE`), the offset
encoder next to it (`ACTION_CALIBRATION_OFFSET`, clamped to ±20), and entry is
`TG_MAN_CALIBRATION` in `buttons.ino`.

---

## LEDs (dual 74HC595)

| Signal | GPIO |
|--------|------|
| DATA | GP11 |
| LATCH | GP12 |
| CLK | GP13 |
| PWM brightness | **GP5** (`PIN_LED_PWM`, DCO3 only) |

`LEDPins[16]` maps logical LEDs to 595 bit indices. LEDs 0–4 follow the wave keys above, 7–11
mirror the manual pot/fader flags. Refresh: `LED_Control_Mux.update()` on Core1 ~31 ms.

**Note:** brightness is not implemented in hardware. On DCO3 `setup1()` parks GP5 at a fixed
`analogWrite(245)` and nothing varies it afterwards; because that call runs after the UART begin,
GP5 stops being `SCREEN_PORT` RX, which costs nothing since the pin has no conductor and the
Screen never transmits. On DCO4 the whole block is compiled out — see the GP5 warning above.

---

## Inactive / dead paths

| Item | Status |
|------|--------|
| `params.ino` apply router | Fully commented |
| `formulas.ino` | Mostly commented |
| `sendSerial()` legacy TX | Empty stub; the body and its flags were deleted |
| USB MIDI / TinyUSB | Includes commented out |
| USB debug prints | `ENABLE_SERIAL` commented out in `Serial.h` |
