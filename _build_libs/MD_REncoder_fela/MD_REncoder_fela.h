#pragma once
#include <Arduino.h>
#ifndef DIR_NONE
#define DIR_NONE 0
#define DIR_CW 1
#define DIR_CCW 2
#endif
class MD_REncoder {
public:
  MD_REncoder(uint8_t = 0, uint8_t = 0) {}
  void begin() {}
  uint8_t read(uint8_t = 0, uint8_t = 0) { return DIR_NONE; }
  uint16_t speed() { return 0; }
};
