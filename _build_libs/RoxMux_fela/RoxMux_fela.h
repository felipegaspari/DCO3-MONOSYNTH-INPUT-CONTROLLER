#pragma once
#include <Arduino.h>
class RoxButton {
public:
  void update(uint8_t, uint16_t, bool) {}
  bool held() { return false; }
  bool pressed() { return false; }
  bool released() { return false; }
  bool doublePressed() { return false; }
};
