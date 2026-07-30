// Send uint16 frame 'u' to the DCO. Currently unused helper.
void sendUint16(uint16_t f) {
  byte *b = (byte *)&f;

  DCO_PORT.write((char *)"u");

  DCO_PORT.write(b, 2);
}

// Send float bytes after 't' to the DCO. Currently unused helper.
void sendFloat(float f) {
  byte *b = (byte *)&f;

  DCO_PORT.print("t");
  byte ndata = 0;
  for (int i = 0; i < 4; i++) {

    DCO_PORT.write(b[i]);
  }
  return;
}

// Send 'k' OK to the DCO. Currently unused helper.
void sendOK() {

  DCO_PORT.write((char *)"k");
  //Serial.println("Sent OK");
}

// Legacy autotune kick to the DCO. Currently unused.
void serial_send_autotune() {
  byte autotune_on = 255;
  DCO_PORT.write((char *)"a");
  DCO_PORT.write(autotune_on);
  DCO_PORT.flush();
  //Serial.println("Sent autotune on");
}

// Screen UI mode signal ('s' + byte).
void serial_send_signal(byte signal) {
#ifdef ENABLE_SCREEN_LINK

  SCREEN_PORT.write((char *)"s");

  SCREEN_PORT.write(signal);
#endif
}


// Send 'p' 16-bit ParamId to the DCO, and to the Screen when sendToAll.
void serial_send_param_change(byte param, uint16_t paramValue, bool sendToAll) {
  byte bytesArray[5] = { (uint8_t)'p', param, highByte(paramValue), lowByte(paramValue), finishByte };
#ifdef ENABLE_SCREEN_LINK
  if (sendToAll) {
    SCREEN_PORT.write(bytesArray, 5);
  }
#endif
#ifdef ENABLE_DCO_LINK
  if (paramValue != -1) {  // paramValue 100 = send to screen only
    DCO_PORT.write(bytesArray, 5);
  }
#endif
}

// Send 'w' 8-bit ParamId to the DCO, and to the Screen when sendToAll.
void serial_send_param_change_byte(byte param, byte paramValue, bool sendToAll) {
  byte bytesArrayByte[4] = { (uint8_t)'w', param, paramValue, finishByte };
#ifdef ENABLE_SCREEN_LINK
  if (sendToAll) {
    SCREEN_PORT.write(bytesArrayByte, 4);
  }
#endif
#ifdef ENABLE_DCO_LINK
  if (paramValue != -1) {  // paramValue 100 = send to screen only
    DCO_PORT.write(bytesArrayByte, 4);
  }
#endif
}

// Send preset name (8 chars) to the DCO via 'q'.
void serial_send_preset_name_to_mainboard() {
  DCO_PORT.write((char *)"q");
  // DCO-side 'q' (input link) expects 8 chars; send first 8 only.
  DCO_PORT.write(presetNameVal, 8);
  DCO_PORT.write(finishByte);
}

// Send preset scroll (number + 16-char name) to the Screen via 'q'.
void serial_send_preset_scroll(byte presetNumber, byte presetNameSerial[]) {

#ifdef ENABLE_SCREEN_LINK

  SCREEN_PORT.write((char *)"q");

  SCREEN_PORT.write(presetNumber);
  // Screen-side 'q' uses 16-character names.
  SCREEN_PORT.write(presetNameSerial, 16);
  SCREEN_PORT.write(finishByte);
#endif
}

// Send save-name character position to the Screen via 'c'.
void serial_send_save_char_select(byte serialPresetChar) {
#ifdef ENABLE_SCREEN_LINK

  SCREEN_PORT.write((char *)"c");

  SCREEN_PORT.write(serialPresetChar);
#endif
}

// Send 'y' byte param to the Screen.
void serialSendParamByteToScreen(byte paramNumber, byte paramValue)
{
 while(SCREEN_PORT.availableForWrite() < 1) {};
  byte bytesArray[4] = {(uint8_t)'y', paramNumber, paramValue, finishByte};
  SCREEN_PORT.write(bytesArray, 4);
}

