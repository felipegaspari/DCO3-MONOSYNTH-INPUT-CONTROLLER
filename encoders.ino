#include "_build_libs/MD_REncoder_fela/src/MD_REncoder_fela.cpp"
#include "include_all.h"

// -----------------------------------------------------------------------------
// Value-knob bindings, one row per plain parameter action. Executed by
// encoder_apply_binding(); see EncoderParamBinding in encoders.h for the
// column meanings. Non-const so the table lives in .data (RAM), keeping the
// core0 panel scan off the flash/XIP path.
// -----------------------------------------------------------------------------
static EncoderParamBinding encoderParamBindings[] = {
    //  action                      value                 type          min max
    //  step spdX2  paramId                                 flags
    {ACTION_portamento_time, &portamentoTime, ENC_VAL_I16, 0, 255, 1, 1,
     ParamId::PARAM_PORTAMENTO_TIME, 0},
    {ACTION_LFO1_to_DCO, &LFO1toDCO, ENC_VAL_I16, 0, 511, 1, 1,
     ParamId::PARAM_LFO1_TO_DCO, ENC_SEND_WORD},
    {ACTION_VCF_keytrack, &VCFKeytrack, ENC_VAL_I16, -256, 255, 1, 1,
     ParamId::PARAM_VCF_KEYTRACK, ENC_SEND_WORD},
    {ACTION_ADSR3_to_DETUNE1, &ADSR3toDETUNE1, ENC_VAL_I16, -511, 511, 1, 2,
     ParamId::PARAM_ADSR3_TO_DETUNE1, ENC_SEND_WORD},
    {ACTION_velocity_to_VCF, &velocityToVCF, ENC_VAL_I8, 0, 20, 1, 0,
     ParamId::PARAM_VELOCITY_TO_VCF, 0},
    {ACTION_velocity_to_VCA, &velocityToVCA, ENC_VAL_I8, 0, 20, 1, 2,
     ParamId::PARAM_VELOCITY_TO_VCA, 0},
    {ACTION_octave, &OSC1Interval, ENC_VAL_I8, 0, 72, 12, 0,
     ParamId::PARAM_OSC1_INTERVAL, 0},
    {ACTION_SQR1_level, &OSC1Level, ENC_VAL_I16, 0, 127, 1, 2,
     ParamId::PARAM_OSC1_LEVEL, 0},
    {ACTION_SQR2_level, &OSC2Level, ENC_VAL_I16, 0, 127, 1, 2,
     ParamId::PARAM_OSC2_LEVEL, 0},
    {ACTION_SUB_level, &SubLevel, ENC_VAL_I16, 0, 127, 1, 2,
     ParamId::PARAM_SUB_LEVEL, 0},
    {ACTION_OSC2_interval, &OSC2Interval, ENC_VAL_I8, 0, 60, 1, 0,
     ParamId::PARAM_OSC2_INTERVAL, 0},
    {ACTION_OSC3_interval, &OSC3Interval, ENC_VAL_I8, 0, 60, 1, 0,
     ParamId::PARAM_OSC3_INTERVAL, 0},
    {ACTION_OSC2_detune, &OSC2Detune, ENC_VAL_I16, 0, 512, 1, 2,
     ParamId::PARAM_OSC2_DETUNE_VAL, ENC_SEND_WORD},
    {ACTION_OSC3_detune, &OSC3Detune, ENC_VAL_I16, 0, 512, 1, 2,
     ParamId::PARAM_OSC3_DETUNE_VAL, ENC_SEND_WORD},
    {ACTION_LFO2_to_OSC2, &LFO2toOSC2DETUNE, ENC_VAL_I16, 0, 255, 1, 1,
     ParamId::PARAM_LFO2_TO_OSC2, 0},
    {ACTION_LFO2_to_OSC3, &LFO2toOSC3DETUNE, ENC_VAL_I16, 0, 255, 1, 1,
     ParamId::PARAM_LFO2_TO_OSC3, 0},
    {ACTION_osc_phase_sync, &oscPhaseSync, ENC_VAL_U16, 0, 225, 1, 0,
     ParamId::PARAM_OSC_PHASE_SYNC, 0}, // TODO: Uncomment this when phase align is implemented
    {ACTION_LFO1_speed, &LFO1Speed, ENC_VAL_I16, 0, 4095, 1, 4,
     ParamId::PARAM_LFO1_SPEED, ENC_SEND_WORD},
    {ACTION_LFO2_speed, &LFO2Speed, ENC_VAL_I16, 0, 4095, 1, 4,
     ParamId::PARAM_LFO2_SPEED, ENC_SEND_WORD},
    {ACTION_VCA_level, &VCALevel, ENC_VAL_I16, 0, 128, 2, 10,
     ParamId::PARAM_VCA_LEVEL, 0},
    {ACTION_LFO1_to_VCA, &LFO1toVCA, ENC_VAL_I16, 0, 1023, 1, 2,
     ParamId::PARAM_LFO1_TO_VCA, ENC_SEND_WORD},
    {ACTION_LFO2_to_PWM, &LFO2toPWM, ENC_VAL_I16, 0, 511, 1, 2,
     ParamId::PARAM_LFO2_TO_PW, ENC_SEND_WORD},
    {ACTION_ADSR3_to_PWM, &ADSR3toPWM, ENC_VAL_I16, -512, 511, 1, 2,
     ParamId::PARAM_ADSR3_TO_PWM, ENC_SEND_WORD | ENC_SEND_OFFSET_512},
    {ACTION_ANALOG_DETUNE, &unisonDetune, ENC_VAL_I16, 0, 127, 1, 1,
     ParamId::PARAM_UNISON_DETUNE, 0},
    {ACTION_ANALOG_DRIFT, &analogDrift, ENC_VAL_I16, 0, 127, 1, 1,
     ParamId::PARAM_ANALOG_DRIFT_AMOUNT, 0},
    {ACTION_ANALOG_DRIFT_SPEED, &analogDriftSpeed, ENC_VAL_I16, 1, 255, 1, 1,
     ParamId::PARAM_ANALOG_DRIFT_SPEED, 0},
    {ACTION_ANALOG_DRIFT_SPREAD, &analogDriftSpread, ENC_VAL_I16, 1, 127, 1, 1,
     ParamId::PARAM_ANALOG_DRIFT_SPREAD, 0},
    {ACTION_ADSR1_ATTACK_CURVE, &ADSR1AttackCurveVal, ENC_VAL_I8, 0, 7, 1, 0,
     ParamId::PARAM_ADSR1_ATTACK_CURVE, 0},
    {ACTION_ADSR1_DECAY_CURVE, &ADSR1DecayCurveVal, ENC_VAL_I8, 0, 7, 1, 0,
     ParamId::PARAM_ADSR1_DECAY_CURVE, 0},
    {ACTION_ADSR2_ATTACK_CURVE, &ADSR2AttackCurveVal, ENC_VAL_I8, 0, 7, 1, 0,
     ParamId::PARAM_ADSR2_ATTACK_CURVE, 0},
    {ACTION_ADSR2_DECAY_CURVE, &ADSR2DecayCurveVal, ENC_VAL_I8, 0, 7, 1, 0,
     ParamId::PARAM_ADSR2_DECAY_CURVE, 0},
    {ACTION_LFO1_to_OSC1, &LFO1toOSC1, ENC_VAL_I8, 0, 255, 1, 2,
     ParamId::PARAM_LFO1_TO_OSC1, 0},
    {ACTION_LFO1_to_OSC2, &LFO1toOSC2, ENC_VAL_I8, 0, 255, 1, 2,
     ParamId::PARAM_LFO1_TO_OSC2, 0},
    {ACTION_LFO2_to_OSC2_coarse, &LFO2toOSC2_coarse, ENC_VAL_U16, 0, 511, 1, 2,
     ParamId::PARAM_LFO2_TO_OSC2_COARSE, ENC_SEND_WORD},
    {ACTION_ADSR1_RELEASE_CURVE, &ADSR1ReleaseCurveVal, ENC_VAL_I8, 0, 7, 1, 0, ParamId::PARAM_ADSR1_RELEASE_CURVE, 0},
    {ACTION_ADSR2_RELEASE_CURVE, &ADSR2ReleaseCurveVal, ENC_VAL_I8, 0, 7, 1, 0, ParamId::PARAM_ADSR2_RELEASE_CURVE, 0},
    {ACTION_ADSR3_ATTACK_CURVE, &ADSR3AttackCurveVal, ENC_VAL_I8, 0, 7, 1, 0, ParamId::PARAM_ADSR3_ATTACK_CURVE, 0},
    {ACTION_ADSR3_DECAY_CURVE, &ADSR3DecayCurveVal, ENC_VAL_I8, 0, 7, 1, 0, ParamId::PARAM_ADSR3_DECAY_CURVE, 0},
    {ACTION_ADSR3_RELEASE_CURVE, &ADSR3ReleaseCurveVal, ENC_VAL_I8, 0, 7, 1, 0, ParamId::PARAM_ADSR3_RELEASE_CURVE, 0},
    {ACTION_VOICE_MODE, &voiceMode, ENC_VAL_I8, 0, 2, 1, 0, ParamId::PARAM_VOICE_MODE, 0},
    {ACTION_VOICE_ALLOC_MODE, &voiceAllocMode, ENC_VAL_I8, 0, 5, 1, 0, ParamId::PARAM_VOICE_ALLOC_MODE, 0},
    {ACTION_SYNC_MODE, &syncMode, ENC_VAL_I8, 0, 2, 1, 0, ParamId::PARAM_SYNC_MODE, 0},
    {ACTION_SOFT_SYNC, &softSync, ENC_VAL_I8, 0, 3, 1, 0, ParamId::PARAM_SOFT_SYNC, 0},
    {ACTION_PORTAMENTO_MODE, &portamentoMode, ENC_VAL_I8, 0, 1, 1, 0, ParamId::PARAM_PORTAMENTO_MODE, 0},
    {ACTION_SUBOSC_DIVIDE, &subOscDivide, ENC_VAL_I8, 0, 8, 1, 0, ParamId::PARAM_SUBOSC_DIVIDE, 0},
};

