#define DCO_PROTOCOL_IMPLEMENT_DMA
#include "include_all.h"

UartDmaTx DcoDma    = { 0 };
UartDmaTx ScreenDma = { 1 };

void input_handle_preset_dir_entry(char cmd, const uint8_t* payload, uint8_t len);
void input_handle_preset_loaded(char, const uint8_t* payload, uint8_t);

// =============================================================================
// 1. DMA Initialization & Polling
// =============================================================================

void serial_dma_init() {
#if INPUT_IS_DCO3
  serial_dma_init_rp2040(0, uart0);
  serial_dma_init_rp2040(1, uart1);
#else
  serial_dma_init_rp2040(0, uart1);
  serial_dma_init_rp2040(1, uart0);
#endif
}

void serial_dma_poll() {
  serial_dma_poll_one(0);
  serial_dma_poll_one(1);
}

// =============================================================================
// 2. Outgoing Senders
// =============================================================================

static inline INPUT_ALWAYS_INLINE void pack_u16_le4(uint8_t* dst, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  encode_u16_le(dst + 0, a);
  encode_u16_le(dst + 2, b);
  encode_u16_le(dst + 4, c);
  encode_u16_le(dst + 6, d);
}

void __not_in_flash_func(serial_send_signal)(byte signal) {
#ifdef ENABLE_SCREEN_LINK
  serial_frame_write(ScreenDma, CMD_SCREEN_SIGNAL, &signal, SERIAL_LEN_SCREEN_SIGNAL);
#endif
}

void __not_in_flash_func(serial_send_param_change)(byte param, uint16_t paramValue, bool sendToAll) {
  uint8_t payload[SERIAL_LEN_PARAM_16];
  encode_param_p(payload, param, (int16_t)paramValue);

#ifdef ENABLE_SCREEN_LINK
  if (sendToAll) {
    serial_frame_write(ScreenDma, CMD_PARAM_16, payload, SERIAL_LEN_PARAM_16);
  }
#endif
#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, CMD_PARAM_16, payload, SERIAL_LEN_PARAM_16);
#endif
}

void __not_in_flash_func(serial_send_param_change_byte)(byte param, byte paramValue, bool sendToAll) {
  serial_send_param_change(param, (uint16_t)paramValue, sendToAll);
}

void serial_send_preset_name_to_mainboard() {
#ifdef ENABLE_DCO_LINK
  serial_frame_write(DcoDma, CMD_PRESET_NAME, presetNameVal, SERIAL_LEN_PRESET_NAME);
#endif
}

void serial_send_preset_scroll(byte presetNumber, byte presetNameSerial[]) {
#ifdef ENABLE_SCREEN_LINK
  uint8_t payload[SERIAL_LEN_SCREEN_PRESET_SCROLL];
  payload[0] = presetNumber;
  for (uint8_t i = 0; i < 16; ++i) {
    payload[1 + i] = presetNameSerial[i];
  }
  serial_frame_write(ScreenDma, CMD_PRESET_NAME, payload, SERIAL_LEN_SCREEN_PRESET_SCROLL);
#endif
}

void serial_send_save_char_select(byte serialPresetChar) {
#ifdef ENABLE_SCREEN_LINK
  serial_frame_write(ScreenDma, CMD_CHAR_SELECT, &serialPresetChar, SERIAL_LEN_CHAR_SELECT);
#endif
}

static void input_send_param16_to_screen(uint8_t id, int16_t value) {
#ifdef ENABLE_SCREEN_LINK
  uint8_t payload[SERIAL_LEN_PARAM_16];
  encode_param_p(payload, id, value);
  serial_frame_write(ScreenDma, CMD_PARAM_16, payload, SERIAL_LEN_PARAM_16);
#else
  (void)id; (void)value;
#endif
}

void __not_in_flash_func(serialSendParamByteToScreen)(byte paramNumber, byte paramValue) {
  input_send_param16_to_screen(paramNumber, (int16_t)(int8_t)paramValue);
}