// ---------------------------------------------------------------------------
// Parser-based receiver for DCO 'x' frames arriving on DCO_PORT RX
// (GP1 <- DCO GP20).
// ---------------------------------------------------------------------------

// Forward a decoded PARAM_32 payload as a full 'x' frame to the Screen (GP4 -> Screen GP13).
static void serial_forward_param32_to_screen(const uint8_t* payload, uint8_t len) {
#ifdef ENABLE_SCREEN_LINK
  if (len != SERIAL_PAYLOAD_LEN_PARAM_32) {
    return;
  }
  byte bytesArray[7] = {
    (uint8_t)'x',
    payload[0], payload[1], payload[2], payload[3], payload[4],
    finishByte
  };
  while (SCREEN_PORT.availableForWrite() < 1) {}
  SCREEN_PORT.write(bytesArray, 7);
#else
  (void)payload;
  (void)len;
#endif
}

// Handle 32-bit PARAM ('x') from the DCO:
//   154 PARAM_GAP_FROM_DCO → forward the same 'x' on to the Screen
//   155 PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO → store + optional 'y' echo
//   value (uint32) lower 16 bits for 155 = [oscIndex:8 | offset:8]
static void input_handle_param32_from_dco(char, const uint8_t* payload, uint8_t len) {
  if (len != SERIAL_PAYLOAD_LEN_PARAM_32) {
    return;
  }

  ParamFrame frame;
  decode_param_x(payload, frame);

  if (frame.id == (uint8_t)PARAM_GAP_FROM_DCO) {
    serial_forward_param32_to_screen(payload, len);
    return;
  }

  if (frame.id != (uint8_t)PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO) {
    return;
  }

  uint16_t packed  = (uint16_t)frame.value;
  uint8_t  oscIndex = (uint8_t)(packed >> 8);
  int8_t   offset   = (int8_t)(packed & 0xFF);

  if (oscIndex < NUM_OSCILLATORS) {
    manualCalibrationInitAmpCompOffset[oscIndex] = offset;
    // If we are currently in manual calibration and this oscillator
    // matches the selected stage, push the freshly loaded offset to
    // the screen so the initial value reflects the DCO's stored one.
    if (manualCalibration) {
      uint8_t currentIndex = (uint8_t)manualCalibrationStage / 2;
      if (oscIndex == currentIndex) {
        serialSendParamByteToScreen(
          ParamId::PARAM_MANUAL_CALIBRATION_OFFSET,
          (uint8_t)manualCalibrationInitAmpCompOffset[currentIndex]
        );
      }
    }
  }
}

// Command table and parser context for the DCO link RX (DCO → Input).
static const SerialCommandDef dcoLinkCommands[] = {
  { SERIAL_CMD_PARAM_32, SERIAL_PAYLOAD_LEN_PARAM_32, input_handle_param32_from_dco },
};

static SerialParserContext dcoLinkParser = {
  SERIAL_WAIT_FOR_CMD,
  0,
  {0},
  0,
  0,
  0
};

// Core1: non-blocking parser pump for inbound DCO 'x' frames.
// The DCO link is in polling mode, so bytes only leave the 32-byte hardware FIFO
// when this runs — roughly every 128 us of headroom at 2.5 Mbaud.
void serial_read_from_dco() {
#ifdef ENABLE_DCO_LINK
  if (dcoLinkParser.state == SERIAL_READ_PAYLOAD) {
    uint32_t now = micros();
    serial_parser_check_timeout(dcoLinkParser, now);
  }

  if (DCO_PORT.available() > 0) {
    uint32_t now = micros();
    while (DCO_PORT.available() > 0) {
      uint8_t b = DCO_PORT.read();
      serial_parser_process_byte(
        dcoLinkParser,
        dcoLinkCommands,
        sizeof(dcoLinkCommands) / sizeof(dcoLinkCommands[0]),
        b,
        now
      );
    }
  }
#endif
}
