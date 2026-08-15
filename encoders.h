#ifndef __ENCODERS_H__
#define __ENCODERS_H__

#include "sram_hot.h"
#include "_build_libs/MD_REncoder_fela/src/MD_REncoder_fela.h"

#define NUM_ENCODERS 11

enum EncoderAction {
  ACTION_NONE,

  ACTION_portamento_time,

  ACTION_LFO1_to_DCO,

  ACTION_VCF_keytrack,

  ACTION_velocity_to_VCF,
  ACTION_velocity_to_VCA,

  ACTION_SQR1_level,
  ACTION_SQR2_level,
  ACTION_SUB_level,

  ACTION_octave,
  ACTION_OSC2_interval,
  ACTION_OSC3_interval,
  ACTION_OSC2_detune,
  ACTION_OSC3_detune,
  ACTION_ANALOG_DETUNE,
  ACTION_ANALOG_DRIFT,
  ACTION_ANALOG_DRIFT_SPEED,
  ACTION_ANALOG_DRIFT_SPREAD,

  ACTION_LFO2_to_OSC2,
  ACTION_LFO2_to_OSC3,

  ACTION_osc_sync_mode,

  ACTION_LFO1_speed,
  ACTION_LFO2_speed,

  ACTION_VCA_level,

  ACTION_LFO1_to_VCA,
  ACTION_LFO2_to_PWM,

  ACTION_ADSR3_to_PWM,
  ACTION_ADSR3_to_DETUNE1,
  ACTION_ADSR_CURVE_ATTACK,
  ACTION_ADSR_CURVE_DECAY,

  // Dedicated per-envelope curve knobs (envelope menu items; the two shared
  // ACTION_ADSR_CURVE_* actions above stay bound to the FUNC curve-select flow).
  ACTION_ADSR1_ATTACK_CURVE,
  ACTION_ADSR1_DECAY_CURVE,
  ACTION_ADSR2_ATTACK_CURVE,
  ACTION_ADSR2_DECAY_CURVE,

  ACTION_select_preset,

  ACTION_select_char,

  ACTION_select_char_pos,

  ACTION_CALIBRATION_OFFSET,
  ACTION_CALIBRATION_STAGE,

  ACTION_MENU_VALUE,
  ACTION_MENU_POS,
};

// --- generic "value knob" bindings -------------------------------------------
//
// Most encoder actions are the same gesture: step a global by
// baseStep + (speedMultX2 * speed) / 2, clamp to [min, max], and send one
// param frame. Those live in encoderParamBindings[] (encoders.ino) and are
// executed by one shared handler; only the genuinely special actions
// (calibration, menus, preset naming, ADSR curve edit) keep their own case.

// How the handler reads/writes the bound global.
enum EncoderValType : uint8_t {
  ENC_VAL_I8,   // int8_t
  ENC_VAL_I16,  // int16_t
  ENC_VAL_U16,  // uint16_t
};

// EncoderParamBinding.flags
static constexpr uint8_t ENC_SEND_WORD = 0x01;        // 16-bit param frame (default: byte)
static constexpr uint8_t ENC_SEND_OFFSET_512 = 0x02;  // send value + 512 (bipolar wire encode)

struct EncoderParamBinding {
  EncoderAction action;
  void* value;          // the existing global this knob edits
  EncoderValType type;  // width/signedness of *value
  int16_t min;
  int16_t max;
  uint8_t baseStep;     // step per detent at speed 0
  uint8_t speedMultX2;  // speed multiplier in half steps: 1 -> 0.5*speed, 2 -> 1*speed
  ParamId paramId;
  uint8_t flags;        // ENC_SEND_* bits
};

struct EncoderStruct {
  MD_REncoder MD_REncoder_Name;
  uint8_t muxPin1;
  uint8_t muxPin2;
  // NORMAL bank: [0] plain turn, [1] latched key held (buttonIsLatched) or
  // save name-edit mode, [2] reserved (never dispatched today).
  EncoderAction actions[3];
  // FUNC bank: [0] plain turn, [1] latched key held or save slot-select mode,
  // [2] ADSR curve-select mode.
  EncoderAction actionsAlt[3];
};

// The third-oscillator positions are live only on the monosynth; the 4x2 panel
// has no OSC3 to steer, so those slots fall back to ACTION_NONE.
#if INPUT_HAS_OSC3_PANEL
#define ENC_OSC3_INTERVAL ACTION_OSC3_interval
#define ENC_OSC3_DETUNE   ACTION_OSC3_detune
#define ENC_LFO2_TO_OSC3  ACTION_LFO2_to_OSC3
#else
#define ENC_OSC3_INTERVAL ACTION_NONE
#define ENC_OSC3_DETUNE   ACTION_NONE
#define ENC_LFO2_TO_OSC3  ACTION_NONE
#endif

