#include "_build_libs/DCO-PROTOCOL/param_router.h"
#include "params.h"

int8_t   manualCalibrationInitAmpCompOffset[NUM_OSCILLATORS] = { 0 };
uint16_t manualAmpComp440[NUM_OSCILLATORS]                   = { 0 };
uint16_t manualPwCenter[NUM_VOICES]                          = { 0 };
int16_t  ampCompDutyOffset[NUM_OSCILLATORS]                  = { 0 };
uint8_t  manualCalibrationStage                              = 0;
bool     manualCalibration                                   = false;


// --- Wave Enables ---
static void apply_param_osc1_saw(int16_t v)   { waveEnable[0][0] = (v != 0); ledRefreshPending = true; }
static void apply_param_osc1_pulse(int16_t v) { waveEnable[0][1] = (v != 0); ledRefreshPending = true; }
static void apply_param_osc1_tri(int16_t v)   { waveEnable[0][2] = (v != 0); ledRefreshPending = true; }
static void apply_param_osc2_saw(int16_t v)   { waveEnable[1][0] = (v != 0); ledRefreshPending = true; }
static void apply_param_osc2_pulse(int16_t v) { waveEnable[1][1] = (v != 0); ledRefreshPending = true; }
static void apply_param_osc2_tri(int16_t v)   { waveEnable[1][2] = (v != 0); ledRefreshPending = true; }
static void apply_param_osc3_saw(int16_t v)   { waveEnable[2][0] = (v != 0); ledRefreshPending = true; }
static void apply_param_osc3_pulse(int16_t v) { waveEnable[2][1] = (v != 0); ledRefreshPending = true; }
static void apply_param_osc3_tri(int16_t v)   { waveEnable[2][2] = (v != 0); ledRefreshPending = true; }

// --- Hardware Switches & Routing ---
static void apply_param_res_comp(int16_t v)            { RESONANCEAmpCompensation = (v != 0); }
static void apply_param_vca_restart(int16_t v)         { VCAADSRRestart = (v != 0); }
static void apply_param_vcf_restart(int16_t v)         { VCFADSRRestart = (v != 0); }
static void apply_param_adsr3_enabled(int16_t v)       { ADSR3Enabled = (v != 0); }
static void apply_param_adsr3_to_osc_select(int16_t v) { ADSR3ToOscSelect = (int8_t)constrain(v, 0, INPUT_ADSR3_TO_OSC_SELECT_MAX); }

// --- LFOs ---
static void apply_param_lfo1_waveform(int16_t v)       { LFO1Waveform = (int8_t)v; }
static void apply_param_lfo2_waveform(int16_t v)       { LFO2Waveform = (int8_t)v; }
static void apply_param_lfo1_speed(int16_t v)          { LFO1Speed = v; }
static void apply_param_lfo2_speed(int16_t v)          { LFO2Speed = v; }
static void apply_param_lfo1_to_dco(int16_t v)         { LFO1toDCO = v; }
static void apply_param_lfo1_to_osc1(int16_t v)        { LFO1toOSC1 = (uint8_t)constrain(v, 0, 255); }
static void apply_param_lfo1_to_osc2(int16_t v)        { LFO1toOSC2 = (uint8_t)constrain(v, 0, 255); }
static void apply_param_lfo1_to_osc3(int16_t v)        { LFO1toOSC3 = (uint8_t)constrain(v, 0, 255); }
static void apply_param_lfo2_to_osc2(int16_t v)        { LFO2toOSC2DETUNE = v; }
static void apply_param_lfo2_to_osc3(int16_t v)        { LFO2toOSC3DETUNE = v; }
static void apply_param_lfo2_to_osc2_coarse(int16_t v) { LFO2toOSC2_coarse = (uint16_t)constrain(v, 0, 511); }
static void apply_param_lfo2_to_osc3_coarse(int16_t v) { LFO2toOSC3_coarse = (uint16_t)constrain(v, 0, 511); }
static void apply_param_lfo1_to_vca(int16_t v)         { LFO1toVCA = v; }
static void apply_param_lfo2_to_pw(int16_t v)          { LFO2toPWM = v; }

