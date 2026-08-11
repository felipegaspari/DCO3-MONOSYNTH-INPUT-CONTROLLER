#ifndef SERIAL_INPUT_PROTOCOL_H
#define SERIAL_INPUT_PROTOCOL_H

#include <stdint.h>
#include "sram_hot.h"

// -----------------------------------------------------------------------------
// DCO ↔ Input (and USB bench) inner protocol: command bytes + payload sizes.
//
// Inner frame (what handlers see; unchanged under RAW vs COBS):
//
//   [1 byte] command
//   [N bytes] payload (little-endian multi-byte fields)
//
// On-wire default: identical to the inner frame (SERIAL_FRAMING_RAW).
// On-wire COBS (#define SERIAL_FRAMING_COBS): COBS(inner) + 0x00. See serial_frame.h.
// Host A/B: dco_control --cobs / DCO_SERIAL_COBS=1.
//
// 0x00 is reserved as the COBS delimiter and is never a command.
// -----------------------------------------------------------------------------

enum InputSerialCmd : uint8_t {
  INPUT_CMD_ADSR1_BLOCK  = 'a',  // EnvVCA times
  INPUT_CMD_ADSR2_BLOCK  = 'b',  // EnvVCF times
  INPUT_CMD_ADSR3_BLOCK  = 'c',  // EnvDCO times (maps to ADSR1_* on DCO)
  INPUT_CMD_FILTER_BLOCK = 'd',
  INPUT_CMD_PARAM_16     = 'p',  // id + int16 LE (Input→DCO + DCO→Input persistable mirror)
  INPUT_CMD_PRESET_NAME  = 'q',  // 16 ASCII chars
  INPUT_CMD_PARAM_32     = 'x',  // id + u32 LE (gap 154 / cal 155 DCO→Input)
  INPUT_CMD_PRESET_DIR_REQUEST = 'N',  // Input→DCO only: send me the whole directory (1 pad byte)
  INPUT_CMD_PRESET_DIR_ENTRY   = 'O',  // DCO→Input: one [slot][name:16] entry
  INPUT_CMD_PRESET_LOADED      = 'L',  // DCO→Input: [slot] just finished loading
};

// Payload sizes (NOT counting the command byte).
//
// ADSR ('a'/'b'/'c'): A, D, S, R as uint16 LE.
//   A/D/R are exp-mapped 0..25000; S is linear 0..4095.
static constexpr uint8_t INPUT_SERIAL_LEN_ADSR_BLOCK   = 8;

// Filter ('d'): CUTOFF, RESONANCE, ADSR2toVCF, LFO2toVCF as uint16 LE.
static constexpr uint8_t INPUT_SERIAL_LEN_FILTER_BLOCK = 8;

// Param ('p'): [id:u8][value:i16 LE]
static constexpr uint8_t INPUT_SERIAL_LEN_PARAM_16     = 3;

// Preset name ('q'): 16 ASCII bytes (space-padded).
static constexpr uint8_t INPUT_SERIAL_LEN_PRESET_NAME  = 16;

// Param32 ('x'): [id:u8][value:u32 LE] — DCO→Input gap/cal; Input relays 154 to Screen.
static constexpr uint8_t INPUT_SERIAL_LEN_PARAM_32     = 5;

// Preset directory request ('N'): Input→DCO, 1 unused/padding byte.
// (payload_len==0 is indistinguishable from "unregistered command" in
// serial_parser_dispatch()/process_byte(), so a true 0-byte frame can't be
// used here even though the byte itself carries no information.)
static constexpr uint8_t INPUT_SERIAL_LEN_PRESET_DIR_REQUEST = 1;

// Preset directory entry ('O'): DCO→Input. [slot:u8][name:16 ASCII].
static constexpr uint8_t INPUT_SERIAL_LEN_PRESET_DIR_ENTRY = 17;

// Preset loaded notice ('L'): DCO→Input. [slot:u8].
static constexpr uint8_t INPUT_SERIAL_LEN_PRESET_LOADED    = 1;

static inline INPUT_ALWAYS_INLINE uint8_t serial_input_payload_len(uint8_t cmd) {
  switch (cmd) {
    case INPUT_CMD_ADSR1_BLOCK:
    case INPUT_CMD_ADSR2_BLOCK:
    case INPUT_CMD_ADSR3_BLOCK:  return INPUT_SERIAL_LEN_ADSR_BLOCK;
    case INPUT_CMD_FILTER_BLOCK: return INPUT_SERIAL_LEN_FILTER_BLOCK;
    case INPUT_CMD_PARAM_16:     return INPUT_SERIAL_LEN_PARAM_16;
    case INPUT_CMD_PRESET_NAME:  return INPUT_SERIAL_LEN_PRESET_NAME;
    case INPUT_CMD_PARAM_32:     return INPUT_SERIAL_LEN_PARAM_32;
    case INPUT_CMD_PRESET_DIR_REQUEST: return INPUT_SERIAL_LEN_PRESET_DIR_REQUEST;
    case INPUT_CMD_PRESET_DIR_ENTRY:   return INPUT_SERIAL_LEN_PRESET_DIR_ENTRY;
    case INPUT_CMD_PRESET_LOADED:      return INPUT_SERIAL_LEN_PRESET_LOADED;
    default:                     return 0;
  }
}

#endif // SERIAL_INPUT_PROTOCOL_H
