/*
 Serial Event and BTN PIN ISR (Int Svc Routine) 
 When new serial data arrives, this sketch adds it to a String.
 When a newline is received, the loop prints the string and clears it.
*/

//  MOTORS
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61); 

// Select which 'port' M1, M2, M3 or M4. In this case, M1
Adafruit_DCMotor *myMotor1 = AFMS.getMotor(1);
Adafruit_DCMotor *myMotor2 = AFMS.getMotor(2);
Adafruit_DCMotor *myMotor3 = AFMS.getMotor(3);
Adafruit_DCMotor *myMotor4 = AFMS.getMotor(4);
//  END MOTORS

String comInpString = "";    // Incoming COM Data
volatile boolean comInpDone = false;  // Incoming COM Data complete

//  LED's
int LED_RED_PIN = 12;  //  1.9v  150
int LED_ORA_PIN = 11;  //  2.1v  100
int LED_YEL_PIN = 10;  //  2.1v  100
int LED_BLU_PIN = 9;   //  2.7v  100
int LED_GRN_PIN = 8;   //  3.8v  51
//  LED's on Buttons
int LED_GRNBTN_PIN = 7;   //  SERVE
int LED_REDBTN_PIN =6;    //  HALT

//  BUTTON's
int BTN_RED_PIN = 2;    //  OUTPUT ISR  Int0
int BTN_GRN_PIN = 3;    //  OUTPUT ISR  Int1
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


void setup ()
{
  Serial.begin(38400);
  Serial.println("RoboPour(tm) ButtonUX!");

  // Reserve 256 bytes
  comInpString.reserve(256);

  //  Initialize PIN's  
  pinMode (LED_RED_PIN, OUTPUT);
  pinMode (LED_ORA_PIN, OUTPUT);
  pinMode (LED_YEL_PIN, OUTPUT);
  pinMode (LED_BLU_PIN, OUTPUT);
  pinMode (LED_GRN_PIN, OUTPUT);  
  pinMode (LED_GRNBTN_PIN, OUTPUT);
  pinMode (LED_REDBTN_PIN, OUTPUT);
  //
  pinMode (BTN_RED_PIN, INPUT_PULLUP);
  pinMode (BTN_GRN_PIN, INPUT_PULLUP);

  //  "POWER" LED (ON)
  TurnOff_LEDs();
  //
  Power_LED_On();  
  GreenButton_LED_On();
  RedButton_LED_On();
  
  //  ISR for HALT and SERVE button presses
  attachInterrupt(0, isr_BTN_RED_Toggle, CHANGE);        
  attachInterrupt(1, isr_BTN_GRN_Toggle, CHANGE);  

  AFMS.begin();  // create with the default frequency 1.6KHz
  Init_Motors();  
}

//  RESET function declared at address 0
void(* resetFunc) (void) = 0;

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
  asm volatile ("  jmp 0");  
}

void loop() {
    
  //	DRINK RECIPE QUEUED?
  if (comInpDone) {
    //  Drink Recipe is Queued, turn on the ORA LED for the User
    Queued_LED_On();
    
    //  Match the LED state from the "Serve" BTN ISR
    if (BTN_GRN_Last != BTN_GRN_STATE) {
      if (BTN_GRN_STATE == HIGH) { 
        
        //  Clear current command we are processing
        comInpDone = false;
        comInpString = "";
        
        //  Set up the UX to show the "Stream" is active

	//	Serve Drink (Simulation)
        ServeDrink();
        delay(500);
        //
      }
      //  Record the new state
      BTN_GRN_Last = BTN_GRN_STATE;
    }
  } else {
    Queued_LED_Off();
  }
  
  //  Match the LED state from the "Halt" BTN ISR
  //if (BTN_RED_Last != BTN_RED_STATE) {
    if (BTN_RED_STATE == HIGH) { 
      //noInterrupts();
      HaltEvent();
      
    } else {
      Error_LED_Off();
    }
    //BTN_RED_Last = BTN_RED_STATE;
  //}
}void ServeDrink() {
  
  Serial.println("Serving Drink...");

  //  Clear the BLU LED and re-enable ISR's
  Queued_LED_Off();
  Stream_LED_On();

  myMotor1->setSpeed(255);
  myMotor1->run(FORWARD);
  //delay(45000);
  //myMotor1->run(RELEASE);
  
  myMotor2->setSpeed(255);
  myMotor2->run(FORWARD);
  //delay(90000);
  //myMotor2->run(RELEASE);
    
  myMotor3->setSpeed(255);
  myMotor3->run(FORWARD);
  //
  myMotor4->setSpeed(255);
  myMotor4->run(FORWARD);
  
  //Flash BLU light while streaming
  //delay(18000);  // 2cl = 0.68 oz
  //delay(2300);  // 2cl = 0.68 oz
  //  Prime Hoses (4600 ms)
  //  Clean Hoses (30000)
  delay(25000);
  Serial.println("Delay completed");
    
  Disable_Motors();
  Stream_LED_Off();
  Serial.println("Done");
}




