#include "include_all.h"
#include "_build_libs/MD_REncoder_fela/src/MD_REncoder_fela.cpp"


// --- ENCODER SERIAL THROTTLE CONFIGURATION ----------------------------------
EncoderThrottle encThrottle[NUM_ENCODERS];

void queue_encoder_param_send(uint8_t encIndex, ParamId paramId, uint16_t wireValue, bool isWord) {
  if (encIndex >= NUM_ENCODERS) return;

  uint32_t now = millis();
  EncoderThrottle &t = encThrottle[encIndex];

  t.paramId = paramId;
  t.wireValue = wireValue;
  t.isWord = isWord;

  // If enough time has passed since the last send, transmit immediately:
  if (now - t.lastSentMillis >= ENCODER_TX_INTERVAL_MS) {
    if (t.isWord) {
      serial_send_param_change((byte)t.paramId, t.wireValue);
    } else {
      serial_send_param_change_byte((byte)t.paramId, (byte)t.wireValue);
    }
    t.lastSentMillis = now;
    t.isDirty = false;
  } else {
    // Too fast: mark dirty, will be transmitted by flush_encoder_throttle()
    t.isDirty = true;
  }
}

void flush_encoder_throttle() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
    EncoderThrottle &t = encThrottle[i];

    // If dirty and cooldown interval has passed, send the latest settled value
    if (t.isDirty && (now - t.lastSentMillis >= ENCODER_TX_INTERVAL_MS)) {
      if (t.isWord) {
        serial_send_param_change((byte)t.paramId, t.wireValue);
      } else {
        serial_send_param_change_byte((byte)t.paramId, (byte)t.wireValue);
      }
      t.lastSentMillis = now;
      t.isDirty = false;
    }
  }
}
// --- END ENCODER SERIAL THROTTLE CONFIGURATION ------------------------------