void input_send_manual_cal_stage() {
  const uint8_t stage = (uint8_t)manualCalibrationStage;
  const uint8_t osc   = INPUT_CAL_STAGE_TO_OSC(stage);
  const uint8_t ch    = INPUT_CAL_PW_CH(osc);

  serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_STAGE, stage, false);
  serialSendParamByteToScreen(ParamId::PARAM_MANUAL_CALIBRATION_STAGE, stage);

  if (INPUT_CAL_STAGE_IS_440(stage)) {
    calibrationVal = (int16_t)manualAmpComp440[osc];
    input_send_param16_to_screen(ParamId::PARAM_AMP_COMP_440, (int16_t)manualAmpComp440[osc]);
  } else if (INPUT_CAL_STAGE_IS_PW_EDIT(stage)) {
    uint8_t validCh = (ch < NUM_VOICES) ? ch : (NUM_VOICES - 1);
    calibrationVal = (int16_t)manualPwCenter[validCh];
    input_send_param16_to_screen(ParamId::PARAM_CAL_PW_CENTER, (int16_t)manualPwCenter[validCh]);
  } else {
    calibrationVal = (int16_t)manualCalibrationInitAmpCompOffset[osc];
    serialSendParamByteToScreen(ParamId::PARAM_MANUAL_CALIBRATION_OFFSET, (uint8_t)manualCalibrationInitAmpCompOffset[osc]);
  }
}

// Live continuous hardware control streaming
void __not_in_flash_func(serial_send_manual_controls)(bool presetLoading) {
  // 1. ADSR 1 Faders
  if (faderRow1ControlManual || presetLoading) {
    // Exponential block -> Mainboard / DCO
    uint8_t dataArrayDCO[SERIAL_LEN_ADSR_BLOCK];
    pack_u16_le4(dataArrayDCO, linToExpLookup[ADSR1_attack], linToExpLookup[ADSR1_decay], ADSR1_sustain, linToExpLookup[ADSR1_release]);
#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, CMD_ADSR1_BLOCK, dataArrayDCO, SERIAL_LEN_ADSR_BLOCK);
#endif

    // Linear bar graph block -> Direct to Screen
#ifdef ENABLE_SCREEN_LINK
    uint8_t dataArrayScreen[SERIAL_LEN_ADSR_BLOCK];
    pack_u16_le4(dataArrayScreen, ADSR1_attack, ADSR1_decay, ADSR1_sustain, ADSR1_release);
    serial_frame_write(ScreenDma, CMD_ADSR1_BLOCK, dataArrayScreen, SERIAL_LEN_ADSR_BLOCK);
#endif
  }

  // 2. ADSR 2 / 3 Faders
  if ((faderRow2ControlManual && !ADSR3Enabled) || presetLoading) {
    // Exponential block -> Mainboard / DCO
    uint8_t dataArrayDCO[SERIAL_LEN_ADSR_BLOCK];
    pack_u16_le4(dataArrayDCO, linToExpLookup[ADSR2_attack], linToExpLookup[ADSR2_decay], ADSR2_sustain, linToExpLookup[ADSR2_release]);
#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, CMD_ADSR2_BLOCK, dataArrayDCO, SERIAL_LEN_ADSR_BLOCK);
#endif

    // Linear bar graph block -> Direct to Screen
#ifdef ENABLE_SCREEN_LINK
    uint8_t dataArrayScreen[SERIAL_LEN_ADSR_BLOCK];
    pack_u16_le4(dataArrayScreen, ADSR2_attack, ADSR2_decay, ADSR2_sustain, ADSR2_release);
    serial_frame_write(ScreenDma, CMD_ADSR2_BLOCK, dataArrayScreen, SERIAL_LEN_ADSR_BLOCK);
#endif
  } else if ((faderRow2ControlManual && ADSR3Enabled) || presetLoading) {
    uint8_t dataArray[SERIAL_LEN_ADSR_BLOCK];
    pack_u16_le4(dataArray, linToExpLookup[ADSR3_attack], linToExpLookup[ADSR3_decay], ADSR3_sustain, linToExpLookup[ADSR3_release]);
#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, CMD_ADSR3_BLOCK, dataArray, SERIAL_LEN_ADSR_BLOCK);
#endif
  }

  // 3. VCF Filter Pots -> Mainboard / DCO
  if (VCFPotsControlManual || presetLoading) {
    uint8_t dataArray[SERIAL_LEN_FILTER_BLOCK];
    pack_u16_le4(dataArray, CUTOFF, RESONANCE, (uint16_t)ADSR2toVCF, LFO2toVCF);
#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, CMD_FILTER_BLOCK, dataArray, SERIAL_LEN_FILTER_BLOCK);
#endif
  }

  // 4. VCA Live Pot -> Mainboard / DCO
  if (VCAPotsControlManual || presetLoading) {
#ifdef ENABLE_DCO_LINK
    uint8_t p[SERIAL_LEN_PARAM_16];
    encode_param_p(p, (uint8_t)ParamId::PARAM_ADSR1_TO_VCA, (int16_t)ADSR1toVCA);
    serial_frame_write(DcoDma, CMD_PARAM_16, p, SERIAL_LEN_PARAM_16);
#endif
  }

  // 5. PWM Live Pot -> DCO Engine
  if (PWMPotsControlManual || presetLoading) {
#ifdef ENABLE_DCO_LINK
    uint8_t p[SERIAL_LEN_PARAM_16];
    encode_param_p(p, (uint8_t)ParamId::PARAM_PW_VALUE, (int16_t)PW);
    serial_frame_write(DcoDma, CMD_PARAM_16, p, SERIAL_LEN_PARAM_16);
#endif
  }
}

