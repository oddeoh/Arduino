//  RF_RemoteSensor_Temp
//  LM: BRB 08/11/2016 6PM EST
//  Copyright (C) Beau Bramlett. All Rights Reserved

//  BN: Arduino/Genuino Uno [Red]
//  VID: 2341
//  PID: 0043
//  SN: 954333437333514161E2

//  BN: Arduino/Genuino Uno [SainSmart Black)
//  VID: 2341
//  PID: 0043
//  SN: 85531303630351F0A130

#define SERIAL_BAUD   57600

// DHT22 - START
// Hum/Temp (AM2302)
#include "DHT.h"
#define DHTTYPE DHT22   //  DHT 22  (AM2302), AM2321
#define DHTPIN 3        //  PWM PIN
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);
//  DHT22 - END

//  RTC DS3231 - START
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
//  GND gnd BLK
//  3.3 vin WHT
//  SDA A4  PUR
//  SCL A5  GRY
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//  RTC DS3231 - END


//  RF69_433
#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>

#define NETWORKID     100  // The same on all nodes that talk to each other
#define NODEID        2    // The unique identifier of this node
#define RECEIVER      1    // The recipient of packets

//Match frequency to the hardware version of the radio on your Feather
#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
//#define FREQUENCY     RF69_915MHZ

#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HCW   true // set to 'true' if you are using an RFM69HCW module


//  DIGITAL PINS
#define RF_G0     2   //  BRN
#define DHT_PIN   3   //  GRN_DARK  (Ref Above)
#define LED2_PIN  4   //  RED
#define BUZZ_PIN  5   //
//  PIR Triangle
#define PIR1_PIN  6   //  000-120 deg
#define PIR2_PIN  7   //  120-240 deg
#define PIR3_PIN  8   //  240-359 deg
//  RF
#define RF_RST    9   //  BLU
#define RF_CS     10  //  GRN
#define RF_MOSI   11  //  YEL
#define RF_MISO   12  //  ORA
#define RF_SCK    13  //  RED

#define RFM69_IRQN    0  // Pin 2 is IRQ 0!
#define RFM69_IRQ     2
#define RFM69_RST     9
#define RFM69_CS      10

int16_t packetnum = 0;  // packet counter, we increment per xmission
RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);

//  RFM END

//  ANALOG PINS
#define LED1_PIN  A3  //  GRN   Tiny Green
#define RTC_SDA   A4  //  PUR
#define RTC_SCL   A5  //  GRY

long loopCnt;
int pir1_State = LOW;
int pir1_Val = 0;
int pir2_State = LOW;
int pir2_Val = 0;
int pir3_State = LOW;
int pir3_Val = 0;

String rowText = "";

void setup() {
  
  //  Configure HW Reset
  //digitalWrite(RESET_PIN, HIGH); //We need to set it HIGH immediately on boot
  //pinMode(RESET_PIN,OUTPUT);     //We can declare it an output ONLY AFTER it's HIGH

//while (!Serial); // wait until serial console is open, remove if not tethered to computer
  Serial.begin(SERIAL_BAUD);

  //  DHT22 Sensor T/H
  Serial.println("DHT22 AM2302 Test!");
  dht.begin();

  //  RTC DS3231
  Serial.println("RTC DS3231 Test!");
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }

  //  RFM69HCW
  Serial.println("RFM69HCW TX Radio Test!");
  // Hard Reset the RFM module
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);
  // Initialize radio
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  if (IS_RFM69HCW) {
    radio.setHighPower();    // Only for RFM69HCW & HW!
  }
  radio.setPowerLevel(31); // power output ranges from 0 (5dBm) to 31 (20dBm)
  radio.encrypt(ENCRYPTKEY);
  //  Output some test values
  Serial.print("rfFreq=");
  Serial.print(FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(" MHz");

  //  Read device temp C
  byte tempC = radio.readTemperature(-1);
  Serial.print("RFTempC=");  Serial.println(tempC);
  
  Serial.println("PIR SentryStick Test!");

  //Serial.println("Setting RTC Time...");
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //delay(1000);
  //rtc.adjust(DateTime(2016, 8, 10, 16, 14, 0));
  //delay(1000);
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // Update RTC with DateTime the sketch was last compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Update RTC with finite DateTime
    //rtc.adjust(DateTime(2016, 8, 10, 15, 43, 0));
  }

  //  PIR Sensors
  //pinMode(PIR1_PIN, INPUT); //  000-120
  //pinMode(PIR2_PIN, INPUT); //  120-240
  //pinMode(PIR3_PIN, INPUT); //  240-360
  //  LED's
  pinMode(LED1_PIN, OUTPUT);  //  Power Green
  //pinMode(LED2_PIN, OUTPUT);  //  Error Red

}


