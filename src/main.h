#ifndef _MAIN_H_
#define _MAIN_H_

#include <Arduino.h>
/*#include "hardware/pio.h"
#include "quadrature.pio.h"

void setup();
void loop();
void checkEncoder();

void displaySetup();
void displayLoop();
void drawMainStatusBar();

void radioSetup();
void radioLoop();
void showHelp();
void showStatus();
void showBandName();*/

struct rssiData {
    uint8_t value = 0;
    bool isHigh = false;
    bool isLow = true;
};

struct snrData {
    uint8_t value = 0;
    bool isHigh = false;
    bool isLow = true;
};

struct multipathData {
    uint8_t value = 0;
    bool isHigh = false;
    bool isLow = true;
};

struct mainScreenData {
    uint8_t currentRadioMode = 0;
    uint16_t freq = 0;
    bool isStereo = false;
    rssiData rssi;
    snrData snr;
    multipathData multipath;
    bool agc = false;
    uint8_t volume = 0;
};

struct FMScreenData {
    bool rdsAvailable = false;
    char *rdsTimePtr;
    char *stationNamePtr;
    char *programInfoPtr;
    char *stationInfoPtr;

    char rdsTime[25];
    char stationName[9];
    char programInfo[65];
    char stationInfo[33];
};

char radioModes[][4] = {"FM ", "AM ", "USB", "LSB"};

void setup();
void loop();


void setupRadio();
void handleRadio();
void setFMMode();
int getStrength(uint8_t rssi);

void setupScreen();
void handleScreen();
void drawMainDataPanel();
void drawSecondaryDataPanelFM();

void handleEncoder();
void showHelp();
void showStatusSerial();

#endif