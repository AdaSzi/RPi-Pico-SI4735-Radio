/*#include <Arduino.h>
#include <TFT_eSPI.h> 
TFT_eSPI tft = TFT_eSPI();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello");
  pinMode(LED_BUILTIN, OUTPUT);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0,4);
  tft.setTextColor(TFT_WHITE);
  tft.println ("Hello World!");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  Serial.println("AAA");
}*/

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include "hardware/pio.h"
#include "quadrature.pio.h"

// Set to tru if you want the calibration to run on each boot
#define REPEAT_CAL true

// Keypad start position, key sizes and spacing
#define KEY_W 95 // Key width
#define KEY_H 95 // Key height
#define KEY_SPACING_X 20 // X gap
#define KEY_SPACING_Y 10 // Y gap

#define KEY_X (KEY_W/2) + KEY_SPACING_X // X-axis centre of the first key
#define KEY_Y (KEY_H/2) + KEY_SPACING_Y // Y-axis centre of the first key

#define KEY_TEXTSIZE 3   // Font size multiplier

// Choose the font you are using
#define LABEL1_FONT &FreeSansOblique12pt7b // Key label font

// Adding a delay between keypressing to give our OS time to respond
uint8_t keydelay = 100;

// Create the screen object
TFT_eSPI tft = TFT_eSPI();

// Creating the labels for the buttons
// <name>[<number-of-lables>][<number-of-chars-per-label]
// The number of chars per label should include the termination \0.
char keyLabel[12][3] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};

// Setting the colour for each button
// You can use the colours defined in TFT_eSPI.h
uint16_t keyColor[12] = {TFT_BLUE, TFT_RED, TFT_GREEN, TFT_NAVY, 
                         TFT_MAROON, TFT_MAGENTA, TFT_ORANGE, TFT_SKYBLUE, 
                         TFT_PURPLE, TFT_CYAN, TFT_PINK, TFT_DARKCYAN
                        };
                        
// Create 12 'keys' to use later
TFT_eSPI_Button key[12];

void buttonpress(int button);
void showHelp();
void showStatus();
void checkRds();


#include <SI4735.h>

#define RESET_PIN 11

#define AM_FUNCTION 1
#define FM_FUNCTION 0

uint16_t currentFrequency;
uint16_t previousFrequency;
uint8_t bandwidthIdx = 0;
const char *bandwidth[] = {"6", "4", "3", "2", "1", "1.8", "2.5"};


PIO pio = pio0;
uint offset = pio_add_program(pio, &quadratureA_program);
uint sm = pio_claim_unused_sm(pio, true);


SI4735 rx;
void showFrequency( uint16_t freq ) {

  if (rx.isCurrentTuneFM())
  {
    Serial.print(String(freq / 100.0, 2));
    Serial.println("MHz ");
  }
  else
  {
    Serial.print(freq);
    Serial.println("kHz");
  }
  
}

