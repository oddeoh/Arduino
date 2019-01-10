
// the setup function runs once when you press reset or power the board
void setup() {
  
  #define PIN_LED00 7   //Builtin LED
  
  #define PIN_LED01 10   //RED     (Back) 
  #define PIN_LED02 6   //GRN     TOP
  #define PIN_LED03 0   //REDx2   T-Right / B-Left
  #define PIN_LED04 3   //REDx2   T-Left  / B-Right
  

  //byte channelVal[MAX_CHANNELS] = {0};
  //boolean array[9] = {FALSE};
   
  
  //  PWM 
  //  D3  (-PWM)  SCL
  //  D6  D6
  //  D9  D9
  //  D10 D10 

  //  DIG
  //  1 0   D0  RX    RED   Back (PWM)
  //  2 1   D1  TX    RED   Front (DIG)
  //  3 2   D2  SDA   RED
  //  4 3   D3  SCL   GRN
  //  5 6   D6        GRN
  //  6 9   D9        GRN
  //  7 10  D10       RED
  //  8 12  D12       RED
      
  #define DIM_MS 30           //  Min delay to create the dim effect
  #define MAX_STEPS     255   //  PWM Resolution Max 
  #define MAX_LEVELS    100   //  Total level options supported    

  //  Assumes 92 BPM average of Adante speed songs for Christmas, 4/4 sig, single quarter note
  #define beatLengthMS  652   //  Total duration of a quarter note to quarter note at the BPM within this PULSE
  #define beatHoldMS    500   //  duartion of the quarter note pulse at BPM before start of next PULSE
  #define beatPauseMS   300   //  duration of the pause before start of next beat PULSE
  
  //  Built-In LED Helper
  pinMode(PIN_LED00, OUTPUT);   //  R1    Built-In (per Slice)
  //
  pinMode(PIN_LED01, OUTPUT);   //  G1
  pinMode(PIN_LED02, OUTPUT);   //  R2x2
  pinMode(PIN_LED03, OUTPUT);   //  R3x2
  pinMode(PIN_LED04, OUTPUT);   //  G2
  
//  pinMode(PIN_LED05, OUTPUT);   /// G3
//  pinMode(PIN_LED06, OUTPUT);   //  G4
//  pinMode(PIN_LED07, OUTPUT);   //  G5
 
}

//   1 2 3 4 5    BACK
//1      R1

//   1 2 3 4 5
//1      G1
//2    R2  R3
//3  G4  G5  G6  
//4     R3  R2 
//5       G7

// the loop function runs over and over again forever
void loop() {
  
  //  SEQUENCE 
  togglePin(PIN_LED01, beatHoldMS, beatPauseMS);
  togglePin(PIN_LED02, beatHoldMS, beatPauseMS);
  togglePin(PIN_LED03, beatHoldMS, beatPauseMS);
  togglePin(PIN_LED04, beatHoldMS, beatPauseMS);
 
//  togglePin(PIN_LED05, beatHoldMS, beatPauseMS);
//  togglePin(PIN_LED06, beatHoldMS, beatPauseMS);
//  togglePin(PIN_LED07, beatHoldMS, beatPauseMS);
  
  /* togglePin_PWM(PIN_LED01, beatHoldMS, beatPauseMS, 10);
  togglePin_PWM(PIN_LED02, beatHoldMS, beatPauseMS, 10);

  togglePin_PWM(PIN_LED01, beatHoldMS, beatPauseMS, 5);
  togglePin_PWM(PIN_LED02, beatHoldMS, beatPauseMS, 5);

  togglePin_PWM(PIN_LED01, beatHoldMS, beatPauseMS, 2);
  togglePin_PWM(PIN_LED02, beatHoldMS, beatPauseMS, 2);
  */
  
}



void togglePin(long pinNum, long holdMS, long pauseMS) {

  digitalWrite(pinNum, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(PIN_LED00, HIGH);
  delay(holdMS);                // wait for a second
  
  digitalWrite(pinNum, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(PIN_LED00, LOW);
  delay(pauseMS);              // wait for a bit
    
}

void togglePin_PWM(long pinNum, long holdMS, long pauseMS, long level ) {  
  
  //  Convert the level to the PWM range (0-255) for output level
  byte pwmVal = map(level, 1, MAX_LEVELS, 0, MAX_STEPS);

  if (level == MAX_LEVELS) { pwmVal = MAX_STEPS; }
  if (level == 0 ) { pwmVal = 0; }

  //  Calculate the hold time
  analogWrite(pinNum, pwmVal);
  delay(holdMS);
  //  Do the off time
  analogWrite(pinNum, LOW);
  delay(pauseMS);
  

 /*
  // fade in from min to max in increments of 5 points:
  fadeSteps = holdMS \ MAX_STEPS;
  //  Fade UP
  for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += fadeSteps) {
    // sets the value (range from 0 to 255):
    analogWrite(pinNum, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(DIM_MS);
  }

  //  Fade DOWN
  for (int fadeValue = 255 ; fadeValue >= 0; fadeValue += fadeSteps) {
    // sets the value (range from 0 to 255):
    analogWrite(pinNum, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(DIM_MS);
  }
*/
 
  
}



