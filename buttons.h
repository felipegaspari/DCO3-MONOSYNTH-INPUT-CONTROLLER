#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include "sram_hot.h"
#include "_build_libs/RoxMux_fela/src/RoxMux_fela.h"

#define NUM_BUTTONS 16

// Buttons 0..LATCHABLE_BUTTON_MAX (the wave/sync keys) support hold-to-latch;
// their latch state also picks the latched encoder bank (encoders.ino).
#define LATCHABLE_BUTTON_MAX 6

bool buttonIsLatched[NUM_BUTTONS];

enum ButtonState {
  BTN_STATE_NONE,
  RELEASED,
  PRESSED,
  HELD,
  DOUBLE
};

enum ButtonAction {
  BTN_ACTION_NONE,

  TG_SAW1,
  TG_SAW2,
  TG_TRI,
  TG_SIN,
  TG_SQR1,
  TG_SQR2,

  TG_SYNC_MODE,

  TG_RESO_AMP_COMP,
  TG_ADSR1_RESTART,
  TG_ADSR2_RESTART,
  TG_ADSR3_TO_OSC_SELECT,

  ADSR1_CURVE_SEL,
  ADSR2_CURVE_SEL,

  TG_VOICE_MODE,

  SELECT_LFO_N,
  TG_LFO1_WAVE,
  TG_LFO2_WAVE,

  TG_FUNC,

  WORK_WITH_PRESETS,
  PRESET_SAVE_SELECT_MODE,
  PRESET_SAVE_MODE,
  SAVE_PRESET,

  TG_MAN_FADERS,
  TG_MAN_POTS,
  TG_MAN_FADER_ROW1,
  TG_MAN_FADER_ROW2,
  TG_MANUAL_VCF_POTS,
  TG_MANUAL_VCA_POTS,
  TG_MANUAL_PWM_POTS,
  TG_MANUAL_ALL,
  TG_ENABLE_ADSR3,

  TG_MAN_CALIBRATION,
  TG_CALIBRATION_MENU,
  TG_ENVELOPE_MENU,

  EXIT,
  BACK,
  SELECT,
  CONFIRM,

  SELECT_ENC_ACTION,
};

enum controlMode {
  NORMAL,
  MENU_NAVIGATION,
  CALIBRATION_MENU,
  MANUAL_CALIBRATION
};

controlMode currentControlMode = NORMAL;

struct ButtonStruct {
  RoxButton button_n;
  uint8_t pin;  // pin or mux array position
  ButtonAction actionReleased;
  ButtonAction actionPressed;
  ButtonAction actionHeld;
  ButtonAction actionDouble;
  ButtonAction actionReleasedAlt;
  ButtonAction actionPressedAlt;
  ButtonAction actionHeldAlt;
  ButtonAction actionMenuNavigationReleased;
  ButtonAction actionMenuNavigationHeld;
};

ButtonStruct buttons[NUM_BUTTONS] = {
  //           pin  released              pressed          held                 double           ALT: released              ALT: pressed     ALT: held            menu: released   menu: held
  { button1,  15, TG_SAW1,              BTN_ACTION_NONE, TG_FUNC,             BTN_ACTION_NONE, /* ALT: */ TG_ADSR3_TO_OSC_SELECT,   BTN_ACTION_NONE, TG_FUNC,             BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button2,  12, TG_SQR1,              BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ TG_RESO_AMP_COMP,         BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button3,   9, TG_TRI,               BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ TG_ADSR1_RESTART,         BTN_ACTION_NONE, ADSR1_CURVE_SEL,     BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button4,   6, TG_SAW2,              BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ TG_ADSR2_RESTART,         BTN_ACTION_NONE, ADSR2_CURVE_SEL,     BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button5,   3, TG_SQR2,              BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ TG_VOICE_MODE,            BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button6,   0, TG_SYNC_MODE,         BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ BTN_ACTION_NONE,          BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button7,  29, BTN_ACTION_NONE,      BTN_ACTION_NONE, TG_ENVELOPE_MENU,    BTN_ACTION_NONE, /* ALT: */ BTN_ACTION_NONE,          BTN_ACTION_NONE, TG_ENVELOPE_MENU,    BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button8,  26, TG_LFO1_WAVE,         BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ BTN_ACTION_NONE,          BTN_ACTION_NONE, TG_ENABLE_ADSR3,     BACK,            EXIT },
  { button9,  23, TG_LFO2_WAVE,         BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ PRESET_SAVE_SELECT_MODE,  BTN_ACTION_NONE, SAVE_PRESET,         SELECT,          CONFIRM },
  { button10, 16, SELECT_ENC_ACTION,    BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ SELECT_ENC_ACTION,        BTN_ACTION_NONE, TG_MAN_FADERS,       BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button11, 20, SELECT_ENC_ACTION,    BTN_ACTION_NONE, TG_CALIBRATION_MENU, BTN_ACTION_NONE, /* ALT: */ SELECT_ENC_ACTION,        BTN_ACTION_NONE, TG_CALIBRATION_MENU, BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button12, 43, TG_MANUAL_VCF_POTS,   BTN_ACTION_NONE, TG_MAN_POTS,         BTN_ACTION_NONE, /* ALT: */ TG_MANUAL_VCF_POTS,       BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button13, 44, TG_MANUAL_VCA_POTS,   BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ TG_MANUAL_VCA_POTS,       BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button14, 45, TG_MANUAL_PWM_POTS,   BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ TG_MANUAL_PWM_POTS,       BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button15, 46, TG_MAN_FADER_ROW2,    BTN_ACTION_NONE, TG_ENABLE_ADSR3,     BTN_ACTION_NONE, /* ALT: */ TG_MAN_FADER_ROW2,        BTN_ACTION_NONE, TG_ENABLE_ADSR3,     BTN_ACTION_NONE, BTN_ACTION_NONE },
  { button16, 47, TG_MAN_FADER_ROW1,    BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, /* ALT: */ TG_MAN_FADER_ROW1,        BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE },
};

static_assert(sizeof(buttons) / sizeof(buttons[0]) == NUM_BUTTONS,
              "buttons[] must have NUM_BUTTONS entries");

// First-press selection tracking (same idea as the encoders' first detent:
// the first press of a retargeted key only shows state, the next one acts).
ButtonAction buttonActionSelected = BTN_ACTION_NONE;
unsigned long buttonActionSelectedMillis = 0;
unsigned long buttonActionSelectedTimeout = 2000;
bool buttonActionIsSelected = false;

void read_encoder_buttons();
// Defined in buttons.ino; also called from encoders.ino (menu toggle items).
static void execute_button_action(ButtonAction action);
void handleLatchedButton(int i);
ButtonAction handleHeldButton(int i);
ButtonAction handleDoublePressedButton(int i);
ButtonAction handlePressedButton(int i);
ButtonAction handleReleasedButton(int i);
void handleUnlatchedButton(int i);

#endif
