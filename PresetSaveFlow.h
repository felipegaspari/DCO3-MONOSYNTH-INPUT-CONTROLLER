#ifndef __PRESET_SAVE_FLOW_H__
#define __PRESET_SAVE_FLOW_H__

#include "Flow.h"
#include "_build_libs/MD_REncoder_fela/src/MD_REncoder_fela.h"
#include "_build_libs/DCO-PROTOCOL/serial_input_protocol.h"

void request_preset_directory();
void serial_send_signal(byte signal);
void get_preset_name(byte presetN, byte (&myarray)[PRESET_NAME_LEN]);
void serial_send_preset_scroll(byte presetNumber, byte presetNameSerial[]);
void serial_send_save_char_select(byte serialPresetChar);
void preset_save_to_board(uint16_t slot);

extern uint8_t presetSelectVal;
extern byte currentPreset;
extern byte presetName[17];
extern byte presetNameVal[PRESET_NAME_LEN];

class PresetSaveFlow : public Flow {
private:
  enum Step { SELECT_SLOT, EDIT_NAME } step;
  uint8_t slot;
  uint8_t charPos;
  byte nameBuffer[PRESET_NAME_LEN];
  bool savedSuccessfully; // Prevents onExit from wiping out the "Preset Saved" banner

  // Fetch the actual name on disk for targetSlot (formats empty slots as clean spaces)
  void fetchSlotName(uint8_t targetSlot, byte* outBuffer) {
    get_preset_name(targetSlot, *(byte(*)[PRESET_NAME_LEN])outBuffer);
    for (int k = 0; k < PRESET_NAME_LEN; k++) {
      if (outBuffer[k] < 32 || outBuffer[k] > 126) {
        outBuffer[k] = ' ';
      }
    }
  }

public:
  void onEnter() override {
    step = SELECT_SLOT;
    slot = currentPreset;
    presetSelectVal = slot;
    charPos = 0;
    savedSuccessfully = false;

    request_preset_directory();
    serial_send_signal(SIGNAL_SAVE_SELECT_ENTER);
    
    // Show current slot's real name (or blank if empty)
    fetchSlotName(slot, nameBuffer);
    serial_send_preset_scroll(slot, nameBuffer);
  }

  void onEncoder(uint8_t encIndex, uint8_t direction, uint16_t speed) override {
    if (step == SELECT_SLOT) {
      // Encoder 9 (Index 8): Scroll slot to save to
      if (encIndex == 8) { 
        int temp = slot + ((direction == DIR_CW) ? 1 : -1) * (1 + speed);
        slot = (uint8_t)constrain(temp, 0, PRESET_NUM_SLOTS - 1);
        presetSelectVal = slot;
        
        // Displays empty slots as blank spaces, not current preset name!
        fetchSlotName(slot, nameBuffer);
        serial_send_preset_scroll(slot, nameBuffer);
      }
    } 
    else if (step == EDIT_NAME) {
      // Encoder 8 (Index 7): Change character
      if (encIndex == 7) { 
        int tempChar = (int)nameBuffer[charPos];
        tempChar += (direction == DIR_CW) ? 1 : -1;
        tempChar = constrain(tempChar, 32, 255);
        nameBuffer[charPos] = (byte)tempChar;
        
        memcpy(presetNameVal, nameBuffer, PRESET_NAME_LEN);
        serial_send_preset_scroll(slot, presetNameVal);
      } 
      // Encoder 9 (Index 8): Move Cursor
      else if (encIndex == 8) { 
        int tempPos = charPos;
        if (direction == DIR_CW) {
          tempPos++;
          if (tempPos > 15) tempPos = 0;
        } else {
          tempPos--;
          if (tempPos < 0) tempPos = 15;
        }
        charPos = (uint8_t)tempPos;
        
        serial_send_save_char_select(charPos);
      }
    }
  }

  void onButton(uint8_t btnIndex, ButtonState state) override {
    // BUTTON 8 (Index 7 / Encoder 8 Push): CANCEL / EXIT
    if (btnIndex == 7) {
      if (state == RELEASED || state == HELD) {
        exitActiveFlow();
      }
    }
    // BUTTON 9 (Index 8 / Encoder 9 Push): FORWARD / CONFIRM
    else if (btnIndex == 8) {
      if (step == SELECT_SLOT) {
        // SINGLE CLICK: Advance to Name Edit
        if (state == RELEASED) {
          step = EDIT_NAME;
          charPos = 0;
          
          // Pre-populate name editor with active patch name if target slot is empty
          fetchSlotName(slot, nameBuffer);
          bool isBlank = true;
          for (int k = 0; k < PRESET_NAME_LEN; k++) {
            if (nameBuffer[k] != ' ') { isBlank = false; break; }
          }
          if (isBlank) {
            memcpy(nameBuffer, presetName, PRESET_NAME_LEN);
          }
          memcpy(presetNameVal, nameBuffer, PRESET_NAME_LEN);

          serial_send_signal(SIGNAL_SAVE_NAME_EDIT);
          serial_send_save_char_select(charPos);
        }
      } 
      else if (step == EDIT_NAME) {
        // HOLD TO SAVE: Commit and save to disk
        if (state == HELD) {
          memcpy(presetNameVal, nameBuffer, PRESET_NAME_LEN);
          presetSelectVal = slot;
          
          preset_save_to_board(slot);
          serial_send_preset_scroll(slot, presetNameVal);
          serial_send_signal(SIGNAL_PRESET_SAVED);
          
          savedSuccessfully = true; // Prevents onExit from killing the banner!
          exitActiveFlow();
        }
      }
    }
  }

  void onExit() override {
    // Only send exit signal if cancelled (if saved, Screen stays on PresetSaved banner)
    if (!savedSuccessfully) {
      serial_send_signal(SIGNAL_SAVE_EXIT);
      serial_send_preset_scroll(currentPreset, presetName);
    }
  }
};

extern PresetSaveFlow presetSaveFlow;

#endif