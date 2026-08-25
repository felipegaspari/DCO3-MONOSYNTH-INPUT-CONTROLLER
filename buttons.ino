#include "include_all.h"

// Flip one of the five panel wave keys and mirror it to the DCO and its LED.
// The key index is also the LED index (see board_model.h).
static void toggle_wave_key(uint8_t key) {
  const InputWaveKey &k = inputWaveKeys[key];
  waveEnable[k.osc][k.wave] = !waveEnable[k.osc][k.wave];
  serial_send_param_change_byte(input_wave_key_param_id(k.osc, k.wave),
                                waveEnable[k.osc][k.wave]);
  set_LED_Status(key, waveEnable[k.osc][k.wave]);
}

// --- preset save flow
// ---------------------------------------------------------
//

// --- calibration menu
// -----------------------------------------------------------
//
// CALIBRATION_MENU entries in menuPos order, following the Screen's calibration
// tabs. calibrationFlag picks the stage on the DCO (PARAM_CALIBRATION_FLAG);
// followUp instead re-dispatches a button action (e.g. enter manual cal).
// =============================================================================
// CALIBRATION MENU ACTIONS & HANDLERS
// =============================================================================

// Option 0: Auto Calibration (Amp-Comp Tables)
static void cal_action_auto_amp_comp() {
  // Tell the DCO to run the automated amplitude compensation routine
  serial_send_param_change_byte(ParamId::PARAM_CALIBRATION_FLAG, 9,
                                /*sendToAll=*/true);

  // (Optional) Send a UI signal or toast to the Screen if needed
  // serial_send_signal(SIGNAL_CALIBRATION_RUNNING);
}

// Option 1: Pulse Width (PW) Calibration
static void cal_action_pw_calibration() {
  // Tell the DCO to run PW center & limit calibration
  serial_send_param_change_byte(ParamId::PARAM_CALIBRATION_FLAG, 2,
                                /*sendToAll=*/true);

  // (Optional) Send any custom PW test parameters here
}

// Option 2: Full Calibration Routine (PW + Amp-Comp)
static void cal_action_full_routine() {
  // Tell the DCO to run the full calibration sequence
  serial_send_param_change_byte(ParamId::PARAM_CALIBRATION_FLAG, 3,
                                /*sendToAll=*/true);
}

// Option 3: Refine Amp-Comp Tables
static void cal_action_refine_amp_comp() {
  // Tell the DCO to run the refine amplitude compensation routine
  serial_send_param_change_byte(ParamId::PARAM_CALIBRATION_FLAG, 5,
                                /*sendToAll=*/true);
}

// Option 4: Manual Calibration Flow
// Option 3: Manual Calibration Flow
static void cal_action_manual_flow() {
  // Launch the flow directly! (The Flow manages the screen and mode
  // transitions)
  enterFlow(&calibrationFlow);
}

// The Menu Table mapping menuPos (0..3) to the functions above
struct CalibrationMenuOption {
  void (*handler)();
};

static const CalibrationMenuOption calibrationMenu[] = {
    {cal_action_auto_amp_comp},   // Pos 0
    {cal_action_pw_calibration},  // Pos 1
    {cal_action_full_routine},    // Pos 2
    {cal_action_manual_flow},     // Pos 3
    {cal_action_refine_amp_comp}, // Pos 4
};

static constexpr int8_t CALIBRATION_MENU_POS_MAX =
    (int8_t)(sizeof(calibrationMenu) / sizeof(calibrationMenu[0]) - 1);

// Leave manual calibration mode (without persisting anything by itself).
static void exit_manual_calibration(controlMode returnTo) {
  currentControlMode = returnTo;
  manualCalibration = false;
  serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_FLAG,
                                manualCalibration);
}

