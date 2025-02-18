
// SPDX-FileCopyrightText: 2022 Limor Fried for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <Arduino.h>
#include "Adafruit_MAX1704X.h"
#include <Adafruit_ST7789.h> 
#include <Fonts/FreeSans12pt7b.h>
#include "Adafruit_LEDBackpack.h"
#include <Preferences.h>
#include <driver/adc.h>
#include <avdweb_Switch.h>

#define RW_MODE false
#define RO_MODE true

unsigned long lastInputTime = 0;
const static unsigned long SCREEN_TIMEOUT_MS = 30000;
uint8_t oldProj = 0, ps = 100;
boolean DisplayState = true;

Preferences arcPrefs;

enum
{
  PSBatt,
  PSUSB,
  PSUndef = 100,
} PowerSource;

Adafruit_7segment rowAled = Adafruit_7segment();
Adafruit_7segment rowBled = Adafruit_7segment();
Adafruit_7segment pattled = Adafruit_7segment();

GFXcanvas16 canvas(240, 135);
uint8_t rowA=0, rowB=0, patt=0;
const uint8_t rowApin = 0, rowBpin = 2, pattpin = 1, proj1Pin = 11, proj3Pin = 10, battCheckPin = A2;
uint8_t proj = 0;

Switch rowAButton(rowApin, INPUT_PULLUP, LOW, 50, 1000, 500);
Switch rowBButton(rowBpin, INPUT_PULLDOWN, HIGH, 50, 1000, 500);
Switch pattButton(pattpin, INPUT_PULLDOWN, HIGH, 50, 1000, 500);

Adafruit_MAX17048 lipo;
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void GetCountersForProj(uint8_t pnum)
{
  uint32_t CurProj = 0;
  switch (pnum)
  {
    case 1:
    {
      CurProj = arcPrefs.getUInt("Proj1");
      break;
    }
    case 2:
    {
      CurProj = arcPrefs.getUInt("Proj2");
      break;
    }
    case 3:
    {
      CurProj = arcPrefs.getUInt("Proj3");
      break;
    }
    default:
    return;
  }
  rowA = CurProj & 255;
  rowB = (CurProj >> 8) & 255;
  patt = (CurProj >> 16) & 255;
}

void SaveCountersForProj(uint8_t pnum)
{
  uint32_t CurProj = 0;
  CurProj |= patt;
  CurProj <<= 8;
  CurProj |= rowB;
  CurProj <<= 8;
  CurProj |= rowA;
  switch (pnum)
  {
    case 1:
    {
      CurProj = arcPrefs.putUInt("Proj1",CurProj);
      break;
    }
    case 2:
    {
      CurProj = arcPrefs.putUInt("Proj2",CurProj);
      break;
    }
    case 3:
    {
      CurProj = arcPrefs.putUInt("Proj3",CurProj);
      break;
    }
    default:
    return;
  }
}

void MyIncrement(void *pArg)
{
  lastInputTime = millis();
  if ( !DisplayState )
  {
    UpdateDisplays();
  }
  else
  {
    uint8_t* pNum = (uint8_t*)pArg;
    *pNum += 1;
  }
}

void MyDecrement(void *pArg)
{
  lastInputTime = millis();
  if ( !DisplayState )
  {
    UpdateDisplays();
  }
  else
  {
    uint8_t* pNum = (uint8_t*)pArg;
    *pNum -= 1;
  }
}

void MyZero(void *pArg)
{
  lastInputTime = millis();
  if ( !DisplayState )
  {
    UpdateDisplays();
  }
  else
  {
    uint8_t* pNum = (uint8_t*)pArg;
    *pNum = 0;
  }
}

float voltage = 0.0;
float oldVoltage = 0.0;

void UpdateDisplays()
{
  TurnDisplaysOn();
  canvas.fillScreen(ST77XX_BLACK);
  canvas.setCursor(0, 17);
  canvas.setTextColor(ST77XX_RED);
  canvas.print("Row Counter A: ");
  canvas.println(rowA, DEC);
  rowAled.print(rowA, DEC);
  rowAled.writeDisplay();
  canvas.setTextColor(ST77XX_GREEN);
  canvas.print("Project Number: ");
  canvas.println(proj, DEC);
  canvas.setTextColor(ST77XX_WHITE); 
  canvas.print("Pattern Count: ");
  canvas.println(patt, DEC);
  pattled.print(patt, DEC);
  pattled.writeDisplay();
  canvas.setTextColor(ST77XX_BLUE); 
  canvas.print(ps == PSUSB ? "USB  " : "Batt " );
  canvas.print(lipo.cellPercent(), 0);
  canvas.println("%");

  canvas.setTextColor(ST77XX_YELLOW);
  canvas.print("Row Counter B: ");
  canvas.println(rowB, DEC);
  rowBled.print(rowB, DEC);
  rowBled.writeDisplay();
  canvas.println("");
  display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
}

