# System overview (Input Controller pointer)

This board is the **input / front-panel controller** (RP2040): muxed **faders, pots, encoders, buttons**, **status LEDs**, **LittleFS preset storage**, UART to the **DCO hub**, and UI/gap fan-out to the **screen**.

Canonical three-board overview: **[`../../DCO/docs/SYSTEM_OVERVIEW.md`](../../DCO/docs/SYSTEM_OVERVIEW.md)**.

### This board’s UARTs (verified in firmware)

| Port | Pins (RX / TX) | Baud | Peer |
|------|----------------|------|------|
| `Serial` | USB | 2 000 000 | Debug |
| `Serial1` (`DCO_PORT`) | GP1 / GP0 | 2 500 000 | DCO hub (panel + param TX on GP0; RX `'x'` 154/155 on GP1) |
| `Serial2` (`SCREEN_PORT`) | GP5 / GP4 | 2 500 000 | Screen (UI TX + forwarded gap `'x'` 154 on GP4; GP5 unwired) |

Port numbers say nothing about the peer, so the code addresses both links through the `DCO_PORT` / `SCREEN_PORT` aliases in `Serial.h`.

**LED PWM** is **GP5** (`PIN_LED_PWM`). Harmless: brightness is not implemented in hardware and GP5 has no conductor. See [`PANEL_AND_PINS.md`](PANEL_AND_PINS.md).

Board-specific detail: [`CONTROL_PIPELINE.md`](CONTROL_PIPELINE.md), [`PRESETS.md`](PRESETS.md), [`REFERENCE_AI.md`](REFERENCE_AI.md).
