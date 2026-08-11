// Screen UI mode signal ('s' + byte).
void __not_in_flash_func(serial_send_signal)(byte signal) {
#ifdef ENABLE_SCREEN_LINK
  serial_frame_write(SCREEN_PORT, (uint8_t)'s', &signal, 1);
#endif
}

// Send slim 'p' (id + i16 LE) to the DCO, and to the Screen when sendToAll.
void __not_in_flash_func(serial_send_param_change)(byte param, uint16_t paramValue, bool sendToAll) {
  uint8_t payload[INPUT_SERIAL_LEN_PARAM_16];
  encode_param_p(payload, param, (int16_t)paramValue);
#ifdef ENABLE_SCREEN_LINK
  if (sendToAll) {
    serial_frame_write(SCREEN_PORT, INPUT_CMD_PARAM_16, payload, INPUT_SERIAL_LEN_PARAM_16);
  }
#endif
#ifdef ENABLE_DCO_LINK
  if (paramValue != (uint16_t)-1) {
    serial_frame_write(DCO_PORT, INPUT_CMD_PARAM_16, payload, INPUT_SERIAL_LEN_PARAM_16);
  }
#endif
}

// Screen keeps slim 'w' [id][u8]; DCO gets 'p' (i16 zero-extended). 255 = screen-only.
void __not_in_flash_func(serial_send_param_change_byte)(byte param, byte paramValue, bool sendToAll) {
#ifdef ENABLE_SCREEN_LINK
  if (sendToAll) {
    uint8_t w[SERIAL_LEN_PARAM_8];
    encode_param_w(w, param, paramValue);
    serial_frame_write(SCREEN_PORT, (uint8_t)'w', w, SERIAL_LEN_PARAM_8);
  }
#endif
#ifdef ENABLE_DCO_LINK
  if (paramValue != (byte)-1) {
    uint8_t p[INPUT_SERIAL_LEN_PARAM_16];
    encode_param_p(p, param, (int16_t)paramValue);
    serial_frame_write(DCO_PORT, INPUT_CMD_PARAM_16, p, INPUT_SERIAL_LEN_PARAM_16);
  }
#endif
}

// Send preset name (16 chars) to the DCO via 'q'.
void serial_send_preset_name_to_mainboard() {
#ifdef ENABLE_DCO_LINK
  serial_frame_write(DCO_PORT, INPUT_CMD_PRESET_NAME, presetNameVal, INPUT_SERIAL_LEN_PRESET_NAME);
#endif
}

// Send preset scroll (number + 16-char name) to the Screen via 'q'.
void serial_send_preset_scroll(byte presetNumber, byte presetNameSerial[]) {
#ifdef ENABLE_SCREEN_LINK
  uint8_t payload[17];
  payload[0] = presetNumber;
  for (uint8_t i = 0; i < 16; ++i) {
    payload[1 + i] = presetNameSerial[i];
  }
  serial_frame_write(SCREEN_PORT, (uint8_t)'q', payload, 17);
#endif
}

// Send save-name character position to the Screen via 'c'.
void serial_send_save_char_select(byte serialPresetChar) {
#ifdef ENABLE_SCREEN_LINK
  serial_frame_write(SCREEN_PORT, (uint8_t)'c', &serialPresetChar, 1);
#endif
}

// Send 'y' byte param to the Screen.
void __not_in_flash_func(serialSendParamByteToScreen)(byte paramNumber, byte paramValue)
{
#ifdef ENABLE_SCREEN_LINK
  uint8_t payload[2] = { paramNumber, paramValue };
  serial_frame_write(SCREEN_PORT, (uint8_t)'y', payload, 2);
#endif
}

// ---------------------------------------------------------------------------
// Parser-based receiver for DCO 'x' / persistable 'p' on DCO_PORT RX
// (GP1 <- DCO GP20).
// ---------------------------------------------------------------------------

static void __not_in_flash_func(serial_forward_param32_to_screen)(const uint8_t* payload, uint8_t len) {
#ifdef ENABLE_SCREEN_LINK
  if (len != INPUT_SERIAL_LEN_PARAM_32) {
    return;
  }
  serial_frame_write(SCREEN_PORT, INPUT_CMD_PARAM_32, payload, len);
#else
  (void)payload;
  (void)len;
#endif
}