void setup()
{
#ifdef DEBUGGING
  Serial.begin(115200);
  while (!Serial) delay(10);
  delay(100);
  Serial.println("Connected... Starting Prefs");
#endif
  // preferences
  arcPrefs.begin("ARCPrefsII", RO_MODE);
  bool tpInit = arcPrefs.isKey("nvsInit");
  if ( !tpInit )
  {
    arcPrefs.end();
    arcPrefs.begin("ARCPrefsII", RW_MODE);
    arcPrefs.putUInt("Proj1",0);
    arcPrefs.putUInt("Proj2",0);
    arcPrefs.putUInt("Proj3",0);
    arcPrefs.putBool("nvsInit", true);
  }
  else
  {
    arcPrefs.end();
    arcPrefs.begin("ARCPrefsII", RW_MODE);
  }
#ifdef DEBUGGING
  Serial.println("Finished with Prefs");
#endif
  lipo.begin();

  rowAled.begin(0x70);
  rowBled.begin(0x72);
  pattled.begin(0x71);
#ifdef DEBUGGING
  Serial.println("Finished with LEDs");
#endif
  display.init(135, 240);           // Init ST7789 240x135
  display.setRotation(3);
  canvas.setFont(&FreeSans12pt7b);
  canvas.setTextColor(ST77XX_WHITE); 
#ifdef DEBUGGING
  Serial.println("Finished Display");
#endif
  pinMode(battCheckPin, INPUT);
  pinMode(proj1Pin, INPUT_PULLUP);
  pinMode(proj3Pin, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // button callbacks
  rowAButton.setSingleClickCallback(MyIncrement,&rowA);
  rowAButton.setDoubleClickCallback(MyDecrement,&rowA);
  rowAButton.setLongPressCallback(MyZero,&rowA);

  rowBButton.setSingleClickCallback(MyIncrement,&rowB);
  rowBButton.setDoubleClickCallback(MyDecrement,&rowB);
  rowBButton.setLongPressCallback(MyZero,&rowB);

  pattButton.setSingleClickCallback(MyIncrement,&patt);
  pattButton.setDoubleClickCallback(MyDecrement,&patt);
  pattButton.setLongPressCallback(MyZero,&patt);
  
  proj = GetProjectFromSelector();
  GetCountersForProj(proj);
  voltage = GetVoltage();
  UpdateDisplays();
#ifdef DEBUGGING
  Serial.println("Exiting Setup");
#endif
}

float GetVoltage()
{
  return (float)analogReadMilliVolts(battCheckPin)/1000.0;
}

void loop()
{
  uint8_t oldRowA, oldRowB, oldPatt, oldProj, oldPS;
  oldPS = ps;
  ps = GetPowerSource();
  if ( oldPS != ps )
  {
    UpdateDisplays();
  }
  else if (PSBatt == ps && millis() - lastInputTime > SCREEN_TIMEOUT_MS )
  {
    TurnDisplaysOff();
  }
  oldVoltage = voltage;
  voltage = GetVoltage();
  oldProj = proj;
  proj = GetProjectFromSelector();
  if ( oldProj != proj )
  {
    lastInputTime = millis();
    GetCountersForProj(proj);
    UpdateDisplays();
  }
  oldRowA = rowA;
  oldRowB = rowB;
  oldPatt = patt;
  rowAButton.poll();
  rowBButton.poll();
  pattButton.poll();
  if ( rowA != oldRowA || rowB != oldRowB || patt != oldPatt )
  {
    SaveCountersForProj(proj);
    lastInputTime = millis();
    UpdateDisplays();
  }
}

uint8_t GetPowerSource()
{
  if ( GetVoltage() > 2.1 )
  {
    return PSUSB;
  }
  else
  {
    return PSBatt;
  }
}

uint8_t GetProjectFromSelector()
{
  if ( LOW == digitalRead(proj1Pin) )
  {
    return 1;
  }
  else if ( LOW == digitalRead(proj3Pin) )
  {
    return 3;
  }
  else
  {
    return 2;
  }
}
void TurnDisplaysOff()
{
  rowAled.setDisplayState(false);
  rowBled.setDisplayState(false);
  pattled.setDisplayState(false);
  display.enableDisplay(false);
  digitalWrite(TFT_BACKLITE, LOW);
  DisplayState = false;
}

void TurnDisplaysOn()
{
  rowAled.setDisplayState(true);
  rowBled.setDisplayState(true);
  pattled.setDisplayState(true);
  display.enableDisplay(true);
  digitalWrite(TFT_BACKLITE, HIGH);
  DisplayState = true;
}