// Mode-specific EXIT/BACK/SELECT/CONFIRM handling. Consumes the action (returns
// BTN_ACTION_NONE) or maps it to a follow-up for execute_button_action().
static ButtonAction
__not_in_flash_func(handle_mode_buttons)(ButtonAction action) {
  switch (currentControlMode) {

  case CALIBRATION_MENU:
    switch (action) {
    case EXIT:
    case BACK:
      // User clicked Exit/Back (Button 8): Close menu
      currentControlMode = NORMAL;
      serial_send_param_change_byte(ParamId::PARAM_UI_CALIBRATION_DISMISS, 0);
      return BTN_ACTION_NONE;

    case SELECT:
    case CONFIRM:
      // User clicked Select (Button 9): Run the chosen calibration action!
      if (menuPos >= 0 && menuPos <= CALIBRATION_MENU_POS_MAX) {
        calibrationMenu[menuPos].handler();
      }
      return BTN_ACTION_NONE;

    default:
      break;
    }
    break;

  case MENU_NAVIGATION:
    switch (action) {
    case EXIT:
    case BACK:
      currentControlMode = NORMAL;
      currentMenu = nullptr;
      serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE, 0);
      return BTN_ACTION_NONE;
    case SELECT:
    case CONFIRM:
      if (currentMenu != nullptr && menuPos >= 0 &&
          menuPos < currentMenu->count) {
        ButtonAction btnAct = currentMenu->items[menuPos].btnAction;
        if (btnAct != BTN_ACTION_NONE) {
          return btnAct;
        }
      }
      return BTN_ACTION_NONE;
    default:
      break;
    }
    break;

  default:
    break;
  }
  return action;
}