// Relay persistable DCO→Input 'p' mirror to Screen toasts (wire i16, not locals).
static void serial_forward_param16_to_screen(const uint8_t* payload, uint8_t len) {
#ifdef ENABLE_SCREEN_LINK
  if (len != INPUT_SERIAL_LEN_PARAM_16) {
    return;
  }
  serial_frame_write(SCREEN_PORT, INPUT_CMD_PARAM_16, payload, len);
#else
  (void)payload;
  (void)len;
#endif
}

// Handle 32-bit PARAM ('x') from the DCO:
//   154 PARAM_GAP_FROM_DCO → forward the same 'x' on to the Screen
//   155 PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO → store + optional 'y' echo
//   value (uint32) lower 16 bits for 155 = [oscIndex:8 | offset:8]
static void __not_in_flash_func(input_handle_param32_from_dco)(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PARAM_32) {
    return;
  }

  ParamFrame frame;
  decode_param_x(payload, frame);

  if (frame.id == (uint8_t)PARAM_GAP_FROM_DCO) {
    serial_forward_param32_to_screen(payload, len);
    return;
  }

  if (frame.id != (uint8_t)PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO) {
    return;
  }

  uint16_t packed  = (uint16_t)frame.value;
  uint8_t  oscIndex = (uint8_t)(packed >> 8);
  int8_t   offset   = (int8_t)(packed & 0xFF);

  if (oscIndex < NUM_OSCILLATORS) {
    manualCalibrationInitAmpCompOffset[oscIndex] = offset;
    if (manualCalibration) {
      uint8_t currentIndex = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
      if (oscIndex == currentIndex) {
        serialSendParamByteToScreen(
          ParamId::PARAM_MANUAL_CALIBRATION_OFFSET,
          (uint8_t)manualCalibrationInitAmpCompOffset[currentIndex]
        );
      }
    }
  }
}

