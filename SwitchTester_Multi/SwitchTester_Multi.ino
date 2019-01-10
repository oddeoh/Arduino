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

  //  SWITCH DETECT Read the Switch Value and set the [wasPushed] flag
  //  Loop through the array of buttons to monitor
  for (int switchPin = 0; switchPin < 2; switchPin++) {
    //  Read this pin's value
    buttons[switchPin].readVal = digitalRead(buttons[switchPin].pin);

    //  Test the state and respond LOW=Closed switch, HIGH=Open Switch
    if (buttons[switchPin].readVal == HIGH) {
      //  Switch pushed/switched ON, record the time
      buttons[switchPin].lastPush = millis();
      // Delays the amount of debounce/settling time
      if (!buttons[switchPin].wasPushed) {
        Serial.print("SWITCH_ON = "); Serial.println(buttons[switchPin].pin);
        buttons[switchPin].wasPushed = true;
        // Disable the LED to flash when pushed
        digitalWrite(buttons[switchPin].ledPin, HIGH);
      }
    }
    else
    {
      //  If the time of the settling period has expired, reset the state, to detect again
      buttons[switchPin].pushSpan = millis() - buttons[switchPin].lastPush;
      if (buttons[switchPin].wasPushed) {
        if (buttons[switchPin].pushSpan > buttons[switchPin].debounce) {
          Serial.print("SWITCH_OFF = "); Serial.println(buttons[switchPin].pin);
          digitalWrite(buttons[switchPin].ledPin, LOW);
          buttons[switchPin].wasPushed = false;
        }
      }
    }
  }
  delay(DEBOUNCE);

}
