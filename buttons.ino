#include "include_all.h"

// Flip one of the five panel wave keys and mirror it to the DCO and its LED.
// The key index is also the LED index (see board_model.h).
static void toggle_wave_key(uint8_t key) {
  const InputWaveKey& k = inputWaveKeys[key];
  waveEnable[k.osc][k.wave] = !waveEnable[k.osc][k.wave];
  serial_send_param_change_byte(input_wave_key_param_id(k.osc, k.wave),
                                waveEnable[k.osc][k.wave]);
  set_LED_Status(key, waveEnable[k.osc][k.wave]);
}

// --- preset save flow ---------------------------------------------------------
//
// saveFlow (Controls.h) is the single source of truth for the save UI:
// IDLE -> SELECT (scroll to a slot) -> NAME_EDIT (edit 16-char name) -> commit.

// Idle -> slot-select. Refresh the directory cache first in case another peer
// (e.g. dco_control) renamed/saved a slot on the DCO since boot.
static void save_flow_enter_select() {
  saveFlow = SaveFlow::SELECT;
  charSelectVal = 0;
  request_preset_directory();
  serial_send_signal(SIGNAL_SAVE_SELECT_ENTER);
}

// Cancel from either save stage: restore current preset name/number and exit.
static void save_flow_cancel() {
  const bool fromNameEdit = (saveFlow == SaveFlow::NAME_EDIT);
  saveFlow = SaveFlow::IDLE;

  if (fromNameEdit) {
    for (int c = 0; c < 16; c++) {
      presetNameVal[c] = presetName[c];
    }
    charSelectVal = 0;
    presetCharPos = 0;
  }
  presetSelectVal = currentPreset;

  serial_send_signal(SIGNAL_SAVE_EXIT);
  serial_send_preset_scroll(currentPreset, presetName);
}

// Slot-select -> name-edit, starting from the current preset name.
static void save_flow_enter_name_edit() {
  serial_send_signal(SIGNAL_SAVE_NAME_EDIT);
  saveFlow = SaveFlow::NAME_EDIT;
  for (int c = 0; c < 16; c++) {
    presetNameVal[c] = presetName[c];
  }
  charSelectVal = presetNameVal[0];
  presetCharPos = 0;
}

// Name-edit -> commit: save on the DCO and show "preset saved" on the Screen
// (the Screen returns to PresetScroll mode after showing the message).
static void save_flow_commit() {
  saveFlow = SaveFlow::IDLE;

  preset_save_to_board(presetSelectVal);

  // After saving, send updated preset name so the screen shows it.
  serial_send_preset_scroll((uint8_t)presetSelectVal, presetNameVal);
  serial_send_signal(SIGNAL_PRESET_SAVED);
  presetCharPos = 0;
}

// --- calibration menu -----------------------------------------------------------
//
// CALIBRATION_MENU entries in menuPos order, following the Screen's calibration
// tabs. calibrationFlag picks the stage on the DCO (PARAM_CALIBRATION_FLAG);
// followUp instead re-dispatches a button action (e.g. enter manual cal).
struct CalibrationMenuEntry {
  uint8_t calibrationFlag;
  ButtonAction followUp;
};

static const CalibrationMenuEntry calibrationMenu[] = {
  { 1, BTN_ACTION_NONE },     // AUTO CALIBRATION: amp-comp tables only
  { 2, BTN_ACTION_NONE },     // PW CALIBRATION: PW center + limits only
  { 3, BTN_ACTION_NONE },     // FULL CALIBRATION: PW stage, then amp-comp
  { 0, TG_MAN_CALIBRATION },  // MANUAL CALIBRATION
};

static constexpr int8_t CALIBRATION_MENU_POS_MAX =
    (int8_t)(sizeof(calibrationMenu) / sizeof(calibrationMenu[0]) - 1);

// Leave manual calibration mode (without persisting anything by itself).
static void exit_manual_calibration(controlMode returnTo) {
  currentControlMode = returnTo;
  manualCalibration = false;
  serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_FLAG, manualCalibration);
}

