// Preset directory cache + board-driven save/load.
//
// The DCO's LittleFS preset store is the single source of truth for the whole
// system; Input caches all 256 slot names in RAM (fetched once at boot and
// again whenever entering preset select/save mode) and asks the DCO to
// save/load/report the live slot.

static constexpr uint16_t INPUT_PRESET_NUM_SLOTS = 256;
static byte presetDir[INPUT_PRESET_NUM_SLOTS][16];

// 'N': ask the DCO to (re)send the whole 256-slot directory.
void request_preset_directory() {
  uint8_t pad = 0;
  serial_frame_write(DcoDma, CMD_PRESET_DIR_REQUEST, &pad,
                     SERIAL_LEN_PRESET_DIR_REQUEST);
}

// 'O': one directory entry pushed by the DCO [slot:u8][name:16 ASCII].
// Registered in Serial.ino's dcoLinkCommands[], hence not static.
void input_handle_preset_dir_entry(char, const uint8_t *payload, uint8_t) {
  uint8_t slot = payload[0];
  if (slot < INPUT_PRESET_NUM_SLOTS) {
    memcpy(presetDir[slot], payload + 1, 16);
  }
}

// 'L': DCO confirms a preset finished loading (Boot recall, MIDI PC, USB, or
// Panel). Central place where the panel syncs its name cache, turns off manual
// controls, and updates the Screen. Registered in Serial.ino's
// dcoLinkCommands[], hence not static.
void input_handle_preset_loaded(char, const uint8_t *payload, uint8_t) {
  // Turn off manual controls so physical pots/faders don't overwrite the loaded
  // preset!
  input_disable_all_manual_controls();
  const uint8_t slot = payload[0];
  if (slot >= INPUT_PRESET_NUM_SLOTS)
    return;

  currentPreset = slot;
  presetSelectVal = currentPreset;
  memcpy(presetName, presetDir[slot], 16);
  presetNameString = String((char *)presetName);
  ledRefreshPending = true;

  // No signal here: the DCO brackets its own recall mirror with
  // Silent/PresetScroll, and sending it here would race ahead of the mirror and
  // lift the silence early.
  serial_send_preset_scroll(currentPreset, presetName);
}

// Copy preset name bytes into caller array from the local RAM cache.
void get_preset_name(byte presetN, byte (&myarray)[16]) {
  if (presetN >= INPUT_PRESET_NUM_SLOTS)
    return;
  memcpy(myarray, presetDir[presetN], 16);
}

// Debug helper: dump cached directory to USB Serial.
void dumpPresetBankToSerial() {
#ifdef ENABLE_SERIAL
  Serial.println(F("=== Preset directory cache (from DCO) ==="));
  for (uint16_t p = 0; p < INPUT_PRESET_NUM_SLOTS; ++p) {
    Serial.print(F("Preset "));
    Serial.print(p);
    Serial.print(F("  name=\""));
    for (uint8_t i = 0; i < 16; ++i) {
      char c = (char)presetDir[p][i];
      if (c < 32)
        c = ' ';
      Serial.print(c);
    }
    Serial.println(F("\""));
  }
  Serial.println(F("=== End of preset directory cache ==="));
#endif
}

// PARAM_PRESET_SAVE (170): Snapshot live state into slot under presetNameVal.
void preset_save_to_board(uint16_t slot) {
  if (slot >= INPUT_PRESET_NUM_SLOTS)
    return;

  input_disable_all_manual_controls();

  serial_send_preset_name_to_mainboard();
  serial_send_param_change_byte(ParamId::PARAM_PRESET_SAVE, (byte)slot, false);

  memcpy(presetDir[slot], presetNameVal, 16);
  memcpy(presetName, presetNameVal, 16);

  saveFlow = SaveFlow::IDLE;
  currentPreset = slot;
  presetSelectVal = currentPreset;

  set_LED_Status(LED_REFRESH_ALL, 0);
}

// PARAM_PRESET_LOAD (171): Request DCO to recall slot.
void preset_load_from_board(uint16_t slot) {
  if (slot >= INPUT_PRESET_NUM_SLOTS)
    return;

  input_disable_all_manual_controls();

  // Ask DCO to load the preset
  serial_send_param_change_byte(ParamId::PARAM_PRESET_LOAD, (byte)slot, false);

  currentPreset = slot;
  presetSelectVal = currentPreset;
  memcpy(presetName, presetDir[slot], 16);
  presetNameString = String((char *)presetName);
  ledRefreshPending = true;

  // Optimistic Screen update ahead of the DCO
  serial_send_preset_scroll(currentPreset, presetName);
  serial_send_signal(SIGNAL_PRESET_LOAD_SCROLL);
}

// Post-save UI/state cleanup
void writePresetActions(uint16_t presetN) {
  input_disable_all_manual_controls();
  saveFlow = SaveFlow::IDLE;
  currentPreset = presetN;
  presetSelectVal = currentPreset;
  set_LED_Status(LED_REFRESH_ALL, 0);
}

// Post-load UI/state cleanup
void loadPresetActions(uint16_t presetN) {
  input_disable_all_manual_controls();
  currentPreset = presetN;
  presetSelectVal = currentPreset;
}