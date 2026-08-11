# System overview (Input Controller pointer)

This board is the **input / front-panel controller** (RP2040): muxed **faders, pots, encoders, buttons**, **status LEDs**, a **RAM-only preset name cache** (preset storage itself lives on the DCO — no LittleFS on this board), a UART to the voice side, and UI/gap fan-out to the **screen**.

The same firmware runs the panel of both instruments; [`../board_model.h`](../board_model.h) selects which. The two differ in what sits on the other end of the panel link:

- **DCO3-MONOSYNTH** — Input talks straight to the DCO. Canonical overview: [`../../DCO/docs/SYSTEM_OVERVIEW.md`](../../DCO/docs/SYSTEM_OVERVIEW.md).
- **DCO4-REBORN** — Input talks to the STM32 Mainboard, which owns the analog envelopes and filter CVs and relays preset traffic on to the DCO. Canonical overview: [`../../DCO/docs/SYSTEM_OVERVIEW.md`](../../DCO/docs/SYSTEM_OVERVIEW.md); relay contract: [`../../MAINBOARD-CONTROLLER/docs/MAINBOARD_REINTEGRATION.md`](../../MAINBOARD-CONTROLLER/docs/MAINBOARD_REINTEGRATION.md).

### This board's UARTs (verified in firmware)

The port numbers are swapped between the two PCBs, so nothing in the code infers the peer from the UART number — it addresses both links through the `DCO_PORT` / `SCREEN_PORT` aliases that `Serial.h` takes from `board_model.h`.

**DCO3-MONOSYNTH**

| Port | Pins (RX / TX) | Baud | Peer |
|------|----------------|------|------|
| `Serial` | USB | 2 000 000 | Debug (`ENABLE_SERIAL`, off by default) |
| `Serial1` (`DCO_PORT`) | GP1 / GP0 | 2 500 000 | DCO |
| `Serial2` (`SCREEN_PORT`) | GP5 / GP4 | 2 500 000 | Screen, TX only (GP5 unwired) |

**DCO4-REBORN**

| Port | Pins (RX / TX) | Baud | Peer |
|------|----------------|------|------|
| `Serial` | USB | 2 000 000 | Debug (`ENABLE_SERIAL`, off by default) |
| `Serial2` (`DCO_PORT`) | GP5 / GP4 | 2 500 000 | Mainboard (PE1 / PE0), which relays to the DCO |
| `Serial1` (`SCREEN_PORT`) | GP1 / GP0 | 2 500 000 | Screen, TX only (GP1 unused) |

Outbound on `DCO_PORT`: panel blocks `'a'`–`'d'`, params `'p'`, the 16-char preset name `'q'`, ParamIds 170/171 for preset save/load, and the `'N'` directory request. Inbound: `'x'` 154 (gap) and 155 (cal offset), the persistable `'p'` mirror, the `'d'` filter block, and the `'O'` / `'L'` preset directory frames.

**LED PWM** is **GP5** and exists on **DCO3 only** (`INPUT_HAS_LED_PWM`). It is harmless there — brightness is not implemented in hardware and GP5 has no conductor. On DCO4 that same pad is the Mainboard RX line and must never be driven. See [`PANEL_AND_PINS.md`](PANEL_AND_PINS.md).

Board-specific detail: [`CONTROL_PIPELINE.md`](CONTROL_PIPELINE.md), [`PRESETS.md`](PRESETS.md), [`REFERENCE_AI.md`](REFERENCE_AI.md).
