#include "main.h"
#include <Wire.h>
#include <SI4735.h>

#include "TFT_eSPI.h"
#include <RotaryEncoder.h>
#include <TFT_eWidget.h>

//#define MY_BACKGROUND 0x52AA
#define MY_BACKGROUND TFT_BLACK

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite mainDataPanel = TFT_eSprite(&tft);
TFT_eSprite secondaryDataPanel = TFT_eSprite(&tft);

TFT_eSprite ScrollTextSprite = TFT_eSprite(&tft);

ButtonWidget buttonVolumeDown = ButtonWidget(&tft);
ButtonWidget buttonVolumeUp = ButtonWidget(&tft);

ButtonWidget* buttonList[] = {&buttonVolumeDown , &buttonVolumeUp};
uint8_t buttonCount = sizeof(buttonList) / sizeof(buttonList[0]);

#define BUTTON_W 100
#define BUTTON_H 50

int ScrollStepCounter = 0;
int ScrollStep = -3; // Must be negative for scrolling from right to left. Use a more neg. number for faster scrolling, but jumpier.
int16_t MsgPixWidth; // The width of your entire message to be scrolled, in pixels.
char MessageToScroll[65] = "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW";
int SpaceBetweenRepeats = 50; // in pixels.
int TopPadding = 10; // to keep the letters from touching the top edge of banner.

#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3


RotaryEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B, RotaryEncoder::LatchMode::FOUR3);

#define color1 0xC638
#define color2 0xC638

mainScreenData mainData;
FMScreenData fmData;


int minimal = 880;
int maximal = 1080;
int strength = 0;

//float freq = 0.00;
SI4735 radio;


void setup()
{  
  Serial.begin(115200);
  Serial.println("\nHello");

  setupRadio();
  setupScreen();
  initButtons();

  drawMainDataPanel();
}

void setupRadio()
{
  Wire.begin();
  radio.setI2CFastModeCustom(100000);
  radio.setAudioMuteMcuPin(AUDIO_MUTE);
  radio.getDeviceI2CAddress(RESET_PIN); // Looks for the I2C bus address and set it.  Returns 0 if error
  radio.setup(RESET_PIN, MW_BAND_TYPE);
  delay(200);
  radio.setTuneFrequencyAntennaCapacitor(0);

  radio.setVolume(63);
  mainData.volume = 63;

  setFMMode();
}

void setupScreen()
{
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  //touch_calibrate();
  uint16_t calData[5] = { 204, 3649, 112, 3823, 4 };
  tft.setTouch(calData);

  
  mainDataPanel.createSprite(320, 100);
  secondaryDataPanel.createSprite(320, 100);
  tft.setTextDatum(TL_DATUM);
  tft.setSwapBytes(true);
  tft.setFreeFont(&Orbitron_Light_24);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  tft.drawString("ABCI1 ", 1, 200, 1);
  tft.drawString("ABCI2 ", 1, 250, 2);
  tft.drawString("ABCI4 ", 1, 300, 4);
  tft.drawString("ABCI6 ", 1, 350, 6);
  tft.drawString("ABCI8 ", 1, 400, 8);

  /*MsgPixWidth = tft.textWidth(MessageToScroll);
  ScrollTextSprite.setColorDepth(8); // How big a sprite you can use and how fast it moves is greatly influenced by the color depth.
  ScrollTextSprite.createSprite(MsgPixWidth + tft.width(), (tft.fontHeight() + TopPadding)); // Sprite width is display plus the space to allow text to scroll from the right.
  ScrollTextSprite.setTextColor(TFT_YELLOW, TFT_BLACK); // Yellow text, black background
  
  // Draw it for the first time:
  ScrollTextSprite.drawString(MessageToScroll, tft.width(), TopPadding, 4); // 
  ScrollStepCounter = (MsgPixWidth / abs(ScrollStep)) + SpaceBetweenRepeats;*/

}