static constexpr uint8_t NUM_ENCODER_BINDINGS =
    sizeof(encoderParamBindings) / sizeof(encoderParamBindings[0]);

static inline int32_t encoder_binding_read(const EncoderParamBinding &b) {
  switch (b.type) {
  case ENC_VAL_I8:
    return *(int8_t *)b.value;
  case ENC_VAL_U16:
    return *(uint16_t *)b.value;
  default:
    return *(int16_t *)b.value;
  }
}

static inline void encoder_binding_write(const EncoderParamBinding &b,
                                         int32_t v) {
  switch (b.type) {
  case ENC_VAL_I8:
    *(int8_t *)b.value = (int8_t)v;
    break;
  case ENC_VAL_U16:
    *(uint16_t *)b.value = (uint16_t)v;
    break;
  default:
    *(int16_t *)b.value = (int16_t)v;
    break;
  }
}

// Run the table-bound handler for action.
static bool __not_in_flash_func(encoder_apply_binding)(EncoderAction action,
                                                       uint8_t direction,
                                                       uint16_t speed) {
  for (uint8_t n = 0; n < NUM_ENCODER_BINDINGS; n++) {
    const EncoderParamBinding &b = encoderParamBindings[n];
    if (b.action != action)
      continue;

      if (encoderActionIsSelected || currentControlMode == MENU_NAVIGATION) {
        const int32_t step = b.baseStep + ((int32_t)b.speedMultX2 * speed) / 2;
        int32_t value = encoder_binding_read(b);
        value += (direction == DIR_CW) ? step : -step;
        value = constrain(value, (int32_t)b.min, (int32_t)b.max);
        encoder_binding_write(b, value);
      }

    int32_t out = encoder_binding_read(b);
    if (b.flags & ENC_SEND_OFFSET_512)
      out += 512;
    if (b.flags & ENC_SEND_WORD) {
      serial_send_param_change((byte)b.paramId, (uint16_t)out);
    } else {
      serial_send_param_change_byte((byte)b.paramId, (uint8_t)out);
    }
    return true;
  }
  return false;
}