// --- Pitch, Intervals & Detune ---
static void apply_param_osc1_interval(int16_t v)  { OSC1Interval = (int8_t)v; }
static void apply_param_osc2_interval(int16_t v)  { OSC2Interval = (int8_t)v; }
static void apply_param_osc3_interval(int16_t v)  { OSC3Interval = (int8_t)v; }
static void apply_param_osc2_detune(int16_t v)    { OSC2Detune = v; }
static void apply_param_osc3_detune(int16_t v)    { OSC3Detune = v; }
static void apply_param_unison_detune(int16_t v)  { unisonDetune = v; }
static void apply_param_portamento_time(int16_t v){ portamentoTime = v; }
static void apply_param_portamento_mode(int16_t v){ portamentoMode = (byte)v; }

// --- Modes & Allocation ---
static void apply_param_osc_sync_mode(int16_t v)   { oscSyncMode = (uint16_t)v; }
static void apply_param_sync_mode(int16_t v)       { syncMode = (byte)v; }
static void apply_param_soft_sync(int16_t v)       { softSync = (uint8_t)v; }
static void apply_param_subosc_divide(int16_t v)   { subOscDivide = (uint8_t)v; }
static void apply_param_voice_mode(int16_t v)      { voiceMode = (byte)constrain(v, 0, 2); }
static void apply_param_voice_alloc_mode(int16_t v){ voiceAllocMode = (byte)constrain(v, 0, 5); }
static void apply_param_filter_mode(int16_t v)     { filterMode = (uint8_t)v; }

// --- Drift, Velocity & Dynamics ---
static void apply_param_drift_amount(int16_t v)   { analogDrift = v; }
static void apply_param_drift_speed(int16_t v)    { analogDriftSpeed = v; }
static void apply_param_drift_spread(int16_t v)   { analogDriftSpread = v; }
static void apply_param_vcf_keytrack(int16_t v)   { VCFKeytrack = v; }
static void apply_param_velocity_to_vcf(int16_t v){ velocityToVCF = (int8_t)v; }
static void apply_param_velocity_to_vca(int16_t v){ velocityToVCA = (int8_t)v; }

// --- Levels ---
static void apply_param_osc1_level(int16_t v) { OSC1Level = v; }
static void apply_param_osc2_level(int16_t v) { OSC2Level = v; }
static void apply_param_sub_level(int16_t v)  { SubLevel = v; }
static void apply_param_osc3_level(int16_t v) { OSC3Level = v; }
static void apply_param_vca_level(int16_t v)  { VCALevel = v; }

// --- Envelopes & Modulation ---
static void apply_param_adsr3_to_pwm(int16_t v)     { ADSR3toPWM = (int16_t)constrain((int32_t)v - 512, -512, 511); }
static void apply_param_adsr3_to_detune1(int16_t v) { ADSR3toDETUNE1 = v; }
static void apply_param_adsr3_pitch_mode(int16_t v) { env_dco_pitch_centered = (v != 0) ? 1 : 0; }
static void apply_param_adsr1_to_vca(int16_t v)     { ADSR1toVCA = v; }
static void apply_param_adsr1_attack_curve(int16_t v){ ADSR1AttackCurveVal = (int8_t)v; }
static void apply_param_adsr1_decay_curve(int16_t v) { ADSR1DecayCurveVal = (int8_t)v; }
static void apply_param_adsr2_attack_curve(int16_t v){ ADSR2AttackCurveVal = (int8_t)v; }
static void apply_param_adsr2_decay_curve(int16_t v) { ADSR2DecayCurveVal = (int8_t)v; }

// --- Distortion, Character & PW ---
static void apply_param_dist_drive(int16_t v) { distDrive = (uint16_t)v; }
static void apply_param_dist_mix(int16_t v)   { distMix = (uint16_t)v; }
static void apply_param_character(int16_t v)  { characterAmount = (uint8_t)constrain(v, 0, 128); }
static void apply_param_pw_value(int16_t v)   { PW = (uint16_t)v; }

// --- Calibration Echo Mirrors ---
// =============================================================================
// Calibration Parameter Appliers (In-RAM Mirrors)
// =============================================================================

static void apply_param_manual_calibration_flag(int16_t v) {
  manualCalibration = (v != 0);
}