void setup() {
  //delay(3000);
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  Serial.println("");

  // Initialise the TFT screen
  tft.init();

  // Set the rotation before we calibrate
  tft.setRotation(1);
  
  // Clear the screen
  tft.fillScreen(TFT_BLACK);

  // Draw the keys ( 3 times 4 loops to create 12)
  for (uint8_t row = 0; row < 3; row++) {   // 3 rows
    for (uint8_t col = 0; col < 4; col++) { // of 4 buttons
      
      uint8_t b = col + row * 4; // The button number is the column we are on added to the row we are on 
                                 // multiplied by the number of buttons per row. C++ uses the Order of operations
                                 // you are used to so first the row is multiplied by 4 and then the col is added.
                                 // The first button is 0.
                                 // Example. col = 2 (third column), row = 1 (second row), becomes: 1 * 4 = 4 --> 4 + 2 = 6. This is the 7th button.

      key[b].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
                        KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
                        KEY_W, KEY_H, TFT_WHITE, keyColor[b], TFT_WHITE,
                        keyLabel[b], KEY_TEXTSIZE);
      key[b].drawButton();
    }
  }


  digitalWrite(RESET_PIN, HIGH);
  
  Serial.println("AM and FM station tuning test.");

  showHelp();

  // Look for the Si47XX I2C bus address
  int16_t si4735Addr = rx.getDeviceI2CAddress(RESET_PIN);
  if ( si4735Addr == 0 ) {
    Serial.println("Si473X not found!");
    Serial.flush();
    while (1);
  } else {
    Serial.print("The SI473X / SI474X I2C address is 0x");
    Serial.println(si4735Addr, HEX);
  }


  delay(500);
  rx.setup(RESET_PIN, FM_FUNCTION);
  // rx.setup(RESET_PIN, -1, 1, SI473X_ANALOG_AUDIO);
  // Starts defaul radio function and band (FM; from 84 to 108 MHz; 103.9 MHz; step 100kHz)
  rx.setFM(8400, 10800, 10650, 10);
  delay(500);
  currentFrequency = previousFrequency = rx.getFrequency();
  rx.setVolume(45);
  showStatus();


  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  quadratureA_program_init(pio, sm, offset, ENCODER_PIN_A, ENCODER_PIN_B);

}

