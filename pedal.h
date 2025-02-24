#include <avdweb_Switch.h>

typedef void (*cbfunc)(uint8_t vPin, uint8_t event);

enum pinState
{
  FPIdle,
  FPSingleClick,
  FPDoubleClick,
  FPHold,
};

class FootPedal
{
  // represents a guitar-style foot pedal with 3 buttons
  // where left shorts ring to sleeve
  // middle shorts tip to sleeve
  // right shorts all three
  //
  // uses the avdweb_Switch class
  // pins are setup with INPUT_PULLUP
  public:
    FootPedal (uint8_t ringPin, uint8_t tipPin, uint8_t VirtualPinForBoth, cbfunc f);
    void poll();

  private:
    uint8_t ringPin_;
    uint8_t tipPin_;
    uint8_t vPin_;
    Switch *ringSwitch;
    Switch *tipSwitch;
    uint32_t lastRingEvent;
    uint32_t lastTipEvent;
    uint8_t ringPinState;
    uint8_t tipPinState;
    cbfunc MyCallBack;
};