// Execute one resolved ButtonAction (mode transitions already handled).
static void __not_in_flash_func(execute_button_action)(ButtonAction action) {
  switch (action) {

  // The five wave keys differ only in which oscillator they toggle, which
  // is a per-panel fact held in board_model.h's INPUT_WAVE_KEY_TABLE.
  case TG_SAW1:
    toggle_wave_key(0);
    break;
  case TG_SQR1:
    toggle_wave_key(1);
    break;
  case TG_TRI:
    toggle_wave_key(2);
    break;
  case TG_SAW2:
    toggle_wave_key(3);
    break;
  case TG_SQR2:
    toggle_wave_key(4);
    break;

  case TG_RESO_AMP_COMP:
    if (buttonActionIsSelected) {
      RESONANCEAmpCompensation = !RESONANCEAmpCompensation;
    }
    serial_send_param_change_byte(ParamId::PARAM_RESONANCE_COMPENSATION,
                                  RESONANCEAmpCompensation);
    break;

    case TG_ADSR1_RESTART:
    if (buttonActionIsSelected || currentControlMode == MENU_NAVIGATION) {
      ADSR1Restart = !ADSR1Restart;
    }
    serial_send_param_change_byte(ParamId::PARAM_ADSR1_RESTART, ADSR1Restart);
    break;

  case TG_ADSR2_RESTART:
    if (buttonActionIsSelected || currentControlMode == MENU_NAVIGATION) {
      ADSR2Restart = !ADSR2Restart;
    }
    serial_send_param_change_byte(ParamId::PARAM_ADSR2_RESTART, ADSR2Restart);
    break;

  case TG_ENABLE_ADSR3:
    if (buttonActionIsSelected || currentControlMode == MENU_NAVIGATION) {
      ADSR3Enabled = !ADSR3Enabled;
      faderRow2ControlManual = false;
    }
    serial_send_param_change_byte(ParamId::PARAM_ADSR3_ENABLED, (uint8_t)ADSR3Enabled);
    LED_Control_Mux.blinkPin(LEDPins[11], ADSR3Enabled);
    set_LED_Status(11, ADSR3Enabled);
    break;

    case TG_ADSR1_MODE:
    if (buttonActionIsSelected || currentControlMode == MENU_NAVIGATION) {
      ADSR1Mode = (ADSR1Mode + 1) % 3;
    }
    serial_send_param_change_byte(ParamId::PARAM_ADSR1_MODE, ADSR1Mode);
    break;

  case TG_ADSR2_MODE:
    if (buttonActionIsSelected || currentControlMode == MENU_NAVIGATION) {
      ADSR2Mode = (ADSR2Mode + 1) % 3;
    }
    serial_send_param_change_byte(ParamId::PARAM_ADSR2_MODE, ADSR2Mode);
    break;

  case TG_ADSR3_MODE:
    if (buttonActionIsSelected || currentControlMode == MENU_NAVIGATION) {
      ADSR3Mode = (ADSR3Mode + 1) % 3;
    }
    serial_send_param_change_byte(ParamId::PARAM_ADSR3_MODE, ADSR3Mode);
    break;

  case TG_ADSR3_TO_OSC_SELECT:
    if (buttonActionIsSelected || currentControlMode == MENU_NAVIGATION) {
      ADSR3ToOscSelect++;
      if (ADSR3ToOscSelect > INPUT_ADSR3_TO_OSC_SELECT_MAX) {
        ADSR3ToOscSelect = 0;
      }
    }
    serial_send_param_change_byte(ParamId::PARAM_ADSR3_TO_OSC_SELECT, ADSR3ToOscSelect);
    break;

  case TG_LFO1_WAVE:
    if (buttonActionIsSelected) {
      LFO1Waveform++;
      if (LFO1Waveform > 9) {
        LFO1Waveform = 1;
      }
    }
    serial_send_param_change_byte(ParamId::PARAM_LFO1_WAVEFORM, LFO1Waveform);
    break;

  case TG_LFO2_WAVE:
    if (buttonActionIsSelected) {
      LFO2Waveform++;
      if (LFO2Waveform > 9) {
        LFO2Waveform = 1;
      }
    }
    serial_send_param_change_byte(ParamId::PARAM_LFO2_WAVEFORM, LFO2Waveform);
    break;

  case TG_VOICE_MODE:
    if (buttonActionIsSelected) {
      voiceMode++;
      if (voiceMode > 2) {
        voiceMode = 0;
      }
    }
    serial_send_param_change_byte(ParamId::PARAM_VOICE_MODE, voiceMode);
    break;

  case TG_FUNC:
    funcKeyMode++;
    if (funcKeyMode > 2)
      funcKeyMode = 0;
    serial_send_param_change_byte(ParamId::PARAM_FUNCTION_KEY, funcKeyMode);
    break;

  case TG_VOICE_ALLOC_MODE:
    if (buttonActionIsSelected) {
      voiceAllocMode++;
      if (voiceAllocMode > 5)
        voiceAllocMode = 0;
    }
    serial_send_param_change_byte(ParamId::PARAM_VOICE_ALLOC_MODE,
                                  voiceAllocMode);
    break;

  case TG_SOFT_SYNC:
    if (buttonActionIsSelected) {
      softSync++;
      if (softSync > 3)
        softSync = 0;
    }
    serial_send_param_change_byte(ParamId::PARAM_SOFT_SYNC, softSync);
    break;

  case TG_PORTAMENTO_MODE:
    if (buttonActionIsSelected) {
      portamentoMode = !portamentoMode;
    }
    serial_send_param_change_byte(ParamId::PARAM_PORTAMENTO_MODE,
                                  portamentoMode);
    break;

  case TG_MAN_FADERS:
    faderControlManual = !faderControlManual;
    faderRow1ControlManual = faderControlManual;
    faderRow2ControlManual = faderControlManual;
    serial_send_param_change_byte(ParamId::PARAM_FADERS_CONTROL_MANUAL,
                                  faderControlManual);
    break;

  case TG_MAN_FADER_ROW1:
    faderRow1ControlManual = !faderRow1ControlManual;
    serial_send_param_change_byte(ParamId::PARAM_FADER_ROW1_CONTROL_MANUAL,
                                  faderRow1ControlManual);
    set_LED_Status(10, faderRow1ControlManual);
    break;

  case TG_MAN_FADER_ROW2:
    faderRow2ControlManual = !faderRow2ControlManual;
    serial_send_param_change_byte(ParamId::PARAM_FADER_ROW2_CONTROL_MANUAL,
                                  faderRow2ControlManual);
    set_LED_Status(11, faderRow2ControlManual);
    break;

  case TG_MAN_POTS:
    potsControlManual = !potsControlManual;
    VCFPotsControlManual = potsControlManual;
    VCAPotsControlManual = potsControlManual;
    PWMPotsControlManual = potsControlManual;
    serial_send_param_change_byte(ParamId::PARAM_POTS_CONTROL_MANUAL,
                                  potsControlManual);
    break;

  case TG_MANUAL_VCF_POTS:
    VCFPotsControlManual = !VCFPotsControlManual;
    serial_send_param_change_byte(ParamId::PARAM_VCF_POTS_CONTROL_MANUAL,
                                  VCFPotsControlManual);
    set_LED_Status(7, VCFPotsControlManual);
    break;

  case TG_MANUAL_VCA_POTS:
    VCAPotsControlManual = !VCAPotsControlManual;
    serial_send_param_change_byte(ParamId::PARAM_VCA_POTS_CONTROL_MANUAL,
                                  VCAPotsControlManual);
    set_LED_Status(8, VCAPotsControlManual);
    break;

  case TG_MANUAL_PWM_POTS:
    PWMPotsControlManual = !PWMPotsControlManual;
    serial_send_param_change_byte(ParamId::PARAM_PWM_POTS_CONTROL_MANUAL,
                                  PWMPotsControlManual);
    set_LED_Status(9, PWMPotsControlManual);
    break;

  case TG_MANUAL_ALL:
    allControlsManual = !allControlsManual;
    VCFPotsControlManual = allControlsManual;
    faderRow2ControlManual = allControlsManual;
    faderRow1ControlManual = allControlsManual;
    PWMPotsControlManual = allControlsManual;
    serial_send_param_change_byte(ParamId::PARAM_ALL_CONTROLS_MANUAL,
                                  allControlsManual);
    break;

  case TG_SYNC_MODE:
    if (buttonActionIsSelected) {
      syncMode++;
      if (syncMode > 2) {
        syncMode = 0;
      }
    }
    serial_send_param_change_byte(ParamId::PARAM_SYNC_MODE, syncMode);
    break;

  case TG_CALIBRATION_MENU:
    menuPos = 0;
    menuPosMax = CALIBRATION_MENU_POS_MAX;
    currentControlMode = CALIBRATION_MENU;
    serial_send_param_change_byte(ParamId::PARAM_UI_CALIBRATION_MENU_MODE, 1);
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION,
                                  (uint8_t)menuPos);
    break;

  case TG_MAN_CALIBRATION:
    // Launch the new Flow!
    enterFlow(&calibrationFlow);
    break;

  case TG_ENVELOPE_MENU:
    currentMenu = &envelopeMenu;
    menuPos = 0;
    menuPosMax = currentMenu->count - 1;
    currentControlMode = MENU_NAVIGATION;
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE, 1);
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION,
                                  (uint8_t)menuPos);
    menu_announce_item(menuPos);
    break;

  case TG_ADSR1_MENU:
    currentMenu = &adsr1Menu;
    menuPos = 0;
    menuPosMax = currentMenu->count - 1;
    currentControlMode = MENU_NAVIGATION;
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE,
                                  2); // Screen ID 2
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION,
                                  (uint8_t)menuPos);
    menu_announce_item(menuPos);
    break;

  case TG_ADSR2_MENU:
    currentMenu = &adsr2Menu;
    menuPos = 0;
    menuPosMax = currentMenu->count - 1;
    currentControlMode = MENU_NAVIGATION;
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE,
                                  3); // Screen ID 3
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION,
                                  (uint8_t)menuPos);
    menu_announce_item(menuPos);
    break;

  case TG_ADSR3_MENU:
    currentMenu = &adsr3Menu;
    menuPos = 0;
    menuPosMax = currentMenu->count - 1;
    currentControlMode = MENU_NAVIGATION;
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE,
                                  4); // Screen ID 4
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION,
                                  (uint8_t)menuPos);
    menu_announce_item(menuPos);
    break;
    case TG_DCO_MENU:
    currentMenu = &dcoMenu;
    menuPos = 0;
    menuPosMax = currentMenu->count - 1;
    currentControlMode = MENU_NAVIGATION;
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE, 6); // Mode 6
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION, (uint8_t)menuPos);
    menu_announce_item(menuPos);
    break;

  case TG_DCO_MOD_MENU:
    currentMenu = &dcoModMenu;
    menuPos = 0;
    menuPosMax = currentMenu->count - 1;
    currentControlMode = MENU_NAVIGATION;
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE, 7); // Mode 7
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION, (uint8_t)menuPos);
    menu_announce_item(menuPos);
    break;

  case TG_MOD_MATRIX_MENU:
    currentMenu = &modMatrixMenu;
    menuPos = 0;
    menuPosMax = currentMenu->count - 1;
    currentControlMode = MENU_NAVIGATION;
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE, 8); // Mode 8
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION, (uint8_t)menuPos);
    menu_announce_item(menuPos);
    break;

  case TG_PLACEHOLDER_MENU:
    currentMenu = &placeholderMenu;
    menuPos = 0;
    menuPosMax = currentMenu->count - 1;
    currentControlMode = MENU_NAVIGATION;
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE,
                                  5); // Screen ID 5
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION,
                                  (uint8_t)menuPos);
    menu_announce_item(menuPos);
    break;

  case PRESET_SAVE_SELECT_MODE:
  case SAVE_PRESET:
    // Whether you short-click or hold Button 9 with FUNC on, launch the flow!
    enterFlow(&presetSaveFlow);
    break;

  // No panel sine key on either instrument; the rest are selection-only or
  // currently unassigned actions.
  case TG_SIN:
  case SELECT_LFO_N:
  case WORK_WITH_PRESETS:
  case PRESET_SAVE_MODE:
  case SELECT_ENC_ACTION:
  case EXIT:
  case BACK:
  case SELECT:
  case CONFIRM:
  case BTN_ACTION_NONE:
    break;
  }
}

