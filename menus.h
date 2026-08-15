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
  { ACTION_NONE,                TG_ADSR1_RESTART,        ParamId::PARAM_VCA_ADSR_RESTART,    (int8_t*)&VCAADSRRestart },
  { ACTION_NONE,                TG_ADSR2_RESTART,        ParamId::PARAM_VCF_ADSR_RESTART,    (int8_t*)&VCFADSRRestart },
  { ACTION_NONE,                TG_ENABLE_ADSR3,         ParamId::PARAM_ADSR3_ENABLED,       (int8_t*)&ADSR3Enabled },
  { ACTION_NONE,                TG_ADSR3_TO_OSC_SELECT,  ParamId::PARAM_ADSR3_TO_OSC_SELECT, &ADSR3ToOscSelect },
  { ACTION_ADSR3_to_PWM,        BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_TO_PWM,        nullptr },
  { ACTION_ADSR3_to_DETUNE1,    BTN_ACTION_NONE,         ParamId::PARAM_ADSR3_TO_DETUNE1,    nullptr },
};

MenuDef envelopeMenu = {
  envelopeMenuItems,
  (int8_t)(sizeof(envelopeMenuItems) / sizeof(envelopeMenuItems[0])),
};

// The open menu while currentControlMode == MENU_NAVIGATION, else nullptr
// (CALIBRATION_MENU keeps it null: the Screen tracks that one as a tab view).
MenuDef* currentMenu = nullptr;

// encoders.ino: re-send the item's param so the Screen toast shows it.
void menu_announce_item(int8_t pos);

#endif
