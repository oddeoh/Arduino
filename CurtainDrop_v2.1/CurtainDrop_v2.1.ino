//  CurtainDrop v2.1
//  LM: 05/10/18 BRB 
//  Copyright (C) 2018 Fluxable LLC

//  Feather nRF52 BLE CPU
//  Feather Wing Relay
//  Green Latching Button
//  Red Momentary Button
//  Powers on with GRN LED flashing
//  Press/Latch GRN Button, RED LED Flashes, Relay is ON
//  Press RED button, Relay is OFF, RED LED is Off
//  Restart with GRN back to latch ... (Loop)

// Software Timer for blinking RED LED
SoftwareTimer blinkGRN;
SoftwareTimer blinkRED;

//  CONFIG SETTINGS
//  S1 11, L1 7
//  S2 15, L2 27
//  R1 4 (soldered on feather wing)
//  S3 16  F SW
//

bool isArmed = false;       //  TOGGLE THE ARMED MODE
int debounceTime = 50;      // Number of milli seconds to keep the Pushed State HIGH

int R1_PIN = 4;             //  MAG RELAY

int SW1_PIN = 11;           // Use Digital Pin for Input
int LED1_PIN = 7;
int SW2_PIN = 15;           // Use Digital Pin for Input
int LED2_PIN = 27;
int SW3_PIN = 16;           //  FOOT SWITCH


int SW1_VAL = LOW;          //  Holds the pin reading (default HIGH)
int SW2_VAL = LOW;          //  Holds the pin reading (default HIGH)
int SW3_VAL = LOW;          //  FOOT SWITCH

bool SW1_wasPushed = false;
bool SW2_wasPushed = false;
bool SW3_wasPushed = false;
unsigned long SW1_lastPush;
unsigned long SW2_lastPush;
unsigned long SW3_lastPush;

//  RF KEY FOB
int signalPin[4] = { 2, 3, 5, 28 };
int signalState = 0;
int lastSignalState[] = { 0, 0, 0, 0 };

void setup ()
{
  Serial.begin(115200);
  Serial.println("Fluxable(tm) 'Curtain Drop' (nRF52 Feather CPU w/ Feather Wing Relay");

  pinMode (SW1_PIN, INPUT);
  pinMode (LED1_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);

  pinMode (SW2_PIN, INPUT);
  pinMode (LED2_PIN, OUTPUT);
  digitalWrite(LED2_PIN, LOW);

  pinMode (SW3_PIN, INPUT);
  digitalWrite(SW3_PIN, LOW);

  pinMode (R1_PIN, OUTPUT);
  digitalWrite(R1_PIN, LOW);

  // Setting the RF FOB signal pins as inputs
  for (int i = 0; i < 4; i++) {
    pinMode(signalPin[i], INPUT);
  }

  // Initialize blinkTimer for 1000 ms and start it
  blinkGRN.begin(1000, blinkGRN_callback);
  blinkGRN.start();

  // Initialize blinkTimer for 1000 ms and start it
  blinkRED.begin(500, blinkRED_callback);

}