EncoderStruct encoders[] = {
  //                    --- NORMAL bank: plain / latched / reserved ---                        --- FUNC bank: plain / latched / curve-select ---
  { enc1, 13, 14, { ACTION_LFO1_to_DCO,   ACTION_ADSR3_to_DETUNE1, ACTION_NONE },       { ACTION_ADSR3_to_DETUNE1, ACTION_NONE,                ACTION_NONE } },
  { enc2, 10, 11, { ACTION_osc_sync_mode, ACTION_NONE,             ACTION_NONE },       { ACTION_portamento_time,  ACTION_NONE,                ACTION_NONE } },
  { enc3,  7,  8, { ACTION_octave,        ACTION_OSC2_interval,    ENC_OSC3_INTERVAL }, { ACTION_OSC2_interval,    ENC_OSC3_INTERVAL,          ACTION_ADSR_CURVE_ATTACK } },
  { enc4,  4,  5, { ACTION_SQR1_level,    ACTION_NONE,             ACTION_NONE },       { ACTION_ANALOG_DETUNE,    ACTION_NONE,                ACTION_ADSR_CURVE_DECAY } },
  { enc5,  1,  2, { ACTION_OSC2_detune,   ACTION_LFO2_to_OSC2,     ENC_OSC3_DETUNE },   { ACTION_ANALOG_DRIFT,     ACTION_ANALOG_DRIFT_SPEED,  ENC_LFO2_TO_OSC3 } },
  { enc6, 30, 31, { ACTION_SQR2_level,    ACTION_NONE,             ACTION_NONE },       { ACTION_VCF_keytrack,     ACTION_ANALOG_DRIFT_SPREAD, ACTION_NONE } },
  { enc7, 27, 28, { ACTION_SUB_level,     ACTION_NONE,             ACTION_NONE },       { ACTION_velocity_to_VCA,  ACTION_NONE,                ACTION_NONE } },
  { enc8, 24, 25, { ACTION_LFO1_speed,    ACTION_select_char,      ACTION_NONE },       { ACTION_velocity_to_VCF,  ACTION_NONE,                ACTION_NONE } },
  { enc9, 21, 22, { ACTION_LFO2_speed,    ACTION_select_char_pos,  ACTION_NONE },       { ACTION_select_preset,    ACTION_select_preset,       ACTION_NONE } },
  { enc10, 42, 17, { ACTION_LFO1_to_VCA,  ACTION_NONE,             ACTION_NONE },       { ACTION_VCA_level,        ACTION_NONE,                ACTION_NONE } },
  { enc11, 18, 19, { ACTION_LFO2_to_PWM,  ACTION_NONE,             ACTION_NONE },       { ACTION_ADSR3_to_PWM,     ACTION_NONE,                ACTION_NONE } },
};

// Per-encoder action override while in MANUAL_CALIBRATION / menu modes.
EncoderAction manualCalibrationActions[NUM_ENCODERS] = { ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_CALIBRATION_OFFSET, ACTION_CALIBRATION_STAGE, ACTION_NONE, ACTION_NONE };
EncoderAction menuNavigationActions[NUM_ENCODERS] = { ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_MENU_VALUE, ACTION_MENU_POS, ACTION_NONE, ACTION_NONE };

static_assert(sizeof(encoders) / sizeof(encoders[0]) == NUM_ENCODERS,
              "encoders[] must have NUM_ENCODERS entries");
static_assert(sizeof(manualCalibrationActions) / sizeof(manualCalibrationActions[0]) == NUM_ENCODERS,
              "manualCalibrationActions[] must have NUM_ENCODERS entries");
static_assert(sizeof(menuNavigationActions) / sizeof(menuNavigationActions[0]) == NUM_ENCODERS,
              "menuNavigationActions[] must have NUM_ENCODERS entries");

// First-detent selection: the first turn of a (re)targeted knob only shows the
// value on the Screen; edits begin once the same action repeats within the
// timeout window.
EncoderAction encoderActionSelected = ACTION_NONE;
unsigned long encoderActionSelectedMillis = 0;
unsigned long encoderActionSelectedTimeout = 2000;
bool encoderActionIsSelected = false;

int8_t menuPos = 0;
int8_t menuPosMax = 0;
int8_t menuValue = 0;

void read_encoders();


// --- ENCODER SERIAL THROTTLE CONFIGURATION ---
// 20 ms = 50 packets/second maximum per encoder link
static constexpr uint32_t ENCODER_TX_INTERVAL_MS = 60;

struct EncoderThrottle {
  ParamId paramId = (ParamId)0;
  uint16_t wireValue = 0;
  bool isWord = false;
  bool isDirty = false;
  uint32_t lastSentMillis = 0;
};

// One throttle tracker per encoder (11 encoders total)
extern EncoderThrottle encThrottle[NUM_ENCODERS];

// Call this from loop() / loop1() to flush pending encoder updates
void flush_encoder_throttle();

// Helper to queue or immediately send an encoder parameter
void queue_encoder_param_send(uint8_t encIndex, ParamId paramId, uint16_t wireValue, bool isWord);

#endif
