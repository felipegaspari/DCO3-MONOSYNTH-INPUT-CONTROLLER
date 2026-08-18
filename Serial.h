#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "sram_hot.h"
#include "board_model.h"

#define ENABLE_DCO_LINK
#define ENABLE_SCREEN_LINK
#define DCO_PORT    INPUT_DCO_PORT_OBJ
#define DCO_RX_PORT INPUT_DCO_RX_PORT_OBJ
#define SCREEN_PORT INPUT_SCREEN_PORT_OBJ

#define DCO_PROTOCOL_IMPLEMENT_DMA

// Shared Protocol Includes
#include "_build_libs/DCO-PROTOCOL/serial_param_protocol.h"
#include "_build_libs/DCO-PROTOCOL/serial_input_protocol.h"
#include "_build_libs/DCO-PROTOCOL/serial_frame.h"
#include "_build_libs/DCO-PROTOCOL/serial_parser.h"
#include "_build_libs/DCO-PROTOCOL/serial_dma_tx.h"

typedef unsigned char byte;

extern UartDmaTx DcoDma;
extern UartDmaTx ScreenDma;

void init_dco_link_parser();
void serial_read_from_dco();
void serial_dma_init();
void serial_dma_poll();

enum ScreenSignal : uint8_t {
  SIGNAL_PRESET_LOAD_SCROLL = 1,
  SIGNAL_SAVE_EXIT          = 2,
  SIGNAL_SAVE_SELECT_ENTER  = 3,
  SIGNAL_SAVE_NAME_EDIT     = 4,
  SIGNAL_PRESET_SAVED       = 5,
  SIGNAL_SAVE_CHAR_SELECT   = 6,
};

void serial_send_signal(byte signal);
void serial_send_param_change_byte(byte param, byte paramValue, bool sendToAll = true);
void serial_send_param_change(byte param, uint16_t paramValue, bool sendToAll = true);
void serialSendParamByteToScreen(byte paramNumber, byte paramValue);
void input_send_manual_cal_stage();
void serial_send_manual_controls(bool presetLoading);
void serial_send_preset_name_to_mainboard();
void serial_send_preset_scroll(byte presetNumber, byte presetNameSerial[]);
void serial_send_save_char_select(byte serialPresetChar);
void input_disable_all_manual_controls();
void sendSerial();

#endif