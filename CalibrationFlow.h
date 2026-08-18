#ifndef __CALIBRATION_FLOW_H__
#define __CALIBRATION_FLOW_H__

#include "Flow.h"

class CalibrationFlow : public Flow {
private:
  bool confirmed = false;

public:
  void onEnter() override {
    confirmed = false;
    manualCalibration = true;
    manualCalibrationStage = 0;
    
    // Notify screen and DCO that manual calibration started
    serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_FLAG, manualCalibration, true);
    serialSendParamByteToScreen(ParamId::PARAM_UI_VOICE_TOPOLOGY, (uint8_t)NUM_OSCILLATORS);
    input_send_manual_cal_stage();
  }

  void onEncoder(uint8_t encIndex, uint8_t direction, uint16_t speed) override {
    // Encoder 9 (Index 8): Change Calibration Stage
    if (encIndex == 8) {
      if (direction == DIR_CW) {
        if (manualCalibrationStage < INPUT_CAL_STAGE_MAX) {
          manualCalibrationStage++;
        }
      } else {
        if (manualCalibrationStage > 0) {
          manualCalibrationStage--;
        }
      }
      input_send_manual_cal_stage();
    }
    // Encoder 8 (Index 7): Adjust Value / Offset
    else if (encIndex == 7) {
      uint8_t index = INPUT_CAL_STAGE_TO_OSC(manualCalibrationStage);
      const int32_t step = 1 + (int32_t)speed;

      // 440 Hz Amplitude Compensation
      if (INPUT_CAL_STAGE_IS_440((uint8_t)manualCalibrationStage)) {
        int32_t amp = (int32_t)manualAmpComp440[index];
        if (amp == 0) amp = AMP_COMP_440_MIN; 
        if (direction == DIR_CW) amp += step; else amp -= step;
        amp = constrain(amp, AMP_COMP_440_MIN, AMP_COMP_440_MAX);
        manualAmpComp440[index] = (uint16_t)amp;
        serial_send_param_change(ParamId::PARAM_AMP_COMP_440, (uint16_t)amp, true);
      } 
      // Pulse Width Editing
      else if (INPUT_CAL_STAGE_IS_PW_EDIT((uint8_t)manualCalibrationStage)) {
        uint8_t ch = INPUT_CAL_PW_CH(index);
        if (ch >= NUM_VOICES) ch = NUM_VOICES - 1;
        int32_t pw = (int32_t)manualPwCenter[ch];
        if (direction == DIR_CW) pw += step; else pw -= step;
        pw = constrain(pw, 0, CAL_PW_CENTER_MAX); 
        manualPwCenter[ch] = (uint16_t)pw;
        serial_send_param_change(ParamId::PARAM_CAL_PW_CENTER, (uint16_t)pw, true);
      } 
      // Standard Amplitude Compensation Offset
      else {
        if (direction == DIR_CW) manualCalibrationInitAmpCompOffset[index]++;
        else manualCalibrationInitAmpCompOffset[index]--;
        manualCalibrationInitAmpCompOffset[index] = constrain(manualCalibrationInitAmpCompOffset[index], -20, 20);
        
        serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_OFFSET, (uint8_t)manualCalibrationInitAmpCompOffset[index], false);
        serialSendParamByteToScreen(ParamId::PARAM_MANUAL_CALIBRATION_OFFSET, (uint8_t)manualCalibrationInitAmpCompOffset[index]);
      }
    }
  }

  void onButton(uint8_t btnIndex, ButtonState state) override {
    if (state != RELEASED) return; 

    // Button 8 (Index 7): BACK (Return to Calibration Menu)
    if (btnIndex == 7) {
      confirmed = false;
      exitActiveFlow();
    }
    // Button 9 (Index 8): SELECT/CONFIRM (Save to DCO & Exit completely)
    else if (btnIndex == 8) {
      confirmed = true;
      // Store calibration on DCO
      serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_STORE, 1);
      // Dismiss the menu on the screen
      serial_send_param_change_byte(ParamId::PARAM_UI_CALIBRATION_DISMISS, 0);
      exitActiveFlow();
    }
  }

  void onExit() override {
    manualCalibration = false;
    serial_send_param_change_byte(ParamId::PARAM_MANUAL_CALIBRATION_FLAG, manualCalibration);

    if (confirmed) {
      // Confirmed/saved -> Return to normal synthesizer play mode
      currentControlMode = NORMAL;
    } else {
      // Cancelled/Back -> Return control to the Calibration Menu tabs
      currentControlMode = CALIBRATION_MENU;
    }
  }
};

extern CalibrationFlow calibrationFlow;

#endif