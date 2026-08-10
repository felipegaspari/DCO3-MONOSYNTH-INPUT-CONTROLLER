#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "sram_hot.h"

#define ENABLE_SERIAL

// Wiring (do not infer the peer from the port number):
//   Serial1  TX GP0 -> DCO GP21    | RX GP1 <- DCO GP20
//   Serial2  TX GP4 -> Screen GP13 | RX GP5 unwired (Screen never transmits)
#define ENABLE_DCO_LINK
#define ENABLE_SCREEN_LINK
#define DCO_PORT    Serial1
#define SCREEN_PORT Serial2

// #define SERIAL_FRAMING_COBS  // must match DCO; host: dco_control --cobs

// Screen 'q' scroll is preset# + 16 chars (17). DCO frames stay ≤8.
#define SERIAL_INNER_MAX_PAYLOAD 17

#include "serial_input_protocol.h"
#include "serial_frame.h"
#include "serial_parser.h"
#include "serial_param_protocol.h"

// Forward declare types that normally come from Arduino.h so the linter
// can understand this header in isolation.
typedef unsigned char byte;

void serial_read_from_dco();  // DCO_PORT RX: 'x' 154/155 + persistable 'p' mirror
void init_dco_link_parser();

float freq;

bool sendDetune2Flag = false;
bool serial_send_portamentoFlag = false;
bool serial_send_oscSyncModeFlag = false;
bool serial_send_OSC1IntervalFlag = false;
bool serial_send_OSC2IntervalFlag = false;
bool serial_send_LFO1SpeedFlag = false;
bool serial_send_LFO1toDCOFlag = false;
bool serial_send_LFO1toDCOWaveChangeFlag = false;
bool serialSendADSR3ControlValuesFlag = false;
bool serialSendADSR3toDCOFlag = false;
bool serialSendADSR3ToOscSelectFlag = false;

void serial_send_signal(byte signal);
void serial_send_param_change_byte(byte param, byte paramValue, bool sendToAll = true);
void serial_send_param_change(byte param, uint16_t paramValue, bool sendToAll = true);
void serialSendParamByteToScreen(byte paramNumber, byte paramValue);
void serial_send_manual_controls(bool presetLoading);

#endif

/*
SIGNAL LIST:

1 LOAD (PRESET SCROLL)
2 LOAD/SAVE EXIT
3 SAVE
4 SAVE - SET NAME
5 SAVE - COMPLETE
6 SAVE - SET NAME - CHAR SELECTION
7
8

*/
