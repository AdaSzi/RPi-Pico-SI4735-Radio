/*#include "main.h"

#include <Arduino.h>
#include "hardware/pio.h"
#include "quadrature.pio.h"

PIO pio = pio0;
uint offset = pio_add_program(pio, &quadratureA_program);
uint sm = pio_claim_unused_sm(pio, true);


#include <TFT_eSPI.h> 
#include "HelveticaMono20pt7b.h"
#include "HelveticaMono30pt7b.h"
TFT_eSPI tft = TFT_eSPI();

#define TFT_TXTBKGDEBUG 0x52AA
//#define TFT_TXTBKGDEBUG 0x0


#include <SI4735.h>
SI4735 si4735;

//https://pu2clr.github.io/SI4735/extras/apidoc/html/


#define AM_FUNCTION 1
#define FM_FUNCTION 0

typedef struct {
  const char *freqName;
  uint16_t   minimumFreq;
  uint16_t   maximumFreq;
  uint16_t   currentFreq;
  uint16_t   currentStep;
} Band;


Band band[] = {{"60m",4700, 5200, 4850, 5},
  {"49m",5700, 6200, 6000, 5},
  {"41m",7100, 7600, 7300, 5},
  {"31m",9300, 10000, 9600, 5},
  {"25m",11400, 12200, 11940, 5},
  {"22m",13500, 13900, 13600, 5},
  {"19m",15000, 15800, 15200, 5},
  {"16m",17400, 17900, 17600, 5},
  {"13m",21400, 21800, 21500, 5},
  {"11m",25600, 27500, 27220, 1}
};

const int lastBand = (sizeof band / sizeof(Band)) - 1;
int  currentFreqIdx = 3; // Default SW band is 31M

uint16_t currentFrequency;




enum bandMode {FM, LSB, USB, AM};
const char *bandModeDesc[] = {"FM ", "LSB", "USB", "AM "};


struct MainStatusBarData
{
    uint8_t bandMode;
    uint16_t frequency; //kHz
    uint8_t rssi;
    uint8_t snr;
    bool atc;
    bool agc;
    
} mainStatusBarData;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  Serial.begin(115200);
  Serial.println("\nHello");

  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  quadratureA_program_init(pio, sm, offset, ENCODER_PIN_A, ENCODER_PIN_B);

  displaySetup();
  radioSetup();
}

void loop() {
  displayLoop();
  radioLoop();

  checkEncoder();
}

void checkEncoder()
{
  pio_sm_exec_wait_blocking(pio, sm, pio_encode_in(pio_x, 32));
  int x = pio_sm_get_blocking(pio, sm);
  static int lastx = 0;
  if(lastx != x)
  {
    lastx = x;
    Serial.print("Encoder: ");
    Serial.println(x);
  }
}


void displaySetup()
{
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0,4);
  tft.setTextColor(TFT_WHITE);
  tft.println ("Hello World!");
  drawMainStatusBar();
}

void displayLoop()
{

}

void drawMainStatusBar()
{
  tft.drawRect(0, 0, 320, 100, TFT_WHITE);

  tft.setTextColor(TFT_WHITE, TFT_TXTBKGDEBUG);  tft.setTextSize(1);
  //tft.unloadFont();  tft.setFreeFont(&HelveticaMono20pt7b);
  tft.setTextDatum(ML_DATUM);
  if(si4735.isCurrentTuneFM())
  {
    //snprintf(buf, 64, "FM %u MHz", si4735.getFrequency());
    tft.drawString("FM", 10, 50, 1);
  }
  else 
  {
    tft.drawString("AM", 10, 50, 1);
    //snprintf(buf, 64, "?? %u kHz", si4735.getFrequency());
  }
  String buf;
  delay(200);
  uint16_t freq = si4735.getFrequency();
  Serial.println ("1");
  buf = String(freq / 100.0, 2);
  Serial.println ("2");
  Serial.println(freq);
  Serial.println ("3");
  delay(200);
    tft.drawString("LKJASFD", 50, 50, 1);
  Serial.println ("4");
  tft.unloadFont();
}



void radioSetup()
{
  Serial.println("Test and validation of the SI4735 Arduino Library.");
  Serial.println("AM and FM station tuning test.");


  // gets and sets the Si47XX I2C bus address.
  int16_t si4735Addr = si4735.getDeviceI2CAddress(RESET_PIN);
  if ( si4735Addr == 0 ) {
    Serial.println("Si473X not found!");
    Serial.flush();
    while (1);
  } else {
    Serial.print("The Si473X I2C address is 0x");
    Serial.println(si4735Addr, HEX);
  }

  showHelp();

  delay(500);

  si4735.setup(RESET_PIN, FM_FUNCTION);

  // Starts defaul radio function and band (FM; from 84 to 108 MHz; 103.9 MHz; step 100kHz)
  si4735.setFM(6400, 10800,  10390, 10);

  delay(500);

  currentFrequency = si4735.getFrequency();
  si4735.setVolume(45);
  showStatusSerial();
}

void radioLoop()
{
  
  if (Serial.available() > 0)
  {
    char key = Serial.read();
    switch (key)
    {
      case '+':
        si4735.volumeUp();
        break;
      case '-':
        si4735.volumeDown();
        break;
      case 'a':
      case 'A':
        si4735.setAM(570, 1710,  810, 10);
        si4735.setAvcAmMaxGain(32); // Sets the maximum gain for automatic volume control on AM mode
        showStatusSerial();
        break;
      case 'f':
      case 'F':
        si4735.setFM(8600, 10800,  10760, 10);
        showStatusSerial();
        break;
      case '2':
        if ( currentFreqIdx < lastBand ) {
          currentFreqIdx++;
        } else {
          currentFreqIdx = 0;
        }
        si4735.setAM(band[currentFreqIdx].minimumFreq, band[currentFreqIdx].maximumFreq, band[currentFreqIdx].currentFreq, band[currentFreqIdx].currentStep);
        si4735.setAvcAmMaxGain(48); // Sets the maximum gain for automatic volume control on AM mode

        delay(100);
        currentFrequency = band[currentFreqIdx].currentFreq;
        showBandName();
        showStatusSerial();
        break;
      case '1':
        if ( currentFreqIdx > 0 ) {
          currentFreqIdx--;
        } else {
          currentFreqIdx = lastBand;
        }
        si4735.setAM(band[currentFreqIdx].minimumFreq, band[currentFreqIdx].maximumFreq, band[currentFreqIdx].currentFreq, band[currentFreqIdx].currentStep);
        delay(100);
        currentFrequency = band[currentFreqIdx].currentFreq;
        showBandName();
        showStatusSerial();
        break;
      case 'W':
      case 'w':
        si4735.setAM(band[currentFreqIdx].minimumFreq, band[currentFreqIdx].maximumFreq, band[currentFreqIdx].currentFreq, band[currentFreqIdx].currentStep);
        delay(100);
        currentFrequency = band[currentFreqIdx].currentFreq;
        showBandName();
        showStatusSerial();         
        break;  
      case 'U':
      case 'u':
        si4735.frequencyUp();
        showStatusSerial();
        break;
      case 'D':
      case 'd':
        si4735.frequencyDown();
        showStatusSerial();
        break;
      case 'S':
        si4735.seekStationUp();
        showStatusSerial();
        break;
      case 's':
        si4735.seekStationDown();
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
  delay(1000);
}

// Show current frequency and status
void showStatusSerial()
{

  delay(250);
  band[currentFreqIdx].currentFreq = currentFrequency = si4735.getFrequency();

  Serial.print("You are tuned on ");
  if (si4735.isCurrentTuneFM() ) {
    Serial.print(String(currentFrequency / 100.0, 2));
    Serial.print("MHz ");
    Serial.print((si4735.getCurrentPilot()) ? "STEREO" : "MONO");
  } else {
    Serial.print(currentFrequency);
    Serial.print("kHz");
  }

  si4735.getCurrentReceivedSignalQuality();
  Serial.print(" [SNR:" );
  Serial.print(si4735.getCurrentSNR());
  Serial.print("dB");

  Serial.print(" RSSI:" );
  Serial.print(si4735.getCurrentRSSI());
  Serial.println("dBuV]");

}


void showBandName() {
  Serial.println("Band: ");
  Serial.println(band[currentFreqIdx].freqName);
  Serial.println("*******");  
}*/