void sendSerial() {}

// =============================================================================
// 3. Ingress Handlers (From Mainboard / DCO Link)
// =============================================================================

static void __not_in_flash_func(input_handle_param32_from_dco)(char, const uint8_t* payload, uint8_t len) {
  ParamFrame frame;
  decode_param_x(payload, frame);

  // 154: Forward real-time gap readout to Screen
  if (frame.id == (uint8_t)PARAM_GAP_FROM_DCO) {
#ifdef ENABLE_SCREEN_LINK
    serial_frame_write(ScreenDma, CMD_PARAM_32, payload, len);
#endif
    return;
  }

  // 155: Manual Offsets ([osc:8 | offset:8])
  if (frame.id == (uint8_t)PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO) {
    uint8_t oscIndex = (uint8_t)(frame.value >> 8);
    int8_t  offset   = (int8_t)(frame.value & 0xFF);
    if (oscIndex < NUM_OSCILLATORS) {
      manualCalibrationInitAmpCompOffset[oscIndex] = offset;
      
      // If this oscillator is currently active, sync encoder state and push to screen
      uint8_t currentOsc = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
      if (oscIndex == currentOsc && !INPUT_CAL_STAGE_IS_440(manualCalibrationStage) && !INPUT_CAL_STAGE_IS_PW_EDIT(manualCalibrationStage)) {
        calibrationVal = (int16_t)offset;
        serialSendParamByteToScreen(ParamId::PARAM_MANUAL_CALIBRATION_OFFSET, (uint8_t)offset);
      }
    }
    return;
  }

  // 159: 440 Hz Amp Comp ([osc:8 | amp440:16])
  if (frame.id == (uint8_t)PARAM_AMP_COMP_440) {
    uint8_t  oscIndex = (uint8_t)(frame.value >> 16);
    uint16_t ampVal   = (uint16_t)(frame.value & 0xFFFF);
    if (oscIndex < NUM_OSCILLATORS) {
      manualAmpComp440[oscIndex] = ampVal;
      
      uint8_t currentOsc = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
      if (oscIndex == currentOsc && INPUT_CAL_STAGE_IS_440(manualCalibrationStage)) {
        calibrationVal = (int16_t)ampVal;
        input_send_param16_to_screen(ParamId::PARAM_AMP_COMP_440, (int16_t)ampVal);
      }
    }
    return;
  }

  // 162: PW Centers ([channel:8 | pwCenter:16])
  if (frame.id == (uint8_t)PARAM_CAL_PW_CENTER) {
    uint8_t  chIndex = (uint8_t)(frame.value >> 16);
    uint16_t pwVal   = (uint16_t)(frame.value & 0xFFFF);
    if (chIndex < NUM_VOICES) {
      manualPwCenter[chIndex] = pwVal;
      
      uint8_t currentOsc = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
      uint8_t currentCh  = INPUT_CAL_PW_CH(currentOsc);
      if (chIndex == currentCh && INPUT_CAL_STAGE_IS_PW_EDIT(manualCalibrationStage)) {
        calibrationVal = (int16_t)pwVal;
        input_send_param16_to_screen(ParamId::PARAM_CAL_PW_CENTER, (int16_t)pwVal);
      }
    }
    return;
  }

  // 161: Scope 50% Duty Trim ([osc:8 | dutyOffset:16])
  if (frame.id == (uint8_t)PARAM_AMP_COMP_DUTY_OFFSET) {
    uint8_t oscIndex = (uint8_t)(frame.value >> 16);
    int16_t dutyVal  = (int16_t)(frame.value & 0xFFFF);
    if (oscIndex < NUM_OSCILLATORS) {
      ampCompDutyOffset[oscIndex] = dutyVal; // Writes to own dedicated array!
    }
    return;
  }
}

