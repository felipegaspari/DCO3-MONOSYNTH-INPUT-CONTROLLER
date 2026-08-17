#ifndef __FLOW_H__
#define __FLOW_H__

#include "buttons.h" // Needed for ButtonState

// The base template for any UI Wizard
class Flow {
public:
  virtual void onEnter() = 0;
  virtual void onEncoder(uint8_t encIndex, uint8_t direction, uint16_t speed) = 0;
  virtual void onButton(uint8_t btnIndex, ButtonState state) = 0;
  virtual void onExit() = 0;
  virtual ~Flow() {}
};

// Global pointers to manage the active flow
extern Flow* activeFlow;
void enterFlow(Flow* flow);
void exitActiveFlow();

#endif