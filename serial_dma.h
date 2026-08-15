#ifndef SERIAL_DMA_H
#define SERIAL_DMA_H

#include <stddef.h>
#include <stdint.h>

// UART TX DMA for DCO_PORT and SCREEN_PORT. Arduino-Pico keeps RX IRQ on those
// UARTs (DCO4: Serial1 RX from Mainboard shares SCREEN_PORT's uart0). Do not
// DCO_PORT.write() / SCREEN_PORT.write() after serial_dma_init(). USB Serial
// is unchanged.

void serial_dma_init();
void serial_dma_poll();

struct UartDmaTx {
  uint8_t id;
  size_t write(const uint8_t *p, size_t n);
};

extern UartDmaTx DcoDma;
extern UartDmaTx ScreenDma;

#endif
