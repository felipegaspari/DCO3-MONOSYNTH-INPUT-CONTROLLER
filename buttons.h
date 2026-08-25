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
enum controlMode {
  NORMAL,
  MENU_NAVIGATION,
  CALIBRATION_MENU,
  PRESET_SAVE_SELECT,
  PRESET_SAVE_NAME
};

controlMode currentControlMode = NORMAL;

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
  TG_ADSR3_RESTART,
  TG_ADSR3_TO_OSC_SELECT,

  // ADSR1_CURVE_SEL, // <-- Removed! Handled directly on Encoders now
  // ADSR2_CURVE_SEL, // <-- Removed! Handled directly on Encoders now

  TG_VOICE_MODE,
  TG_VOICE_ALLOC_MODE, // <-- NEW
  TG_SOFT_SYNC,        // <-- NEW
  TG_PORTAMENTO_MODE,  // <-- NEW
  TG_ADSR1_MODE,
  TG_ADSR2_MODE,
  TG_ADSR3_MODE,

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

  TG_DCO_MENU,
  TG_DCO_MOD_MENU,
  TG_MOD_MATRIX_MENU,
  TG_ADSR1_MENU,
  TG_ADSR2_MENU,
  TG_ADSR3_MENU,
  TG_PLACEHOLDER_MENU,

  EXIT,
  BACK,
  SELECT,
  CONFIRM,

  SELECT_ENC_ACTION,
};

struct ButtonStruct {
  RoxButton button_n;
  uint8_t pin;
  ButtonAction actionReleased;
  ButtonAction actionPressed;
  ButtonAction actionHeld;
  ButtonAction actionDouble;
  ButtonAction actionReleasedAlt;
  ButtonAction actionPressedAlt;
  ButtonAction actionHeldAlt;
  ButtonAction actionReleasedAlt2;
  ButtonAction actionPressedAlt2;
  ButtonAction actionHeldAlt2;
  ButtonAction actionMenuNavigationReleased;
  ButtonAction actionMenuNavigationHeld;
};

ButtonStruct buttons[NUM_BUTTONS] = {
  //           pin  rel           press            held                 dbl              ALT: rel                   ALT: press       ALT: held             ALT2: rel            ALT2: press      ALT2: held           menu: rel        menu: held
  { {},  15, TG_SAW1,              BTN_ACTION_NONE, TG_FUNC,             BTN_ACTION_NONE, TG_ADSR3_TO_OSC_SELECT,    BTN_ACTION_NONE, TG_FUNC,             TG_DCO_MENU,         BTN_ACTION_NONE, TG_FUNC,             TG_DCO_MENU, BTN_ACTION_NONE },
  { {},  12, TG_SQR1,              BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, TG_RESO_AMP_COMP,          BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_DCO_MOD_MENU,     BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_DCO_MOD_MENU, BTN_ACTION_NONE },
  { {},   9, TG_TRI,               BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, TG_ADSR1_RESTART,          BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_MOD_MATRIX_MENU,  BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_MOD_MATRIX_MENU, BTN_ACTION_NONE },
  { {},   6, TG_SAW2,              BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, TG_ADSR2_RESTART,          BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_ADSR1_MENU,       BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_ADSR1_MENU, BTN_ACTION_NONE },
  { {},   3, TG_SQR2,              BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, TG_VOICE_MODE,             BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_ADSR2_MENU,       BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_ADSR2_MENU, BTN_ACTION_NONE },
  { {},   0, TG_SYNC_MODE,         BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE,           BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_ADSR3_MENU,       BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_ADSR3_MENU, BTN_ACTION_NONE },
  { {},  29, BTN_ACTION_NONE,      BTN_ACTION_NONE, TG_ENVELOPE_MENU,    BTN_ACTION_NONE, BTN_ACTION_NONE,           BTN_ACTION_NONE, TG_ENVELOPE_MENU,    TG_PLACEHOLDER_MENU, BTN_ACTION_NONE, TG_ENVELOPE_MENU,    TG_PLACEHOLDER_MENU, BTN_ACTION_NONE },
  { {},  26, TG_LFO1_WAVE,         BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE,           BTN_ACTION_NONE, TG_ENABLE_ADSR3,     BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE,     BACK,            EXIT },
  { {},  23, TG_LFO2_WAVE,         BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, PRESET_SAVE_SELECT_MODE,   BTN_ACTION_NONE, SAVE_PRESET,         BTN_ACTION_NONE,     BTN_ACTION_NONE, BTN_ACTION_NONE,     SELECT,          CONFIRM },
  { {},  16, SELECT_ENC_ACTION,    BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, SELECT_ENC_ACTION,         BTN_ACTION_NONE, TG_MAN_FADERS,       SELECT_ENC_ACTION,   BTN_ACTION_NONE, TG_MAN_FADERS,       BTN_ACTION_NONE, BTN_ACTION_NONE },
  { {},  20, SELECT_ENC_ACTION,    BTN_ACTION_NONE, TG_CALIBRATION_MENU, BTN_ACTION_NONE, SELECT_ENC_ACTION,         BTN_ACTION_NONE, TG_CALIBRATION_MENU, SELECT_ENC_ACTION,   BTN_ACTION_NONE, TG_CALIBRATION_MENU, BTN_ACTION_NONE, BTN_ACTION_NONE },
  { {},  43, TG_MANUAL_VCF_POTS,   BTN_ACTION_NONE, TG_MAN_POTS,         BTN_ACTION_NONE, TG_MANUAL_VCF_POTS,        BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_MANUAL_VCF_POTS,  BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_MANUAL_VCF_POTS, BTN_ACTION_NONE },
  { {},  44, TG_MANUAL_VCA_POTS,   BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, TG_MANUAL_VCA_POTS,        BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_MANUAL_VCA_POTS,  BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_MANUAL_VCA_POTS, BTN_ACTION_NONE },
  { {},  45, TG_MANUAL_PWM_POTS,   BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, TG_MANUAL_PWM_POTS,        BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_MANUAL_PWM_POTS,  BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_MANUAL_PWM_POTS, BTN_ACTION_NONE },
  { {},  46, TG_MAN_FADER_ROW2,    BTN_ACTION_NONE, TG_ENABLE_ADSR3,     BTN_ACTION_NONE, TG_MAN_FADER_ROW2,         BTN_ACTION_NONE, TG_ENABLE_ADSR3,     TG_MAN_FADER_ROW2,   BTN_ACTION_NONE, TG_ENABLE_ADSR3,     TG_MAN_FADER_ROW2, BTN_ACTION_NONE },
  { {},  47, TG_MAN_FADER_ROW1,    BTN_ACTION_NONE, BTN_ACTION_NONE,     BTN_ACTION_NONE, TG_MAN_FADER_ROW1,         BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_MAN_FADER_ROW1,   BTN_ACTION_NONE, BTN_ACTION_NONE,     TG_MAN_FADER_ROW1, BTN_ACTION_NONE },
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