void initButtons() {
  uint16_t x = (tft.width() - BUTTON_W) / 2;
  uint16_t y = tft.height() / 2 - BUTTON_H + 30;

  buttonVolumeDown.initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_RED, TFT_BLACK, "Vol-", 1);
  buttonVolumeDown.setPressAction(buttonVolumeDownPressAction);
  buttonVolumeDown.setReleaseAction(buttonVolumeDownReleaseAction);
  //buttonVolumeDown.drawSmoothButton(false, 1, TFT_BLACK); // 3 is outline width, TFT_BLACK is the surrounding background colour for anti-aliasing

  y = tft.height() / 2 + 50;
  buttonVolumeUp.initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_GREEN, TFT_BLACK, "Vol+", 1);
  buttonVolumeUp.setPressAction(buttonVolumeUpPressAction);
  buttonVolumeUp.setReleaseAction(buttonVolumeUpReleaseAction);
  //buttonVolumeUp.drawSmoothButton(false, 1, TFT_BLACK); // 3 is outline width, TFT_BLACK is the surrounding background colour for anti-aliasing

  for (uint8_t b = 0; b < buttonCount; b++) {
    buttonList[b]->drawSmoothButton(false, 1, TFT_BLACK);
  }
}

void loop()
{
  handleEncoder();
  handleRadio();
  handleScreen();
  handleButtons();

  uint16_t x = 0, y = 0; // To store the touch coordinates

  bool pressed = tft.getTouch(&x, &y);
  if (pressed) {
    tft.fillCircle(x, y, 2, TFT_WHITE);
    Serial.print("x,y = ");
    Serial.print(x);
    Serial.print(",");
    Serial.println(y);
  }

  /*ScrollStepCounter--; 
  if (ScrollStepCounter <= 0)
  {
    ScrollTextSprite.fillSprite(TFT_BLACK);
    ScrollStepCounter = (MsgPixWidth / abs(ScrollStep)) + SpaceBetweenRepeats;
    MsgPixWidth = ScrollTextSprite.drawString(fmData.programInfo, tft.width(),TopPadding, 4);
  }
  ScrollTextSprite.scroll(ScrollStep);
  ScrollTextSprite.pushSprite(0, 300);*/

  if (Serial.available() > 0)
  {
    char key = Serial.read();
    switch (key)
    {
      case '+':
        radio.volumeUp();
        break;
      case '-':
        radio.volumeDown();
        break;
      case 'a':
      case 'A':
        radio.setAM(570, 1710,  810, 10);
        radio.setAvcAmMaxGain(32); // Sets the maximum gain for automatic volume control on AM mode
        break;
      case 'f':
      case 'F':
        radio.setFM(8600, 10800,  10760, 10);
        break;
      case 'U':
      case 'u':
        radio.frequencyUp();
        showStatusSerial();
        break;
      case 'D':
      case 'd':
        radio.frequencyDown();
        showStatusSerial();
        break;
      case 'S':
        radio.seekStationUp();
        showStatusSerial();
        break;
      case 's':
        radio.seekStationDown();
        showStatusSerial();
        break;
      case 'X':
        showStatusSerial();
        break;
      case '?':
        showHelp();
        break;
      default:
        break;
    }
  }
}

