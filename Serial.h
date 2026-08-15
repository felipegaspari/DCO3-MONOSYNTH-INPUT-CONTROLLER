#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "sram_hot.h"
#include "board_model.h"

// USB CDC debug print. Off by default; the panel loop is timing sensitive.
// #define ENABLE_SERIAL

// Which UART faces which peer is a per-PCB fact, so it comes from board_model.h
// rather than being written out here. DCO_PORT is the panel's outbound link: the
// DCO itself on DCO3, the Mainboard (which relays to the DCO) on DCO4.
// DCO_RX_PORT is inbound: same UART as DCO_PORT on DCO3; Serial1 GP1 on DCO4.
#define ENABLE_DCO_LINK
#define ENABLE_SCREEN_LINK
#define DCO_PORT    INPUT_DCO_PORT_OBJ
#define DCO_RX_PORT INPUT_DCO_RX_PORT_OBJ
#define SCREEN_PORT INPUT_SCREEN_PORT_OBJ
// TX on DCO_PORT / SCREEN_PORT is uart DMA (DcoDma / ScreenDma). RX stays
// Arduino IRQ on DCO_RX_PORT. Do not DCO_PORT.write() / SCREEN_PORT.write().

// #define SERIAL_FRAMING_COBS  // must match DCO/Mainboard; host: dco_control --cobs

// Screen 'q' scroll is preset# + 16 chars (17), the largest frame either link
// carries; the inbound 'O' directory entry is also 17.
#define SERIAL_INNER_MAX_PAYLOAD 17

#include "serial_input_protocol.h"
#include "serial_frame.h"
#include "serial_parser.h"
#include "serial_param_protocol.h"
#include "serial_dma.h"

// Forward declare types that normally come from Arduino.h so the linter
// can understand this header in isolation.
typedef unsigned char byte;

// DCO_RX_PORT: 'x' 154/155, persistable 'p' mirror, 'd' filter block, and the
// 'O'/'L' preset directory frames from the DCO (DCO4: Serial1 GP1).
void serial_read_from_dco();
void init_dco_link_parser();

float freq;

// Screen UI mode signals, sent as 's' + byte (serial_send_signal). The values
// are the Screen firmware's contract; do not renumber.
enum ScreenSignal : uint8_t {
  SIGNAL_PRESET_LOAD_SCROLL = 1,  // preset loaded -> show preset scroll
  SIGNAL_SAVE_EXIT          = 2,  // leave the load/save UI
  SIGNAL_SAVE_SELECT_ENTER  = 3,  // enter save slot-select mode
  SIGNAL_SAVE_NAME_EDIT     = 4,  // save: edit the preset name
  SIGNAL_PRESET_SAVED       = 5,  // save complete (show message)
  SIGNAL_SAVE_CHAR_SELECT   = 6,  // save: name char selection
};

void serial_send_signal(byte signal);
void serial_send_param_change_byte(byte param, byte paramValue, bool sendToAll = true);
void serial_send_param_change(byte param, uint16_t paramValue, bool sendToAll = true);
void serialSendParamByteToScreen(byte paramNumber, byte paramValue);
void input_send_manual_cal_stage();
void serial_send_manual_controls(bool presetLoading);
// Stage the 16-char preset name on the DCO via 'q', ahead of PARAM_PRESET_SAVE
// (presetStorage.ino: preset_save_to_board).
void serial_send_preset_name_to_mainboard();

#endif
