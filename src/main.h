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
    char *rdsTimePtr = NULL;
    char *stationNamePtr = NULL;
    char *programInfoPtr = NULL;
    char *stationInfoPtr = NULL;

    char rdsTime[25] = "\0";
    char stationName[9] = "\0";
    char programInfo[65] = "\0";
    char stationInfo[33] = "\0";
};

char radioModes[][4] = {"FM ", "AM ", "USB", "LSB"};

void setup();
void loop();


void setupRadio();
void handleRadio();
void setFMMode();
void VolumeDown();
void VolumeUp();
int getStrength(uint8_t rssi);

void setupScreen();
void handleScreen();
void touch_calibrate();
void drawMainDataPanel();
void drawSecondaryDataPanelFM();

void initButtons();
void handleButtons();

void buttonVolumeDownPressAction();
void buttonVolumeDownReleaseAction();
void buttonVolumeUpPressAction();
void buttonVolumeUpReleaseAction();



void handleEncoder();
void showHelp();
void showStatusSerial();

void clearFMScreenData();

#endif