unsigned long nextRadioDataUpdate = 0;
void handleRadio()
{
  if (millis() > nextRadioDataUpdate) {
    nextRadioDataUpdate = millis() + 500;

    radio.getCurrentReceivedSignalQuality();
    mainData.freq = radio.getFrequency();
    mainData.isStereo = radio.getCurrentPilot();
    mainData.multipath.value = radio.getCurrentMultipath();

    mainData.rssi.value = radio.getCurrentRSSI();
    mainData.rssi.isLow = radio.getCurrentRssiDetectLow();
    mainData.rssi.isHigh = radio.getCurrentRssiDetectHigh();

    mainData.snr.value = radio.getCurrentSNR();
    mainData.snr.isLow = radio.getCurrentSnrDetectLow();
    mainData.snr.isHigh = radio.getCurrentSnrDetectHigh();

    mainData.multipath.value = radio.getCurrentMultipath();
    mainData.multipath.isLow = radio.getCurrentMultipathDetectLow();
    mainData.multipath.isHigh = radio.getCurrentMultipathDetectHigh();

    mainData.agc = radio.isAgcEnabled();

    strength = getStrength(mainData.rssi.value);
  }

  if(mainData.currentRadioMode == 0){
    
    /*char *rdsTime;
    char *stationName;
    char *programInfo;
    char *stationInfo;
    
    radio.getRdsAllData(&stationName, &stationInfo, &programInfo, &rdsTime);
    if (rdsTime != NULL) {
      //Serial.println(rdsTime);
      //strncpy(fmData.rdsTime, rdsTime, 32);
      strncpy(fmData.rdsTime, rdsTime, 25);
    }

    if (stationName != NULL) {
      //Serial.println(stationName);
      //strncpy(fmData.stationName, stationName, 255);
      strncpy(fmData.stationName, stationName, 9);
    }
    
    if (programInfo != NULL) {
      //Serial.println(programInfo);
      //strncpy(fmData.programInfo, programInfo, 255);
      strncpy(fmData.programInfo, programInfo, 65);
    }
    
    if (stationInfo != NULL) {
      //Serial.println(stationInfo);
      //strncpy(fmData.stationInfo, stationInfo, 255);
      strncpy(fmData.stationInfo, stationInfo, 33);
    }*/


    
    fmData.rdsAvailable = radio.getRdsAllData(&fmData.stationNamePtr, &fmData.stationInfoPtr, &fmData.programInfoPtr, &fmData.rdsTimePtr);

  }
}

void handleScreen()
{
  static unsigned long nextSecondaryDataPanelUpdate = 0;
  static uint8_t lastCrcMain = 0, lastCrcSecondary = 0;

  uint8_t crcMain = 0;
  uint8_t *p = (uint8_t *)&mainData;
  for(uint16_t i = 0; i < sizeof(mainData); i++) {
    crcMain ^= p[i];
  }
  
  if (crcMain != lastCrcMain) {
    lastCrcMain = crcMain;
    drawMainDataPanel();
  }

  if(mainData.currentRadioMode == 0) {
    
    uint8_t crcSecondary = 0;
    uint8_t *p = (uint8_t *)&fmData;
    for(int i = 0; i < sizeof(fmData); i++) {
      crcSecondary ^= p[i];
    }
    
    if (crcSecondary != lastCrcSecondary) {
      lastCrcSecondary = crcSecondary;
      drawSecondaryDataPanelFM();
    }
  }
      //drawSecondaryDataPanelFM();

  /*if (millis() > nextSecondaryDataPanelUpdate) {
    nextSecondaryDataPanelUpdate = millis() + 500;
    drawSecondaryDataPanelFM();
  }*/
}