static void input_handle_param16_from_dco(char, const uint8_t* payload, uint8_t) {
  ParamFrame frame;
  decode_param_p(payload, frame);
  update_parameters((uint8_t)frame.id, (int16_t)frame.value);
}

static void input_apply_adsr_block_from_dco(const uint8_t* payload, uint16_t& attack, uint16_t& decay, uint16_t& sustain, uint16_t& release) {
  attack  = exp_to_lin_index(decode_u16_le(payload + 0));
  decay   = exp_to_lin_index(decode_u16_le(payload + 2));
  sustain = decode_u16_le(payload + 4);
  release = exp_to_lin_index(decode_u16_le(payload + 6));
}

static void input_handle_adsr1_from_dco(char, const uint8_t* payload, uint8_t) {
  input_apply_adsr_block_from_dco(payload, ADSR1_attack, ADSR1_decay, ADSR1_sustain, ADSR1_release);
}

static void input_handle_adsr2_from_dco(char, const uint8_t* payload, uint8_t) {
  input_apply_adsr_block_from_dco(payload, ADSR2_attack, ADSR2_decay, ADSR2_sustain, ADSR2_release);
}

static void input_handle_adsr3_from_dco(char, const uint8_t* payload, uint8_t) {
  input_apply_adsr_block_from_dco(payload, ADSR3_attack, ADSR3_decay, ADSR3_sustain, ADSR3_release);
}

static void input_handle_filter_block_from_dco(char, const uint8_t* payload, uint8_t) {
  CUTOFF     = decode_u16_le(payload + 0);
  RESONANCE  = decode_u16_le(payload + 2);
  ADSR2toVCF = decode_i16_le(payload + 4);
  LFO2toVCF  = (int16_t)decode_u16_le(payload + 6);
}