static void apply_param_manual_calibration_stage(int16_t v) {
  manualCalibrationStage = (uint8_t)v;
  uint8_t osc = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
  uint8_t ch  = INPUT_CAL_PW_CH(osc);

  // Pre-load encoder state to match the stored value for this stage
  if (INPUT_CAL_STAGE_IS_440(manualCalibrationStage)) {
    calibrationVal = (int16_t)manualAmpComp440[osc];
  } else if (INPUT_CAL_STAGE_IS_PW_EDIT(manualCalibrationStage)) {
    if (ch < NUM_VOICES) calibrationVal = (int16_t)manualPwCenter[ch];
  } else {
    calibrationVal = (int16_t)manualCalibrationInitAmpCompOffset[osc];
  }
}

static void apply_param_manual_calibration_offset(int16_t v) {
  uint8_t osc = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
  if (osc < NUM_OSCILLATORS) {
    manualCalibrationInitAmpCompOffset[osc] = (int8_t)v;
    calibrationVal = (int16_t)(int8_t)v;
  }
}

static void apply_param_amp_comp_440(int16_t v) {
  uint8_t osc = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
  if (osc < NUM_OSCILLATORS) {
    manualAmpComp440[osc] = (uint16_t)v;
    if (INPUT_CAL_STAGE_IS_440(manualCalibrationStage)) {
      calibrationVal = v;
    }
  }
}

static void apply_param_cal_pw_center(int16_t v) {
  uint8_t osc = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
  uint8_t ch  = INPUT_CAL_PW_CH(osc);
  if (ch < NUM_VOICES) {
    manualPwCenter[ch] = (uint16_t)v;
    if (INPUT_CAL_STAGE_IS_PW_EDIT(manualCalibrationStage)) {
      calibrationVal = v;
    }
  }
}

static void apply_param_amp_comp_duty_offset(int16_t v) {
  uint8_t osc = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
  if (osc < NUM_OSCILLATORS) {
    ampCompDutyOffset[osc] = v;
  }
}


// --- Mod Matrix Slots (0..7) ---
#define MOD_SLOT_APPLIERS(N) \
static void apply_param_mod_slot##N##_source(int16_t v) { modSlotSource[N] = (uint8_t)v; } \
static void apply_param_mod_slot##N##_dest(int16_t v)   { modSlotDest[N]   = (uint8_t)v; } \
static void apply_param_mod_slot##N##_depth(int16_t v)  { modSlotDepth[N]  = v; }

MOD_SLOT_APPLIERS(0)
MOD_SLOT_APPLIERS(1)
MOD_SLOT_APPLIERS(2)
MOD_SLOT_APPLIERS(3)
MOD_SLOT_APPLIERS(4)
MOD_SLOT_APPLIERS(5)
MOD_SLOT_APPLIERS(6)
MOD_SLOT_APPLIERS(7)
#undef MOD_SLOT_APPLIERS

// =============================================================================
// Router Table & Jump Setup
// =============================================================================

