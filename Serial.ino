#define DCO_PROTOCOL_IMPLEMENT_DMA
#include "include_all.h"

UartDmaTx DcoDma    = { 0 };
UartDmaTx ScreenDma = { 1 };

// Forward declarations from presetStorage.ino
void input_handle_preset_dir_entry(char cmd, const uint8_t* payload, uint8_t len);
void input_handle_preset_loaded(char, const uint8_t* payload, uint8_t);

// =============================================================================
// 1. DMA Initialization & Polling (RP2040)
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
// 2. Outgoing Senders (Panel -> DCO / Mainboard / Screen)
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
  if (paramValue != (uint16_t)-1) {
    serial_frame_write(DcoDma, CMD_PARAM_16, payload, SERIAL_LEN_PARAM_16);
  }
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
  (void)id;
  (void)value;
#endif
}

void __not_in_flash_func(serialSendParamByteToScreen)(byte paramNumber, byte paramValue) {
  input_send_param16_to_screen(paramNumber, (int16_t)(int8_t)paramValue);
}

void input_send_manual_cal_stage() {
  const uint8_t stage = (uint8_t)manualCalibrationStage;
  const uint8_t osc   = INPUT_CAL_STAGE_TO_OSC(stage);
  serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_STAGE, stage, /*sendToAll=*/false);
  serialSendParamByteToScreen(ParamId::PARAM_MANUAL_CALIBRATION_STAGE, stage);
  if (INPUT_CAL_STAGE_IS_440(stage)) {
    if (manualAmpComp440[osc] != 0) {
      input_send_param16_to_screen(ParamId::PARAM_AMP_COMP_440, (int16_t)manualAmpComp440[osc]);
    }
  } else if (INPUT_CAL_STAGE_IS_PW_EDIT(stage)) {
    uint8_t ch = INPUT_CAL_PW_CH(osc);
    if (ch >= NUM_VOICES) ch = NUM_VOICES - 1;
    if (manualPwCenter[ch] != 0) {
      input_send_param16_to_screen(ParamId::PARAM_CAL_PW_CENTER, (int16_t)manualPwCenter[ch]);
    }
  } else {
    serialSendParamByteToScreen(ParamId::PARAM_MANUAL_CALIBRATION_OFFSET, (uint8_t)manualCalibrationInitAmpCompOffset[osc]);
  }
}