// DCO→Input persistable 'p' mirror (USB/MIDI/dco_control, and now DCO-side
// preset loads too). Update the in-RAM synth-state locals below (no LittleFS
// involved — Input has none), forward wire 'p' to Screen toasts. Never re-TX
// to DCO (loop / Aux).
static void input_handle_param16_from_dco(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PARAM_16) {
    return;
  }

  ParamFrame frame;
  decode_param_p(payload, frame);
  const uint8_t id = frame.id;
  const int16_t v  = (int16_t)frame.value;

  if (id >= (uint8_t)ParamId::PARAM_MOD_SLOT0_SOURCE &&
      id <= (uint8_t)ParamId::PARAM_MOD_SLOT7_DEPTH) {
    const uint8_t slot  = (uint8_t)((id - (uint8_t)ParamId::PARAM_MOD_SLOT0_SOURCE) / 3);
    const uint8_t field = (uint8_t)((id - (uint8_t)ParamId::PARAM_MOD_SLOT0_SOURCE) % 3);
    if (slot < MOD_SLOT_COUNT_INPUT) {
      if (field == 0) {
        modSlotSource[slot] = (uint8_t)v;
      } else if (field == 1) {
        modSlotDest[slot] = (uint8_t)v;
      } else {
        modSlotDepth[slot] = v;
      }
    }
  } else {
    switch (id) {
      case ParamId::PARAM_OSC1_SAW_ENABLE:    waveEnable[0][0] = (v != 0); break;
      case ParamId::PARAM_OSC1_PULSE_ENABLE:  waveEnable[0][1] = (v != 0); break;
      case ParamId::PARAM_OSC1_TRI_ENABLE:    waveEnable[0][2] = (v != 0); break;
      case ParamId::PARAM_OSC2_SAW_ENABLE:    waveEnable[1][0] = (v != 0); break;
      case ParamId::PARAM_OSC2_PULSE_ENABLE:  waveEnable[1][1] = (v != 0); break;
      case ParamId::PARAM_OSC2_TRI_ENABLE:    waveEnable[1][2] = (v != 0); break;
      case ParamId::PARAM_OSC3_SAW_ENABLE:    waveEnable[2][0] = (v != 0); break;
      case ParamId::PARAM_OSC3_PULSE_ENABLE:  waveEnable[2][1] = (v != 0); break;
      case ParamId::PARAM_OSC3_TRI_ENABLE:    waveEnable[2][2] = (v != 0); break;

      case ParamId::PARAM_RESONANCE_COMPENSATION: RESONANCEAmpCompensation = (v != 0); break;
      case ParamId::PARAM_VCA_ADSR_RESTART:       VCAADSRRestart = (v != 0); break;
      case ParamId::PARAM_VCF_ADSR_RESTART:       VCFADSRRestart = (v != 0); break;
      case ParamId::PARAM_ADSR3_ENABLED:          ADSR3Enabled = (v != 0); break;

      case ParamId::PARAM_ADSR3_TO_OSC_SELECT:
        ADSR3ToOscSelect = (int8_t)constrain(v, 0, INPUT_ADSR3_TO_OSC_SELECT_MAX);
        break;

      case ParamId::PARAM_LFO1_WAVEFORM: LFO1Waveform = (int8_t)v; break;
      case ParamId::PARAM_LFO2_WAVEFORM: LFO2Waveform = (int8_t)v; break;

      case ParamId::PARAM_OSC1_INTERVAL: OSC1Interval = (int8_t)v; break;
      case ParamId::PARAM_OSC2_INTERVAL: OSC2Interval = (int8_t)v; break;
      case ParamId::PARAM_OSC3_INTERVAL: OSC3Interval = (int8_t)v; break;

      case ParamId::PARAM_OSC2_DETUNE_VAL: OSC2Detune = v; break;
      case ParamId::PARAM_OSC3_DETUNE_VAL: OSC3Detune = v; break;
      case ParamId::PARAM_LFO2_TO_OSC2:    LFO2toOSC2DETUNE = v; break;
      case ParamId::PARAM_LFO2_TO_OSC3:    LFO2toOSC3DETUNE = v; break;

      case ParamId::PARAM_OSC_SYNC_MODE:   oscSyncMode = (uint16_t)v; break;
      case ParamId::PARAM_PORTAMENTO_TIME: portamentoTime = v; break;
      case ParamId::PARAM_PORTAMENTO_MODE: portamentoMode = (byte)v; break;
      case ParamId::PARAM_VOICE_MODE:      voiceMode = (byte)constrain(v, 0, 2); break;
      case ParamId::PARAM_UNISON_DETUNE:   unisonDetune = v; break;
      case ParamId::PARAM_SYNC_MODE:       syncMode = (byte)v; break;
      case ParamId::PARAM_SOFT_SYNC:       softSync = (uint8_t)v; break;
      case ParamId::PARAM_SUBOSC_DIVIDE:   subOscDivide = (uint8_t)v; break;
      case ParamId::PARAM_FILTER_MODE:     filterMode = (uint8_t)v; break;

      case ParamId::PARAM_ANALOG_DRIFT_AMOUNT: analogDrift = v; break;
      case ParamId::PARAM_ANALOG_DRIFT_SPEED:  analogDriftSpeed = v; break;
      case ParamId::PARAM_ANALOG_DRIFT_SPREAD: analogDriftSpread = v; break;

      case ParamId::PARAM_VCF_KEYTRACK:     VCFKeytrack = v; break;
      case ParamId::PARAM_VELOCITY_TO_VCF:  velocityToVCF = (int8_t)v; break;
      case ParamId::PARAM_VELOCITY_TO_VCA:  velocityToVCA = (int8_t)v; break;

      case ParamId::PARAM_OSC1_LEVEL: OSC1Level = v; break;
      case ParamId::PARAM_OSC2_LEVEL: OSC2Level = v; break;
      case ParamId::PARAM_SUB_LEVEL:  SubLevel = v; break;
      case ParamId::PARAM_OSC3_LEVEL: OSC3Level = v; break;

      case ParamId::PARAM_LFO1_TO_DCO: LFO1toDCO = v; break;
      case ParamId::PARAM_LFO1_SPEED:  LFO1Speed = v; break;
      case ParamId::PARAM_LFO2_SPEED:  LFO2Speed = v; break;
      case ParamId::PARAM_VCA_LEVEL:   VCALevel = v; break;
      case ParamId::PARAM_LFO1_TO_VCA: LFO1toVCA = v; break;
      case ParamId::PARAM_LFO2_TO_PW:  LFO2toPWM = v; break;
      case ParamId::PARAM_ADSR3_TO_PWM:
        ADSR3toPWM = (int16_t)constrain((int32_t)v - 512, -512, 511);
        break;
      case ParamId::PARAM_ADSR3_TO_DETUNE1: ADSR3toDETUNE1 = v; break;

      case ParamId::PARAM_ADSR1_ATTACK_CURVE: ADSR1AttackCurveVal = (int8_t)v; break;
      case ParamId::PARAM_ADSR1_DECAY_CURVE:  ADSR1DecayCurveVal = (int8_t)v; break;
      case ParamId::PARAM_ADSR2_ATTACK_CURVE: ADSR2AttackCurveVal = (int8_t)v; break;
      case ParamId::PARAM_ADSR2_DECAY_CURVE:  ADSR2DecayCurveVal = (int8_t)v; break;

      case ParamId::PARAM_DIST_DRIVE: distDrive = (uint16_t)v; break;
      case ParamId::PARAM_DIST_MIX:   distMix = (uint16_t)v; break;

      case ParamId::PARAM_PW_VALUE:     PW = (uint16_t)v; break;
      case ParamId::PARAM_ADSR1_TO_VCA: ADSR1toVCA = v; break;

      case ParamId::PARAM_LFO1_TO_OSC1:
        LFO1toOSC1 = (uint8_t)constrain(v, 0, 255);
        break;
      case ParamId::PARAM_LFO1_TO_OSC2:
        LFO1toOSC2 = (uint8_t)constrain(v, 0, 255);
        break;
      case ParamId::PARAM_LFO1_TO_OSC3:
        LFO1toOSC3 = (uint8_t)constrain(v, 0, 255);
        break;
      case ParamId::PARAM_LFO2_TO_OSC2_COARSE:
        LFO2toOSC2_coarse = (uint16_t)constrain(v, 0, 511);
        break;
      case ParamId::PARAM_LFO2_TO_OSC3_COARSE:
        LFO2toOSC3_coarse = (uint16_t)constrain(v, 0, 511);
        break;
      case ParamId::PARAM_CHARACTER:
        characterAmount = (uint8_t)constrain(v, 0, 128);
        break;
      case ParamId::PARAM_ADSR3_PITCH_MODE:
        env_dco_pitch_centered = (v != 0) ? 1 : 0;
        break;

      default:
        break;
    }
  }

  serial_forward_param16_to_screen(payload, len);
}

