/**
 * @file INPUT-CONTROLLER-Serial.ino
 * @brief Input Controller Serial Communications and Frame Parsing.
 * 
 * @details Manages bidirectional communication between the physical control panel (pots,
 * faders, buttons) and the downstream DCO engine / Screen UI. It converts analog readings
 * into protocol-compliant structs and routes incoming UI/Telemetry updates to local RAM.
 */

 #define DCO_PROTOCOL_IMPLEMENT_DMA
 #include "include_all.h"
 
 UartDmaTx DcoDma    = { 0 };
 UartDmaTx ScreenDma = { 1 };
 
 void input_handle_preset_dir_entry(char cmd, const uint8_t* payload, uint8_t len);
 void input_handle_preset_loaded(char, const uint8_t* payload, uint8_t);
 
 // =============================================================================
 // 1. DMA Initialization & Polling
 // =============================================================================
 
 /**
  * @brief Initializes hardware DMA channels for the RP2040 UARTs.
  * @details Swaps physical UART definitions based on the host board variant (DCO3 vs DCO4).
  */
 void serial_dma_init() {
 #if INPUT_IS_DCO3
   serial_dma_init_rp2040(0, uart0);
   serial_dma_init_rp2040(1, uart1);
 #else
   serial_dma_init_rp2040(0, uart1);
   serial_dma_init_rp2040(1, uart0);
 #endif
 }
 
 /**
  * @brief Polls DMA completion flags to free transmit buffers.
  */
 void serial_dma_poll() {
   serial_dma_poll_one(0);
   serial_dma_poll_one(1);
 }
 
 // =============================================================================
 // 2. Outgoing Senders
 // =============================================================================
 
 /**
  * @brief Sends a generic UI signal/mode change to the Screen controller.
  * @param signal The ScreenMode enum value to activate.
  */
 void __not_in_flash_func(serial_send_signal)(byte signal) {
 #ifdef ENABLE_SCREEN_LINK
   serial_frame_write(ScreenDma, CMD_SCREEN_SIGNAL, &signal, SERIAL_LEN_SCREEN_SIGNAL);
 #endif
 }
 
 /**
  * @brief Dispatches a 16-bit parameter change to the DCO Engine and optionally the Screen.
  * @param param Parameter ID (ParamId enum).
  * @param paramValue Bipolar or unsigned 16-bit payload.
  * @param sendToAll If true, also relays the parameter to the Screen UI.
  */
 void __not_in_flash_func(serial_send_param_change)(byte param, uint16_t paramValue, bool sendToAll) {
 #ifdef ENABLE_SCREEN_LINK
   if (sendToAll) {
     transmit_param16(ScreenDma, param, (int16_t)paramValue);
   }
 #endif
 #ifdef ENABLE_DCO_LINK
   transmit_param16(DcoDma, param, (int16_t)paramValue);
 #endif
 }
 
 /**
  * @brief Helper for dispatching 8-bit parameter changes.
  */
 void __not_in_flash_func(serial_send_param_change_byte)(byte param, byte paramValue, bool sendToAll) {
   serial_send_param_change(param, (uint16_t)paramValue, sendToAll);
 }
 
 /**
  * @brief Sends the active preset name string back down to the Mainboard.
  */
 void serial_send_preset_name_to_mainboard() {
 #ifdef ENABLE_DCO_LINK
   serial_frame_write(DcoDma, CMD_PRESET_NAME, presetNameVal, SERIAL_LEN_PRESET_NAME);
 #endif
 }
 
 /**
  * @brief Requests the Screen UI to render a specific preset name in a scroll view.
  * @param presetNumber Absolute LittleFS slot index.
  * @param presetNameSerial 16-byte character array containing the name.
  */
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
 
 /**
  * @brief Sends the actively selected character index for the Save Preset text editor.
  */
 void serial_send_save_char_select(byte serialPresetChar) {
 #ifdef ENABLE_SCREEN_LINK
   serial_frame_write(ScreenDma, CMD_CHAR_SELECT, &serialPresetChar, SERIAL_LEN_CHAR_SELECT);
 #endif
 }
 
 /**
  * @brief Internal helper to forward a parameter exclusively to the Screen UI.
  */
 static void input_send_param16_to_screen(uint8_t id, int16_t value) {
 #ifdef ENABLE_SCREEN_LINK
   transmit_param16(ScreenDma, id, value);
 #else
   (void)id; (void)value;
 #endif
 }
 
 /**
  * @brief External hook to forward an 8-bit parameter exclusively to the Screen UI.
  */
 void __not_in_flash_func(serialSendParamByteToScreen)(byte paramNumber, byte paramValue) {
   input_send_param16_to_screen(paramNumber, (int16_t)(int8_t)paramValue);
 }
 
 /**
  * @brief Syncs manual calibration UI state (Stage, Offset, AMP/PW Target) to the Screen.
  */
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
 
 /**
  * @brief Continuously streams analog panel state (Pots/Faders) to the DCO and Screen.
  * @details Packages ADSRs into exponential curves for the synth engine and linear
  * visual curves for the Screen UI.
  * @param presetLoading If true, forces a flush of all hardware state regardless of lock status.
  */
 void __not_in_flash_func(serial_send_manual_controls)(bool presetLoading) {
   // 1. ADSR 1 Faders
   if (faderRow1ControlManual || presetLoading) {
     // Exponential block -> Mainboard / DCO
     AdsrBlock dcoBlk = {
       linToExpLookup[ADSR1_attack],
       linToExpLookup[ADSR1_decay],
       ADSR1_sustain,
       linToExpLookup[ADSR1_release]
     };
 #ifdef ENABLE_DCO_LINK
     serial_frame_write(DcoDma, CMD_ADSR1_BLOCK, (const uint8_t*)&dcoBlk, sizeof(AdsrBlock));
 #endif
 
     // Linear bar graph block -> Direct to Screen
 #ifdef ENABLE_SCREEN_LINK
     AdsrBlock screenBlk = {
       ADSR1_attack,
       ADSR1_decay,
       ADSR1_sustain,
       ADSR1_release
     };
     serial_frame_write(ScreenDma, CMD_ADSR1_BLOCK, (const uint8_t*)&screenBlk, sizeof(AdsrBlock));
 #endif
   }
 
   // 2. ADSR 2 / 3 Faders
   if ((faderRow2ControlManual && !ADSR3Enabled) || presetLoading) {
     // Exponential block -> Mainboard / DCO
     AdsrBlock dcoBlk = {
       linToExpLookup[ADSR2_attack],
       linToExpLookup[ADSR2_decay],
       ADSR2_sustain,
       linToExpLookup[ADSR2_release]
     };
 #ifdef ENABLE_DCO_LINK
     serial_frame_write(DcoDma, CMD_ADSR2_BLOCK, (const uint8_t*)&dcoBlk, sizeof(AdsrBlock));
 #endif
 
     // Linear bar graph block -> Direct to Screen
 #ifdef ENABLE_SCREEN_LINK
     AdsrBlock screenBlk = {
       ADSR2_attack,
       ADSR2_decay,
       ADSR2_sustain,
       ADSR2_release
     };
     serial_frame_write(ScreenDma, CMD_ADSR2_BLOCK, (const uint8_t*)&screenBlk, sizeof(AdsrBlock));
 #endif
   } else if ((faderRow2ControlManual && ADSR3Enabled) || presetLoading) {
     AdsrBlock dcoBlk = {
       linToExpLookup[ADSR3_attack],
       linToExpLookup[ADSR3_decay],
       ADSR3_sustain,
       linToExpLookup[ADSR3_release]
     };
 #ifdef ENABLE_DCO_LINK
     serial_frame_write(DcoDma, CMD_ADSR3_BLOCK, (const uint8_t*)&dcoBlk, sizeof(AdsrBlock));
 #endif
   }
 
   // 3. VCF Filter Pots -> Mainboard / DCO
   if (VCFPotsControlManual || presetLoading) {
     FilterBlock fltBlk = {
       CUTOFF,
       RESONANCE,
       (int16_t)ADSR2toVCF,
       LFO2toVCF
     };
 #ifdef ENABLE_DCO_LINK
     serial_frame_write(DcoDma, CMD_FILTER_BLOCK, (const uint8_t*)&fltBlk, sizeof(FilterBlock));
 #endif
   }
 
   // 4. VCA Live Pot -> Mainboard / DCO
   if (VCAPotsControlManual || presetLoading) {
 #ifdef ENABLE_DCO_LINK
     transmit_param16(DcoDma, (uint8_t)ParamId::PARAM_ADSR1_TO_VCA, (int16_t)ADSR1toVCA);
 #endif
   }
 
   // 5. PWM Live Pot -> DCO Engine
   if (PWMPotsControlManual || presetLoading) {
 #ifdef ENABLE_DCO_LINK
     transmit_param16(DcoDma, (uint8_t)ParamId::PARAM_PW_VALUE, (int16_t)PW);
 #endif
   }
 }
 
 void sendSerial() {} // Compatibility shim
 
 // =============================================================================
 // 3. Ingress Handlers (From Mainboard / DCO Link)
 // =============================================================================
 
 /**
  * @brief Parses incoming 32-bit parameter frames (mostly calibration/telemetry).
  */
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
       ampCompDutyOffset[oscIndex] = dutyVal;
     }
     return;
   }
 }
 
 /**
  * @brief Dispatches generic 16-bit incoming parameters to local panel routing.
  */
 static void __not_in_flash_func(input_handle_param16_from_dco)(char, const uint8_t* payload, uint8_t) {
   ParamFrame frame;
   decode_param_p(payload, frame);
   update_parameters((uint8_t)frame.id, (int16_t)frame.value);
 }
 
 /**
  * @brief Restores incoming block ADSRs by casting and converting back to linear indices.
  */
 static void __not_in_flash_func(input_apply_adsr_block_from_dco)(const uint8_t* payload, uint16_t& attack, uint16_t& decay, uint16_t& sustain, uint16_t& release) {
   const AdsrBlock* blk = (const AdsrBlock*)payload;
   attack  = exp_to_lin_index(blk->attack);
   decay   = exp_to_lin_index(blk->decay);
   sustain = blk->sustain;
   release = exp_to_lin_index(blk->release);
 }
 
 static void __not_in_flash_func(input_handle_adsr1_from_dco)(char, const uint8_t* payload, uint8_t) {
   input_apply_adsr_block_from_dco(payload, ADSR1_attack, ADSR1_decay, ADSR1_sustain, ADSR1_release);
 }
 
 static void __not_in_flash_func(input_handle_adsr2_from_dco)(char, const uint8_t* payload, uint8_t) {
   input_apply_adsr_block_from_dco(payload, ADSR2_attack, ADSR2_decay, ADSR2_sustain, ADSR2_release);
 }
 
 static void __not_in_flash_func(input_handle_adsr3_from_dco)(char, const uint8_t* payload, uint8_t) {
   input_apply_adsr_block_from_dco(payload, ADSR3_attack, ADSR3_decay, ADSR3_sustain, ADSR3_release);
 }
 
 /**
  * @brief Restores incoming block filter states.
  */
 static void __not_in_flash_func(input_handle_filter_block_from_dco)(char, const uint8_t* payload, uint8_t) {
   const FilterBlock* blk = (const FilterBlock*)payload;
   CUTOFF     = blk->cutoff;
   RESONANCE  = blk->resonance;
   ADSR2toVCF = blk->env2_to_vcf;
   LFO2toVCF  = (int16_t)blk->lfo2_to_vcf;
 }
 
 /**
  * @brief Inbound Preset Name Ingress (17 bytes: [slot:u8][name:16 ASCII] or 16 bytes raw)
  */
 static void __not_in_flash_func(input_handle_preset_name_from_dco)(char, const uint8_t* payload, uint8_t len) {
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
 // Domain Block Ingress from DCO (Direct apply_param_* dispatch)
 // =============================================================================
 
 /** @brief 1. Oscillator & Voice Configuration Block ('v') */
 static void __not_in_flash_func(input_handle_patch_osc_block_from_dco)(char, const uint8_t* payload, uint8_t) {
   const PatchOscBlock* blk = (const PatchOscBlock*)payload;
 
   // --- Wave button mirrors (LED flags) ---
   apply_param_osc1_saw((blk->wave_enables & (1u << 0)) != 0);
   apply_param_osc1_pulse((blk->wave_enables & (1u << 1)) != 0);
   apply_param_osc1_tri((blk->wave_enables & (1u << 2)) != 0);
   apply_param_osc2_saw((blk->wave_enables & (1u << 3)) != 0);
   apply_param_osc2_pulse((blk->wave_enables & (1u << 4)) != 0);
   apply_param_osc2_tri((blk->wave_enables & (1u << 5)) != 0);
   apply_param_osc3_saw((blk->wave_enables & (1u << 6)) != 0);
   apply_param_osc3_pulse((blk->wave_enables & (1u << 7)) != 0);
   apply_param_osc3_tri((blk->wave_enables & (1u << 8)) != 0);
 
   // --- Pitch, Intervals & Voice ---
   apply_param_osc1_interval(blk->osc1_interval);
   apply_param_osc2_interval(blk->osc2_interval);
   apply_param_osc3_interval(blk->osc3_interval);
   apply_param_osc2_detune(blk->osc2_detune);
   apply_param_unison_detune(blk->unison_detune);
   apply_param_voice_mode(blk->voice_mode);
   apply_param_voice_alloc_mode(blk->voice_alloc_mode);
   apply_param_sync_mode(blk->sync_mode);
   apply_param_soft_sync(blk->soft_sync);
   apply_param_subosc_divide(blk->subosc_divide);
 
   // --- Analog Drift, Portamento & Character ---
   apply_param_drift_amount(blk->analog_drift);
   apply_param_drift_speed(blk->analog_drift_speed);
   apply_param_drift_spread(blk->analog_drift_spread);
   apply_param_portamento_time(blk->portamento_time);
   apply_param_portamento_mode(blk->portamento_mode);
   apply_param_character(blk->character);
 
   ledRefreshPending = true;
 }
 
 /** @brief 2. LFO & Modulation Block ('l') */
 static void __not_in_flash_func(input_handle_patch_lfo_block_from_dco)(char, const uint8_t* payload, uint8_t) {
   const PatchLfoBlock* blk = (const PatchLfoBlock*)payload;
 
   // --- LFO Speeds & Waveforms ---
   apply_param_lfo1_speed(blk->lfo1_speed);
   apply_param_lfo1_waveform(blk->lfo1_waveform);
   apply_param_lfo2_speed(blk->lfo2_speed);
   apply_param_lfo2_waveform(blk->lfo2_waveform);
 
   // --- LFO Pitch Depths & Routings ---
   apply_param_lfo1_to_dco(blk->lfo1_to_dco);
   apply_param_lfo1_to_osc1(blk->lfo1_to_osc1);
   apply_param_lfo1_to_osc2(blk->lfo1_to_osc2);
   apply_param_lfo1_to_osc3(blk->lfo1_to_osc3);
   apply_param_lfo2_to_osc2(blk->lfo2_to_osc2);
   apply_param_lfo2_to_osc3(blk->lfo2_to_osc3);
   apply_param_lfo2_to_osc2_coarse(blk->lfo2_to_osc2_coarse);
   apply_param_lfo2_to_osc3_coarse(blk->lfo2_to_osc3_coarse);
   apply_param_lfo2_to_pw(blk->lfo2_to_pw);
   apply_param_lfo1_to_vca(blk->lfo1_to_vca);
 
   // --- Pulse Width & Envelopes ---
   apply_param_pw_value(blk->pw_value);
   apply_param_adsr1_to_vca(blk->adsr1_to_vca);
   apply_param_adsr3_to_pwm(blk->adsr3_to_pwm);
   apply_param_adsr3_to_detune1(blk->adsr3_to_detune1);
   apply_param_adsr3_pitch_mode(blk->adsr3_pitch_mode);
   apply_param_adsr3_to_osc_select(blk->adsr3_to_osc_select);
 }
 
 /** @brief 3. Mod Matrix Block ('M') */
 static void __not_in_flash_func(input_handle_patch_mod_block_from_dco)(char, const uint8_t* payload, uint8_t) {
   const PatchModBlock* blk = (const PatchModBlock*)payload;
 
   for (uint8_t i = 0; i < MOD_SLOT_COUNT_INPUT; i++) {
     modSlotSource[i] = blk->slots[i].src;
     modSlotDest[i]   = blk->slots[i].dest;
     modSlotDepth[i]  = blk->slots[i].depth;
   }
 }
 
 /** @brief 4. Mixer, Curves, VCA & Filter Modes Block ('Q' / 'X') */
 static void __not_in_flash_func(input_handle_patch_mix_block_from_dco)(char, const uint8_t* payload, uint8_t) {
  const PatchMixBlock* blk = (const PatchMixBlock*)payload;

  // --- Levels & Filter ---
  apply_param_osc1_level(blk->osc1_level);
  apply_param_osc2_level(blk->osc2_level);
  apply_param_osc3_level(blk->osc3_level);
  apply_param_sub_level(blk->sub_level);
  apply_param_vca_level(blk->vca_level);
  apply_param_filter_mode(blk->filter_mode);

  // --- Dynamics & Keytracking ---
  apply_param_velocity_to_vcf(blk->velocity_to_vcf);
  apply_param_velocity_to_vca(blk->velocity_to_vca);
  apply_param_vcf_keytrack(blk->vcf_keytrack);
  apply_param_adsr1_to_vca(blk->adsr1_to_vca);
  apply_param_dist_drive(blk->dist_drive);
  apply_param_dist_mix(blk->dist_mix);

  // --- Curve Shaping ---
  apply_param_adsr1_attack_curve(blk->adsr1_attack_curve);
  apply_param_adsr1_decay_curve(blk->adsr1_decay_curve);
  apply_param_adsr1_release_curve(blk->adsr1_release_curve); 
  apply_param_adsr2_attack_curve(blk->adsr2_attack_curve);
  apply_param_adsr2_decay_curve(blk->adsr2_decay_curve);
  apply_param_adsr2_release_curve(blk->adsr2_release_curve); 
  apply_param_adsr3_attack_curve(blk->adsr3_attack_curve);   
  apply_param_adsr3_decay_curve(blk->adsr3_decay_curve);     
  apply_param_adsr3_release_curve(blk->adsr3_release_curve); 
  apply_param_vcf_trigger_mode(blk->vcf_trigger_mode);       

  // --- Hardware Flags & Restarts ---
  apply_param_res_comp((blk->misc_flags & (1 << 0)) != 0);
  apply_param_vca_restart((blk->misc_flags & (1 << 1)) != 0);
  apply_param_vcf_restart((blk->misc_flags & (1 << 2)) != 0);
  apply_param_adsr3_enabled((blk->misc_flags & (1 << 3)) != 0);
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
 
 /**
  * @brief Mounts the parser tables for incoming commands from the Mainboard/DCO.
  */
 void init_dco_link_parser() {
   serial_parser_reset(dcoLinkParser);
   serial_command_table_init(
     dcoLinkLut,
     dcoLinkCommands,
     sizeof(dcoLinkCommands) / sizeof(dcoLinkCommands[0])
   );
 }
 
 /**
  * @brief Reads available incoming bytes and drives the state machine.
  */
 void __not_in_flash_func(serial_read_from_dco)() {
 #ifdef ENABLE_DCO_LINK
   while (DCO_RX_PORT.available() > 0) {
     serial_parser_drain(dcoLinkParser, dcoLinkLut, DCO_RX_PORT, 255);
   }
 #endif
 }
 
 /**
  * @brief Suspends all manual panel pot/fader scanning (used during preset load/save).
  */
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