void HaltEvent() {
    Disable_Motors();
    //Disable_ISRs();      
    //  Turn Off LED's
    Power_LED_Off();
    Data_LED_Off();
    Queued_LED_Off();
    Stream_LED_Off();
    GreenButton_LED_Off();
    RedButton_LED_Off();
    //
    Error_LED_On();     
    delay(3000);
    //resetFunc();
    software_Reset();
    //  SYSTEM RESETS!!!
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX.  
  Runs between each loop() run, using a delay inside loop() can delay
  response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  
  while (Serial.available()) {
    // get the new byte:
    char inpChar = (char)Serial.read(); 
    // add it to the inputString:
    comInpString += inpChar;   
		
    //  Toggle the "Data Activity" LED TX/RX (YEL)
    if(LED_YEL_STATE == HIGH){
      LED_YEL_STATE = LOW;
    } else {
      LED_YEL_STATE = HIGH;
    }
    digitalWrite(LED_YEL_PIN, LED_YEL_STATE);
    delay(100);
	
    // if the incoming character is a CR (\r) or a LF (\n) set a flag so the main loop can process it
    if (inpChar == '\r') {
      comInpDone = true;
      Serial.println("Your characters were " + comInpString);
      Data_LED_Off();
    } 
  }
    
}


//  ISR Toggle global button state on press
void isr_BTN_RED_Toggle() {
  //if((long)(millis() - BTN_RED_BounceTime) >= debounceTime) {
    if (BTN_RED_STATE == LOW) {
      BTN_RED_STATE = HIGH;
      HaltEvent();
    } else {
      BTN_RED_STATE = LOW;
    }
    //BTN_RED_BounceTime = millis();   
  //}
}

//  ISR Toggle global button press state
void isr_BTN_GRN_Toggle() {
  if((long)(millis() - BTN_GRN_BounceTime) >= debounceTime) {
    if (BTN_GRN_STATE == LOW) {
      BTN_GRN_STATE = HIGH;
    } else {
      BTN_GRN_STATE = LOW;
    }
    BTN_GRN_BounceTime = millis();   
  }
}

//  Sets them to max speed and ready for use
void Init_Motors() {
  myMotor1
  myMotor1->setSpeed(255);
  myMotor1->run(FORWARD);
  myMotor1->run(RELEASE);
  //
  myMotor2->setSpeed(255);
  myMotor2->run(FORWARD);
  myMotor2->run(RELEASE);
  //
  myMotor3->setSpeed(255);
  myMotor3->run(FORWARD);
  myMotor3->run(RELEASE);
  //
  myMotor4->setSpeed(255);
  myMotor4->run(FORWARD);
  myMotor4->run(RELEASE);  
}


void Disable_Motors() {
  myMotor1->run(RELEASE);
  myMotor2->run(RELEASE);
  myMotor3->run(RELEASE);
  myMotor4->run(RELEASE); 
}


//  TURN OFF All LED's
void TurnOff_LEDs() {
  Power_LED_Off();   //  GRN
  Queued_LED_Off();  //  ORA
  Error_LED_Off();   //  RED
  Stream_LED_Off();  //  BLU
  Data_LED_Off();    //  YEL 
  GreenButton_LED_Off();   //  GRN Button
  RedButton_LED_Off();    //  RED Button
}

//  TURN ON All LED's
void TurnOn_LEDs() {
  Power_LED_On();   	//  GRN
  Queued_LED_On();  	//  ORA
  Error_LED_On();   	//  RED
  Stream_LED_On();  	//  BLU
  Data_LED_On();    	//  YEL  
  GreenButton_LED_On();	//	GRN BTN
  RedButton_LED_On();	//	RED BTN
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