void drawMainDataPanel()
{
  mainDataPanel.fillSprite(TFT_BLACK);
  mainDataPanel.drawRect(0, 0, 320, 100, TFT_WHITE);
  
  //mainDataPanel.setTextColor(TFT_WHITE, TFT_BLACK);
  mainDataPanel.setTextColor(TFT_WHITE, MY_BACKGROUND);
  
  mainDataPanel.drawString(radioModes[mainData.currentRadioMode], 1, 1, 4);
  mainDataPanel.drawFloat(mainData.freq/100.0, 1, 50, 1, 7);

  if(mainData.currentRadioMode == 0) mainDataPanel.drawString("MHz", 200, 1, 4);
  else mainDataPanel.drawString("kHz", 200, 1, 4);

  char buf[32];
  sprintf(buf, "Vol: %u", mainData.volume);  
  mainDataPanel.drawString(buf, 270, 20, 2);

  sprintf(buf, "RSSI: %u dBuV", mainData.rssi.value);
  if(mainData.rssi.isLow) mainDataPanel.setTextColor(TFT_RED, MY_BACKGROUND);
  if(mainData.rssi.isHigh) mainDataPanel.setTextColor(TFT_GREEN, MY_BACKGROUND);
  mainDataPanel.drawString(buf, 1, 50, 2);
  mainDataPanel.setTextColor(TFT_WHITE, MY_BACKGROUND);

  sprintf(buf, "SNR: %u dB", mainData.snr.value);
  if(mainData.snr.isLow) mainDataPanel.setTextColor(TFT_RED, MY_BACKGROUND);
  if(mainData.snr.isHigh) mainDataPanel.setTextColor(TFT_GREEN, MY_BACKGROUND);
  mainDataPanel.drawString(buf, 100, 50, 2);
  mainDataPanel.setTextColor(TFT_WHITE, MY_BACKGROUND);

  sprintf(buf, "MULT: %u%%", mainData.multipath.value);
  if(mainData.multipath.isLow) mainDataPanel.setTextColor(TFT_RED, MY_BACKGROUND);
  if(mainData.multipath.isHigh) mainDataPanel.setTextColor(TFT_GREEN, MY_BACKGROUND);
  mainDataPanel.drawString(buf, 200, 50, 2);
  mainDataPanel.setTextColor(TFT_WHITE, MY_BACKGROUND);
  
  if(mainData.agc) mainDataPanel.drawString("AGC", 280, 50, 2);

  for (int i = 0; i < strength; i++)
  {
    if (i < 9)
      mainDataPanel.fillRect(3 + (i * 4), 93 - (i * 1), 2, 4 + (i * 1), TFT_GREEN);
    else
      mainDataPanel.fillRect(3 + (i * 4), 93 - (i * 1), 2, 4 + (i * 1), TFT_RED);
  }
  
  /*if (mainData.isStereo)
    mainDataPanel.drawString("Stereo", 278, 1, 2);
  else
    mainDataPanel.drawString("Mono", 278, 1, 2);*/
  mainDataPanel.drawString((radio.getCurrentPilot()) ? "STEREO" : "MONO", 278, 1, 2);

  mainDataPanel.pushSprite(0, 0);
}


void drawSecondaryDataPanelFM()
{  
  /*static char rdsTime[32];
  static char stationName[255];
  static char programInfo[255];
  static char stationInfo[255];*/

  secondaryDataPanel.fillSprite(TFT_BLACK);
  secondaryDataPanel.drawRect(0, 0, 320, 100, TFT_WHITE);

  //mainDataPanel.setTextColor(TFT_WHITE, TFT_BLACK);
  mainDataPanel.setTextColor(TFT_WHITE, MY_BACKGROUND);
  //tft.setFreeFont(&Orbitron_Light_24);
  if(fmData.rdsAvailable) secondaryDataPanel.drawString("RDS", 150, 1, 4);

  if (fmData.rdsTimePtr != NULL) strncpy(fmData.rdsTime, fmData.rdsTimePtr, 25);
  if (fmData.stationNamePtr != NULL) strncpy(fmData.stationName, fmData.stationNamePtr, 9);  
  if (fmData.programInfoPtr != NULL)
  {
    strncpy(fmData.programInfo, fmData.programInfoPtr, 65);  
    
    #define toFind 13 //CR character is transmitted at the end of RDS text
    char * pch;
    pch = strchr (fmData.programInfo, toFind);
    if (pch != NULL) {
      uint8_t i = pch - fmData.programInfo;
      if (i < 62) {
        fmData.programInfo[i] = '\0';
      }
    }
  } 
  //if (fmData.stationInfoPtr != NULL) strncpy(fmData.stationInfo, fmData.stationInfoPtr, 33);

  
  secondaryDataPanel.drawString(fmData.stationName, 2, 1, 4);
  secondaryDataPanel.drawString(fmData.programInfo, 2, 30, 4);

  secondaryDataPanel.drawString(fmData.rdsTime, 255, 1, 4);
  //secondaryDataPanel.drawString(fmData.stationInfo, 2, 60, 4);






  
  secondaryDataPanel.drawRect(0, 0, 320, 100, TFT_WHITE);
  secondaryDataPanel.pushSprite(0, 101);
}