// Inbound Preset Name Ingress (17 bytes: [slot:u8][name:16 ASCII] or 16 bytes raw)
static void input_handle_preset_name_from_dco(char, const uint8_t* payload, uint8_t len) {
  if (len == SERIAL_LEN_SCREEN_PRESET_SCROLL) {
    const uint8_t slot = payload[0];
    if (slot < PRESET_NUM_SLOTS) {
      currentPreset = slot;
      presetSelectVal = slot;
      memcpy(presetDir[slot], payload + 1, 16);
      memcpy(presetName, payload + 1, 16);
      memcpy(presetNameVal, payload + 1, 16);
      presetNameString = String((char*)presetName);
      ledRefreshPending = true;
    }
  } else if (len == SERIAL_LEN_PRESET_NAME) {
    memcpy(presetName, payload, 16);
    memcpy(presetNameVal, payload, 16);
    if (currentPreset < PRESET_NUM_SLOTS) {
      memcpy(presetDir[currentPreset], payload, 16);
    }
    presetNameString = String((char*)presetName);
    ledRefreshPending = true;
  }
}

// =============================================================================
// Domain Block Ingress from DCO (All Routed via update_parameters)
// =============================================================================

// 1. Oscillator & Voice Configuration Block ('v')
static void input_handle_patch_osc_block_from_dco(char, const uint8_t* payload, uint8_t) {
  const PatchOscBlock* blk = (const PatchOscBlock*)payload;

  // Wave buttons/LEDs (Bitmask unpacking)
  update_parameters(PARAM_OSC1_SAW_ENABLE,   (blk->wave_enables & (1u << 0)) != 0);
  update_parameters(PARAM_OSC1_PULSE_ENABLE, (blk->wave_enables & (1u << 1)) != 0);
  update_parameters(PARAM_OSC1_TRI_ENABLE,   (blk->wave_enables & (1u << 2)) != 0);
  update_parameters(PARAM_OSC2_SAW_ENABLE,   (blk->wave_enables & (1u << 3)) != 0);
  update_parameters(PARAM_OSC2_PULSE_ENABLE, (blk->wave_enables & (1u << 4)) != 0);
  update_parameters(PARAM_OSC2_TRI_ENABLE,   (blk->wave_enables & (1u << 5)) != 0);
  update_parameters(PARAM_OSC3_SAW_ENABLE,   (blk->wave_enables & (1u << 6)) != 0);
  update_parameters(PARAM_OSC3_PULSE_ENABLE, (blk->wave_enables & (1u << 7)) != 0);
  update_parameters(PARAM_OSC3_TRI_ENABLE,   (blk->wave_enables & (1u << 8)) != 0);

  // Intervals, Detunes & Modes
  update_parameters(PARAM_OSC1_INTERVAL,       blk->osc1_interval);
  update_parameters(PARAM_OSC2_INTERVAL,       blk->osc2_interval);
  update_parameters(PARAM_OSC3_INTERVAL,       blk->osc3_interval);
  update_parameters(PARAM_OSC2_DETUNE_VAL,     blk->osc2_detune);
  update_parameters(PARAM_UNISON_DETUNE,       blk->unison_detune);
  update_parameters(PARAM_VOICE_MODE,          blk->voice_mode);
  update_parameters(PARAM_VOICE_ALLOC_MODE,    blk->voice_alloc_mode);
  update_parameters(PARAM_SYNC_MODE,           blk->sync_mode);
  update_parameters(PARAM_SOFT_SYNC,           blk->soft_sync);
  update_parameters(PARAM_SUBOSC_DIVIDE,       blk->subosc_divide);
  update_parameters(PARAM_ANALOG_DRIFT_AMOUNT, blk->analog_drift);
  update_parameters(PARAM_ANALOG_DRIFT_SPEED,  blk->analog_drift_speed);
  update_parameters(PARAM_ANALOG_DRIFT_SPREAD, blk->analog_drift_spread);
  update_parameters(PARAM_PORTAMENTO_TIME,     blk->portamento_time);
  update_parameters(PARAM_PORTAMENTO_MODE,     blk->portamento_mode);
  update_parameters(PARAM_CHARACTER,           blk->character);

  ledRefreshPending = true; // Triggers immediate physical LED update on panel
}