// 'd' filter block echoed back after a preset recall (and, on DCO4, after any
// Mainboard-side filter change). Track the locals so the pots pick up from the
// recalled values, and pass the frame on for the Screen's filter display.
static void input_handle_filter_block_from_dco(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_FILTER_BLOCK) return;

  CUTOFF     = decode_u16_le(payload + 0);
  RESONANCE  = decode_u16_le(payload + 2);
  ADSR2toVCF = decode_i16_le(payload + 4);
  LFO2toVCF  = decode_u16_le(payload + 6);

#ifdef ENABLE_SCREEN_LINK
  serial_frame_write(SCREEN_PORT, INPUT_CMD_FILTER_BLOCK, payload, INPUT_SERIAL_LEN_FILTER_BLOCK);
#endif
}

static const SerialCommandDef dcoLinkCommands[] = {
  { INPUT_CMD_PARAM_16, INPUT_SERIAL_LEN_PARAM_16, input_handle_param16_from_dco },
  { INPUT_CMD_PARAM_32, INPUT_SERIAL_LEN_PARAM_32, input_handle_param32_from_dco },
  { INPUT_CMD_FILTER_BLOCK, INPUT_SERIAL_LEN_FILTER_BLOCK, input_handle_filter_block_from_dco },
  { INPUT_CMD_PRESET_DIR_ENTRY, INPUT_SERIAL_LEN_PRESET_DIR_ENTRY, input_handle_preset_dir_entry },
  { INPUT_CMD_PRESET_LOADED,    INPUT_SERIAL_LEN_PRESET_LOADED,    input_handle_preset_loaded    },
};

static SerialCommandTable dcoLinkLut;
static SerialParserContext dcoLinkParser = {};

void init_dco_link_parser() {
  serial_command_table_init(
    dcoLinkLut,
    dcoLinkCommands,
    sizeof(dcoLinkCommands) / sizeof(dcoLinkCommands[0])
  );
}

void __not_in_flash_func(serial_read_from_dco)() {
#ifdef ENABLE_DCO_LINK
  serial_parser_drain(dcoLinkParser, dcoLinkLut, DCO_PORT, SERIAL_DRAIN_BYTE_BUDGET);
#endif
}
