## Serial & Parameter Protocol – Usage Guide

Shared **inner** serial + ParamId infrastructure. This board is the UART hub: DCO ↔ Input → Screen.

### Input Controller notes (this repo)

- **DCO** (`DCO_PORT` = `Serial1`, TX GP0 / RX GP1): slim LE `'a'`–`'d'`, `'p'` `[id][i16 LE]`, `'q'` 8 chars. Former `'e'`/`'f'` are `'p'` **222** / **210**. Byte UI params go to DCO as `'p'` (u8 zero-extended).
- **Screen** (`SCREEN_PORT` = `Serial2`, TX GP4): slim `'a'`/`'b'` linear faders LE, `'p'` when `sendToAll`, Screen-only `'w'`/`'y'`/`'s'`/`'c'`, `'q'` = preset# + 16 chars.
- Inbound: `serial_read_from_dco()` LUT-drains slim `'x'` (5 B). Gap 154 is relayed as slim `'x'`; cal 155 stored locally. See [`CONTROL_PIPELINE.md`](CONTROL_PIPELINE.md).
- UARTs: IRQ (`setPollingMode(false)`), FIFO 512, 2.5 Mbaud. Manual blocks @ 1 ms; encoder `'p'`/`'w'` immediate on Core0.
- Framing: default RAW. `#define SERIAL_FRAMING_COBS` in `Serial.h` must match DCO/Screen. `SERIAL_INNER_MAX_PAYLOAD` is **17** here (Screen `'q'`). Timeout 500 µs.
- `params.ino` apply-router is **commented out**; this MCU is primarily a sender.
- Panel / pin detail: [`PANEL_AND_PINS.md`](PANEL_AND_PINS.md). Flash DCO + Input + Screen together.

---

## 1. Shared headers

| Header | Role |
|--------|------|
| `params_def.h` | Canonical `enum ParamId` (includes 210 / 222) |
| `param_router.h` | Table-driven apply (unused live on this board) |
| `serial_input_protocol.h` | DCO inner cmds + sizes (`'a'`–`'d'`, `'p'`, `'q'`, `'x'`) |
| `serial_param_protocol.h` | LE encode/decode for `'p'` / `'w'` / `'x'` |
| `serial_frame.h` | Buffer COBS + `serial_frame_write()`. Default RAW |
| `serial_parser.h` | O(1) LUT, 500 µs idle timeout, drain budget 64 |
| `serial_protocol.h` | Stub → `serial_input_protocol.h` |

---

## 2. Using the parameter protocol in a new MCU

### 2.1. Set up `params_def.h` and `param_router.h`

1. **Include the shared headers** in your new project:

   ```cpp
   #include "params_def.h"
   #include "param_router.h"
   ```

2. **Pick the value type** this MCU will use for incoming parameters:

   - DCO‑style: `int16_t`.
   - Mainboard‑style: `int32_t`.

3. **Create a router module** (`params.ino` / `.cpp`) on the new MCU:

   ```cpp
   using ParamValueT     = int32_t;          // or int16_t
   using ParamDescriptor = ParamDescriptorT<ParamValueT>;

   static void apply_param_lfo1_waveform(ParamValueT v) {
     LFO1Waveform = (int8_t)v;
     LFO1_class.setWaveForm(LFO1Waveform);
   }

   static const ParamDescriptor paramTable[] = {
     { PARAM_LFO1_WAVEFORM, apply_param_lfo1_waveform },
   };

   inline void update_parameters(uint16_t rawId, ParamValueT value) {
     param_router_apply<ParamValueT>(
       paramTable,
       sizeof(paramTable) / sizeof(paramTable[0]),
       rawId,
       value
     );
   }
   ```

4. Anywhere in this MCU you can now call `update_parameters(paramId, value)` and let the router dispatch.

---

## 3. Using `'p'` / `'w'` / `'x'` frames

```cpp
#include "serial_param_protocol.h"
#include "serial_input_protocol.h"
#include "serial_frame.h"
#include "serial_parser.h"
```

TX:

```cpp
uint8_t p[INPUT_SERIAL_LEN_PARAM_16];
encode_param_p(p, id, (int16_t)value);
serial_frame_write(port, INPUT_CMD_PARAM_16, p, INPUT_SERIAL_LEN_PARAM_16);
```

RX: `SerialCommandDef[]` → `serial_command_table_init()` → `serial_parser_drain(ctx, lut, port, SERIAL_DRAIN_BYTE_BUDGET)`.

Handlers always see inner cmd + payload (LE, no finish). `'w'` is Screen-only 8-bit UI; DCO only accepts `'p'`.

---

## 4. Adding a serial command (rare)

1. Add cmd + payload length to `serial_input_protocol.h` if it is a DCO-link command.
2. Screen-only cmds stay in the Screen LUT / Input TX helpers (`'w'`/`'y'`/`'s'`/`'c'`, 17-byte `'q'`).
3. Keep `0x00` unused. Prefer LE. Send via `serial_frame_write()`.

---

## 5. Parser notes

- O(1) lookup: `payload_len[cmd]==0` means ignore.
- Timeout (`SERIAL_FRAME_TIMEOUT_US` = 500 µs) only when mid-frame and the stream is idle.
- Drain snapshots `available()` once, then reads up to 64 bytes.

---

## 6. New MCU checklist

1. Copy `params_def.h`, `param_router.h`, `serial_input_protocol.h`, `serial_param_protocol.h`, `serial_frame.h`, `serial_parser.h`.
2. Override `SERIAL_INNER_MAX_PAYLOAD` before include if you need Screen `'q'` (17).
3. Per UART: LUT + `serial_parser_drain()` + `serial_frame_write()` for TX.
4. Keep ParamIds and inner layouts identical across MCUs. Match `SERIAL_FRAMING_COBS` on every peer.
