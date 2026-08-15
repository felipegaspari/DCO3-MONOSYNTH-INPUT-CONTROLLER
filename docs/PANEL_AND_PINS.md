# Input Controller — panel and pin map

Hardware mapping for the **INPUT-CONTROLLER** sketch (RP2040 front panel). One source tree serves
both instruments: the mux, analog, digital and LED pins below are identical on the two PCBs, but
the **UART assignment**, the **GP5 LED-PWM question**, the **wave-key wiring** and the
**calibration staging** are per-model and come from `board_model.h`, selected by
`INPUT_BOARD_MODEL`.

---

## UART

The two PCBs swap which UART faces which peer, so the peer can **never** be inferred from the port
number. The code names `DCO_PORT` (outbound), `DCO_RX_PORT` (inbound), and `SCREEN_PORT` from
`Serial.h`. On DCO3 those first two are the same UART. On DCO4 `DCO_PORT` is Serial2 TX to the
Mainboard and `DCO_RX_PORT` is Serial1 GP1 from the Mainboard (same UART as Screen TX).

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
| `Serial2` | `DCO_PORT` | TX **GP4** → Mainboard PE0; RX unused | 2 500 000 | STM32 Mainboard outbound (`'a'`–`'d'` / `'p'` / `'q'` / `'N'`) |
| `Serial1` | `SCREEN_PORT` / `DCO_RX_PORT` | TX **GP0** → Screen GP13; RX **GP1** ← Mainboard PE1 | 2 500 000 | Screen TX, and Mainboard inbound (`'p'`/`'x'`/`'a'`–`'d'`/`'O'`/`'L'`) |

Inbound is **GP1**, not GP5. `INPUT_HAS_LED_PWM` stays 0; do not drive GP5 as PWM.

Both links run at 2 500 000 baud on either model, with FIFO 512 and IRQ mode
(`setPollingMode(false)`). They are brought up in `setup1()` from `INPUT_DCO_RX_PIN`,
`INPUT_DCO_TX_PIN`, `INPUT_SCREEN_RX_PIN` and `INPUT_SCREEN_TX_PIN`.

---

## MCU module (`DCO_MCU_BOARD`)

Same flag as the DCO, from the superproject `project_config.h`. Panel mux/UART pins do **not**
use GPIO 23 or 24 (mux **channel** 23 on button9 is not GPIO 23). Analog SIG is GP27.

| `DCO_MCU_BOARD` | GP23 | GP24 |
|-----------------|------|------|
| WeAct RP2040 | Onboard KEY, `INPUT_PULLUP`. Press toggles FUNC (`PARAM_FUNCTION_KEY`) | Unused (DCO analog board-fix only) |
| Pico / Pico 2 | `SMPS_PS_PIN` OUT HIGH (RT6150 PWM; quieter 3.3 V for the GP27 ADC) | VBUS sense — not driven |

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

DCO3 walks **saw → pulse → 440 Hz** on every oscillator (`stage = osc × 3 + sub`, 9 stages, max 8).
DCO4 packs **7 stages per voice pair** (28 stages, max 27): A saw / tri / pulse-PW / 440, B saw / pulse / 440.
`INPUT_CAL_STAGE_MAX` bounds the stage encoder; `INPUT_CAL_STAGE_TO_OSC(stage)` (`cal_stage_to_osc_n`)
turns a stage into the oscillator index used for offsets / `manualAmpComp440[]` / `manualPwCenter[]`:

| | DCO3 | DCO4 |
|---|------|------|
| Oscillators (`NUM_OSCILLATORS`) | 3 | 8 |
| Stage walk | uniform 3 | packed A4+B3 |
| Stage count / `INPUT_CAL_STAGE_MAX` | 9 stages, max 8 | 28 stages, max 27 |
| `INPUT_CAL_STAGE_TO_OSC(stage)` | `cal_stage_to_osc_n` | packed A4+B3 |

The stage encoder lives in `encoders.ino` (`ACTION_CALIBRATION_STAGE`) and calls
`input_send_manual_cal_stage()`. The offset encoder next to it (`ACTION_CALIBRATION_OFFSET`) sends
param **153** (offset ±20) on saw/tri/pulse, **162** (PW_CENTER, 0..1023) on DCO4 A's pulse-PW
substage, and param **159** (amp @ 440) on 440 Hz substages. Entry is `TG_MAN_CALIBRATION` in `buttons.ino`.

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
Screen never transmits. On DCO4 inbound is GP1 and the PWM block stays compiled out.

---

## Inactive / dead paths

| Item | Status |
|------|--------|
| `params.ino` apply router | Fully commented |
| `formulas.ino` | Mostly commented |
| `sendSerial()` legacy TX | Empty stub; the body and its flags were deleted |
| USB MIDI / TinyUSB | Includes commented out |
| USB debug prints | `ENABLE_SERIAL` commented out in `Serial.h` |
