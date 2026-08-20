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
  ACTION_osc_phase_sync,
  ACTION_LFO1_speed,
  ACTION_LFO2_speed,
  ACTION_VCA_level,
  ACTION_LFO1_to_VCA,
  ACTION_LFO2_to_PWM,
  ACTION_ADSR3_to_PWM,
  ACTION_ADSR3_to_DETUNE1,

  ACTION_ADSR1_ATTACK_CURVE,
  ACTION_ADSR1_DECAY_CURVE,
  ACTION_ADSR2_ATTACK_CURVE,
  ACTION_ADSR2_DECAY_CURVE,

  ACTION_LFO1_to_OSC1,         
  ACTION_LFO1_to_OSC2,         
  ACTION_LFO2_to_OSC2_coarse,  

  ACTION_select_preset,
  ACTION_select_char,
  ACTION_select_char_pos,
  ACTION_CALIBRATION_OFFSET,
  ACTION_CALIBRATION_STAGE,
  ACTION_MENU_VALUE,
  ACTION_MENU_POS,
};

enum EncoderValType : uint8_t {
  ENC_VAL_I8,   // int8_t
  ENC_VAL_I16,  // int16_t
  ENC_VAL_U16,  // uint16_t
};

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
  EncoderAction actions[3];
  EncoderAction actionsAlt[3];
  EncoderAction actionsAlt2[3];
};

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
  //       pins     actions (normal)                                            actionsAlt (Func 1)                                          actionsAlt2 (Func 2)
  { enc1,  13, 14, { ACTION_LFO1_to_DCO,   ACTION_ADSR3_to_DETUNE1, ACTION_NONE }, { ACTION_ADSR3_to_DETUNE1, ACTION_NONE,               ACTION_NONE }, { ACTION_LFO1_to_OSC1,       ACTION_NONE, ACTION_NONE } },
  { enc2,  10, 11, { ACTION_osc_phase_sync,ACTION_NONE,             ACTION_NONE }, { ACTION_portamento_time,  ACTION_NONE,               ACTION_NONE }, { ACTION_LFO1_to_OSC2,       ACTION_NONE, ACTION_NONE } },
  { enc3,   7,  8, { ACTION_octave,        ACTION_OSC2_interval,    ENC_OSC3_INTERVAL }, { ACTION_OSC2_interval,    ENC_OSC3_INTERVAL,         ACTION_NONE }, { ACTION_ADSR1_ATTACK_CURVE, ACTION_NONE, ACTION_NONE } },
  { enc4,   4,  5, { ACTION_SQR1_level,    ACTION_NONE,             ACTION_NONE }, { ACTION_ANALOG_DETUNE,    ACTION_NONE,               ACTION_NONE }, { ACTION_ADSR1_DECAY_CURVE,  ACTION_NONE, ACTION_NONE } },
  { enc5,   1,  2, { ACTION_OSC2_detune,   ACTION_LFO2_to_OSC2,     ENC_OSC3_DETUNE },   { ACTION_ANALOG_DRIFT,     ACTION_ANALOG_DRIFT_SPEED, ENC_LFO2_TO_OSC3 }, { ACTION_ADSR2_ATTACK_CURVE, ACTION_NONE, ACTION_NONE } },
  { enc6,  30, 31, { ACTION_SQR2_level,    ACTION_NONE,             ACTION_NONE }, { ACTION_VCF_keytrack,     ACTION_ANALOG_DRIFT_SPREAD,ACTION_NONE }, { ACTION_ADSR2_DECAY_CURVE,  ACTION_NONE, ACTION_NONE } },
  { enc7,  27, 28, { ACTION_SUB_level,     ACTION_NONE,             ACTION_NONE }, { ACTION_velocity_to_VCA,  ACTION_NONE,               ACTION_NONE }, { ACTION_NONE,               ACTION_NONE, ACTION_NONE } },
  { enc8,  24, 25, { ACTION_LFO1_speed,    ACTION_select_char,      ACTION_NONE }, { ACTION_velocity_to_VCF,  ACTION_NONE,               ACTION_NONE }, { ACTION_NONE,               ACTION_NONE, ACTION_NONE } },
  { enc9,  21, 22, { ACTION_LFO2_speed,    ACTION_select_char_pos,  ACTION_NONE }, { ACTION_select_preset,    ACTION_select_preset,      ACTION_NONE }, { ACTION_select_preset,      ACTION_select_preset, ACTION_NONE } },
  { enc10, 42, 17, { ACTION_LFO1_to_VCA,   ACTION_NONE,             ACTION_NONE }, { ACTION_VCA_level,        ACTION_NONE,               ACTION_NONE }, { ACTION_NONE,               ACTION_NONE, ACTION_NONE } },
  { enc11, 18, 19, { ACTION_LFO2_to_PWM,   ACTION_NONE,             ACTION_NONE }, { ACTION_ADSR3_to_PWM,     ACTION_NONE,               ACTION_NONE }, { ACTION_LFO2_to_OSC2_coarse,ACTION_NONE, ACTION_NONE } },
};

// Preset Save Mode Tables: only the relevant encoders have actions, all others are ACTION_NONE
EncoderAction presetSaveSelectActions[NUM_ENCODERS] = { 
  ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, 
  ACTION_NONE,           // Enc 8
  ACTION_select_preset,  // Enc 9: scroll slot
  ACTION_NONE, ACTION_NONE 
};

EncoderAction presetSaveNameActions[NUM_ENCODERS] = { 
  ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE,  
  ACTION_select_char,     // Enc 7: change letter
  ACTION_select_char_pos, // Enc 8: change position
  ACTION_NONE,
  ACTION_NONE, ACTION_NONE 
};

static_assert(sizeof(presetSaveSelectActions) / sizeof(presetSaveSelectActions[0]) == NUM_ENCODERS,
              "presetSaveSelectActions[] must have NUM_ENCODERS entries");
static_assert(sizeof(presetSaveNameActions) / sizeof(presetSaveNameActions[0]) == NUM_ENCODERS,
              "presetSaveNameActions[] must have NUM_ENCODERS entries");
              
EncoderAction menuNavigationActions[NUM_ENCODERS] = { ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_NONE, ACTION_MENU_VALUE, ACTION_MENU_POS, ACTION_NONE, ACTION_NONE };

static_assert(sizeof(encoders) / sizeof(encoders[0]) == NUM_ENCODERS,
              "encoders[] must have NUM_ENCODERS entries");

static_assert(sizeof(menuNavigationActions) / sizeof(menuNavigationActions[0]) == NUM_ENCODERS,
              "menuNavigationActions[] must have NUM_ENCODERS entries");

EncoderAction encoderActionSelected = ACTION_NONE;
unsigned long encoderActionSelectedMillis = 0;
unsigned long encoderActionSelectedTimeout = 2000;
bool encoderActionIsSelected = false;

int8_t menuPos = 0;
int8_t menuPosMax = 0;
int8_t menuValue = 0;

void read_encoders();

#endif