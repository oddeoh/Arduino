
// Specifically for use with the Adafruit Feather, the pins are pre-set here!

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>

//  LED's
int LED_RED_PIN = 12;   //  1.9v  150
int LED_GRN_PIN = 11;   //  3.8v  51   8 is used

//  BUTTON's
int BTN_RED_PIN = 10;   //  OUTPUT ISR  Int0
int BTN_GRN_PIN = 9;    //  OUTPUT ISR  Int1

//int LED_ORA_PIN = 12;   //  2.1v  100
//int LED_YEL_PIN = 12;   //  2.1v  100
//int LED_BLU_PIN = 12;   //  2.7v  100

//  LED's on Buttons
int LED_GRNBTN_PIN = 6; //  PLAY
int LED_REDBTN_PIN = 5; //  HALT

int BTN_GRN_Last = LOW;
int BTN_RED_Last = LOW;

//  STATE
volatile int BTN_GRN_STATE = LOW;
volatile int BTN_RED_STATE = LOW;

volatile int LED_YEL_STATE = LOW;
volatile int LED_RED_STATE = LOW;
volatile int LED_GRN_STATE = LOW;
volatile int LED_ORA_STATE = LOW;
volatile int LED_BLU_STATE = LOW;
volatile int LED_GRNBTN_STATE = LOW;
volatile int LED_REDBTN_STATE = LOW;

//  Button Debouncing
long debounceTime = 20;
volatile unsigned long BTN_GRN_BounceTime = 0;
volatile unsigned long BTN_RED_BounceTime = 0;

// These are the pins used
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)

// Feather M0 or 32u4
#if defined(__AVR__) || defined(ARDUINO_SAMD_FEATHER_M0)
#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

// Feather ESP8266
#elif defined(ESP8266)
#define VS1053_CS      16     // VS1053 chip select pin (output)
#define VS1053_DCS     15     // VS1053 Data/command select pin (output)
#define CARDCS          2     // Card chip select pin
#define VS1053_DREQ     0     // VS1053 Data request, ideally an Interrupt pin

// Feather ESP32
#elif defined(ESP32)
#define VS1053_CS      32     // VS1053 chip select pin (output)
#define VS1053_DCS     33     // VS1053 Data/command select pin (output)
#define CARDCS         14     // Card chip select pin
#define VS1053_DREQ    15     // VS1053 Data request, ideally an Interrupt pin

// Feather Teensy3
#elif defined(TEENSYDUINO)
#define VS1053_CS       3     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          8     // Card chip select pin
#define VS1053_DREQ     4     // VS1053 Data request, ideally an Interrupt pin

// WICED feather
#elif defined(ARDUINO_STM32_FEATHER)
#define VS1053_CS       PC7     // VS1053 chip select pin (output)
#define VS1053_DCS      PB4     // VS1053 Data/command select pin (output)
#define CARDCS          PC5     // Card chip select pin
#define VS1053_DREQ     PA15    // VS1053 Data request, ideally an Interrupt pin

#elif defined(ARDUINO_FEATHER52)
#define VS1053_CS       30     // VS1053 chip select pin (output)
#define VS1053_DCS      11     // VS1053 Data/command select pin (output)
#define CARDCS          27     // Card chip select pin
#define VS1053_DREQ     31     // VS1053 Data request, ideally an Interrupt pin
#endif

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

int     ptr = 0;
String  trackId;