// Mode-specific EXIT/BACK/SELECT/CONFIRM handling. Consumes the action (returns
// BTN_ACTION_NONE) or maps it to a follow-up for execute_button_action().
static ButtonAction __not_in_flash_func(handle_mode_buttons)(ButtonAction action) {
  switch (currentControlMode) {

    case MANUAL_CALIBRATION:
      switch (action) {
        case EXIT:
        case BACK:
          exit_manual_calibration(CALIBRATION_MENU);
          return BTN_ACTION_NONE;
        case SELECT:
          return BTN_ACTION_NONE;
        case CONFIRM:
          exit_manual_calibration(NORMAL);
          // Explicitly request the DCO to persist the current
          // manualCalibrationOffset[] array to its filesystem. This keeps
          // the MANUAL_CALIBRATION_FLAG free for UI/mode control while
          // making CONFIRM the only path that actually stores offsets.
          serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_STORE, 1);
          // After confirming manual calibration, also dismiss the calibration
          // menu on the screen so we fully return to the main UI.
          serial_send_param_change_byte(ParamId::PARAM_UI_CALIBRATION_DISMISS, 0);
          return BTN_ACTION_NONE;
        default:
          break;
      }
      break;

    case CALIBRATION_MENU:
      switch (action) {
        case EXIT:
        case BACK:
          currentControlMode = NORMAL;
          serial_send_param_change_byte(ParamId::PARAM_UI_CALIBRATION_DISMISS, 0);
          return BTN_ACTION_NONE;
        case SELECT:
          if (menuPos >= 0 && menuPos <= CALIBRATION_MENU_POS_MAX) {
            const CalibrationMenuEntry& entry = calibrationMenu[menuPos];
            if (entry.followUp != BTN_ACTION_NONE) {
              return entry.followUp;
            }
            if (entry.calibrationFlag != 0) {
              serial_send_param_change_byte(ParamId::PARAM_CALIBRATION_FLAG, entry.calibrationFlag);
            }
          }
          return BTN_ACTION_NONE;
        case CONFIRM:
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
      serial_send_param_change_byte(ParamId::PARAM_RESONANCE_COMPENSATION, RESONANCEAmpCompensation);
      break;

    case TG_ADSR1_RESTART:
      if (ADSR1CurveSelect == true) {
        ADSR1CurveSelect = false;
        serial_send_param_change_byte(ParamId::PARAM_ADSR1_ATTACK_CURVE, -1);
      } else {
        if (buttonActionIsSelected) {
          VCAADSRRestart = !VCAADSRRestart;
        }
        serial_send_param_change_byte(ParamId::PARAM_VCA_ADSR_RESTART, VCAADSRRestart);
      }
      break;

    case TG_ADSR2_RESTART:
      if (ADSR2CurveSelect == true) {
        ADSR2CurveSelect = false;
        serial_send_param_change_byte(ParamId::PARAM_ADSR2_ATTACK_CURVE, -1);
      } else {
        if (buttonActionIsSelected) {
          VCFADSRRestart = !VCFADSRRestart;
        }
        serial_send_param_change_byte(ParamId::PARAM_VCF_ADSR_RESTART, VCFADSRRestart);
      }
      break;

    case TG_LFO1_WAVE:
      if (buttonActionIsSelected) {
        LFO1Waveform++;
        if (LFO1Waveform > 4) {
          LFO1Waveform = 1;
        }
      }
      serial_send_param_change_byte(ParamId::PARAM_LFO1_WAVEFORM, LFO1Waveform);
      break;

    case TG_LFO2_WAVE:
      if (buttonActionIsSelected) {
        LFO2Waveform++;
        if (LFO2Waveform > 4) {
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
      funcKeyOn = !funcKeyOn;
      serial_send_param_change_byte(ParamId::PARAM_FUNCTION_KEY, (uint8_t)funcKeyOn);
      break;

    case PRESET_SAVE_SELECT_MODE:
      if (saveFlow == SaveFlow::IDLE) {
        save_flow_enter_select();
      } else {
        save_flow_cancel();
      }
      break;

    case SAVE_PRESET:
      if (saveFlow == SaveFlow::NAME_EDIT) {
        save_flow_commit();
      } else if (saveFlow == SaveFlow::SELECT) {
        save_flow_enter_name_edit();
      }
      // If idle, ignore SAVE_PRESET.
      break;

    case TG_MAN_FADERS:
      faderControlManual = !faderControlManual;
      faderRow1ControlManual = faderControlManual;
      faderRow2ControlManual = faderControlManual;
      serial_send_param_change_byte(ParamId::PARAM_FADERS_CONTROL_MANUAL, faderControlManual);
      break;

    case TG_MAN_FADER_ROW1:
      faderRow1ControlManual = !faderRow1ControlManual;
      serial_send_param_change_byte(ParamId::PARAM_FADER_ROW1_CONTROL_MANUAL, faderRow1ControlManual);
      set_LED_Status(10, faderRow1ControlManual);
      break;

    case TG_MAN_FADER_ROW2:
      faderRow2ControlManual = !faderRow2ControlManual;
      serial_send_param_change_byte(ParamId::PARAM_FADER_ROW2_CONTROL_MANUAL, faderRow2ControlManual);
      set_LED_Status(11, faderRow2ControlManual);
      break;

    case TG_MAN_POTS:
      potsControlManual = !potsControlManual;
      VCFPotsControlManual = potsControlManual;
      VCAPotsControlManual = potsControlManual;
      PWMPotsControlManual = potsControlManual;
      serial_send_param_change_byte(ParamId::PARAM_POTS_CONTROL_MANUAL, potsControlManual);
      break;

    case TG_MANUAL_VCF_POTS:
      VCFPotsControlManual = !VCFPotsControlManual;
      serial_send_param_change_byte(ParamId::PARAM_VCF_POTS_CONTROL_MANUAL, VCFPotsControlManual);
      set_LED_Status(7, VCFPotsControlManual);
      break;

    case TG_MANUAL_VCA_POTS:
      VCAPotsControlManual = !VCAPotsControlManual;
      serial_send_param_change_byte(ParamId::PARAM_VCA_POTS_CONTROL_MANUAL, VCAPotsControlManual);
      set_LED_Status(8, VCAPotsControlManual);
      break;

    case TG_MANUAL_PWM_POTS:
      PWMPotsControlManual = !PWMPotsControlManual;
      serial_send_param_change_byte(ParamId::PARAM_PWM_POTS_CONTROL_MANUAL, PWMPotsControlManual);
      set_LED_Status(9, PWMPotsControlManual);
      break;

    case TG_MANUAL_ALL:
      allControlsManual = !allControlsManual;
      VCFPotsControlManual = allControlsManual;
      faderRow2ControlManual = allControlsManual;
      faderRow1ControlManual = allControlsManual;
      PWMPotsControlManual = allControlsManual;
      serial_send_param_change_byte(ParamId::PARAM_ALL_CONTROLS_MANUAL, allControlsManual);
      break;

    case TG_ENABLE_ADSR3:
      ADSR3Enabled = !ADSR3Enabled;
      faderRow2ControlManual = false;
      serial_send_param_change_byte(ParamId::PARAM_ADSR3_ENABLED, (uint8_t)ADSR3Enabled);
      LED_Control_Mux.blinkPin(LEDPins[11], ADSR3Enabled);
      set_LED_Status(11, ADSR3Enabled);
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

    case ADSR1_CURVE_SEL:
      if (buttonActionIsSelected) {
        ADSR1CurveSelect = !ADSR1CurveSelect;
      }
      serial_send_param_change_byte(ParamId::PARAM_ADSR1_ATTACK_CURVE, -1);
      break;

    case ADSR2_CURVE_SEL:
      if (buttonActionIsSelected) {
        ADSR2CurveSelect = !ADSR2CurveSelect;
      }
      serial_send_param_change_byte(ParamId::PARAM_ADSR2_ATTACK_CURVE, -1);
      break;

    case TG_ADSR3_TO_OSC_SELECT:
      if (buttonActionIsSelected) {
        ADSR3ToOscSelect++;
        if (ADSR3ToOscSelect > INPUT_ADSR3_TO_OSC_SELECT_MAX) {
          ADSR3ToOscSelect = 0;
        }
      }
      serial_send_param_change_byte(ParamId::PARAM_ADSR3_TO_OSC_SELECT, ADSR3ToOscSelect);
      break;

    case TG_CALIBRATION_MENU:
      menuPos = 0;
      menuPosMax = CALIBRATION_MENU_POS_MAX;
      currentControlMode = CALIBRATION_MENU;
      serial_send_param_change_byte(ParamId::PARAM_UI_CALIBRATION_MENU_MODE, 1);
      serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION, (uint8_t)menuPos);
      break;

    case TG_MAN_CALIBRATION:
      manualCalibration = true;
      currentControlMode = MANUAL_CALIBRATION;
      manualCalibrationStage = 0;
      serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_FLAG, manualCalibration, true);
      serialSendParamByteToScreen(ParamId::PARAM_UI_VOICE_TOPOLOGY, (uint8_t)NUM_OSCILLATORS);
      input_send_manual_cal_stage();
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

// Core0 ~99 µs: update RoxButtons from mux and dispatch ButtonAction handlers.
void __not_in_flash_func(read_encoder_buttons)() {

  if ((millis() - buttonActionSelectedMillis) > buttonActionSelectedTimeout) {
    buttonActionSelected = BTN_ACTION_NONE;
    buttonActionIsSelected = false;
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {

    ButtonStruct& button = buttons[i];

    ButtonAction action = BTN_ACTION_NONE;
    ButtonState state = BTN_STATE_NONE;

    button.button_n.update(valorMUX1[button.pin], 50, LOW);

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
      if (button.button_n.held()) {
        action = button.actionMenuNavigationHeld;
      } else if (button.button_n.released(true)) {
        action = button.actionMenuNavigationReleased;
      }
      action = handle_mode_buttons(action);
    }

    execute_button_action(action);
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

// Resolve held-button action for button index i.
ButtonAction __not_in_flash_func(handleHeldButton)(int i) {
  ButtonStruct& button = buttons[i];
  return funcKeyOn ? button.actionHeldAlt : button.actionHeld;
}

// Resolve double-press action for button index i.
ButtonAction __not_in_flash_func(handleDoublePressedButton)(int i) {
  return buttons[i].actionDouble;
}

// Resolve press action for button index i.
ButtonAction __not_in_flash_func(handlePressedButton)(int i) {
  ButtonStruct& button = buttons[i];
  return funcKeyOn ? button.actionPressedAlt : button.actionPressed;
}

// Resolve release action for button index i (also clears any latch state).
ButtonAction __not_in_flash_func(handleReleasedButton)(int i) {
  ButtonStruct& button = buttons[i];
  if (i <= LATCHABLE_BUTTON_MAX) {
    buttonIsLatched[i] = false;
    set_LED_Status(LED_REFRESH_ALL, 0);
    LED_Control_Mux.blinkPin(LEDPins[i], 0);
  }
  return funcKeyOn ? button.actionReleasedAlt : button.actionReleased;
}

// Unlatch bookkeeping for button index i (wave keys only).
void __not_in_flash_func(handleUnlatchedButton)(int i) {
  if (i <= LATCHABLE_BUTTON_MAX) {
    buttonIsLatched[i] = false;
    set_LED_Status(LED_REFRESH_ALL, 0);
    LED_Control_Mux.blinkPin(LEDPins[i], 0);
  }
}