void handleEncoder()
{  
  static int encoderCount = 0;
  encoder.tick();

  //encoder.getMillisBetweenRotations();
  static int deb = 0;

  if (digitalRead(ENCODER_PUSH_BUTTON) == 0)
  {
    if (deb == 0)
    {
      deb = 1;
      // radio.setAudioMute(muted);
      drawMainDataPanel();
      //delay(200);
    }
  }
  else
    deb = 0;

  if(encoder.getPosition() != encoderCount){
    Serial.println(encoder.getPosition());
    encoderCount = encoder.getPosition();

    if(encoderCount != mainData.freq){
      mainData.freq = encoderCount;
      if(mainData.currentRadioMode == 0) mainData.freq = encoderCount * 10;
      else mainData.freq = encoderCount;
      radio.setFrequency(mainData.freq);
      clearFMScreenData();
    }
  }
  /*if(encoder.getDirection() == RotaryEncoder::Direction::CLOCKWISE) {
    radio.frequencyUp();
    Serial.println("ENC UP");
  }
  else if(encoder.getDirection() == RotaryEncoder::Direction::COUNTERCLOCKWISE){
    radio.frequencyDown();
    Serial.println("ENC DN");
  }
  else if(encoder.getDirection() == RotaryEncoder::Direction::NOROTATION);*/
  /*int newPos = encoder.getPosition();
  if (pos != newPos)
  {

    if (newPos > pos)
    {
      value = value - 1;
      encoderCount = -1;
    }
    if (newPos < pos)
    {
      value = value + 1;
      encoderCount = 1;
    }

    pos = newPos;

    drawMainDataPanel();
    showStatusSerial();
  }

  
  if (encoderCount == 1)
    radio.frequencyUp();
  else if (encoderCount == -1)
    radio.frequencyDown();*/
}

int getStrength(uint8_t rssi)
{

  if ((rssi >= 0) and (rssi <= 1))
    return 1; // S0
  if ((rssi > 1) and (rssi <= 1))
    return 2; // S1
  if ((rssi > 2) and (rssi <= 3))
    return  3; // S2
  if ((rssi > 3) and (rssi <= 4))
    return  4; // S3
  if ((rssi > 4) and (rssi <= 10))
    return  5; // S4
  if ((rssi > 10) and (rssi <= 16))
    return 6; // S5
  if ((rssi > 16) and (rssi <= 22))
    return 7; // S6
  if ((rssi > 22) and (rssi <= 28))
    return  8; // S7
  if ((rssi > 28) and (rssi <= 34))
    return 9; // S8
  if ((rssi > 34) and (rssi <= 44))
    return 10; // S9
  if ((rssi > 44) and (rssi <= 54))
    return 11; // S9 +10
  if ((rssi > 54) and (rssi <= 64))
    return 12; // S9 +20
  if ((rssi > 64) and (rssi <= 74))
    return 13; // S9 +30
  if ((rssi > 74) and (rssi <= 84))
    return 14; // S9 +40
  if ((rssi > 84) and (rssi <= 94))
    return 15; // S9 +50
  if (rssi > 94)
    return 16; // S9 +60
  if (rssi > 95)
    return 17; //>S9 +60

  return 17;

}

void setFMMode(){  
  radio.setFM(6400, 10800, 9660, 10);
  mainData.currentRadioMode = 0;
  delay(200);
  mainData.freq = radio.getFrequency();
  encoder.setPosition(mainData.freq/10);
  radio.setRdsConfig(3, 3, 3, 3, 3);
  radio.setFifoCount(1);
  clearFMScreenData();
}

void VolumeDown(){
  radio.volumeDown();
  mainData.volume = radio.getVolume();
}

void VolumeUp(){
  radio.volumeUp();
  mainData.volume = radio.getVolume();
}