// -----------------------------------------------------------------------------
// Value-knob bindings, one row per plain parameter action. Executed by
// encoder_apply_binding(); see EncoderParamBinding in encoders.h for the
// column meanings. Non-const so the table lives in .data (RAM), keeping the
// core0 panel scan off the flash/XIP path.
// -----------------------------------------------------------------------------
static EncoderParamBinding encoderParamBindings[] = {
  //  action                      value                 type          min    max  step spdX2  paramId                                 flags
  { ACTION_portamento_time,     &portamentoTime,      ENC_VAL_I16,     0,   255,  1,  1, ParamId::PARAM_PORTAMENTO_TIME,     0 },
  { ACTION_LFO1_to_DCO,         &LFO1toDCO,           ENC_VAL_I16,     0,   511,  1,  1, ParamId::PARAM_LFO1_TO_DCO,         ENC_SEND_WORD },
  { ACTION_VCF_keytrack,        &VCFKeytrack,         ENC_VAL_I16,  -256,   255,  1,  1, ParamId::PARAM_VCF_KEYTRACK,        ENC_SEND_WORD },
  { ACTION_ADSR3_to_DETUNE1,    &ADSR3toDETUNE1,      ENC_VAL_I16,  -511,   511,  1,  2, ParamId::PARAM_ADSR3_TO_DETUNE1,    ENC_SEND_WORD },
  { ACTION_velocity_to_VCF,     &velocityToVCF,       ENC_VAL_I8,      0,    20,  1,  0, ParamId::PARAM_VELOCITY_TO_VCF,     0 },
  { ACTION_velocity_to_VCA,     &velocityToVCA,       ENC_VAL_I8,      0,    20,  1,  2, ParamId::PARAM_VELOCITY_TO_VCA,     0 },
  { ACTION_octave,              &OSC1Interval,        ENC_VAL_I8,      0,    72, 12,  0, ParamId::PARAM_OSC1_INTERVAL,       0 },
  { ACTION_SQR1_level,          &OSC1Level,           ENC_VAL_I16,     0,   127,  1,  2, ParamId::PARAM_OSC1_LEVEL,          0 },
  { ACTION_SQR2_level,          &OSC2Level,           ENC_VAL_I16,     0,   127,  1,  2, ParamId::PARAM_OSC2_LEVEL,          0 },
  { ACTION_SUB_level,           &SubLevel,            ENC_VAL_I16,     0,   127,  1,  2, ParamId::PARAM_SUB_LEVEL,           0 },
  { ACTION_OSC2_interval,       &OSC2Interval,        ENC_VAL_I8,      0,    60,  1,  0, ParamId::PARAM_OSC2_INTERVAL,       0 },
  { ACTION_OSC3_interval,       &OSC3Interval,        ENC_VAL_I8,      0,    60,  1,  0, ParamId::PARAM_OSC3_INTERVAL,       0 },
  { ACTION_OSC2_detune,         &OSC2Detune,          ENC_VAL_I16,     0,   512,  1,  2, ParamId::PARAM_OSC2_DETUNE_VAL,     ENC_SEND_WORD },
  { ACTION_OSC3_detune,         &OSC3Detune,          ENC_VAL_I16,     0,   512,  1,  2, ParamId::PARAM_OSC3_DETUNE_VAL,     ENC_SEND_WORD },
  { ACTION_LFO2_to_OSC2,        &LFO2toOSC2DETUNE,    ENC_VAL_I16,     0,   255,  1,  1, ParamId::PARAM_LFO2_TO_OSC2,        0 },
  { ACTION_LFO2_to_OSC3,        &LFO2toOSC3DETUNE,    ENC_VAL_I16,     0,   255,  1,  1, ParamId::PARAM_LFO2_TO_OSC3,        0 },
  { ACTION_osc_sync_mode,       &oscSyncMode,         ENC_VAL_U16,     0,   225,  1,  0, ParamId::PARAM_OSC_SYNC_MODE,       0 },
  { ACTION_LFO1_speed,          &LFO1Speed,           ENC_VAL_I16,     0,  4095,  1,  4, ParamId::PARAM_LFO1_SPEED,          ENC_SEND_WORD },
  { ACTION_LFO2_speed,          &LFO2Speed,           ENC_VAL_I16,     0,  4095,  1,  4, ParamId::PARAM_LFO2_SPEED,          ENC_SEND_WORD },
  { ACTION_VCA_level,           &VCALevel,            ENC_VAL_I16,     0,   128,  2, 10, ParamId::PARAM_VCA_LEVEL,           0 },
  { ACTION_LFO1_to_VCA,         &LFO1toVCA,           ENC_VAL_I16,     0,  1023,  1,  2, ParamId::PARAM_LFO1_TO_VCA,         ENC_SEND_WORD },
  { ACTION_LFO2_to_PWM,         &LFO2toPWM,           ENC_VAL_I16,     0,   511,  1,  2, ParamId::PARAM_LFO2_TO_PW,          ENC_SEND_WORD },
  { ACTION_ADSR3_to_PWM,        &ADSR3toPWM,          ENC_VAL_I16,  -512,   511,  1,  2, ParamId::PARAM_ADSR3_TO_PWM,        ENC_SEND_WORD | ENC_SEND_OFFSET_512 },
  { ACTION_ANALOG_DETUNE,       &unisonDetune,        ENC_VAL_I16,     0,   127,  1,  1, ParamId::PARAM_UNISON_DETUNE,       0 },
  { ACTION_ANALOG_DRIFT,        &analogDrift,         ENC_VAL_I16,     0,   127,  1,  1, ParamId::PARAM_ANALOG_DRIFT_AMOUNT, 0 },
  { ACTION_ANALOG_DRIFT_SPEED,  &analogDriftSpeed,    ENC_VAL_I16,     1,   255,  1,  1, ParamId::PARAM_ANALOG_DRIFT_SPEED,  0 },
  { ACTION_ANALOG_DRIFT_SPREAD, &analogDriftSpread,   ENC_VAL_I16,     1,   127,  1,  1, ParamId::PARAM_ANALOG_DRIFT_SPREAD, 0 },
};

static constexpr uint8_t NUM_ENCODER_BINDINGS =
    sizeof(encoderParamBindings) / sizeof(encoderParamBindings[0]);

static inline int32_t encoder_binding_read(const EncoderParamBinding& b) {
  switch (b.type) {
    case ENC_VAL_I8: return *(int8_t*)b.value;
    case ENC_VAL_U16: return *(uint16_t*)b.value;
    default: return *(int16_t*)b.value;
  }
}

static inline void encoder_binding_write(const EncoderParamBinding& b, int32_t v) {
  switch (b.type) {
    case ENC_VAL_I8: *(int8_t*)b.value = (int8_t)v; break;
    case ENC_VAL_U16: *(uint16_t*)b.value = (uint16_t)v; break;
    default: *(int16_t*)b.value = (int16_t)v; break;
  }
}

