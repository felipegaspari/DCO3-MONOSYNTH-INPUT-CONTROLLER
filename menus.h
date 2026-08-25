#ifndef __MENUS_H__
#define __MENUS_H__

// --- MENU_NAVIGATION framework -------------------------------------------------
//
// One controlMode (MENU_NAVIGATION) serves every toast-driven menu; the open
// menu is currentMenu. Encoder 9 (ACTION_MENU_POS) scrolls menuPos and
// re-announces the selected item on the Screen's param toast; encoder 8
// (ACTION_MENU_VALUE) edits it. Button 9 SELECT/CONFIRM fires toggle items,
// button 8 BACK/EXIT returns to NORMAL. Menus are entered from a ButtonAction
// (e.g. TG_ENVELOPE_MENU on button 7 held). See docs/MENUS.md.
//
// Each item is one of:
//  - value item:  encAction != ACTION_NONE, edited through the
//    encoderParamBindings[] table (encoders.ino), which also handles the
//    announce (param re-send) when scrolling onto it;
//  - toggle item: btnAction != BTN_ACTION_NONE, fired through
//    execute_button_action() (buttons.ino); announceParam/stateValue tell the
//    scroll announce what to show.

struct MenuItem {
  EncoderAction encAction;  // ACTION_NONE for toggle items
  ButtonAction btnAction;   // BTN_ACTION_NONE for value items
  ParamId announceParam;    // toggle items: param re-sent on scroll
  int8_t* stateValue;       // toggle items: current state read on scroll
};

struct MenuDef {
  const MenuItem* items;
  int8_t count;
};

// --- envelope menu ---------------------------------------------------------------
//
// Non-const so the table lives in .data (RAM), same as encoderParamBindings[].
MenuItem envelopeMenuItems[] = {
  //  encoder action (value)      button action (toggle)   announce param                       state
  { ACTION_ADSR1_ATTACK_CURVE,  BTN_ACTION_NONE,         ParamId::PARAM_ADSR1_ATTACK_CURVE,  nullptr },
  { ACTION_ADSR1_DECAY_CURVE,   BTN_ACTION_NONE,         ParamId::PARAM_ADSR1_DECAY_CURVE,   nullptr },
  { ACTION_ADSR2_ATTACK_CURVE,  BTN_ACTION_NONE,         ParamId::PARAM_ADSR2_ATTACK_CURVE,  nullptr },
  { ACTION_ADSR2_DECAY_CURVE,   BTN_ACTION_NONE,         ParamId::PARAM_ADSR2_DECAY_CURVE,   nullptr },
  { ACTION_NONE,                TG_ADSR1_RESTART,        ParamId::PARAM_ADSR1_RESTART,    (int8_t*)&ADSR1Restart },
  { ACTION_NONE,                TG_ADSR2_RESTART,        ParamId::PARAM_ADSR2_RESTART,    (int8_t*)&ADSR2Restart },
  { ACTION_NONE,                TG_ENABLE_ADSR3,         ParamId::PARAM_ADSR3_ENABLED,       (int8_t*)&ADSR3Enabled },
  { ACTION_NONE,                TG_ADSR3_TO_OSC_SELECT,  ParamId::PARAM_ADSR3_TO_OSC_SELECT, &ADSR3ToOscSelect },
  { ACTION_ADSR3_to_PWM,        BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_TO_PWM,        nullptr },
  { ACTION_ADSR3_to_DETUNE1,    BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_TO_DETUNE1,    nullptr },
};

MenuDef envelopeMenu = {
  envelopeMenuItems,
  (int8_t)(sizeof(envelopeMenuItems) / sizeof(envelopeMenuItems[0])),
};

// --- Mode 2: ADSR 1 ---
MenuItem adsr1MenuArray[] = {
  { ACTION_ADSR1_ATTACK_CURVE,  BTN_ACTION_NONE,  ParamId::PARAM_ADSR1_ATTACK_CURVE,  nullptr },
  { ACTION_ADSR1_DECAY_CURVE,   BTN_ACTION_NONE,  ParamId::PARAM_ADSR1_DECAY_CURVE,   nullptr },
  { ACTION_ADSR1_RELEASE_CURVE, BTN_ACTION_NONE,  ParamId::PARAM_ADSR1_RELEASE_CURVE, nullptr },
  { ACTION_ADSR1_RESTART,       TG_ADSR1_RESTART, ParamId::PARAM_ADSR1_RESTART,       (int8_t*)&ADSR1Restart },
  { ACTION_ADSR1_MODE,          TG_ADSR1_MODE,    ParamId::PARAM_ADSR1_MODE,          (int8_t*)&ADSR1Mode }
};
MenuDef adsr1Menu = { adsr1MenuArray, 5 };

