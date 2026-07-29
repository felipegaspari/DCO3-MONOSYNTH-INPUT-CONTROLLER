# System overview (Input Controller pointer)

This board is the **input / front-panel controller** (RP2040): muxed **faders, pots, encoders, buttons**, **status LEDs**, **LittleFS preset storage**, UART to the **DCO hub**, and UI/gap fan-out to the **screen**.

Canonical three-board overview: **[`../../DCO/docs/SYSTEM_OVERVIEW.md`](../../DCO/docs/SYSTEM_OVERVIEW.md)**.

### This board’s UARTs (verified in firmware)

| Port | Pins (RX / TX) | Baud | Peer |
|------|----------------|------|------|
| `Serial` | USB | 2 000 000 | Debug |
| `Serial1` | GP1 / GP0 | 2 500 000 | Screen (UI TX + forwarded gap `'x'` 154) |
| `Serial2` | GP5 / GP4 | 2 500 000 | DCO hub (panel TX; RX `'x'` 154/155) |

**LED PWM** is **GP6** (`PIN_LED_PWM`) so GP5 stays free for Serial2 RX. See [`PANEL_AND_PINS.md`](PANEL_AND_PINS.md).

Board-specific detail: [`CONTROL_PIPELINE.md`](CONTROL_PIPELINE.md), [`PRESETS.md`](PRESETS.md), [`REFERENCE_AI.md`](REFERENCE_AI.md).
