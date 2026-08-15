static inline INPUT_ALWAYS_INLINE void pack_u16_le4(uint8_t* dst, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  encode_u16_le(dst + 0, a);
  encode_u16_le(dst + 2, b);
  encode_u16_le(dst + 4, c);
  encode_u16_le(dst + 6, d);
}

// @1 ms / preset load: slim LE 'a'..'d'/'p' out on DCO_PORT, and 'a'/'b' to the Screen.
void __not_in_flash_func(serial_send_manual_controls)(bool presetLoading) {
  if (faderRow1ControlManual || presetLoading) {
    uint8_t dataArrayDCO[8];
    uint16_t ADSR1_attack_serial  = linToExpLookup[ADSR1_attack];
    uint16_t ADSR1_decay_serial   = linToExpLookup[ADSR1_decay];
    uint16_t ADSR1_release_serial = linToExpLookup[ADSR1_release];
    pack_u16_le4(dataArrayDCO,
                 ADSR1_attack_serial, ADSR1_decay_serial,
                 ADSR1_sustain, ADSR1_release_serial);

    uint8_t dataArrayScreen[8];
    pack_u16_le4(dataArrayScreen,
                 ADSR1_attack, ADSR1_decay, ADSR1_sustain, ADSR1_release);

#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, INPUT_CMD_ADSR1_BLOCK, dataArrayDCO, INPUT_SERIAL_LEN_ADSR_BLOCK);
#endif
#ifdef ENABLE_SCREEN_LINK
    serial_frame_write(ScreenDma, INPUT_CMD_ADSR1_BLOCK, dataArrayScreen, INPUT_SERIAL_LEN_ADSR_BLOCK);
#endif
  }

  if ((faderRow2ControlManual && !ADSR3Enabled) || presetLoading) {
    uint8_t dataArrayDCO[8];
    uint16_t ADSR2_attack_serial  = linToExpLookup[ADSR2_attack];
    uint16_t ADSR2_decay_serial   = linToExpLookup[ADSR2_decay];
    uint16_t ADSR2_release_serial = linToExpLookup[ADSR2_release];
    pack_u16_le4(dataArrayDCO,
                 ADSR2_attack_serial, ADSR2_decay_serial,
                 ADSR2_sustain, ADSR2_release_serial);

    uint8_t dataArrayScreen[8];
    pack_u16_le4(dataArrayScreen,
                 ADSR2_attack, ADSR2_decay, ADSR2_sustain, ADSR2_release);

#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, INPUT_CMD_ADSR2_BLOCK, dataArrayDCO, INPUT_SERIAL_LEN_ADSR_BLOCK);
#endif
#ifdef ENABLE_SCREEN_LINK
    serial_frame_write(ScreenDma, INPUT_CMD_ADSR2_BLOCK, dataArrayScreen, INPUT_SERIAL_LEN_ADSR_BLOCK);
#endif
  }

  if ((faderRow2ControlManual && ADSR3Enabled) || presetLoading) {
    uint8_t dataArray[8];
    uint16_t ADSR3_attack_serial  = linToExpLookup[ADSR3_attack];
    uint16_t ADSR3_decay_serial   = linToExpLookup[ADSR3_decay];
    uint16_t ADSR3_release_serial = linToExpLookup[ADSR3_release];
    pack_u16_le4(dataArray,
                 ADSR3_attack_serial, ADSR3_decay_serial,
                 ADSR3_sustain, ADSR3_release_serial);
#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, INPUT_CMD_ADSR3_BLOCK, dataArray, INPUT_SERIAL_LEN_ADSR_BLOCK);
#endif
  }

  if (VCFPotsControlManual || presetLoading) {
    uint8_t dataArray[8];
    pack_u16_le4(dataArray, CUTOFF, RESONANCE,
                 (uint16_t)ADSR2toVCF, LFO2toVCF);
#ifdef ENABLE_DCO_LINK
    serial_frame_write(DcoDma, INPUT_CMD_FILTER_BLOCK, dataArray, INPUT_SERIAL_LEN_FILTER_BLOCK);
#endif
  }

  if (VCAPotsControlManual || presetLoading) {
#ifdef ENABLE_DCO_LINK
    uint8_t p[INPUT_SERIAL_LEN_PARAM_16];
    encode_param_p(p, (uint8_t)ParamId::PARAM_ADSR1_TO_VCA, (int16_t)ADSR1toVCA);
    serial_frame_write(DcoDma, INPUT_CMD_PARAM_16, p, INPUT_SERIAL_LEN_PARAM_16);
#endif
  }

  if (PWMPotsControlManual || presetLoading) {
#ifdef ENABLE_DCO_LINK
    uint8_t p[INPUT_SERIAL_LEN_PARAM_16];
    encode_param_p(p, (uint8_t)ParamId::PARAM_PW_VALUE, (int16_t)PW);
    serial_frame_write(DcoDma, INPUT_CMD_PARAM_16, p, INPUT_SERIAL_LEN_PARAM_16);
#endif
  }
}

// Legacy flag-driven big-endian TX ('r'/'t'/'y'/'z'/'l'/'m'/'b'/'s'/'w'/'c')
// retired; the panel uses the slim LE frames above.
void sendSerial() {
}