void loop ()
{
  //  SWITCH DETECT Read the Switch Value and set the [SW1_wasPushed] flag
  SW1_VAL = digitalRead (SW1_PIN);
  if (SW1_VAL == HIGH)
  {
    //  Switch pushed/switched ON, record the time
    SW1_lastPush = millis();
    // Delays the amount of debounce/settling time
    if (!SW1_wasPushed) {
      SW1_wasPushed = true;
      isArmed = true;
      Serial.println("ARMED!!!");
      Serial.println("SWITCH1_ON");
      //  TODO BLINK OFF (GREEN)
      blinkGRN.stop();
      digitalWrite(LED1_PIN, HIGH); //  GRN LED ON
      blinkRED.start();
      digitalWrite(R1_PIN, HIGH);   //  RLY     ON
    }
  }
  else
  {
    //  If the time of the settling period has expired, reset the state, to detect again
    if (SW1_wasPushed) {
      if ((millis() - SW1_lastPush) > debounceTime) {
        Serial.print("SWITCH1_OFF");
        digitalWrite(LED1_PIN, LOW);  //  GREEN OFF
        blinkRED.stop();
        digitalWrite(LED2_PIN, LOW);  //  RED   OFF
        digitalWrite(R1_PIN, LOW);    //  RELAY OFF
        SW1_wasPushed = false;
        isArmed = false;
        Serial.println("Un-armed!!!");
        //  Recycle back to starting mode
        blinkGRN.start();
      }
    }
  }

  //  SWITCH DETECT Read the Switch Value and set the [SW1_wasPushed] flag
  SW2_VAL = digitalRead (SW2_PIN);
  if (SW2_VAL == HIGH)
  {
    //  Switch pushed/switched ON, record the time
    SW2_lastPush = millis();
    // Delays the amount of debounce/settling time
    if (!SW2_wasPushed) {
      Serial.println("SWITCH2_ON");
      SW2_wasPushed = true;
      isArmed = false;
      Serial.println("Un-armed!!!");
      digitalWrite(R1_PIN, LOW);    //  RELAY OFF
      digitalWrite(LED2_PIN, LOW);  //  LED RED OFF
      blinkRED.stop();
      blinkGRN.stop();
    }
  }
  else
  {
    //  If the time of the settling period has expired, reset the state, to detect again
    if (SW2_wasPushed) {
      if ((millis() - SW2_lastPush) > debounceTime) {
        Serial.print("SWITCH2_OFF");
        digitalWrite(LED2_PIN, LOW);  //  RED OFF
        digitalWrite(R1_PIN, LOW);    //  RELAY OFF
        blinkRED.stop();
        blinkGRN.stop();
        SW2_wasPushed = false;
        isArmed = false;
        Serial.println("Un-armed!!!");
      }
    }
  }

  //  Only check RF FOB and Foot Switch if this is ARMED
  if (isArmed == true) {

    //  For each of the 4 pins we loop thru and check the state.
    for (int i = 0; i < 4; i++) {
      // Read the current pin
      signalState = digitalRead(signalPin[i]);
      if (signalState == HIGH) {
        Serial.println("RF KEY ON");
        isArmed = false;
        Serial.println("Un-armed!!!");
        digitalWrite(R1_PIN, LOW);    //  RELAY OFF
        digitalWrite(LED2_PIN, LOW);  //  LED RED OFF
        blinkRED.stop();
        blinkGRN.stop();
      }
    }

    //  SWITCH DETECT Read the Switch Value and set the [SW1_wasPushed] flag
    SW3_VAL = digitalRead (SW3_PIN);
    if (SW3_VAL == HIGH)
    {
      //  Switch pushed/switched ON, record the time
      SW3_lastPush = millis();
      // Delays the amount of debounce/settling time
      if (!SW3_wasPushed) {
        Serial.println("SWITCH3_ON");
        SW3_wasPushed = true;
        isArmed = false;
        Serial.println("Un-armed!!!");
        digitalWrite(R1_PIN, LOW);    //  RELAY OFF
        digitalWrite(LED2_PIN, LOW);  //  LED RED OFF
        blinkRED.stop();
        blinkGRN.stop();
      }
    }
    else
    {
      //  If the time of the settling period has expired, reset the state, to detect again
      if (SW3_wasPushed) {
        if ((millis() - SW3_lastPush) > debounceTime) {
          Serial.print("SWITCH3_OFF");
          digitalWrite(LED2_PIN, LOW);  //  RED OFF
          digitalWrite(R1_PIN, LOW);    //  RELAY OFF
          blinkRED.stop();
          blinkGRN.stop();
          SW2_wasPushed = false;
          isArmed = false;
          Serial.println("Un-armed!!!");
        }
      }
    }

  }

}

//  GRN 1000ms blinker
void blinkGRN_callback(TimerHandle_t xTimerID)
{
  (void) xTimerID;
  digitalToggle(LED1_PIN);
}

//  GRN 1000ms blinker
void blinkRED_callback(TimerHandle_t xTimerID)
{
  (void) xTimerID;
  digitalToggle(LED2_PIN);
}

