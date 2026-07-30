#ifndef __SERIAL_H__
#define __SERIAL_H__

#define ENABLE_SERIAL

// Wiring (do not infer the peer from the port number):
//   Serial1  TX GP0 -> DCO GP21    | RX GP1 <- DCO GP20
//   Serial2  TX GP4 -> Screen GP13 | RX GP5 unwired (Screen never transmits)
#define ENABLE_DCO_LINK
#define ENABLE_SCREEN_LINK
#define DCO_PORT    Serial1
#define SCREEN_PORT Serial2

#include "serial_param_protocol.h"
#include "serial_protocol.h"
#include "serial_parser.h"

// Forward declare types that normally come from Arduino.h so the linter
// can understand this header in isolation.
typedef unsigned char byte;

void serial_read_from_dco();  // DCO_PORT RX (GP1 <- DCO GP20): DCO 'x' (154 forward, 155 store)

float freq;

byte finishByte = 1;

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

void serial_send_param_change_byte(byte param, byte paramValue, bool sendToAll = true);
void serial_send_param_change(byte param, uint16_t paramValue, bool sendToAll = true);

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