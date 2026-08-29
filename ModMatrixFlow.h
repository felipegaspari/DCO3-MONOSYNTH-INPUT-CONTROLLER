#ifndef __MOD_MATRIX_FLOW_H__
#define __MOD_MATRIX_FLOW_H__

#include "Flow.h"
#include "params.h"
#include "_build_libs/DCO-PROTOCOL/serial_input_protocol.h"

class ModMatrixFlow : public Flow {
private:
  uint8_t slot;

public:
  void onEnter() override {
    slot = 0;
    // Tell the Screen to enter Menu Mode 8 (Mod Matrix)
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE, 8); 
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION, slot);
  }

  void onEncoder(uint8_t encIndex, uint8_t direction, uint16_t speed) override {
    // ENC 9 (Index 8): Scroll Slot
    if (encIndex == 8) {
      int temp = slot + ((direction == DIR_CW) ? 1 : -1);
      slot = (uint8_t)constrain(temp, 0, MOD_SLOT_COUNT_INPUT - 1);
      serial_send_param_change_byte(ParamId::PARAM_UI_MENU_POSITION, slot);
    }
    // ENC 6 (Index 5): Change Source
    else if (encIndex == 5) {
      int temp = modSlotSource[slot];
      temp += (direction == DIR_CW) ? 1 : -1;
      temp = constrain(temp, 0, MOD_SRC_COUNT - 1);
      modSlotSource[slot] = (uint8_t)temp;
      
      uint8_t paramId = ParamId::PARAM_MOD_SLOT0_SOURCE + (slot * 3);
      serial_send_param_change_byte(paramId, modSlotSource[slot]);
    }
    // ENC 7 (Index 6): Change Destination
    else if (encIndex == 6) {
      int temp = modSlotDest[slot];
      temp += (direction == DIR_CW) ? 1 : -1;
      temp = constrain(temp, 0, MOD_DEST_COUNT - 1);
      modSlotDest[slot] = (uint8_t)temp;
      
      uint8_t paramId = ParamId::PARAM_MOD_SLOT0_DEST + (slot * 3);
      serial_send_param_change_byte(paramId, modSlotDest[slot]);
    }
    // ENC 8 (Index 7): Change Amount
    else if (encIndex == 7) {
        // Smooth acceleration for the +/- 4096 range
        int32_t step = 1 + ((int32_t)speed * 10);
        int32_t temp = modSlotDepth[slot];
        temp += (direction == DIR_CW) ? step : -step;
        temp = constrain(temp, -4096, 4096); // <--- Fixed bounds
        modSlotDepth[slot] = (int16_t)temp;
        
        uint8_t paramId = ParamId::PARAM_MOD_SLOT0_DEPTH + (slot * 3);
        serial_send_param_change(paramId, (uint16_t)modSlotDepth[slot]);
    }
  }

  void onButton(uint8_t btnIndex, ButtonState state) override {
    // 1. BUTTON 8 (Index 7): BACK / EXIT
    if (btnIndex == 7) {
      if (state == RELEASED || state == HELD) {
        exitActiveFlow();
      }
      return;
    }

    // 2. BUTTON 9 (Index 8): CLEAR SLOT SHORTCUT
    if (btnIndex == 8) {
      if (state == RELEASED) {
        modSlotSource[slot] = SRC_OFF;
        modSlotDest[slot]   = DEST_PITCH;
        modSlotDepth[slot]  = 0;
        
        uint8_t baseId = ParamId::PARAM_MOD_SLOT0_SOURCE + (slot * 3);
        serial_send_param_change_byte(baseId, modSlotSource[slot]);
        serial_send_param_change_byte(baseId + 1, modSlotDest[slot]);
        serial_send_param_change(baseId + 2, 0);
      }
      return;
    }

    // 3. ANY OTHER BUTTON: Jump directly to other menus!
    if (state == RELEASED) {
      ButtonAction action = buttons[btnIndex].actionMenuNavigationReleased;
      
      // If the button is mapped to another menu (and not Mod Matrix itself), jump to it!
      if (action != BTN_ACTION_NONE && action != TG_MOD_MATRIX_MENU) {
        exitActiveFlow();              // Cleanly closes Mod Matrix
        execute_button_action(action); // Instantly opens the target menu (e.g. ADSR1, DCO, etc.)
      }
    }
  }

  void onExit() override {
    // Clear the menu mode to exit the screen state
    serial_send_param_change_byte(ParamId::PARAM_UI_MENU_MODE, 0); 
    currentControlMode = NORMAL;
  }
};

extern ModMatrixFlow modMatrixFlow;

#endif