static const ParamDescriptorT<int16_t> paramTable[] = {
  { PARAM_OSC1_SAW_ENABLE,     apply_param_osc1_saw },
  { PARAM_OSC1_PULSE_ENABLE,   apply_param_osc1_pulse },
  { PARAM_OSC1_TRI_ENABLE,     apply_param_osc1_tri },
  { PARAM_OSC2_SAW_ENABLE,     apply_param_osc2_saw },
  { PARAM_OSC2_PULSE_ENABLE,   apply_param_osc2_pulse },
  { PARAM_OSC2_TRI_ENABLE,     apply_param_osc2_tri },
  { PARAM_OSC3_SAW_ENABLE,     apply_param_osc3_saw },
  { PARAM_OSC3_PULSE_ENABLE,   apply_param_osc3_pulse },
  { PARAM_OSC3_TRI_ENABLE,     apply_param_osc3_tri },

  { PARAM_RESONANCE_COMPENSATION, apply_param_res_comp },
  { PARAM_VCA_ADSR_RESTART,       apply_param_vca_restart },
  { PARAM_VCF_ADSR_RESTART,       apply_param_vcf_restart },
  { PARAM_ADSR3_ENABLED,          apply_param_adsr3_enabled },
  { PARAM_ADSR3_TO_OSC_SELECT,    apply_param_adsr3_to_osc_select },

  { PARAM_LFO1_WAVEFORM,       apply_param_lfo1_waveform },
  { PARAM_LFO2_WAVEFORM,       apply_param_lfo2_waveform },
  { PARAM_LFO1_SPEED,          apply_param_lfo1_speed },
  { PARAM_LFO2_SPEED,          apply_param_lfo2_speed },
  { PARAM_LFO1_TO_DCO,         apply_param_lfo1_to_dco },
  { PARAM_LFO1_TO_OSC1,        apply_param_lfo1_to_osc1 },
  { PARAM_LFO1_TO_OSC2,        apply_param_lfo1_to_osc2 },
  { PARAM_LFO1_TO_OSC3,        apply_param_lfo1_to_osc3 },
  { PARAM_LFO2_TO_OSC2,        apply_param_lfo2_to_osc2 },
  { PARAM_LFO2_TO_OSC3,        apply_param_lfo2_to_osc3 },
  { PARAM_LFO2_TO_OSC2_COARSE, apply_param_lfo2_to_osc2_coarse },
  { PARAM_LFO2_TO_OSC3_COARSE, apply_param_lfo2_to_osc3_coarse },
  { PARAM_LFO1_TO_VCA,         apply_param_lfo1_to_vca },
  { PARAM_LFO2_TO_PW,          apply_param_lfo2_to_pw },

  { PARAM_OSC1_INTERVAL,       apply_param_osc1_interval },
  { PARAM_OSC2_INTERVAL,       apply_param_osc2_interval },
  { PARAM_OSC3_INTERVAL,       apply_param_osc3_interval },
  { PARAM_OSC2_DETUNE_VAL,     apply_param_osc2_detune },
  { PARAM_OSC3_DETUNE_VAL,     apply_param_osc3_detune },
  { PARAM_UNISON_DETUNE,       apply_param_unison_detune },
  { PARAM_PORTAMENTO_TIME,     apply_param_portamento_time },
  { PARAM_PORTAMENTO_MODE,     apply_param_portamento_mode },

  { PARAM_OSC_SYNC_MODE,       apply_param_osc_sync_mode },
  { PARAM_SYNC_MODE,           apply_param_sync_mode },
  { PARAM_SOFT_SYNC,           apply_param_soft_sync },
  { PARAM_SUBOSC_DIVIDE,       apply_param_subosc_divide },
  { PARAM_VOICE_MODE,          apply_param_voice_mode },
  { PARAM_VOICE_ALLOC_MODE,    apply_param_voice_alloc_mode },
  { PARAM_FILTER_MODE,         apply_param_filter_mode },

  { PARAM_ANALOG_DRIFT_AMOUNT, apply_param_drift_amount },
  { PARAM_ANALOG_DRIFT_SPEED,  apply_param_drift_speed },
  { PARAM_ANALOG_DRIFT_SPREAD, apply_param_drift_spread },
  { PARAM_VCF_KEYTRACK,        apply_param_vcf_keytrack },
  { PARAM_VELOCITY_TO_VCF,     apply_param_velocity_to_vcf },
  { PARAM_VELOCITY_TO_VCA,     apply_param_velocity_to_vca },

  { PARAM_OSC1_LEVEL,          apply_param_osc1_level },
  { PARAM_OSC2_LEVEL,          apply_param_osc2_level },
  { PARAM_SUB_LEVEL,           apply_param_sub_level },
  { PARAM_OSC3_LEVEL,          apply_param_osc3_level },
  { PARAM_VCA_LEVEL,           apply_param_vca_level },

  { PARAM_ADSR3_TO_PWM,        apply_param_adsr3_to_pwm },
  { PARAM_ADSR3_TO_DETUNE1,    apply_param_adsr3_to_detune1 },
  { PARAM_ADSR3_PITCH_MODE,    apply_param_adsr3_pitch_mode },
  { PARAM_ADSR1_TO_VCA,        apply_param_adsr1_to_vca },
  { PARAM_ADSR1_ATTACK_CURVE,  apply_param_adsr1_attack_curve },
  { PARAM_ADSR1_DECAY_CURVE,   apply_param_adsr1_decay_curve },
  { PARAM_ADSR2_ATTACK_CURVE,  apply_param_adsr2_attack_curve },
  { PARAM_ADSR2_DECAY_CURVE,   apply_param_adsr2_decay_curve },

  { PARAM_DIST_DRIVE,          apply_param_dist_drive },
  { PARAM_DIST_MIX,            apply_param_dist_mix },
  { PARAM_CHARACTER,           apply_param_character },
  { PARAM_PW_VALUE,            apply_param_pw_value },

  { PARAM_AMP_COMP_440,        apply_param_amp_comp_440 },
  { PARAM_CAL_PW_CENTER,       apply_param_cal_pw_center },
  { PARAM_AMP_COMP_DUTY_OFFSET, apply_param_amp_comp_duty_offset },

  { PARAM_MOD_SLOT0_SOURCE, apply_param_mod_slot0_source },
  { PARAM_MOD_SLOT0_DEST,   apply_param_mod_slot0_dest },
  { PARAM_MOD_SLOT0_DEPTH,  apply_param_mod_slot0_depth },
  { PARAM_MOD_SLOT1_SOURCE, apply_param_mod_slot1_source },
  { PARAM_MOD_SLOT1_DEST,   apply_param_mod_slot1_dest },
  { PARAM_MOD_SLOT1_DEPTH,  apply_param_mod_slot1_depth },
  { PARAM_MOD_SLOT2_SOURCE, apply_param_mod_slot2_source },
  { PARAM_MOD_SLOT2_DEST,   apply_param_mod_slot2_dest },
  { PARAM_MOD_SLOT2_DEPTH,  apply_param_mod_slot2_depth },
  { PARAM_MOD_SLOT3_SOURCE, apply_param_mod_slot3_source },
  { PARAM_MOD_SLOT3_DEST,   apply_param_mod_slot3_dest },
  { PARAM_MOD_SLOT3_DEPTH,  apply_param_mod_slot3_depth },
  { PARAM_MOD_SLOT4_SOURCE, apply_param_mod_slot4_source },
  { PARAM_MOD_SLOT4_DEST,   apply_param_mod_slot4_dest },
  { PARAM_MOD_SLOT4_DEPTH,  apply_param_mod_slot4_depth },
  { PARAM_MOD_SLOT5_SOURCE, apply_param_mod_slot5_source },
  { PARAM_MOD_SLOT5_DEST,   apply_param_mod_slot5_dest },
  { PARAM_MOD_SLOT5_DEPTH,  apply_param_mod_slot5_depth },
  { PARAM_MOD_SLOT6_SOURCE, apply_param_mod_slot6_source },
  { PARAM_MOD_SLOT6_DEST,   apply_param_mod_slot6_dest },
  { PARAM_MOD_SLOT6_DEPTH,  apply_param_mod_slot6_depth },
  { PARAM_MOD_SLOT7_SOURCE, apply_param_mod_slot7_source },
  { PARAM_MOD_SLOT7_DEST,   apply_param_mod_slot7_dest },
  { PARAM_MOD_SLOT7_DEPTH,  apply_param_mod_slot7_depth },
  { PARAM_MANUAL_CALIBRATION_FLAG,  apply_param_manual_calibration_flag },
  { PARAM_MANUAL_CALIBRATION_STAGE, apply_param_manual_calibration_stage },
  { PARAM_MANUAL_CALIBRATION_OFFSET,apply_param_manual_calibration_offset },
  { PARAM_AMP_COMP_440,             apply_param_amp_comp_440 },
  { PARAM_CAL_PW_CENTER,            apply_param_cal_pw_center },
  { PARAM_AMP_COMP_DUTY_OFFSET,     apply_param_amp_comp_duty_offset },
};

static void (*inputParamJump[PARAM_ROUTER_JUMP_SIZE])(int16_t) = { nullptr };

void init_param_router() {
  param_router_build_jump(
    inputParamJump,
    paramTable,
    sizeof(paramTable) / sizeof(paramTable[0])
  );
}

void update_parameters(uint8_t id, int16_t value) {
  param_router_apply(inputParamJump, id, value);
}
