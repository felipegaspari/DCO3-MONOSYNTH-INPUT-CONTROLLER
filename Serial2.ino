static inline INPUT_ALWAYS_INLINE void pack_u16_le4(uint8_t* dst, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  encode_u16_le(dst + 0, a);
  encode_u16_le(dst + 2, b);
  encode_u16_le(dst + 4, c);
  encode_u16_le(dst + 6, d);
}

// @1 ms / preset load: TX manual control blocks to the DCO, and 'a'/'b' to the Screen.
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
    serial_frame_write(DCO_PORT, INPUT_CMD_ADSR1_BLOCK, dataArrayDCO, INPUT_SERIAL_LEN_ADSR_BLOCK);
#endif
#ifdef ENABLE_SCREEN_LINK
    serial_frame_write(SCREEN_PORT, INPUT_CMD_ADSR1_BLOCK, dataArrayScreen, INPUT_SERIAL_LEN_ADSR_BLOCK);
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
    serial_frame_write(DCO_PORT, INPUT_CMD_ADSR2_BLOCK, dataArrayDCO, INPUT_SERIAL_LEN_ADSR_BLOCK);
#endif
#ifdef ENABLE_SCREEN_LINK
    serial_frame_write(SCREEN_PORT, INPUT_CMD_ADSR2_BLOCK, dataArrayScreen, INPUT_SERIAL_LEN_ADSR_BLOCK);
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
    serial_frame_write(DCO_PORT, INPUT_CMD_ADSR3_BLOCK, dataArray, INPUT_SERIAL_LEN_ADSR_BLOCK);
#endif
  }

  if (VCFPotsControlManual || presetLoading) {
    uint8_t dataArray[8];
    pack_u16_le4(dataArray, CUTOFF, RESONANCE,
                 (uint16_t)ADSR2toVCF, LFO2toVCF);
#ifdef ENABLE_DCO_LINK
    serial_frame_write(DCO_PORT, INPUT_CMD_FILTER_BLOCK, dataArray, INPUT_SERIAL_LEN_FILTER_BLOCK);
#endif
  }

  if (VCAPotsControlManual || presetLoading) {
#ifdef ENABLE_DCO_LINK
    uint8_t p[INPUT_SERIAL_LEN_PARAM_16];
    encode_param_p(p, (uint8_t)ParamId::PARAM_ADSR1_TO_VCA, (int16_t)ADSR1toVCA);
    serial_frame_write(DCO_PORT, INPUT_CMD_PARAM_16, p, INPUT_SERIAL_LEN_PARAM_16);
#endif
  }

  if (PWMPotsControlManual || presetLoading) {
#ifdef ENABLE_DCO_LINK
    uint8_t p[INPUT_SERIAL_LEN_PARAM_16];
    encode_param_p(p, (uint8_t)ParamId::PARAM_PW_VALUE, (int16_t)PW);
    serial_frame_write(DCO_PORT, INPUT_CMD_PARAM_16, p, INPUT_SERIAL_LEN_PARAM_16);
#endif
  }
}

// Legacy flag-driven DCO TX (portamento/sync/etc). Not scheduled in loop1 today.
// Gates are `>= 1`: a hardware UART reports 0 or 1, never a free-byte count, so
// asking for room for a whole frame would never let anything through. A full FIFO
// leaves the flag set for the next call.
void sendSerial() {  // to DCO


  if (serial_send_portamentoFlag) {
    if (DCO_PORT.availableForWrite() >= 1) {
      byte byteArray[2] = { (uint8_t)'r', (uint8_t)portamentoTime };
      DCO_PORT.write(byteArray, 2);
      serial_send_portamentoFlag = false;
    }
  }

  if (serial_send_oscSyncModeFlag) {
    if (DCO_PORT.availableForWrite() >= 1) {
      byte byteArray[2] = { (uint8_t)'t', (uint8_t)oscSyncMode };
      DCO_PORT.write(byteArray, 2);
      serial_send_oscSyncModeFlag = false;
    }
  }

  if (serial_send_OSC1IntervalFlag) {
    if (DCO_PORT.availableForWrite() >= 1) {
      byte byteArray[2] = { (uint8_t)'y', (uint8_t)OSC1Interval };
      DCO_PORT.write(byteArray, 2);
      serial_send_OSC1IntervalFlag = false;
    }
  }

  if (serial_send_OSC2IntervalFlag) {
    if (DCO_PORT.availableForWrite() >= 1) {
      byte byteArray[2] = { (uint8_t)'z', (uint8_t)OSC2Interval };
      DCO_PORT.write(byteArray, 2);
      serial_send_OSC2IntervalFlag = false;
    }
  }

  if (serial_send_LFO1SpeedFlag) {
    if (DCO_PORT.availableForWrite() >= 1) {
      byte *b = (byte *)&LFO1Speed;
      byte byteArray[3] = { (byte)'l', b[0], b[1] };
      DCO_PORT.write(byteArray, 3);
      serial_send_LFO1SpeedFlag = false;
    }
  }

  if (serial_send_LFO1toDCOFlag) {
    if (DCO_PORT.availableForWrite() >= 1) {
      byte *b = (byte *)&LFO1toDCO;
      byte byteArray[3] = { (byte)'m', b[0], b[1] };
      DCO_PORT.write(byteArray, 3);
      serial_send_LFO1toDCOFlag = false;
    }
  }

  if (serial_send_LFO1toDCOWaveChangeFlag) {
    if (DCO_PORT.availableForWrite() >= 1) {
      byte byteArray[2] = { (uint8_t)'b', (uint8_t)LFO1Waveform };
      DCO_PORT.write(byteArray, 2);
      serial_send_LFO1toDCOWaveChangeFlag = false;
    }
  }

  if (serialSendADSR3ControlValuesFlag) {
    if (DCO_PORT.availableForWrite() >= 1) {
      byte ADSR3BytesArray[5];
      ADSR3BytesArray[0] = (byte)'s';
      ADSR3BytesArray[1] = (byte)(ADSR3_attack / 16);
      ADSR3BytesArray[2] = (byte)(ADSR3_decay / 16);
      ADSR3BytesArray[3] = (byte)(ADSR3_sustain / 16);
      ADSR3BytesArray[4] = (byte)(ADSR3_release / 16);
      DCO_PORT.write(ADSR3BytesArray, 5);
      serialSendADSR3ControlValuesFlag = false;
    }
  }

  if (serialSendADSR3toDCOFlag) {
    if (DCO_PORT.availableForWrite() >= 1) {
      byte *b = (byte *)&ADSR3toDETUNE1;
      byte byteArray[3] = { (byte)'w', b[0], b[1] };
      DCO_PORT.write(byteArray, 3);
      serialSendADSR3toDCOFlag = false;
    }
  }
  if (serialSendADSR3ToOscSelectFlag) {
    if (DCO_PORT.availableForWrite() >= 1) {
      byte byteArray[2] = { (byte)'c', (uint8_t)ADSR3ToOscSelect };
      DCO_PORT.write(byteArray, 2);
      serialSendADSR3ToOscSelectFlag = false;
    }
  }
}