void loop() {
  // Wait a few seconds between measurements.
  delay(5000);
  loopCnt++;

  //  Loop Index/Count
  rowText = "IDX=";
  rowText += loopCnt;
  
  // Reading temperature or humidity takes about 250-2000 ms!
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  //float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    tone(BUZZ_PIN, 5000, 2000);
    return;
  }

  //  RTC D/T
  //  Read current time
  DateTime now = rtc.now();
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  //float hic = dht.computeHeaatIndex(t, h, false); 
  rowText += ",TIM="; 
  rowText += now.month();
  rowText += '/';
  rowText += now.day();
  rowText += '/';
  rowText += now.year();
  rowText += ' ';
  //Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  
  rowText += now.hour();
  rowText += ':';
  rowText += now.minute();
  rowText += ':';
  rowText += now.second();

  //  RH
  rowText += ",RH=";
  rowText += h;
  rowText += ",TempF=";
  rowText += f;
  rowText += ",HeatF=";
  rowText += hif;
  
//  //  PIR 000-120 deg
//  //pir1_Val = digitalRead(PIR1_PIN);  // read input value
//  if (pir1_Val == HIGH) {            // check if the input is HIGH
//    //if (pir1_State == LOW) {
//    // we have just turned on
//    Serial.print(",PIR1=1");
//    // We only want to print on the output change, not state
//    pir1_State = HIGH;
//    //}
//  } else {
//    //if (pir1_State == HIGH) {
//    // we have just turned of
//    Serial.print(",PIR1=0");
//    // We only want to print on the output change, not state
//    pir1_State = LOW;
//    //}
//  }
//  
//  //  PIR 120-240 deg
//  //pir2_Val = digitalRead(PIR2_PIN);  // read input value
//  if (pir2_Val == HIGH) {            // check if the input is HIGH
//    //if (pir2_State == LOW) {
//    // we have just turned on
//    Serial.print(",PIR2=1");
//    // We only want to print on the output change, not state
//    pir2_State = HIGH;
//    //}
//  } else {
//    //if (pir2_State == HIGH) {
//    // we have just turned of
//    Serial.print(",PIR2=0");
//    // We only want to print on the output change, not state
//    pir2_State = LOW;
//    //}
//  }
//  
//  //  PIR 240-360 deg
//  //pir3_Val = digitalRead(PIR3_PIN);  // read input value
//  if (pir3_Val == HIGH) {            // check if the input is HIGH
//    //if (pir3_State == LOW) {
//    // we have just turned on
//    Serial.print(",PIR3=1");
//    // We only want to print on the output change, not state
//    pir3_State = HIGH;
//    //}
//  } else {
//    //if (pir3_State == HIGH) {
//    // we have just turned of
//    Serial.print(",PIR3=0");
//    // We only want to print on the output change, not state
//    pir3_State = LOW;
//    //}
//  }


  //  Read device temp C
  byte tempC = radio.readTemperature(-1);
  rowText += ",RFTempC=";  
  rowText += tempC;
    
  //  # # # Cap this row off
  Serial.println(rowText);
  int charLen;
  charLen = rowText.length();
  Serial.println(charLen);
  
  //  TX Data Packets     12345678901234567890
  char radiopacket[150];
  rowText.toCharArray(radiopacket,charLen);
  
  //itoa(packetnum++, radiopacket+13, 10);
  Serial.print("Sending '"); Serial.print(radiopacket); Serial.println("'");
    
  if (radio.sendWithRetry(RECEIVER, radiopacket, strlen(radiopacket))) { //target node Id, message as string or byte array, message length
    Serial.println("OK");
    Blink(LED1_PIN, 50, 3); //blink LED 3 times, 50ms between blinks
  }

  radio.receiveDone(); //put radio in RX mode
  Serial.flush(); //make sure all serial data is clocked out before sleeping the MCU
  
  //  Flash and Beep for the 1st 3 poll events after startup
  if (loopCnt < 4) {
    digitalWrite(LED1_PIN, HIGH);
    //digitalWrite(LED2_PIN, HIGH);
    tone(BUZZ_PIN, 5000, 1000);
    delay(1500);
    digitalWrite(LED1_PIN, LOW);
    //digitalWrite(LED2_PIN, LOW);
    delay(500);
  } else {
    //  Every 30 seconds flash LED_1
    if (loopCnt % 6 == 0) {
      digitalWrite(LED1_PIN, HIGH);
      delay(250);
      digitalWrite(LED1_PIN, LOW);
    } else {
      if (loopCnt > 440) {
        // Clear the reset bit
        Serial.println("");
        asm volatile ("  jmp 0");
      }
    }

  }

}

void Blink(byte PIN, byte DELAY_MS, byte loops)
{
  for (byte i=0; i<loops; i++)
  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);
  }
}