// --- Mode 3: ADSR 2 ---
MenuItem adsr2MenuArray[] = {
  { ACTION_ADSR2_ATTACK_CURVE,  BTN_ACTION_NONE,  ParamId::PARAM_ADSR2_ATTACK_CURVE,  nullptr },
  { ACTION_ADSR2_DECAY_CURVE,   BTN_ACTION_NONE,  ParamId::PARAM_ADSR2_DECAY_CURVE,   nullptr },
  { ACTION_ADSR2_RELEASE_CURVE, BTN_ACTION_NONE,  ParamId::PARAM_ADSR2_RELEASE_CURVE, nullptr },
  { ACTION_ADSR2_RESTART,       TG_ADSR2_RESTART, ParamId::PARAM_ADSR2_RESTART,       (int8_t*)&ADSR2Restart },
  { ACTION_ADSR2_MODE,          TG_ADSR2_MODE,    ParamId::PARAM_ADSR2_MODE,          (int8_t*)&ADSR2Mode }
};
MenuDef adsr2Menu = { adsr2MenuArray, 5 };

// --- Mode 4: ADSR 3 ---
MenuItem adsr3MenuArray[] = {
  { ACTION_ADSR3_ATTACK_CURVE,  BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_ATTACK_CURVE,  nullptr },
  { ACTION_ADSR3_DECAY_CURVE,   BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_DECAY_CURVE,   nullptr },
  { ACTION_ADSR3_RELEASE_CURVE, BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_RELEASE_CURVE, nullptr },
  { ACTION_ADSR3_RESTART,       BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_RESTART,       (int8_t*)&ADSR3Restart },
  { ACTION_ADSR3_MODE,          BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_MODE,          (int8_t*)&ADSR3Mode },
  { ACTION_ADSR3_to_OSC_SELECT, BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_TO_OSC_SELECT, &ADSR3ToOscSelect },
  { ACTION_ADSR3_to_PWM,        BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_TO_PWM,        nullptr },
  { ACTION_ADSR3_to_DETUNE1,    BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_TO_DETUNE1,    nullptr },
  { ACTION_NONE,                TG_ENABLE_ADSR3,         ParamId::PARAM_ADSR3_ENABLED,       (int8_t*)&ADSR3Enabled }
};
MenuDef adsr3Menu = { adsr3MenuArray, 9 };

// --- Mode 6: DCO MENU ---
#if PROJECT_INSTRUMENT == 3 
MenuItem dcoMenuArray[] = {
  { ACTION_VOICE_MODE,       BTN_ACTION_NONE, ParamId::PARAM_VOICE_MODE,       nullptr },
  { ACTION_VOICE_ALLOC_MODE, BTN_ACTION_NONE, ParamId::PARAM_VOICE_ALLOC_MODE, nullptr },
  { ACTION_octave,           BTN_ACTION_NONE, ParamId::PARAM_OSC1_INTERVAL,    nullptr },
  { ACTION_OSC2_interval,    BTN_ACTION_NONE, ParamId::PARAM_OSC2_INTERVAL,    nullptr },
  { ACTION_OSC3_interval,    BTN_ACTION_NONE, ParamId::PARAM_OSC3_INTERVAL,    nullptr },
  { ACTION_osc_phase_sync,   BTN_ACTION_NONE, ParamId::PARAM_OSC_PHASE_SYNC,   nullptr },
  { ACTION_SYNC_MODE,        BTN_ACTION_NONE, ParamId::PARAM_SYNC_MODE,        nullptr },
  { ACTION_SOFT_SYNC,        BTN_ACTION_NONE, ParamId::PARAM_SOFT_SYNC,        nullptr }
};
MenuDef dcoMenu = { dcoMenuArray, 8 };
#else
MenuItem dcoMenuArray[] = {
  { ACTION_VOICE_MODE,       BTN_ACTION_NONE, ParamId::PARAM_VOICE_MODE,       nullptr },
  { ACTION_VOICE_ALLOC_MODE, BTN_ACTION_NONE, ParamId::PARAM_VOICE_ALLOC_MODE, nullptr },
  { ACTION_octave,           BTN_ACTION_NONE, ParamId::PARAM_OSC1_INTERVAL,    nullptr },
  { ACTION_OSC2_interval,    BTN_ACTION_NONE, ParamId::PARAM_OSC2_INTERVAL,    nullptr },
  { ACTION_osc_phase_sync,   BTN_ACTION_NONE, ParamId::PARAM_OSC_PHASE_SYNC,   nullptr },
  { ACTION_SYNC_MODE,        BTN_ACTION_NONE, ParamId::PARAM_SYNC_MODE,        nullptr },
  { ACTION_SOFT_SYNC,        BTN_ACTION_NONE, ParamId::PARAM_SOFT_SYNC,        nullptr }
};
MenuDef dcoMenu = { dcoMenuArray, 7 };
#endif