#include "main.h"
#include <Wire.h>
#include <SI4735.h>

#include "TFT_eSPI.h"
#include <RotaryEncoder.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3


RotaryEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B, RotaryEncoder::LatchMode::TWO03);

#define color1 0xC638
#define color2 0xC638

volatile int encoderCount = 0;

int value = 980;
int minimal = 880;
int maximal = 1080;
int strength = 0;

float freq = 0.00;
SI4735 radio;

bool muted = 0;
int deb = 0;

void setup()
{  
  Serial.begin(115200);
  Serial.println("\nHello");

  tft.begin();
  //tft.writecommand(0x11);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  Wire.begin();
  radio.setI2CFastModeCustom(100000);
  radio.getDeviceI2CAddress(RESET_PIN); // Looks for the I2C bus address and set it.  Returns 0 if error
  radio.setup(RESET_PIN, MW_BAND_TYPE);
  delay(200);
  radio.setTuneFrequencyAntennaCapacitor(0);
  radio.setFM(6400, 10800, 10390, 10);
  delay(200);
  freq = radio.getFrequency() / 100.0;

  radio.setVolume(58);

  spr.createSprite(320, 170);
  tft.setTextDatum(4);
  tft.setSwapBytes(true);
  tft.setFreeFont(&Orbitron_Light_24);
  tft.setTextColor(color1, TFT_BLACK);

  drawSprite();
}

void readEncoder()
{

  static int pos = 0;

  encoderCount = 0;

  encoder.tick();

  if (digitalRead(0) == 0)
  {
    if (deb == 0)
    {
      deb = 1;
      muted = !muted;
      // radio.setAudioMute(muted);
      drawSprite();
      //delay(200);
    }
  }
  else
    deb = 0;

  int newPos = encoder.getPosition();
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

    drawSprite();
    showStatusSerial();
  }
}

void drawSprite()
{

  // if(muted==false)

  if (encoderCount == 1)
    radio.frequencyUp();
  else if (encoderCount == -1)
    radio.frequencyDown();
  
  freq = radio.getFrequency() / 100.0;
  value = freq * 10;

  // strength=getStrength();

  //spr.fillSprite(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.drawFloat(freq, 1, 100, 40, 7);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  
  for (int i = 0; i < strength; i++)
  {
    if (i < 9)
      spr.fillRect(100 + (i * 4), 50 - (i * 1), 2, 4 + (i * 1), TFT_GREEN);
    else
      spr.fillRect(100 + (i * 4), 50 - (i * 1), 2, 4 + (i * 1), TFT_RED);
  }
  
  if (radio.getCurrentPilot())
    tft.drawString("Stereo", 275, 31, 2);
  else
    tft.drawString("Mono", 275, 31, 2);

  //spr.drawLine(160, 114, 160, 170, TFT_RED);
  //spr.pushSprite(0, 170);
}

int getStrength()
{

  uint8_t rssi;

  rssi = radio.getCurrentRSSI();

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

void loop()
{
  readEncoder();
  radio.getCurrentReceivedSignalQuality();
  strength = getStrength();

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
  delay(5);
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