void loop() {

  uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

  // Pressed will be set true is there is a valid touch on the screen
  bool pressed = tft.getTouch(&t_x, &t_y);
  
  // Check if any key coordinate boxes contain the touch coordinates
  for (uint8_t b = 0; b < 12; b++) {
    if (pressed && key[b].contains(t_x, t_y)) {
      key[b].press(true);  // tell the button it is pressed
    } else {
      key[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // Check if any key has changed state
  for (uint8_t b = 0; b < 12; b++) {

    if (key[b].justReleased()) key[b].drawButton(); // draw normal again

    if (key[b].justPressed()) 
    {
      key[b].drawButton(true);  // draw invert (background becomes text colour and text becomes background colour
      buttonpress(b); // Call the button press function and pass the button number
    }
  }


  if (Serial.available() > 0)
  {
    char key = Serial.read();
    switch (key)
    {
    case '+':
      rx.volumeUp();
      break;
    case '-':
      rx.volumeDown();
      break;
    case 'a':
    case 'A':
      rx.setAM(520, 1750, 810, 10);
      rx.setAvcAmMaxGain(48); // Sets the maximum gain for automatic volume control on AM mode.
      rx.setSeekAmLimits(520, 1750);
      rx.setSeekAmSpacing(10); // spacing 10kHz
      break;
    case 'f':
    case 'F':
      rx.setFM(8600, 10800, 10390, 50);
      rx.setSeekAmRssiThreshold(0);
      rx.setSeekAmSNRThreshold(10);
      break;
    case '1':
      rx.setAM(100, 30000, 7200, 5);
      rx.setSeekAmLimits(100, 30000);   // Range for seeking.
      rx.setSeekAmSpacing(1); // spacing 1kHz
      Serial.println("\nALL - LW/MW/SW");
      break;
    case 'U':
    case 'u':
      rx.frequencyUp();
      break;
    case 'D':
    case 'd':
      rx.frequencyDown();
      break;
    case 'b':
    case 'B':
      if (rx.isCurrentTuneFM())
      {
        Serial.println("Not valid for FM");
      }
      else
      {
        if (bandwidthIdx > 6)
          bandwidthIdx = 0;
        rx.setBandwidth(bandwidthIdx, 1);
        Serial.print("Filter - Bandwidth: ");
        Serial.print(String(bandwidth[bandwidthIdx]));
        Serial.println(" kHz");
        bandwidthIdx++;
      }
      break;
    case 'S':
      rx.seekStationProgress(showFrequency,1);
      // rx.seekStationUp();
      break;
    case 's':
      rx.seekStationProgress(showFrequency,0);
      // rx.seekStationDown();
      break;
    case '0':
      showStatus();
      break;
    case '4':
      rx.setFrequencyStep(1);
      Serial.println("\nStep 1");
      break;  
    case '5':
      rx.setFrequencyStep(5);
      Serial.println("\nStep 5");
      break;    
    case '6':
      rx.setFrequencyStep(10);
      Serial.println("\nStep 10");
      break;
    case '7':
      rx.setFrequencyStep(100);
      Serial.println("\nStep 100");      
      break;
    case '8':
      rx.setFrequencyStep(1000);
      Serial.println("\nStep 1000");    
      break;
    case '?':
      showHelp();
      break;
    default:
      break;
    }
  }
  delay(100);
  currentFrequency = rx.getCurrentFrequency();
  if (currentFrequency != previousFrequency)
  {
    previousFrequency = currentFrequency;
    showStatus();
    delay(300);
  }
  digitalWrite(LED_BUILTIN, HIGH);

  pio_sm_exec_wait_blocking(pio, sm, pio_encode_in(pio_x, 32));
  int x = pio_sm_get_blocking(pio, sm);
  Serial.print("Encoder: ");
  Serial.println(x);
}

void buttonpress(int button)
{
  //Handle a button press. Buttons are 0 indexed, meaning that the first button is button 0.
  switch(button){
    case 0:
     // Sending a combination of CTRL, ALT and a Function key.
     Serial.println("Button 1 pressed");
     break;
    case 1:
     // Sending a combination of CTRL, ALT and a Function key.
     Serial.println("Button 2 pressed");
     break;
    case 2:
     // Sending a combination of CTRL, ALT and a Function key.
     Serial.println("Button 3 pressed");
     break;
    case 3:
     Serial.println("Button 4 pressed");
     break;
    case 4:
     Serial.println("Button 5 pressed");
     break;
    case 5:
     Serial.println("Button 6 pressed");
     break;
    case 6:
     Serial.println("Button 7 pressed");
     break;
    case 7:
     Serial.println("Button 8 pressed");
     break;
    case 8:
     Serial.println("Button 9 pressed");
     break;
    case 9:
     Serial.println("Button 10 pressed");
     // Nothing for now
     break;
    case 10:
     Serial.println("Button 11 pressed");
     // Nothing for now
     break;
    case 11:
     Serial.println("Button 12 pressed");
     // Nothing for now
     break;
  }
}

void showHelp()
{
  Serial.println("The SI473X / SI474X I2C circuit test");
  Serial.println("Type F to FM; A to MW; 1 to All Band (100kHz to 30MHz)");
  Serial.println("Type U to increase and D to decrease the frequency");
  Serial.println("Type S or s to seek station Up or Down");
  Serial.println("Type + or - to volume Up or Down");
  Serial.println("Type 0 to show current status");
  Serial.println("Type B to change Bandwidth filter");
  Serial.println("Type 4 to 8 (4 to step 1; 5 to step 5kHz; 6 to 10kHz; 7 to 100kHz; 8 to 1000kHz)");
  Serial.println("Type ? to this help.");
  Serial.println("==================================================");
  delay(1000);
}

void showStatus()
{
  // rx.getStatus();
  previousFrequency = currentFrequency = rx.getFrequency();
  rx.getCurrentReceivedSignalQuality();
  Serial.print("You are tuned on ");
  if (rx.isCurrentTuneFM())
  {
    Serial.print(String(currentFrequency / 100.0, 2));
    Serial.print("MHz ");
    Serial.print((rx.getCurrentPilot()) ? "STEREO" : "MONO");
  }
  else
  {
    Serial.print(currentFrequency);
    Serial.print("kHz");
  }
  Serial.print(" [SNR:");
  Serial.print(rx.getCurrentSNR());
  Serial.print("dB");

  Serial.print(" Signal:");
  Serial.print(rx.getCurrentRSSI());
  Serial.println("dBuV]");
}