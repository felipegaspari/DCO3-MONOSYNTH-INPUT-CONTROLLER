// Send uint16 frame 'u' on Serial2. Currently unused helper.
void sendUint16(uint16_t f) {
  byte *b = (byte *)&f;

  Serial2.write((char *)"u");

  Serial2.write(b, 2);
}

// Send float bytes after 't' on Serial2. Currently unused helper.
void sendFloat(float f) {
  byte *b = (byte *)&f;

  Serial2.print("t");
  byte ndata = 0;
  for (int i = 0; i < 4; i++) {

    Serial2.write(b[i]);
  }
  return;
}

// Send 'k' OK on Serial2. Currently unused helper.
void sendOK() {

  Serial2.write((char *)"k");
  //Serial.println("Sent OK");
}

// Legacy autotune kick on Serial2. Currently unused.
void serial_send_autotune() {
  byte autotune_on = 255;
  Serial2.write((char *)"a");
  Serial2.write(autotune_on);
  Serial2.flush();
  //Serial.println("Sent autotune on");
}

// Screen UI mode signal ('s' + byte) on Serial1.
void serial_send_signal(byte signal) {
#ifdef ENABLE_SERIAL1

  Serial1.write((char *)"s");

  Serial1.write(signal);
#endif
}


// Send 'p' 16-bit ParamId to Screen (optional) and Mainboard Serial2.
void serial_send_param_change(byte param, uint16_t paramValue, bool sendToAll) {
  byte bytesArray[5] = { (uint8_t)'p', param, highByte(paramValue), lowByte(paramValue), finishByte };
#ifdef ENABLE_SERIAL1
  if (sendToAll) {
    Serial1.write(bytesArray, 5);
  }
#endif
#ifdef ENABLE_SERIAL2
  if (paramValue != -1) {  // paramValue 100 = send to screen only
    Serial2.write(bytesArray, 5);
  }
#endif
}

// Send 'w' 8-bit ParamId to Screen (optional) and Mainboard Serial2.
void serial_send_param_change_byte(byte param, byte paramValue, bool sendToAll) {
  byte bytesArrayByte[4] = { (uint8_t)'w', param, paramValue, finishByte };
#ifdef ENABLE_SERIAL1
  if (sendToAll) {
    Serial1.write(bytesArrayByte, 4);
  }
#endif
#ifdef ENABLE_SERIAL2
  if (paramValue != -1) {  // paramValue 100 = send to screen only
    Serial2.write(bytesArrayByte, 4);
  }
#endif
}

// Send preset name (8 chars) to Mainboard via 'q' on Serial2.
void serial_send_preset_name_to_mainboard() {
  Serial2.write((char *)"q");
  // Mainboard-side 'q' (input link) expects 8 chars; send first 8 only.
  Serial2.write(presetNameVal, 8);
  Serial2.write(finishByte);
}

// Send preset scroll (number + 16-char name) to Screen via 'q' on Serial1.
void serial_send_preset_scroll(byte presetNumber, byte presetNameSerial[]) {

#ifdef ENABLE_SERIAL1

  Serial1.write((char *)"q");

  Serial1.write(presetNumber);
  // Screen-side 'q' uses 16-character names.
  Serial1.write(presetNameSerial, 16);
  Serial1.write(finishByte);
#endif
}

// Send save-name character position to Screen via 'c' on Serial1.
void serial_send_save_char_select(byte serialPresetChar) {
#ifdef ENABLE_SERIAL1

  Serial1.write((char *)"c");

  Serial1.write(serialPresetChar);
#endif
}

// Send 'y' byte param to Screen on Serial1.
void serialSendParamByteToScreen(byte paramNumber, byte paramValue)
{
 while(Serial1.availableForWrite() < 1) {};
  byte bytesArray[4] = {(uint8_t)'y', paramNumber, paramValue, finishByte};
  Serial1.write(bytesArray, 4);
}

// ---------------------------------------------------------------------------
// Parser-based receiver for DCO 'x' frames on Serial2 (hub path).
// ---------------------------------------------------------------------------

// Forward a decoded PARAM_32 payload as a full 'x' frame to Screen (Serial1).
static void serial_forward_param32_to_screen(const uint8_t* payload, uint8_t len) {
#ifdef ENABLE_SERIAL1
  if (len != SERIAL_PAYLOAD_LEN_PARAM_32) {
    return;
  }
  byte bytesArray[7] = {
    (uint8_t)'x',
    payload[0], payload[1], payload[2], payload[3], payload[4],
    finishByte
  };
  while (Serial1.availableForWrite() < 7) {}
  Serial1.write(bytesArray, 7);
#else
  (void)payload;
  (void)len;
#endif
}

// Handle 32-bit PARAM ('x') from DCO on Serial2:
//   154 PARAM_GAP_FROM_DCO → forward same 'x' to Screen (replaces DCO Screen UART)
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

// Command table and parser context for Serial2 (DCO → Input).
static const SerialCommandDef dcoSerial2Commands[] = {
  { SERIAL_CMD_PARAM_32, SERIAL_PAYLOAD_LEN_PARAM_32, input_handle_param32_from_dco },
};

static SerialParserContext dcoSerial2Parser = {
  SERIAL_WAIT_FOR_CMD,
  0,
  {0},
  0,
  0,
  0
};

// Core1: non-blocking Serial2 parser pump for inbound DCO 'x' frames.
void serial_read_from_dco() {
#ifdef ENABLE_SERIAL2
  if (dcoSerial2Parser.state == SERIAL_READ_PAYLOAD) {
    uint32_t now = micros();
    serial_parser_check_timeout(dcoSerial2Parser, now);
  }

  if (Serial2.available() > 0) {
    uint32_t now = micros();
    while (Serial2.available() > 0) {
      uint8_t b = Serial2.read();
      serial_parser_process_byte(
        dcoSerial2Parser,
        dcoSerial2Commands,
        sizeof(dcoSerial2Commands) / sizeof(dcoSerial2Commands[0]),
        b,
        now
      );
    }
  }
#endif
}

// Legacy name: previously parsed 'x' on Serial1 (wrong peer for hub). Kept as no-op
// alias site compatibility — prefer serial_read_from_dco().
void serial_read_from_mainboard() {
  serial_read_from_dco();
}
