#include "pedal.h"
//#define DEBUGGING

FootPedal::FootPedal(uint8_t ringPin, uint8_t tipPin, uint8_t VirtualPinForBoth, cbfunc f)
{
  ringSwitch   = new Switch(ringPin, INPUT_PULLUP, LOW, 50, 1000, 500);
  tipSwitch    = new Switch(tipPin, INPUT_PULLUP, LOW, 50, 1000, 500);
  vPin_        = VirtualPinForBoth;
  ringPin_     = ringPin;
  tipPin_      = tipPin;
  MyCallBack = f;
}

void FootPedal::poll()
{
  ringSwitch->poll();
  if ( ringSwitch->singleClick() )
  {
#ifdef DEBUGGING
    Serial.println("ring click");
#endif
    lastRingEvent = millis();
    ringPinState = FPSingleClick;
  }
  else if ( ringSwitch->doubleClick() )
  {
    lastRingEvent = millis();
    ringPinState = FPDoubleClick;
}
  else if ( ringSwitch->longPress() )
  {
    lastRingEvent = millis();
    ringPinState = FPHold;
  }

  tipSwitch->poll();
  if ( tipSwitch->singleClick() )
  {
#ifdef DEBUGGING
    Serial.println("tip click");
#endif
    lastTipEvent = millis();
    tipPinState = FPSingleClick;
  }
  else if ( tipSwitch->doubleClick() )
  {
    lastTipEvent = millis();
    tipPinState = FPDoubleClick;
  }
  else if ( tipSwitch->longPress() )
  {
    lastTipEvent = millis();
    tipPinState = FPHold;
  }

  // ensure at least THRESHHOLD ms of quiet before determining what happened
  const uint8_t THRESHHOLD = 2;
  uint32_t now = millis();
  if ( lastTipEvent !=0 && lastRingEvent !=0 )
  {
    uint32_t deltaT = lastTipEvent > lastRingEvent ? lastTipEvent - lastRingEvent : lastRingEvent - lastTipEvent;
    if ( (now - lastTipEvent > THRESHHOLD) && (now - lastRingEvent > THRESHHOLD) &&  deltaT <= THRESHHOLD )
    {
      // we have 2 valid events! If they are the same, process them as virtual pin event, and clear
      // otherwise process them separately and clear
      if ( lastTipEvent == lastRingEvent )
      {
        MyCallBack(vPin_,tipPinState);
#ifdef DEBUGGING
        Serial.print("tip+ring callback ");
        Serial.println(tipPinState,DEC);
#endif
      }
      else
      {
        MyCallBack(tipPin_, tipPinState);
        MyCallBack(ringPin_, ringPinState);
      }
      lastTipEvent = 0;
      lastRingEvent = 0;
      tipPinState = FPIdle;
      ringPinState = FPIdle;
    }
  }
  else if ( lastTipEvent !=0 )
  {
    if ( now - lastTipEvent > THRESHHOLD )
    {
        MyCallBack(tipPin_, tipPinState);
#ifdef DEBUGGING
        Serial.print("tip callback ");
        Serial.println(tipPinState,DEC);
#endif
        tipPinState = FPIdle;
        lastTipEvent = 0;
    }
  }
  else if ( lastRingEvent !=0 )
  {
    if ( now - lastRingEvent > THRESHHOLD )
    {
        MyCallBack(ringPin_, ringPinState);
#ifdef DEBUGGING
        Serial.print("ring callback ");
        Serial.println(ringPinState,DEC);
#endif
        ringPinState = FPIdle;
        lastRingEvent = 0;
    }
  }
}