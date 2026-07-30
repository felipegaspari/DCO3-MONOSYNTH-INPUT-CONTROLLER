# Input Controller — panel and pin map

Hardware mapping for **DCO4_Input_Controller** (RP2040 front panel).

---

## UART

| Port | Pins | Baud | Peer |
|------|------|------|------|
| `Serial` | USB | 2 000 000 | Debug |
| `Serial1` (`DCO_PORT`) | TX **GP0** → DCO GP21, RX **GP1** ← DCO GP20 | 2 500 000 | DCO — panel blocks `'a'`..`'f'`, ParamId frames, 9-byte `'q'` out; `'x'` 154/155 back in |
| `Serial2` (`SCREEN_PORT`) | TX **GP4** → Screen GP13; RX **GP5** unwired | 2 500 000 | Screen — UI frames and the relayed DCO gap, TX only (the Screen never transmits) |

FIFO 512, polling mode. Brought up in `setup1()`.

Port numbers say nothing about the peer, so the code addresses each link through the aliases
`DCO_PORT` (= `Serial1`) and `SCREEN_PORT` (= `Serial2`) declared in `Serial.h`. `Serial1` is the
two-way DCO link, on the pair the archived STM32 Mainboard used to occupy; the wires were
re-terminated at the DCO, so no pin on this board changed.

---

## Analog / digital mux (CD74HC4067)

Shared select lines for analog + digital muxes:

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

Digital scan fills `valorMUX1[0..47]` (3 banks × 16 channels). Analog reads fill `muxAnalogRaw[0..15]` then Kalman-filtered `muxAnalogData[]`.

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
| `VCFPotsControlManual` | Pots → CUTOFF, RESONANCE, ADSR2toVCF, LFO2toVCF |
| `VCAPotsControlManual` | Pot5 → ADSR1toVCA |
| `PWMPotsControlManual` | Pot6 → PW |

---

## Encoders (mux channels)

11× `MD_REncoder` via digital mux pin pairs — see `encoders[]` in `encoders.h` (`muxPin1`/`muxPin2` into `valorMUX1`). Actions are layered (normal / alt / menu / cal) via `EncoderAction`.

---

## Buttons

16× `RoxButton` on mux channels — see `buttons.h` / `buttons.ino`. Modes include wave toggles, LFO select, preset load/save, manual fader/pot enable, function key, calibration UI.

---

## LEDs (dual 74HC595)

| Signal | GPIO |
|--------|------|
| DATA | GP11 |
| LATCH | GP12 |
| CLK | GP13 |
| PWM brightness | **GP5** (`PIN_LED_PWM`) |

`LEDPins[16]` maps logical LEDs to 595 bit indices. Refresh: `LED_Control_Mux.update()` on Core1 ~31 ms.

**Note:** brightness is not implemented in hardware. `setup1()` parks GP5 at a fixed
`analogWrite(245)` and nothing varies it afterwards. Because that call runs after `Serial2.begin()`,
GP5 stops being `Serial2` RX — which costs nothing, since the pin has no conductor and the Screen
never transmits.

---

## Inactive / dead paths

| Item | Status |
|------|--------|
| `params.ino` apply router | Fully commented |
| `formulas.ino` | Mostly commented |
| `sendSerial()` legacy TX | Not called (`loop1` site commented) |
| USB MIDI / TinyUSB | Includes commented out |
