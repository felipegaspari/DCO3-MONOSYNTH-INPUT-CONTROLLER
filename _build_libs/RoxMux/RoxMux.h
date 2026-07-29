#pragma once
#include <Arduino.h>
template<uint8_t N>
class Rox74HC595 {
public:
  void begin(int, int, int, int) {}
  void setBrightness(int) {}
  void allOff() {}
  void writePin(uint8_t, bool) {}
  void update() {}
};