static EncoderAction __not_in_flash_func(resolve_encoder_action)(const EncoderStruct& encoder, int i) {
  switch (currentControlMode) {
    case NORMAL: {
      EncoderAction action;
      if (funcKeyMode == 2) {
        action = buttonIsLatched[i] ? encoder.actionsAlt2[1] : encoder.actionsAlt2[0];
      } else if (funcKeyMode == 1) {
        action = buttonIsLatched[i] ? encoder.actionsAlt[1] : encoder.actionsAlt[0];
      } else {
        action = buttonIsLatched[i] ? encoder.actions[1] : encoder.actions[0];
      }

      if (action == encoderActionSelected) {
        encoderActionIsSelected = true;
      } else {
        encoderActionSelected = action;
        encoderActionIsSelected = false;
      }
      encoderActionSelectedMillis = millis();

      return action;
    }
    case MENU_NAVIGATION:
    case CALIBRATION_MENU:
      return menuNavigationActions[i];
  }
  return ACTION_NONE;
}
// Core0 ~99 µs: read all encoders and dispatch EncoderAction (ParamId / UI /
// cal).
void __not_in_flash_func(read_encoders)() {

  if ((millis() - encoderActionSelectedMillis) > encoderActionSelectedTimeout) {
    encoderActionSelected = ACTION_NONE;
    encoderActionIsSelected = false;
  }

  for (int i = 0; i < NUM_ENCODERS; i++) {

    EncoderStruct& encoder = encoders[i];
    int16_t delta = encoder.MD_REncoder_Name.readDelta(valorMUX1[encoder.muxPin1], valorMUX1[encoder.muxPin2], 20);
    if (delta == 0) continue;

    uint8_t direction = (delta > 0) ? DIR_CW : DIR_CCW;
    uint16_t speed = encoder.MD_REncoder_Name.speed();

    // =========================================================================
    // THE FLOW INTERCEPTOR
    // If a Wizard is active, hand the encoder entirely to the Flow and bypass everything else!
    // =========================================================================
    if (activeFlow != nullptr) {
      activeFlow->onEncoder(i, direction, speed);
      continue; 
    }

    // --- Normal Synth Operation Below ---
    EncoderAction currentAction = resolve_encoder_action(encoder, i);

    if (encoder_apply_binding(currentAction, direction, speed)) {
      continue;
    }

    switch (currentAction) {
      case ACTION_select_preset:
            {
              int tempPreset = (int)presetSelectVal;
              if (direction == DIR_CW) {
                tempPreset += (1 + speed);
              } else {
                tempPreset -= (1 + speed);
              }
              presetSelectVal = (uint8_t)constrain(tempPreset, 0, PRESET_NUM_SLOTS - 1);
              preset_load_from_board(presetSelectVal);
              break;
            }
        break;

        case ACTION_MENU_POS:
        if (direction == DIR_CW) menuPos++;
        else menuPos--;
        menuPos = constrain(menuPos, 0, menuPosMax);
        serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION, (uint8_t)menuPos);
        menu_announce_item(menuPos); // Announces value over UART so screen stays in sync
        break;

        case ACTION_MENU_VALUE:
        if (currentMenu && menuPos >= 0 && menuPos < currentMenu->count) {
          const MenuItem& item = currentMenu->items[menuPos];
          if (item.encAction != ACTION_NONE) {
            encoder_apply_binding(item.encAction, direction, speed);
          } else if (item.btnAction != BTN_ACTION_NONE) {
            // For toggle switches (e.g. Restart ON/OFF), turning the encoder also toggles it
            execute_button_action(item.btnAction);
          }
        }
        break;
        default:
        break;
    }
  }
}

void __not_in_flash_func(menu_announce_item)(int8_t pos) {
  if (!currentMenu)
    return;
  const MenuItem &item = currentMenu->items[pos];

  if (item.btnAction != BTN_ACTION_NONE && item.stateValue != nullptr) {
    // Announce a toggle state
    serial_send_param_change_byte((byte)item.announceParam,
                                  (uint8_t)*item.stateValue);
  } else if (item.encAction != ACTION_NONE) {
    // Announce a value state by finding its binding
    for (uint8_t n = 0; n < NUM_ENCODER_BINDINGS; n++) {
      if (encoderParamBindings[n].action == item.encAction) {
        const EncoderParamBinding &b = encoderParamBindings[n];
        int32_t out = encoder_binding_read(b);
        if (b.flags & ENC_SEND_OFFSET_512)
          out += 512;
        if (b.flags & ENC_SEND_WORD) {
          serial_send_param_change((byte)b.paramId, (uint16_t)out);
        } else {
          serial_send_param_change_byte((byte)b.paramId, (uint8_t)out);
        }
        break;
      }
    }
  }
}