// Full 1ms live stream to DCO/Mainboard; screen updates on change only to protect display FPS
void __not_in_flash_func(serial_send_manual_controls)(bool presetLoading) {
  // 1. ADSR 1 Faders
  if (faderRow1ControlManual || presetLoading) {
    uint8_t dataArrayDCO[SERIAL_LEN_ADSR_BLOCK];
    pack_u16_le4(dataArrayDCO, linToExpLookup[ADSR1_attack], linToExpLookup[ADSR1_decay], ADSR1_sustain, linToExpLookup[ADSR1_release]);

#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, CMD_ADSR1_BLOCK, dataArrayDCO, SERIAL_LEN_ADSR_BLOCK);
#endif

#ifdef ENABLE_SCREEN_LINK
    static uint16_t scr_a1_a = 0xFFFF, scr_a1_d = 0xFFFF, scr_a1_s = 0xFFFF, scr_a1_r = 0xFFFF;
    if (presetLoading || ADSR1_attack != scr_a1_a || ADSR1_decay != scr_a1_d || 
        ADSR1_sustain != scr_a1_s || ADSR1_release != scr_a1_r) {
      scr_a1_a = ADSR1_attack; scr_a1_d = ADSR1_decay;
      scr_a1_s = ADSR1_sustain; scr_a1_r = ADSR1_release;

      uint8_t dataArrayScreen[SERIAL_LEN_ADSR_BLOCK];
      pack_u16_le4(dataArrayScreen, ADSR1_attack, ADSR1_decay, ADSR1_sustain, ADSR1_release);
      serial_frame_write(ScreenDma, CMD_ADSR1_BLOCK, dataArrayScreen, SERIAL_LEN_ADSR_BLOCK);
    }
#endif
  }

  // 2. ADSR 2 / 3 Faders
  if ((faderRow2ControlManual && !ADSR3Enabled) || presetLoading) {
    uint8_t dataArrayDCO[SERIAL_LEN_ADSR_BLOCK];
    pack_u16_le4(dataArrayDCO, linToExpLookup[ADSR2_attack], linToExpLookup[ADSR2_decay], ADSR2_sustain, linToExpLookup[ADSR2_release]);

#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, CMD_ADSR2_BLOCK, dataArrayDCO, SERIAL_LEN_ADSR_BLOCK);
#endif

#ifdef ENABLE_SCREEN_LINK
    static uint16_t scr_a2_a = 0xFFFF, scr_a2_d = 0xFFFF, scr_a2_s = 0xFFFF, scr_a2_r = 0xFFFF;
    if (presetLoading || ADSR2_attack != scr_a2_a || ADSR2_decay != scr_a2_d || 
        ADSR2_sustain != scr_a2_s || ADSR2_release != scr_a2_r) {
      scr_a2_a = ADSR2_attack; scr_a2_d = ADSR2_decay;
      scr_a2_s = ADSR2_sustain; scr_a2_r = ADSR2_release;

      uint8_t dataArrayScreen[SERIAL_LEN_ADSR_BLOCK];
      pack_u16_le4(dataArrayScreen, ADSR2_attack, ADSR2_decay, ADSR2_sustain, ADSR2_release);
      serial_frame_write(ScreenDma, CMD_ADSR2_BLOCK, dataArrayScreen, SERIAL_LEN_ADSR_BLOCK);
    }
#endif
  } else if ((faderRow2ControlManual && ADSR3Enabled) || presetLoading) {
    uint8_t dataArray[SERIAL_LEN_ADSR_BLOCK];
    pack_u16_le4(dataArray, linToExpLookup[ADSR3_attack], linToExpLookup[ADSR3_decay], ADSR3_sustain, linToExpLookup[ADSR3_release]);
#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, CMD_ADSR3_BLOCK, dataArray, SERIAL_LEN_ADSR_BLOCK);
#endif
  }

  // 3. VCF Filter Pots
  if (VCFPotsControlManual || presetLoading) {
    uint8_t dataArray[SERIAL_LEN_FILTER_BLOCK];
    pack_u16_le4(dataArray, CUTOFF, RESONANCE, (uint16_t)ADSR2toVCF, LFO2toVCF);
#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, CMD_FILTER_BLOCK, dataArray, SERIAL_LEN_FILTER_BLOCK);
#endif
  }

  // 4. VCA Pots
  if (VCAPotsControlManual || presetLoading) {
#ifdef ENABLE_DCO_LINK
    uint8_t p[SERIAL_LEN_PARAM_16];
    encode_param_p(p, (uint8_t)ParamId::PARAM_ADSR1_TO_VCA, (int16_t)ADSR1toVCA);
    serial_frame_write(DcoDma, CMD_PARAM_16, p, SERIAL_LEN_PARAM_16);
#endif
  }

  // 5. PWM Pots
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
// 3. Ingress Handlers (DCO / Mainboard -> Input Controller)
// =============================================================================

#define INPUT_RELAYS_DCO_TO_SCREEN INPUT_IS_DCO3

static void __not_in_flash_func(serial_forward_param32_to_screen)(const uint8_t* payload, uint8_t len) {
#if defined(ENABLE_SCREEN_LINK) && INPUT_RELAYS_DCO_TO_SCREEN
  serial_frame_write(ScreenDma, CMD_PARAM_32, payload, len);
#else
  (void)payload;
  (void)len;
#endif
}

static void serial_forward_param16_to_screen(const uint8_t* payload, uint8_t len) {
#if defined(ENABLE_SCREEN_LINK) && INPUT_RELAYS_DCO_TO_SCREEN
  serial_frame_write(ScreenDma, CMD_PARAM_16, payload, len);
#else
  (void)payload;
  (void)len;
#endif
}

static void serial_forward_cal_amp_pw_to_screen_dco4(const uint8_t* payload, uint8_t len) {
#if defined(ENABLE_SCREEN_LINK) && !INPUT_RELAYS_DCO_TO_SCREEN
  serial_frame_write(ScreenDma, CMD_PARAM_16, payload, len);
#else
  (void)payload;
  (void)len;
#endif
}

static void __not_in_flash_func(input_handle_param32_from_dco)(char, const uint8_t* payload, uint8_t len) {
  ParamFrame frame;
  decode_param_x(payload, frame);

  if (frame.id == (uint8_t)PARAM_GAP_FROM_DCO) {
    serial_forward_param32_to_screen(payload, len);
    return;
  }

  if (frame.id == (uint8_t)PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO) {
    uint16_t packed   = (uint16_t)frame.value;
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
}

static void input_handle_param16_from_dco(char, const uint8_t* payload, uint8_t len) {
  ParamFrame frame;
  decode_param_p(payload, frame);

  // Dispatch to O(1) jump table in params.ino
  update_parameters((uint8_t)frame.id, (int16_t)frame.value);

  // Forward manual cal trims to screen on DCO4
  if (frame.id == (uint8_t)ParamId::PARAM_AMP_COMP_440 || frame.id == (uint8_t)ParamId::PARAM_CAL_PW_CENTER) {
    serial_forward_cal_amp_pw_to_screen_dco4(payload, len);
  }
  serial_forward_param16_to_screen(payload, len);
}

static void input_apply_adsr_block_from_dco(const uint8_t* payload, uint16_t& attack, uint16_t& decay, uint16_t& sustain, uint16_t& release) {
  attack  = exp_to_lin_index(decode_u16_le(payload + 0));
  decay   = exp_to_lin_index(decode_u16_le(payload + 2));
  sustain = decode_u16_le(payload + 4);
  release = exp_to_lin_index(decode_u16_le(payload + 6));
}

static void input_forward_adsr_block_to_screen(uint8_t cmd, uint16_t attack, uint16_t decay, uint16_t sustain, uint16_t release) {
#if defined(ENABLE_SCREEN_LINK) && INPUT_RELAYS_DCO_TO_SCREEN
  uint8_t payload[SERIAL_LEN_ADSR_BLOCK];
  encode_u16_le(payload + 0, attack);
  encode_u16_le(payload + 2, decay);
  encode_u16_le(payload + 4, sustain);
  encode_u16_le(payload + 6, release);
  serial_frame_write(ScreenDma, cmd, payload, SERIAL_LEN_ADSR_BLOCK);
#else
  (void)cmd; (void)attack; (void)decay; (void)sustain; (void)release;
#endif
}

static void input_handle_adsr1_from_dco(char, const uint8_t* payload, uint8_t) {
  input_apply_adsr_block_from_dco(payload, ADSR1_attack, ADSR1_decay, ADSR1_sustain, ADSR1_release);
  input_forward_adsr_block_to_screen(CMD_ADSR1_BLOCK, ADSR1_attack, ADSR1_decay, ADSR1_sustain, ADSR1_release);
}

static void input_handle_adsr2_from_dco(char, const uint8_t* payload, uint8_t) {
  input_apply_adsr_block_from_dco(payload, ADSR2_attack, ADSR2_decay, ADSR2_sustain, ADSR2_release);
  input_forward_adsr_block_to_screen(CMD_ADSR2_BLOCK, ADSR2_attack, ADSR2_decay, ADSR2_sustain, ADSR2_release);
}

static void input_handle_adsr3_from_dco(char, const uint8_t* payload, uint8_t) {
  input_apply_adsr_block_from_dco(payload, ADSR3_attack, ADSR3_decay, ADSR3_sustain, ADSR3_release);
}

static void input_send_ui_param_to_screen(uint8_t id, int16_t value) {
#if defined(ENABLE_SCREEN_LINK) && INPUT_RELAYS_DCO_TO_SCREEN
  uint8_t payload[SERIAL_LEN_PARAM_16];
  encode_param_p(payload, id, value);
  serial_frame_write(ScreenDma, CMD_PARAM_16, payload, SERIAL_LEN_PARAM_16);
#else
  (void)id;
  (void)value;
#endif
}

static void input_handle_filter_block_from_dco(char, const uint8_t* payload, uint8_t) {
  const uint16_t cutoff     = decode_u16_le(payload + 0);
  const uint16_t resonance  = decode_u16_le(payload + 2);
  const int16_t  adsr2toVCF = decode_i16_le(payload + 4);
  const int16_t  lfo2toVCF  = (int16_t)decode_u16_le(payload + 6);

  if (cutoff != CUTOFF) {
    CUTOFF = cutoff;
    input_send_ui_param_to_screen((uint8_t)ParamId::PARAM_UI_CUTOFF, (int16_t)cutoff);
  }
  if (resonance != RESONANCE) {
    RESONANCE = resonance;
    input_send_ui_param_to_screen((uint8_t)ParamId::PARAM_UI_RESONANCE, (int16_t)resonance);
  }
  if (adsr2toVCF != ADSR2toVCF) {
    ADSR2toVCF = adsr2toVCF;
    input_send_ui_param_to_screen((uint8_t)ParamId::PARAM_UI_ADSR2_TO_VCF, adsr2toVCF);
  }
  if (lfo2toVCF != LFO2toVCF) {
    LFO2toVCF = lfo2toVCF;
    input_send_ui_param_to_screen((uint8_t)ParamId::PARAM_UI_LFO2_TO_VCF, lfo2toVCF);
  }
}

// Domain Block Ingress Handlers (Defined ABOVE dcoLinkCommands table)
static void input_handle_patch_osc_block_from_dco(char, const uint8_t* payload, uint8_t) {
  const PatchOscBlock* blk = (const PatchOscBlock*)payload;
  OSC1Interval     = blk->osc1_interval;
  OSC2Interval     = blk->osc2_interval;
  OSC3Interval     = blk->osc3_interval;
  OSC2Detune       = blk->osc2_detune;
  unisonDetune     = blk->unison_detune;
  voiceMode        = blk->voice_mode;
  voiceAllocMode   = blk->voice_alloc_mode;
  syncMode         = blk->sync_mode;
  softSync         = blk->soft_sync;
  subOscDivide     = blk->subosc_divide;
  analogDrift      = blk->analog_drift;
  analogDriftSpeed = blk->analog_drift_speed;
  analogDriftSpread= blk->analog_drift_spread;
  portamentoTime   = blk->portamento_time;
  portamentoMode   = blk->portamento_mode;
  characterAmount  = blk->character;
}

static void input_handle_patch_lfo_block_from_dco(char, const uint8_t* payload, uint8_t) {
  const PatchLfoBlock* blk = (const PatchLfoBlock*)payload;
  LFO1Waveform           = blk->lfo1_waveform;
  LFO2Waveform           = blk->lfo2_waveform;
  LFO1Speed              = blk->lfo1_speed;
  LFO2Speed              = blk->lfo2_speed;
  LFO1toDCO              = blk->lfo1_to_dco;
  LFO1toOSC1             = blk->lfo1_to_osc1;
  LFO1toOSC2             = blk->lfo1_to_osc2;
  LFO1toOSC3             = blk->lfo1_to_osc3;
  LFO2toOSC2DETUNE       = blk->lfo2_to_osc2;
  LFO2toOSC3DETUNE       = blk->lfo2_to_osc3;
  LFO2toOSC2_coarse      = blk->lfo2_to_osc2_coarse;
  LFO2toOSC3_coarse      = blk->lfo2_to_osc3_coarse;
  LFO2toPWM              = blk->lfo2_to_pw;
  LFO1toVCA              = blk->lfo1_to_vca;
  PW                     = blk->pw_value;
  ADSR1toVCA             = blk->adsr1_to_vca;
  ADSR3toPWM             = blk->adsr3_to_pwm;
  ADSR3toDETUNE1         = blk->adsr3_to_detune1;
  env_dco_pitch_centered = blk->adsr3_pitch_mode;
  ADSR3ToOscSelect       = blk->adsr3_to_osc_select;
}

static void input_handle_patch_mod_block_from_dco(char, const uint8_t* payload, uint8_t) {
  const PatchModBlock* blk = (const PatchModBlock*)payload;
  for (uint8_t i = 0; i < 8; i++) {
    modSlotSource[i] = blk->slots[i].src;
    modSlotDest[i]   = blk->slots[i].dest;
    modSlotDepth[i]  = blk->slots[i].depth;
  }
}

// =============================================================================
// 4. Ingress Table & Parsing
// =============================================================================

static const SerialCommandDef dcoLinkCommands[] = {
  { CMD_PARAM_16,         SERIAL_LEN_PARAM_16,         input_handle_param16_from_dco },
  { CMD_PARAM_32,         SERIAL_LEN_PARAM_32,         input_handle_param32_from_dco },
  { CMD_ADSR1_BLOCK,      SERIAL_LEN_ADSR_BLOCK,       input_handle_adsr1_from_dco },
  { CMD_ADSR2_BLOCK,      SERIAL_LEN_ADSR_BLOCK,       input_handle_adsr2_from_dco },
  { CMD_ADSR3_BLOCK,      SERIAL_LEN_ADSR_BLOCK,       input_handle_adsr3_from_dco },
  { CMD_FILTER_BLOCK,     SERIAL_LEN_FILTER_BLOCK,     input_handle_filter_block_from_dco },
  { CMD_PRESET_DIR_ENTRY, SERIAL_LEN_PRESET_DIR_ENTRY, input_handle_preset_dir_entry },
  { CMD_PRESET_LOADED,    SERIAL_LEN_PRESET_LOADED,    input_handle_preset_loaded    },
  { CMD_BLOCK_OSC,        SERIAL_LEN_BLOCK_OSC,        input_handle_patch_osc_block_from_dco },
  { CMD_BLOCK_LFO,        SERIAL_LEN_BLOCK_LFO,        input_handle_patch_lfo_block_from_dco },
  { CMD_BLOCK_MOD,        SERIAL_LEN_BLOCK_MOD,        input_handle_patch_mod_block_from_dco },
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
  ledRefreshPending      = true; // Refresh button LEDs if applicable
}