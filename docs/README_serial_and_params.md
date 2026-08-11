## Serial & Parameter Protocol – Input Controller

The wire format, the command table, the parser and the `ParamId` enum are shared
by every board and documented once in
[`DCO-PROTOCOL/README.md`](../../DCO-PROTOCOL/README.md). The headers come from
that library through `_build_libs/DCO-PROTOCOL`; there is no copy in this sketch
folder any more.

This page covers only what is specific to the Input Controller, which is the UART
hub: DCO ↔ Input → Screen.

---

## Links

- **DCO** (`DCO_PORT` = `Serial1`, TX GP0 / RX GP1). On DCO4 this port faces the
  Mainboard, which relays to the DCO. Outbound: slim LE `'a'`–`'d'`, `'p'`
  `[id][i16 LE]`, `'q'` 16 chars. The former `'e'`/`'f'` commands are now `'p'`
  ids **222** / **210**. Byte-sized UI params go to the DCO as `'p'`, zero-extended.
- **Screen** (`SCREEN_PORT` = `Serial2`, TX GP4). Slim `'a'`/`'b'` linear faders
  LE, `'p'` when `sendToAll`, the Screen-only `'w'`/`'y'`/`'s'`/`'c'`, and `'q'`
  as preset number + 16 chars (17 bytes).
- **Inbound**: `serial_read_from_dco()` LUT-drains slim `'x'` (5 B), the
  persistable `'p'` mirror (3 B), and the `'O'`/`'L'` preset directory frames.
  Gap 154 is relayed on to the Screen as slim `'x'`; cal 155 is stored locally;
  `'p'` writes in-RAM synth locals and forwards to Screen toasts without
  re-transmitting to the DCO. See [`CONTROL_PIPELINE.md`](CONTROL_PIPELINE.md).

Preset directory sync (the DCO is the only preset store, see
[`PRESETS.md`](PRESETS.md)): Input sends `'N'` with one unused byte, the DCO
answers with 256 × `'O'` (`[slot][name:16]`), plus `'L'` (`[slot]`) after every
load.

## Board-specific settings

- `SERIAL_INNER_MAX_PAYLOAD` is **17** here, sized for the Screen `'q'` scroll and
  the inbound `'O'`; the DCO uses 36.
- Framing defaults to RAW; `#define SERIAL_FRAMING_COBS` in `Serial.h` must match
  the DCO and Screen. Flash DCO + Input + Screen together.
- UARTs are IRQ-driven (`setPollingMode(false)`), FIFO 512, 2.5 Mbaud. Manual
  blocks go out every 1 ms; encoder `'p'`/`'w'` are sent immediately on Core 0.
- This board pins the serial hot path in SRAM: `sram_hot.h` defines
  `INPUT_ALWAYS_INLINE`, and `Serial.h` includes it before the protocol headers,
  so the shared code is decorated here and nowhere else.
- The `params.ino` apply-router is commented out; this MCU is primarily a sender.
- Panel and pin detail: [`PANEL_AND_PINS.md`](PANEL_AND_PINS.md).

## Sending a parameter from this board

```cpp
uint8_t p[INPUT_SERIAL_LEN_PARAM_16];
encode_param_p(p, id, (int16_t)value);
serial_frame_write(port, INPUT_CMD_PARAM_16, p, INPUT_SERIAL_LEN_PARAM_16);
```

Receiving is the usual `SerialCommandDef[]` → `serial_command_table_init()` →
`serial_parser_drain(ctx, lut, port, SERIAL_DRAIN_BYTE_BUDGET)`. `'w'` is the
Screen-only 8-bit UI frame; the DCO only accepts `'p'`.

Screen-only commands (`'w'`, `'y'`, `'s'`, `'c'` and the 17-byte `'q'`) stay in
the Screen LUT and the Input TX helpers rather than the shared header, since they
never cross the DCO link.