// Instructions
void showHelp() {
  Serial.println("Type F to FM; A to MW; and 1 or 2 to SW");
  Serial.println("Type U to increase and D to decrease the frequency");
  Serial.println("Type S or s to seek station Up or Down");
  Serial.println("Type + or - to volume Up or Down");
  Serial.println("Type X to show current status");
  Serial.println("Type W to switch to SW");
  Serial.println("Type 1 to go to the next SW band");
  Serial.println("Type 2 to go to the previous SW band");
  Serial.println("Type ? to this help.");
  Serial.println("==================================================");
  //delay(1000);
}

// Show current frequency and status
void showStatusSerial()
{

  //delay(250);
  uint16_t currentFrequency = radio.getFrequency();

  Serial.print("You are tuned on ");
  if (radio.isCurrentTuneFM() ) {
    Serial.print(String(currentFrequency / 100.0, 2));
    Serial.print("MHz ");
    Serial.print((radio.getCurrentPilot()) ? "STEREO" : "MONO");
  } else {
    Serial.print(currentFrequency);
    Serial.print("kHz");
  }

  radio.getCurrentReceivedSignalQuality();
  Serial.print(" [SNR:" );
  Serial.print(radio.getCurrentSNR());
  Serial.print("dB");

  Serial.print(" RSSI:" );
  Serial.print(radio.getCurrentRSSI());
  Serial.println("dBuV]");

}

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // Calibrate
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 0);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.println("Touch corners as indicated");

  tft.setTextFont(1);
  tft.println();

  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

  Serial.println(); Serial.println();
  Serial.println("// Use this calibration code in setup():");
  Serial.print("  uint16_t calData[5] = ");
  Serial.print("{ ");

  for (uint8_t i = 0; i < 5; i++)
  {
    Serial.print(calData[i]);
    if (i < 4) Serial.print(", ");
  }

  Serial.println(" };");
  Serial.print("  tft.setTouch(calData);");
  Serial.println(); Serial.println();

  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Calibration complete!");
  tft.println("Calibration code sent to Serial port.");

  delay(4000);
}

void clearFMScreenData()
{
  fmData.rdsAvailable = false;
  fmData.rdsTimePtr = NULL;
  fmData.stationNamePtr = NULL;
  fmData.programInfoPtr = NULL;
  fmData.stationInfoPtr = NULL;

  fmData.rdsTime[0] = '\0';
  fmData.stationName[0] = '\0';
  fmData.programInfo[0] = '\0';
  fmData.stationInfo[0] = '\0';
}


void handleButtons()
{
  static uint32_t scanTime = millis();
  uint16_t t_x = 9999, t_y = 9999; // To store the touch coordinates

  // Scan keys every 50ms at most
  if (millis() - scanTime >= 50) {
    // Pressed will be set true if there is a valid touch on the screen
    bool pressed = tft.getTouch(&t_x, &t_y);
    scanTime = millis();
    for (uint8_t b = 0; b < buttonCount; b++) {
      if (pressed) {
        if (buttonList[b]->contains(t_x, t_y)) {
          buttonList[b]->press(true);
          buttonList[b]->pressAction();
        }
      }
      else {
        buttonList[b]->press(false);
        buttonList[b]->releaseAction();
      }
    }
  }
}

void buttonVolumeDownPressAction()
{
  if(buttonVolumeDown.justPressed()) {
    VolumeDown();
    buttonVolumeDown.drawSmoothButton(true);
    Serial.println("buttonVolumeDown button just pressed");
  }
}

void buttonVolumeDownReleaseAction(){ if(buttonVolumeDown.justReleased()) buttonVolumeDown.drawSmoothButton(false);}

void buttonVolumeUpPressAction()
{
  if(buttonVolumeUp.justPressed()) {
    VolumeUp();
    buttonVolumeUp.drawSmoothButton(true);
    Serial.println("buttonVolumeUp button just pressed");
  }
}

void buttonVolumeUpReleaseAction(){ if(buttonVolumeUp.justReleased()) buttonVolumeUp.drawSmoothButton(false);}