void __not_in_flash_func(read_encoder_buttons)() {

  if ((millis() - buttonActionSelectedMillis) > buttonActionSelectedTimeout) {
    buttonActionSelected = BTN_ACTION_NONE;
    buttonActionIsSelected = false;
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {

    ButtonStruct &button = buttons[i];
    button.button_n.update(valorMUX1[button.pin], 50, LOW);

    // =========================================================================
    // 1. FLOW INTERCEPTION
    // =========================================================================
    if (activeFlow != nullptr) {
      if (button.button_n.held()) {
        activeFlow->onButton(i, HELD);
      } else if (button.button_n.released(true)) {
        activeFlow->onButton(i, RELEASED);
      }
      continue; // Flow handles it directly
    }

    // =========================================================================
    // 2. SYNTH CONTROLS (NORMAL vs MENU / CALIBRATION)
    // =========================================================================
    ButtonAction action = BTN_ACTION_NONE;
    ButtonState state = BTN_STATE_NONE;

    if (currentControlMode == NORMAL) {
      if (button.button_n.latched()) {
        handleLatchedButton(i);
      } else if (button.button_n.held()) {
        state = HELD;
        action = handleHeldButton(i);
      } else if (button.button_n.doublePressed()) {
        state = DOUBLE;
        action = handleDoublePressedButton(i);
      } else if (button.button_n.pressed()) {
        state = PRESSED;
        action = handlePressedButton(i);
      } else if (button.button_n.released(true)) {
        state = RELEASED;
        action = handleReleasedButton(i);
      } else if (button.button_n.unlatched()) {
        handleUnlatchedButton(i);
      }

      if (state != BTN_STATE_NONE) {
        if (action == buttonActionSelected) {
          buttonActionIsSelected = true;
        } else {
          buttonActionSelected = action;
          buttonActionIsSelected = false;
        }
        buttonActionSelectedMillis = millis();
      }

    } else {
      // In Menu & Calibration modes, directly check held vs released:
      if (button.button_n.held()) {
        action = button.actionMenuNavigationHeld;
      } else if (button.button_n.released(true)) {
        action = button.actionMenuNavigationReleased;
      }
      action = handle_mode_buttons(action);
    }

    if (action != BTN_ACTION_NONE) {
      execute_button_action(action);
    }
  }
}

// Latched-button bookkeeping for button index i (wave keys only).
void __not_in_flash_func(handleLatchedButton)(int i) {
  if (i <= LATCHABLE_BUTTON_MAX) {
    buttonIsLatched[i] = true;
    update_LED_Control(i, true);
    LED_Control_Mux.blinkPin(LEDPins[i], true);
  }
}

ButtonAction __not_in_flash_func(handleHeldButton)(int i) {
  ButtonStruct &button = buttons[i];
  if (funcKeyMode == 2)
    return button.actionHeldAlt2;
  if (funcKeyMode == 1)
    return button.actionHeldAlt;
  return button.actionHeld;
}

ButtonAction __not_in_flash_func(handlePressedButton)(int i) {
  ButtonStruct &button = buttons[i];
  if (funcKeyMode == 2)
    return button.actionPressedAlt2;
  if (funcKeyMode == 1)
    return button.actionPressedAlt;
  return button.actionPressed;
}

ButtonAction __not_in_flash_func(handleReleasedButton)(int i) {
  ButtonStruct &button = buttons[i];
  if (i <= LATCHABLE_BUTTON_MAX) {
    buttonIsLatched[i] = false;
    set_LED_Status(LED_REFRESH_ALL, 0);
    LED_Control_Mux.blinkPin(LEDPins[i], 0);
  }
  if (funcKeyMode == 2)
    return button.actionReleasedAlt2;
  if (funcKeyMode == 1)
    return button.actionReleasedAlt;
  return button.actionReleased;
}

// Resolve double-press action for button index i.
ButtonAction __not_in_flash_func(handleDoublePressedButton)(int i) {
  return buttons[i].actionDouble;
}

// Unlatch bookkeeping for button index i (wave keys only).
void __not_in_flash_func(handleUnlatchedButton)(int i) {
  if (i <= LATCHABLE_BUTTON_MAX) {
    buttonIsLatched[i] = false;
    set_LED_Status(LED_REFRESH_ALL, 0);
    LED_Control_Mux.blinkPin(LEDPins[i], 0);
  }
}
