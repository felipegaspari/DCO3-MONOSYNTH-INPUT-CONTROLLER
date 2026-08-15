#include "include_all.h"
#include "hardware/dma.h"
#include "hardware/uart.h"
#include "pico/mutex.h"
#include <string.h>

// Ping-pong so a 1 ms ADSR/filter burst can queue while the previous transfer
// drains. 256 B holds several stuffed frames (max inner payload is 17).
static constexpr uint16_t SERIAL_DMA_BUF_SIZE = 256;
static constexpr uint8_t SERIAL_DMA_ENGINES = 2;

struct SerialDmaEngine {
  uart_inst_t *uart;
  mutex_t mutex;
  int chan;
  dma_channel_config cfg;
  uint8_t buf[2][SERIAL_DMA_BUF_SIZE];
  uint16_t len[2];
  uint8_t fill;
  bool sending;
};

static SerialDmaEngine serial_dma_eng[SERIAL_DMA_ENGINES];

UartDmaTx DcoDma = { 0 };
UartDmaTx ScreenDma = { 1 };

static void serial_dma_poll_one(uint8_t i) {
  SerialDmaEngine &e = serial_dma_eng[i];
  if (e.chan < 0 || e.uart == nullptr) {
    return;
  }
  if (e.sending) {
    if (dma_channel_is_busy((uint)e.chan)) {
      return;
    }
    e.sending = false;
  }
  const uint16_t count = e.len[e.fill];
  if (count == 0) {
    return;
  }
  const uint8_t send = e.fill;
  e.len[send] = 0;
  e.fill ^= 1u;
  e.sending = true;
  dma_channel_set_write_addr((uint)e.chan, &uart_get_hw(e.uart)->dr, false);
  dma_channel_set_read_addr((uint)e.chan, e.buf[send], false);
  dma_channel_set_trans_count((uint)e.chan, count, true);
}

static void serial_dma_init_one(uint8_t i, uart_inst_t *uart) {
  SerialDmaEngine &e = serial_dma_eng[i];
  mutex_init(&e.mutex);
  e.uart = uart;
  e.chan = dma_claim_unused_channel(true);
  e.cfg = dma_channel_get_default_config((uint)e.chan);
  channel_config_set_transfer_data_size(&e.cfg, DMA_SIZE_8);
  channel_config_set_read_increment(&e.cfg, true);
  channel_config_set_write_increment(&e.cfg, false);
  channel_config_set_dreq(&e.cfg, uart_get_dreq(uart, true));
  dma_channel_configure((uint)e.chan, &e.cfg, &uart_get_hw(uart)->dr, nullptr, 0, false);
  e.len[0] = 0;
  e.len[1] = 0;
  e.fill = 0;
  e.sending = false;
}

static size_t serial_dma_write_one(uint8_t i, const uint8_t *p, size_t n) {
  SerialDmaEngine &e = serial_dma_eng[i];
  if (e.chan < 0 || e.uart == nullptr || p == nullptr || n == 0) {
    return 0;
  }
  if (n > SERIAL_DMA_BUF_SIZE) {
    return 0;
  }
  mutex_enter_blocking(&e.mutex);
  serial_dma_poll_one(i);
  if ((size_t)e.len[e.fill] + n > SERIAL_DMA_BUF_SIZE) {
    if (!e.sending && e.len[e.fill] > 0) {
      serial_dma_poll_one(i);
    }
    if ((size_t)e.len[e.fill] + n > SERIAL_DMA_BUF_SIZE) {
      mutex_exit(&e.mutex);
      return 0;
    }
  }
  memcpy(e.buf[e.fill] + e.len[e.fill], p, n);
  e.len[e.fill] = (uint16_t)(e.len[e.fill] + n);
  serial_dma_poll_one(i);
  mutex_exit(&e.mutex);
  return n;
}

void serial_dma_init() {
#if INPUT_IS_DCO3
  serial_dma_init_one(0, uart0);
  serial_dma_init_one(1, uart1);
#else
  serial_dma_init_one(0, uart1);
  serial_dma_init_one(1, uart0);
#endif
}

void serial_dma_poll() {
  for (uint8_t i = 0; i < SERIAL_DMA_ENGINES; ++i) {
    mutex_enter_blocking(&serial_dma_eng[i].mutex);
    serial_dma_poll_one(i);
    mutex_exit(&serial_dma_eng[i].mutex);
  }
}

size_t UartDmaTx::write(const uint8_t *p, size_t n) {
  if (id >= SERIAL_DMA_ENGINES) {
    return 0;
  }
  return serial_dma_write_one(id, p, n);
}
