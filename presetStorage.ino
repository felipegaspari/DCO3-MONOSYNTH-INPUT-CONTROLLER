// Allocate RAM array based on canonical DCO-PROTOCOL constants
byte presetDir[PRESET_NUM_SLOTS][PRESET_NAME_LEN] = {0};
byte presetNameVal[PRESET_NAME_LEN] = { 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32 };


void request_preset_directory() {
  uint8_t pad = 0;
  serial_frame_write(DcoDma, CMD_PRESET_DIR_REQUEST, &pad, SERIAL_LEN_PRESET_DIR_REQUEST);
}

void input_handle_preset_dir_entry(char, const uint8_t *payload, uint8_t) {
  uint8_t slot = payload[0];
  if (slot < PRESET_NUM_SLOTS) {
    memcpy(presetDir[slot], payload + 1, PRESET_NAME_LEN);
  }
}

void input_handle_preset_loaded(char, const uint8_t *payload, uint8_t) {
  input_disable_all_manual_controls();
  const uint8_t slot = payload[0];
  if (slot >= PRESET_NUM_SLOTS) return;

  currentPreset = slot;
  presetSelectVal = currentPreset;
  memcpy(presetName, presetDir[slot], PRESET_NAME_LEN);
  memcpy(presetNameVal, presetDir[slot], PRESET_NAME_LEN);
  presetNameString = String((char *)presetName);
  ledRefreshPending = true;

  // =========================================================================
  // FIX: Unfreeze the screen immediately and draw the new preset!
  // =========================================================================
  serial_send_signal(SCREEN_SIGNAL_NORMAL); // Signal 1 = ScreenMode::PresetScroll (Unfreeze)
  serial_send_preset_scroll(currentPreset, presetName);
}

void get_preset_name(byte presetN, byte (&myarray)[PRESET_NAME_LEN]) {
  if (presetN >= PRESET_NUM_SLOTS) return;
  memcpy(myarray, presetDir[presetN], PRESET_NAME_LEN);
}

void preset_save_to_board(uint16_t slot) {
  if (slot >= PRESET_NUM_SLOTS) return;

  input_disable_all_manual_controls();

  serial_send_preset_name_to_mainboard();
  serial_send_param_change_byte(ParamId::PARAM_PRESET_SAVE, (byte)slot, false);

  memcpy(presetDir[slot], presetNameVal, PRESET_NAME_LEN);
  memcpy(presetName, presetNameVal, PRESET_NAME_LEN);

  currentPreset = slot;
  presetSelectVal = currentPreset;

  set_LED_Status(LED_REFRESH_ALL, 0);
}

void preset_load_from_board(uint16_t slot) {
  if (slot >= PRESET_NUM_SLOTS) return;

  // 1. Drop local panel overrides
  input_disable_all_manual_controls();

  // 2. Request DCO to load the preset
  serial_send_param_change_byte(ParamId::PARAM_PRESET_LOAD, (byte)slot, false);

  // 3. Keep local selector in sync
  currentPreset = (byte)slot;
  presetSelectVal = currentPreset;
}

void writePresetActions(uint16_t presetN) {
  input_disable_all_manual_controls();
  currentPreset = presetN;
  presetSelectVal = currentPreset;
  set_LED_Status(LED_REFRESH_ALL, 0);
}

void loadPresetActions(uint16_t presetN) {
  input_disable_all_manual_controls();
  currentPreset = presetN;
  presetSelectVal = currentPreset;
}