void setup() {
  Serial.begin(38400);

  // if you're using Bluefruit or LoRa/RFM Feather, disable the BLE interface
  pinMode(8, INPUT_PULLUP);
  // bug with MIDI even when not in use
  pinMode(1, INPUT_PULLUP);

  // Wait for serial port to be opened, remove this line for 'standalone' operation
  while (!Serial) {
    delay(1);
  }

  Serial.println("Oddeoh(tm) Live UX!");

  Serial.println("\n\nAdafruit VS1053 Feather Test");

  //  Initialize PIN's
  pinMode (LED_RED_PIN, OUTPUT);
  pinMode (LED_GRN_PIN, OUTPUT);

  pinMode (BTN_RED_PIN, INPUT_PULLUP);
  pinMode (BTN_GRN_PIN, INPUT_PULLUP);

  //  ISR for HALT and SERVE button presses
  attachInterrupt(2, isr_BTN_RED_Toggle, CHANGE);
  attachInterrupt(3, isr_BTN_GRN_Toggle, CHANGE);

  if (! musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
  }

  Serial.println(F("VS1053 found"));

  // Make a tone to indicate VS1053 is working
  Serial.println("Sending test tone");
  musicPlayer.setVolume(10, 10);
  musicPlayer.sineTest(0x44, 1500);
  

  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  Serial.println("SD OK!");

  // list files
  //printDirectory(SD.open("/"), 0);    TRK_5.MP3 7193119
  
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(1, 1);
  musicPlayer.sineTest(0x44, 1500);

//#if defined(__AVR_ATmega32U4__)
  // Timer interrupts are not suggested, better to use DREQ interrupt!
  // but we don't have them on the 32u4 feather...
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int
//#elif defined(ESP32)
  // no IRQ! doesn't work yet :/
//#else
  // If DREQ is on an interrupt pin we can do background
  // audio playing
  //musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
//#endif

  Power_LED_On();

  // Play a file in the background, REQUIRES interrupts!
  //Serial.println(F("Playing full track 001"));
  //musicPlayer.playFullFile("TRK_1.mp3");

  //Serial.println(F("Playing track 002"));
  //musicPlayer.startPlayingFile("TRK_2.mp3");

  BTN_GRN_STATE = LOW;
  BTN_RED_STATE = LOW;
  
}

void loop() {

  Serial.print(".");

  //  Read for a HALT to interrupt the playing file..
  while (musicPlayer.playingMusic) {
    Serial.println("Player.Playing");
    Error_LED_Off();

    //isr_BTN_RED_Toggle();
    BTN_RED_STATE = digitalRead(BTN_RED_STATE);
    if (BTN_RED_STATE == HIGH ) {
      //  They want to stop the file
      BTN_RED_STATE = LOW;
      Error_LED_On();
      //musicPlayer.stopPlaying();
      Serial.println("Player.StopPlaying");
    }
    delay(100);
  }

  // File is NOT playing in the background
  while (musicPlayer.stopped()) {
    Serial.println("Player.Stopped");
    Error_LED_On();
    
    BTN_GRN_STATE = digitalRead(BTN_GRN_STATE);
    if (BTN_GRN_STATE == HIGH) {
      BTN_GRN_STATE = LOW;
      Error_LED_Off();
      //  GET THE NEXT FILE POINTED TO
      ptr = ptr + 1;
      if (ptr > 8) {
        ptr = 1;
      }
      Serial.println("Player.PlayFullFile");
      trackId = "TRK_";
      trackId.concat(ptr);
      trackId.concat(".mp3");
      const char *charBuf = trackId.c_str();
      musicPlayer.playFullFile(charBuf);
      //musicPlayer.startPlayingFile(charBuf);
      Serial.println(ptr);
      delay(100);
    }
  }

  delay(100);
}

//  ISR Toggle global button state on press
void isr_BTN_RED_Toggle() {
  Serial.println("RED BTN HIT");
  if ((long)(millis() - BTN_RED_BounceTime) >= debounceTime) {
    if (BTN_RED_STATE == LOW) {
      BTN_RED_STATE = HIGH;
      //HaltEvent();
    } else {
      BTN_RED_STATE = LOW;
    }
    BTN_RED_BounceTime = millis();
  }
}

//  ISR Toggle global button press state
void isr_BTN_GRN_Toggle() {
  Serial.println("GRN BTN HIT");
  if ((long)(millis() - BTN_GRN_BounceTime) >= debounceTime) {
    if (BTN_GRN_STATE == LOW) {
      BTN_GRN_STATE = HIGH;
    } else {
      BTN_GRN_STATE = LOW;
    }
    BTN_GRN_BounceTime = millis();
  }
}

void HaltEvent() {
  //Disable_Motors();
  //Disable_ISRs();
  //  Turn Off LED's
  Power_LED_Off();
  //Data_LED_Off();
  //Queued_LED_Off();
  //Stream_LED_Off();
  //GreenButton_LED_Off();
  //RedButton_LED_Off();
  //
  Error_LED_On();
  delay(3000);
  //resetFunc();
  //software_Reset();
  //  SYSTEM RESETS!!!
}

