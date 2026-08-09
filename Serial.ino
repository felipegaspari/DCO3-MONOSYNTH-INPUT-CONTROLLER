// Screen UI mode signal ('s' + byte).
void serial_send_signal(byte signal) {
#ifdef ENABLE_SCREEN_LINK
  serial_frame_write(SCREEN_PORT, (uint8_t)'s', &signal, 1);
#endif
}

// Send slim 'p' (id + i16 LE) to the DCO, and to the Screen when sendToAll.
void serial_send_param_change(byte param, uint16_t paramValue, bool sendToAll) {
  uint8_t payload[INPUT_SERIAL_LEN_PARAM_16];
  encode_param_p(payload, param, (int16_t)paramValue);
#ifdef ENABLE_SCREEN_LINK
  if (sendToAll) {
    serial_frame_write(SCREEN_PORT, INPUT_CMD_PARAM_16, payload, INPUT_SERIAL_LEN_PARAM_16);
  }
#endif
#ifdef ENABLE_DCO_LINK
  if (paramValue != (uint16_t)-1) {
    serial_frame_write(DCO_PORT, INPUT_CMD_PARAM_16, payload, INPUT_SERIAL_LEN_PARAM_16);
  }
#endif
}

// Screen keeps slim 'w' [id][u8]; DCO gets 'p' (i16 zero-extended). 255 = screen-only.
void serial_send_param_change_byte(byte param, byte paramValue, bool sendToAll) {
#ifdef ENABLE_SCREEN_LINK
  if (sendToAll) {
    uint8_t w[SERIAL_LEN_PARAM_8];
    encode_param_w(w, param, paramValue);
    serial_frame_write(SCREEN_PORT, (uint8_t)'w', w, SERIAL_LEN_PARAM_8);
  }
#endif
#ifdef ENABLE_DCO_LINK
  if (paramValue != (byte)-1) {
    uint8_t p[INPUT_SERIAL_LEN_PARAM_16];
    encode_param_p(p, param, (int16_t)paramValue);
    serial_frame_write(DCO_PORT, INPUT_CMD_PARAM_16, p, INPUT_SERIAL_LEN_PARAM_16);
  }
#endif
}

// Send preset name (8 chars) to the DCO via 'q'.
void serial_send_preset_name_to_mainboard() {
#ifdef ENABLE_DCO_LINK
  serial_frame_write(DCO_PORT, INPUT_CMD_PRESET_NAME, presetNameVal, INPUT_SERIAL_LEN_PRESET_NAME);
#endif
}

// Send preset scroll (number + 16-char name) to the Screen via 'q'.
void serial_send_preset_scroll(byte presetNumber, byte presetNameSerial[]) {
#ifdef ENABLE_SCREEN_LINK
  uint8_t payload[17];
  payload[0] = presetNumber;
  for (uint8_t i = 0; i < 16; ++i) {
    payload[1 + i] = presetNameSerial[i];
  }
  serial_frame_write(SCREEN_PORT, (uint8_t)'q', payload, 17);
#endif
}

// Send save-name character position to the Screen via 'c'.
void serial_send_save_char_select(byte serialPresetChar) {
#ifdef ENABLE_SCREEN_LINK
  serial_frame_write(SCREEN_PORT, (uint8_t)'c', &serialPresetChar, 1);
#endif
}

// Send 'y' byte param to the Screen.
void serialSendParamByteToScreen(byte paramNumber, byte paramValue)
{
#ifdef ENABLE_SCREEN_LINK
  uint8_t payload[2] = { paramNumber, paramValue };
  serial_frame_write(SCREEN_PORT, (uint8_t)'y', payload, 2);
#endif
}

// ---------------------------------------------------------------------------
// Parser-based receiver for DCO 'x' frames arriving on DCO_PORT RX
// (GP1 <- DCO GP20).
// ---------------------------------------------------------------------------

static void serial_forward_param32_to_screen(const uint8_t* payload, uint8_t len) {
#ifdef ENABLE_SCREEN_LINK
  if (len != INPUT_SERIAL_LEN_PARAM_32) {
    return;
  }
  serial_frame_write(SCREEN_PORT, INPUT_CMD_PARAM_32, payload, len);
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
  if (len != INPUT_SERIAL_LEN_PARAM_32) {
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

static const SerialCommandDef dcoLinkCommands[] = {
  { INPUT_CMD_PARAM_32, INPUT_SERIAL_LEN_PARAM_32, input_handle_param32_from_dco },
};

static SerialCommandTable dcoLinkLut;
static SerialParserContext dcoLinkParser = {};

void init_dco_link_parser() {
  serial_command_table_init(
    dcoLinkLut,
    dcoLinkCommands,
    sizeof(dcoLinkCommands) / sizeof(dcoLinkCommands[0])
  );
}

void serial_read_from_dco() {
#ifdef ENABLE_DCO_LINK
  serial_parser_drain(dcoLinkParser, dcoLinkLut, DCO_PORT, SERIAL_DRAIN_BYTE_BUDGET);
#endif
}
