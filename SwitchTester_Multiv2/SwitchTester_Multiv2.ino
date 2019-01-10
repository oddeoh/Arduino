//  Switch Testing Circuit w/ Debounce
//  Set the Digital Input Pin to use and the debounce/settling time in milliseconds
//  Includes support for lighted switches (disables the LED during press for visual confirmation)

//  CONFIG SETTINGS
#define DEBOUNCE 50

typedef struct
{
  byte          pin;        //  Pin to read for Switch
  byte          ledPin;     //  Pin for LED toggle
  unsigned long debounce;   //  Number of milli seconds to keep the Pushed State HIGH
  unsigned int  readVal;
  boolean       wasPushed;  //  Flag tracking ON or OFF state
  unsigned long lastPush;   //  Record the time that we measured a shock
  unsigned long pushSpan;   //  How long the button was pressed
} Button;

Button buttons[1];


void setup ()
{
  Serial.begin(115200);
  Serial.println("Fluxable(tm) Arduino Core MULTI Switch Checker w/ Debounce");
//  S1  7
//  L1  11
//  S2  15
//  L2  27
  
  buttons[0].pin = 11;
  buttons[0].ledPin = 7;
  buttons[0].debounce = DEBOUNCE;
  buttons[0].wasPushed = false;
  buttons[0].lastPush = 0;
  buttons[0].pushSpan = 0;
  buttons[0].readVal = LOW;
  
  buttons[1].pin = 15;
  buttons[1].ledPin = 27;
  buttons[1].debounce = DEBOUNCE;
  buttons[1].wasPushed = false;
  buttons[1].lastPush = 0;
  buttons[1].pushSpan = 0;
  buttons[1].readVal = LOW;

  pinMode (buttons[0].pin, INPUT);
  pinMode (buttons[1].pin, INPUT);

  pinMode (buttons[0].ledPin, OUTPUT);
  pinMode (buttons[1].ledPin, OUTPUT);
  
  digitalWrite(buttons[0].ledPin, HIGH);
  digitalWrite(buttons[1].ledPin, HIGH);
}

void loop ()
{


}