// 2. LFO & Modulation Block ('l')
static void input_handle_patch_lfo_block_from_dco(char, const uint8_t* payload, uint8_t) {
  const PatchLfoBlock* blk = (const PatchLfoBlock*)payload;

  update_parameters(PARAM_LFO1_WAVEFORM,        blk->lfo1_waveform);
  update_parameters(PARAM_LFO2_WAVEFORM,        blk->lfo2_waveform);
  update_parameters(PARAM_LFO1_SPEED,           blk->lfo1_speed);
  update_parameters(PARAM_LFO2_SPEED,           blk->lfo2_speed);
  update_parameters(PARAM_LFO1_TO_DCO,          blk->lfo1_to_dco);
  update_parameters(PARAM_LFO1_TO_OSC1,         blk->lfo1_to_osc1);
  update_parameters(PARAM_LFO1_TO_OSC2,         blk->lfo1_to_osc2);
  update_parameters(PARAM_LFO1_TO_OSC3,         blk->lfo1_to_osc3);
  update_parameters(PARAM_LFO2_TO_OSC2,         blk->lfo2_to_osc2);
  update_parameters(PARAM_LFO2_TO_OSC3,         blk->lfo2_to_osc3);
  update_parameters(PARAM_LFO2_TO_OSC2_COARSE,  blk->lfo2_to_osc2_coarse);
  update_parameters(PARAM_LFO2_TO_OSC3_COARSE,  blk->lfo2_to_osc3_coarse);
  update_parameters(PARAM_LFO2_TO_PW,           blk->lfo2_to_pw);
  update_parameters(PARAM_LFO1_TO_VCA,          blk->lfo1_to_vca);
  update_parameters(PARAM_PW_VALUE,             blk->pw_value);
  update_parameters(PARAM_ADSR1_TO_VCA,         blk->adsr1_to_vca);
  update_parameters(PARAM_ADSR3_TO_PWM,         blk->adsr3_to_pwm); // Math handled in apply_param
  update_parameters(PARAM_ADSR3_TO_DETUNE1,     blk->adsr3_to_detune1);
  update_parameters(PARAM_ADSR3_PITCH_MODE,     blk->adsr3_pitch_mode);
  update_parameters(PARAM_ADSR3_TO_OSC_SELECT,  blk->adsr3_to_osc_select);
}

// 3. Mod Matrix Block ('M')
static void input_handle_patch_mod_block_from_dco(char, const uint8_t* payload, uint8_t) {
  const PatchModBlock* blk = (const PatchModBlock*)payload;

  for (uint8_t i = 0; i < 8; i++) {
    update_parameters(PARAM_MOD_SLOT0_SOURCE + i * 3, blk->slots[i].src);
    update_parameters(PARAM_MOD_SLOT0_DEST   + i * 3, blk->slots[i].dest);
    update_parameters(PARAM_MOD_SLOT0_DEPTH  + i * 3, blk->slots[i].depth);
  }
}

// 4. Mixer, Curves, VCA & Filter Modes Block ('X')
static void input_handle_patch_mix_block_from_dco(char, const uint8_t* payload, uint8_t) {
  const PatchMixBlock* blk = (const PatchMixBlock*)payload;

  update_parameters(PARAM_OSC1_LEVEL,          blk->osc1_level);
  update_parameters(PARAM_OSC2_LEVEL,          blk->osc2_level);
  update_parameters(PARAM_OSC3_LEVEL,          blk->osc3_level);
  update_parameters(PARAM_SUB_LEVEL,           blk->sub_level);
  update_parameters(PARAM_VCA_LEVEL,           blk->vca_level);
  update_parameters(PARAM_FILTER_MODE,         blk->filter_mode);
  update_parameters(PARAM_VELOCITY_TO_VCF,     blk->velocity_to_vcf);
  update_parameters(PARAM_VELOCITY_TO_VCA,     blk->velocity_to_vca);
  update_parameters(PARAM_VCF_KEYTRACK,        blk->vcf_keytrack);
  update_parameters(PARAM_ADSR1_TO_VCA,        blk->adsr1_to_vca);
  update_parameters(PARAM_DIST_DRIVE,          blk->dist_drive);
  update_parameters(PARAM_DIST_MIX,            blk->dist_mix);
  update_parameters(PARAM_ADSR1_ATTACK_CURVE,  blk->adsr1_attack_curve);
  update_parameters(PARAM_ADSR1_DECAY_CURVE,   blk->adsr1_decay_curve);
  update_parameters(PARAM_ADSR2_ATTACK_CURVE,  blk->adsr2_attack_curve);
  update_parameters(PARAM_ADSR2_DECAY_CURVE,   blk->adsr2_decay_curve);

  // Boolean flags
  update_parameters(PARAM_RESONANCE_COMPENSATION, (blk->misc_flags & (1 << 0)) != 0);
  update_parameters(PARAM_VCA_ADSR_RESTART,       (blk->misc_flags & (1 << 1)) != 0);
  update_parameters(PARAM_VCF_ADSR_RESTART,       (blk->misc_flags & (1 << 2)) != 0);
}