//  TURN OFF All LED's
void TurnOff_LEDs() {
  Power_LED_Off();   //  GRN
  //Queued_LED_Off();  //  ORA
  Error_LED_Off();   //  RED
  //Stream_LED_Off();  //  BLU
  //Data_LED_Off();    //  YEL
  //GreenButton_LED_Off();   //  GRN Button
  //RedButton_LED_Off();    //  RED Button
}

//  TURN ON All LED's
void TurnOn_LEDs() {
  Power_LED_On();     //  GRN
  //Queued_LED_On();    //  ORA
  Error_LED_On();     //  RED
  //Stream_LED_On();    //  BLU
  //Data_LED_On();      //  YEL
  //GreenButton_LED_On(); //  GRN BTN
  //RedButton_LED_On(); //  RED BTN
}

//  DISABLE All ISRs
void Disable_ISRs() {
  noInterrupts();
  detachInterrupt(1);
  detachInterrupt(0);
}

//  LED's ON/OFF
void Power_LED_Off() {
  //  Turn OFF the "Power" LED
  LED_GRN_STATE = LOW;
  digitalWrite(LED_GRN_PIN, LED_GRN_STATE);
}
void Power_LED_On() {
  //  Turn ON the "Power" LED
  LED_GRN_STATE = HIGH;
  digitalWrite(LED_GRN_PIN, LED_GRN_STATE);
}
void Queued_LED_Off() {
  //  Turn OFF the "Queued" LED
  LED_ORA_STATE = LOW;
  digitalWrite(LED_ORA_PIN, LED_ORA_STATE);
}
void Queued_LED_On() {
  //  Turn ON the "Queued" LED
  LED_ORA_STATE = HIGH;
  digitalWrite(LED_ORA_PIN, LED_ORA_STATE);
}
void Error_LED_Off() {
  //  Turn OFF the "Error" LED
  LED_RED_STATE = LOW;
  digitalWrite(LED_RED_PIN, LED_RED_STATE);
}
void Error_LED_On() {
  //  Turn ON the "Error" LED
  LED_RED_STATE = HIGH;
  digitalWrite(LED_RED_PIN, LED_RED_STATE);
}
void Stream_LED_Off() {
  //  Turn OFF the "Stream" LED
  LED_BLU_STATE = LOW;
  digitalWrite(LED_BLU_PIN, LED_BLU_STATE);
}
void Stream_LED_On() {
  //  Turn ON the "Stream" LED
  LED_BLU_STATE = HIGH;
  digitalWrite(LED_BLU_PIN, LED_BLU_STATE);
}
void Data_LED_Off() {
  //  Turn OFF the "Data" LED
  LED_YEL_STATE = LOW;
  digitalWrite(LED_YEL_PIN, LED_YEL_STATE);
}
void Data_LED_On() {
  //  Turn ON the "Data" LED
  LED_YEL_STATE = HIGH;
  digitalWrite(LED_YEL_PIN, LED_YEL_STATE);
}

//  LED's on the Buttons
void GreenButton_LED_Off() {
  //  Turn OFF the "Green Button" LED
  LED_GRNBTN_STATE = LOW;
  digitalWrite(LED_GRNBTN_PIN, LED_GRNBTN_STATE);
}
void GreenButton_LED_On() {
  //  Turn ON the "Data" LED
  LED_GRNBTN_STATE = HIGH;
  digitalWrite(LED_GRNBTN_PIN, LED_GRNBTN_STATE);
}
//  LED's on the Buttons
void RedButton_LED_Off() {
  //  Turn OFF the "Red Button" LED
  LED_REDBTN_STATE = LOW;
  digitalWrite(LED_REDBTN_PIN, LED_REDBTN_STATE);
}
void RedButton_LED_On() {
  //  Turn ON the "Data" LED
  LED_REDBTN_STATE = HIGH;
  digitalWrite(LED_REDBTN_PIN, LED_REDBTN_STATE);
}


/// File listing helper
void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      //Serial.println("**nomorefiles**");
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}