// Run the table-bound handler for action, if it has a binding. Edits only when
// the knob is already "selected" (first detent just shows the value), then
// queues the param for throttled transmission so the Screen and DCO stay in sync.
static bool __not_in_flash_func(encoder_apply_binding)(uint8_t encIndex, EncoderAction action, uint8_t direction, uint16_t speed) {
  for (uint8_t n = 0; n < NUM_ENCODER_BINDINGS; n++) {
    const EncoderParamBinding& b = encoderParamBindings[n];
    if (b.action != action) continue;

    if (encoderActionIsSelected) {
      const int32_t step = b.baseStep + ((int32_t)b.speedMultX2 * speed) / 2;
      int32_t value = encoder_binding_read(b);
      value += (direction == DIR_CW) ? step : -step;
      value = constrain(value, (int32_t)b.min, (int32_t)b.max);
      encoder_binding_write(b, value);
    }

    int32_t out = encoder_binding_read(b);
    if (b.flags & ENC_SEND_OFFSET_512) out += 512;
    bool isWord = (b.flags & ENC_SEND_WORD) != 0;

    // Throttle transmission to avoid saturating UART / Screen / DCO RX FIFO
    queue_encoder_param_send(encIndex, b.paramId, (uint16_t)out, isWord);
    return true;
  }
  return false;
}