// --- Mode 7: DCO MODULATION (Detunes, Slew, Sub & LFO Pitch Routes) ---
#if INPUT_HAS_OSC3_PANEL
MenuItem dcoModMenuArray[] = {
  // Original Pitch, Slew & Sub parameters
  { ACTION_OSC2_detune,         BTN_ACTION_NONE, ParamId::PARAM_OSC2_DETUNE_VAL,     nullptr },
  { ACTION_OSC3_detune,         BTN_ACTION_NONE, ParamId::PARAM_OSC3_DETUNE_VAL,     nullptr },
  { ACTION_ANALOG_DETUNE,       BTN_ACTION_NONE, ParamId::PARAM_UNISON_DETUNE,       nullptr },
  { ACTION_portamento_time,     BTN_ACTION_NONE, ParamId::PARAM_PORTAMENTO_TIME,     nullptr },
  { ACTION_PORTAMENTO_MODE,     BTN_ACTION_NONE, ParamId::PARAM_PORTAMENTO_MODE,     nullptr },
  { ACTION_SUBOSC_DIVIDE,       BTN_ACTION_NONE, ParamId::PARAM_SUBOSC_DIVIDE,       nullptr },

  // Added LFO Pitch Routes
  { ACTION_LFO1_to_DCO,         BTN_ACTION_NONE, ParamId::PARAM_LFO1_TO_DCO,         nullptr },
  { ACTION_LFO1_to_OSC1,        BTN_ACTION_NONE, ParamId::PARAM_LFO1_TO_OSC1,        nullptr },
  { ACTION_LFO1_to_OSC2,        BTN_ACTION_NONE, ParamId::PARAM_LFO1_TO_OSC2,        nullptr },
  { ACTION_LFO1_to_OSC3,        BTN_ACTION_NONE, ParamId::PARAM_LFO1_TO_OSC3,        nullptr },
  { ACTION_LFO2_to_OSC2,        BTN_ACTION_NONE, ParamId::PARAM_LFO2_TO_OSC2,        nullptr },
  { ACTION_LFO2_to_OSC3,        BTN_ACTION_NONE, ParamId::PARAM_LFO2_TO_OSC3,        nullptr },
  { ACTION_LFO2_to_OSC2_coarse, BTN_ACTION_NONE, ParamId::PARAM_LFO2_TO_OSC2_COARSE, nullptr },
  { ACTION_LFO2_to_OSC3_coarse, BTN_ACTION_NONE, ParamId::PARAM_LFO2_TO_OSC3_COARSE, nullptr }
};
MenuDef dcoModMenu = { dcoModMenuArray, 14 };
#else
MenuItem dcoModMenuArray[] = {
  // Original Pitch & Slew parameters
  { ACTION_OSC2_detune,         BTN_ACTION_NONE, ParamId::PARAM_OSC2_DETUNE_VAL,     nullptr },
  { ACTION_ANALOG_DETUNE,       BTN_ACTION_NONE, ParamId::PARAM_UNISON_DETUNE,       nullptr },
  { ACTION_portamento_time,     BTN_ACTION_NONE, ParamId::PARAM_PORTAMENTO_TIME,     nullptr },
  { ACTION_PORTAMENTO_MODE,     BTN_ACTION_NONE, ParamId::PARAM_PORTAMENTO_MODE,     nullptr },

  // Added LFO Pitch Routes
  { ACTION_LFO1_to_DCO,         BTN_ACTION_NONE, ParamId::PARAM_LFO1_TO_DCO,         nullptr },
  { ACTION_LFO1_to_OSC1,        BTN_ACTION_NONE, ParamId::PARAM_LFO1_TO_OSC1,        nullptr },
  { ACTION_LFO1_to_OSC2,        BTN_ACTION_NONE, ParamId::PARAM_LFO1_TO_OSC2,        nullptr },
  { ACTION_LFO2_to_OSC2,        BTN_ACTION_NONE, ParamId::PARAM_LFO2_TO_OSC2,        nullptr },
  { ACTION_LFO2_to_OSC2_coarse, BTN_ACTION_NONE, ParamId::PARAM_LFO2_TO_OSC2_COARSE, nullptr }
};
MenuDef dcoModMenu = { dcoModMenuArray, 9 };
#endif

// --- Mode 8: MOD MATRIX MENU (Stub) ---
MenuItem modMatrixMenuArray[] = {
  { ACTION_NONE, BTN_ACTION_NONE, ParamId::PARAM_SINE_STATUS, nullptr }
};
MenuDef modMatrixMenu = { modMatrixMenuArray, 1 };

// --- Placeholder Menu ---
MenuItem placeholderMenuArray[] = {
  { ACTION_NONE, BTN_ACTION_NONE, ParamId::PARAM_SINE_STATUS, nullptr } // Dummy item
};
MenuDef placeholderMenu = { placeholderMenuArray, 1 };

// The open menu while currentControlMode == MENU_NAVIGATION, else nullptr
// (CALIBRATION_MENU keeps it null: the Screen tracks that one as a tab view).
MenuDef* currentMenu = nullptr;

// encoders.ino: re-send the item's param so the Screen toast shows it.
void menu_announce_item(int8_t pos);

#endif