// =============================================================================
// 4. Ingress Table & Parsing
// =============================================================================

static const SerialCommandDef dcoLinkCommands[] = {
  { CMD_PARAM_16,         SERIAL_LEN_PARAM_16,             input_handle_param16_from_dco },
  { CMD_PARAM_32,         SERIAL_LEN_PARAM_32,             input_handle_param32_from_dco },
  { CMD_ADSR1_BLOCK,      SERIAL_LEN_ADSR_BLOCK,           input_handle_adsr1_from_dco },
  { CMD_ADSR2_BLOCK,      SERIAL_LEN_ADSR_BLOCK,           input_handle_adsr2_from_dco },
  { CMD_ADSR3_BLOCK,      SERIAL_LEN_ADSR_BLOCK,           input_handle_adsr3_from_dco },
  { CMD_FILTER_BLOCK,     SERIAL_LEN_FILTER_BLOCK,         input_handle_filter_block_from_dco },
  { CMD_PRESET_DIR_ENTRY, SERIAL_LEN_PRESET_DIR_ENTRY,     input_handle_preset_dir_entry },
  { CMD_PRESET_LOADED,    SERIAL_LEN_PRESET_LOADED,        input_handle_preset_loaded },
  { CMD_PRESET_NAME,      SERIAL_LEN_SCREEN_PRESET_SCROLL, input_handle_preset_name_from_dco },
  { CMD_BLOCK_OSC,        SERIAL_LEN_BLOCK_OSC,            input_handle_patch_osc_block_from_dco },
  { CMD_BLOCK_LFO,        SERIAL_LEN_BLOCK_LFO,            input_handle_patch_lfo_block_from_dco },
  { CMD_BLOCK_MOD,        SERIAL_LEN_BLOCK_MOD,            input_handle_patch_mod_block_from_dco },
  { CMD_BLOCK_MIX,        SERIAL_LEN_BLOCK_MIX,            input_handle_patch_mix_block_from_dco },
};

static SerialCommandTable dcoLinkLut;
static SerialParserContext dcoLinkParser = {};

void init_dco_link_parser() {
  serial_parser_reset(dcoLinkParser);
  serial_command_table_init(
    dcoLinkLut,
    dcoLinkCommands,
    sizeof(dcoLinkCommands) / sizeof(dcoLinkCommands[0])
  );
}

void __not_in_flash_func(serial_read_from_dco)() {
#ifdef ENABLE_DCO_LINK
  while (DCO_RX_PORT.available() > 0) {
    serial_parser_drain(dcoLinkParser, dcoLinkLut, DCO_RX_PORT, 255);
  }
#endif
}

void input_disable_all_manual_controls() {
  faderRow1ControlManual = false;
  faderRow2ControlManual = false;
  VCFPotsControlManual   = false;
  VCAPotsControlManual   = false;
  PWMPotsControlManual   = false;
#ifdef ALL_CONTROLS_MANUAL
  allControlsManual      = false;
#endif
  ledRefreshPending      = true;
}