// Map a detent on encoder i to an action for the current control mode. In
// NORMAL mode this also runs the first-detent selection tracking.
static EncoderAction __not_in_flash_func(resolve_encoder_action)(const EncoderStruct& encoder, int i) {
  switch (currentControlMode) {
    case NORMAL: {
      EncoderAction action;
      if (saveFlow != SaveFlow::IDLE) {
        action = (saveFlow == SaveFlow::NAME_EDIT) ? encoder.actions[1] : encoder.actionsAlt[1];
      } else if (ADSR1CurveSelect || ADSR2CurveSelect) {
        action = encoder.actionsAlt[2];
      } else if (funcKeyOn) {
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
    case MANUAL_CALIBRATION:
      return manualCalibrationActions[i];
    case MENU_NAVIGATION:
    case CALIBRATION_MENU:
      return menuNavigationActions[i];
  }
  return ACTION_NONE;
}

// Core0 ~99 µs: read all encoders and dispatch EncoderAction (ParamId / UI / cal).
void __not_in_flash_func(read_encoders)() {

  if ((millis() - encoderActionSelectedMillis) > encoderActionSelectedTimeout) {
    encoderActionSelected = ACTION_NONE;
    encoderActionIsSelected = false;
  }

  for (int i = 0; i < NUM_ENCODERS; i++) {

    EncoderStruct& encoder = encoders[i];

    uint8_t direction = encoder.MD_REncoder_Name.read(valorMUX1[encoder.muxPin1], valorMUX1[encoder.muxPin2]);
    if (!direction) {
      continue;
    }
    uint16_t speed = encoder.MD_REncoder_Name.speed();

    EncoderAction currentAction = resolve_encoder_action(encoder, i);

    // Plain value knobs are table-driven (throttled inside encoder_apply_binding)
    if (encoder_apply_binding(i, currentAction, direction, speed)) {
      continue;
    }

    switch (currentAction) {

      case ACTION_ADSR_CURVE_ATTACK:
        {
          int a = 0;
          if (encoderActionIsSelected) {
            a = (direction == DIR_CW) ? 1 : -1;
          }
          if (ADSR1CurveSelect == true) {
            ADSR1AttackCurveVal = constrain(ADSR1AttackCurveVal + a, 0, 7);
            queue_encoder_param_send(i, ParamId::PARAM_ADSR1_ATTACK_CURVE, (uint8_t)ADSR1AttackCurveVal, false);
          } else if (ADSR2CurveSelect == true) {
            ADSR2AttackCurveVal = constrain(ADSR2AttackCurveVal + a, 0, 7);
            queue_encoder_param_send(i, ParamId::PARAM_ADSR2_ATTACK_CURVE, (uint8_t)ADSR2AttackCurveVal, false);
          }
          break;
        }
      case ACTION_ADSR_CURVE_DECAY:
        {
          int a = 0;
          if (encoderActionIsSelected) {
            a = (direction == DIR_CW) ? 1 : -1;
          }
          if (ADSR1CurveSelect == true) {
            ADSR1DecayCurveVal = constrain(ADSR1DecayCurveVal + a, 0, 7);
            queue_encoder_param_send(i, ParamId::PARAM_ADSR1_DECAY_CURVE, (uint8_t)ADSR1DecayCurveVal, false);
          } else if (ADSR2CurveSelect == true) {
            ADSR2DecayCurveVal = constrain(ADSR2DecayCurveVal + a, 0, 7);
            queue_encoder_param_send(i, ParamId::PARAM_ADSR2_DECAY_CURVE, (uint8_t)ADSR2DecayCurveVal, false);
          }
          break;
        }

      case ACTION_select_preset:
        if (direction == DIR_CW) {
          presetSelectVal = presetSelectVal + (1 + (1 * speed));
        } else {
          presetSelectVal = presetSelectVal - (1 + (1 * speed));
        }
        presetSelectVal = constrain(presetSelectVal, 0, 255);
        if (saveFlow != SaveFlow::IDLE) {
          // Update screen with selected preset name (16 chars)
          byte presetNameScroll[16];
          get_preset_name(presetSelectVal, presetNameScroll);
          serial_send_preset_scroll((uint8_t)presetSelectVal, presetNameScroll);
        } else {
          preset_load_from_board(presetSelectVal);
        }
        break;

      case ACTION_select_char:
        if (direction == DIR_CW) {
          charSelectVal = charSelectVal + 1;
        } else {
          charSelectVal = charSelectVal - 1;
        }
        charSelectVal = constrain(charSelectVal, 32, 255);
        presetNameVal[presetCharPos] = charSelectVal;
        serial_send_preset_scroll(presetSelectVal, presetNameVal);
        break;

      case ACTION_select_char_pos:
        if (direction == DIR_CW) {
          presetCharPos = presetCharPos + 1;
        } else {
          presetCharPos = presetCharPos - 1;
        }
        if (presetCharPos > 15) {
          presetCharPos = 0;
        }
        charSelectVal = presetNameVal[presetCharPos];
        serial_send_save_char_select(presetCharPos);
        break;

      case ACTION_CALIBRATION_STAGE:
        {
          if (direction == DIR_CW) {
            if (manualCalibrationStage < INPUT_CAL_STAGE_MAX) {
              manualCalibrationStage = (uint8_t)(manualCalibrationStage + 1);
            }
          } else if (manualCalibrationStage > 0) {
            manualCalibrationStage = (uint8_t)(manualCalibrationStage - 1);
          }
          input_send_manual_cal_stage();
          break;
        }
      case ACTION_CALIBRATION_OFFSET:
        {
          uint8_t index = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
          const int32_t step = 1 + (int32_t)speed;
          if (INPUT_CAL_STAGE_IS_440((uint8_t)manualCalibrationStage)) {
            int32_t amp = (int32_t)manualAmpComp440[index];
            if (amp == 0) amp = AMP_COMP_440_MIN;
            if (direction == DIR_CW) {
              amp += step;
            } else {
              amp -= step;
            }
            amp = constrain(amp, AMP_COMP_440_MIN, AMP_COMP_440_MAX);
            manualAmpComp440[index] = (uint16_t)amp;
            queue_encoder_param_send(i, ParamId::PARAM_AMP_COMP_440, (uint16_t)amp, true);
          } else if (INPUT_CAL_STAGE_IS_PW_EDIT((uint8_t)manualCalibrationStage)) {
            uint8_t ch = INPUT_CAL_PW_CH(index);
            if (ch >= NUM_VOICES) ch = NUM_VOICES - 1;
            int32_t pw = (int32_t)manualPwCenter[ch];
            if (direction == DIR_CW) {
              pw += step;
            } else {
              pw -= step;
            }
            pw = constrain(pw, 0, CAL_PW_CENTER_MAX);
            manualPwCenter[ch] = (uint16_t)pw;
            queue_encoder_param_send(i, ParamId::PARAM_CAL_PW_CENTER, (uint16_t)pw, true);
          } else {
            if (direction == DIR_CW) {
              manualCalibrationInitAmpCompOffset[index] = manualCalibrationInitAmpCompOffset[index] + 1;
            } else {
              manualCalibrationInitAmpCompOffset[index] = manualCalibrationInitAmpCompOffset[index] - 1;
            }
            manualCalibrationInitAmpCompOffset[index] = constrain(manualCalibrationInitAmpCompOffset[index], -20, 20);
            serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_OFFSET,
                                          (uint8_t)manualCalibrationInitAmpCompOffset[index],
                                          /*sendToAll=*/false);
            serialSendParamByteToScreen(ParamId::PARAM_MANUAL_CALIBRATION_OFFSET,
                                        (uint8_t)manualCalibrationInitAmpCompOffset[index]);
          }
        }
        break;

      case ACTION_MENU_POS:
        if (direction == DIR_CW) {
          menuPos = menuPos + 1;
        } else {
          menuPos = menuPos - 1;
        }
        menuPos = constrain(menuPos, 0, menuPosMax);
        queue_encoder_param_send(i, ParamId::PARAM_UI_MENU_POSITION, (uint8_t)menuPos, false);
        break;

      default:
        break;
    }
  }

  // Flush any pending throttled encoder transmissions whose timer elapsed
  flush_encoder_throttle();
}