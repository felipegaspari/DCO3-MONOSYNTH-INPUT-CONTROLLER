# Input Controller — presets (LittleFS)

Preset ownership for DCO4 lives on the **Input Controller** (the old Mainboard preset store is archived).

---

## Storage

| Item | Value |
|------|--------|
| FS | LittleFS |
| File | `"presetBank1"` |
| Presets | `NUM_PRESETS` = **256** |
| Slot size | `flashPresetSize` = **180** bytes |
| Legacy slot | `LEGACY_FLASH_PRESET_SIZE` = **140** (format version 0) |
| Format version | `flashData[2]` = **1** on new saves (`PRESET_FORMAT_VERSION`) |
| Bank size | 256 × 180 |
| RAM | Full bank in `presetBank1Buffer[]`; working slot in `flashData[180]` |

`initFS()` creates the file if missing. If an existing bank is still **256 × 140**, it expands each slot in RAM (copy 140, zero-pad to 180) and rewrites the file. Old patch bytes `0..139` are preserved; the new tail defaults until the preset is re-saved at version 1.

---

## Slot layout (format version 1)

Treat [`loadPreset`](../presetStorage.ino) / [`writePreset`](../presetStorage.ino) as source of truth. Summary:

| Bytes | Contents |
|-------|----------|
| 0..1 | Flag bits (OSC1 waves, ADSR restarts, `ADSR3Enabled`, …) |
| 2 | Format version |
| 3 | OSC2/3 wave enable bits |
| 4..134 | Classic scalars (intervals, levels, LFOs, ADSRs, name at 119..134) |
| 135..139 | Unused padding |
| 140 | `filterMode` (`PARAM_FILTER_MODE`) |
| 141 | `softSync` (`PARAM_SOFT_SYNC`) |
| 142 | `subOscDivide` (`PARAM_SUBOSC_DIVIDE`) |
| 143 | `portamentoMode` (`PARAM_PORTAMENTO_MODE`) |
| 144..145 | `distDrive` (`PARAM_DIST_DRIVE`) |
| 146..147 | `distMix` (`PARAM_DIST_MIX`) |
| 148..179 | Mod matrix slots 0..7 — 4 bytes each: src, dest, depth_hi, depth_lo (`0xFF` = empty) |

Version **0** slots (including freshly migrated pads) load the v1 tail as **defaults**: empty mod matrix, dist 0, modes 0 — they do not interpret zero-padded bytes as active mod routes.

Session UI flags (PWM/VCA/VCF pot manual, fader rows) are **not** recalled as sound; PWM pots manual is always TX’d 0 on load.

---

## Name field

Name characters are at offsets **119..134** (16 bytes packed; UI helpers often use the first 12). Screen `'q'` scroll uses 16 bytes.

Helpers: `load_preset_name`, `get_preset_name`.

---

## Load / save flow

1. **Select** — encoder/button UI sets `presetSelectVal`; Screen gets `'q'` scroll updates.
2. **Load** — `loadPreset(n)` copies slot → `flashData`, unpacks into locals/flags, then re-TX’s ParamIds + `serial_send_manual_controls(true)` over serial.
3. **Save** — UI gathers name via char select (`'c'` / `'s'` signals); `writePreset` packs `flashData` (version 1) and writes through the RAM bank back to LittleFS.

---

## Related flags

`presetSelect`, `presetSaveSelectMode`, `presetSaveMode`, `presetSaved`, `funcKeyOn` — see `Controls.h` and button actions in `buttons.ino`.
