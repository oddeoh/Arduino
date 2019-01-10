//  Switch Testing Circuit w/ Debounce
//  Set the Digital Input Pin to use and the debounce/settling time in milliseconds
//  Includes support for lighted switches (disables the LED during press for visual confirmation)

//  CONFIG SETTINGS  
//  S1 11, L1 7
//  S2 15, L2 27
int debounceTime = 50;      // Number of milli seconds to keep the Pushed State HIGH

int SW1_PIN = 11;           // Use Digital Pin for Input
int LED1_PIN = 7;

int SW2_PIN = 15;           // Use Digital Pin for Input
int LED2_PIN = 27;

int SW1_VAL = LOW;          //  Holds the pin reading (default HIGH)
int SW2_VAL = LOW;          //  Holds the pin reading (default HIGH)

boolean SW1_wasPushed = false;  //  Flag tracking ON or OFF state
unsigned long SW1_lastPush; //  Record the time that we measured a shock

boolean SW2_wasPushed = false;  //  Flag tracking ON or OFF state
unsigned long SW2_lastPush; //  Record the time that we measured a shock

void setup ()
{
  Serial.begin(115200);  
  
  Serial.println("Fluxable(tm) Arduino Core Switch Checker w/ Debounce");
  pinMode (SW1_PIN, INPUT);   
  pinMode (LED1_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);

  pinMode (SW2_PIN, INPUT);   
  pinMode (LED2_PIN, OUTPUT);
  digitalWrite(LED2_PIN, LOW);
    
}

void loop ()
{ 
  //  SWITCH DETECT Read the Switch Value and set the [SW1_wasPushed] flag
  SW1_VAL = digitalRead (SW1_PIN);
  
  //  Test the state and respond LOW=Closed switch, HIGH=Open Switch  
  if (SW1_VAL == HIGH)
  {
    //  Switch pushed/switched ON, record the time
    SW1_lastPush = millis(); 
    // Delays the amount of debounce/settling time
    if (!SW1_wasPushed){
      SW1_wasPushed = true;
      Serial.println("SWITCH1_ON");
      digitalWrite(LED1_PIN, HIGH);
    }
  }
  else
  {
    //  If the time of the settling period has expired, reset the state, to detect again
    if(SW1_wasPushed) {
      if((millis() - SW1_lastPush) > debounceTime){
        Serial.print("SWITCH1_OFF"); 
        digitalWrite(LED1_PIN, LOW);
        SW1_wasPushed = false;
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
    if (!SW2_wasPushed){
      Serial.println("SWITCH2_ON");
      SW2_wasPushed = true;
      digitalWrite(LED2_PIN, HIGH);
    }
  }
  else
  {
    //  If the time of the settling period has expired, reset the state, to detect again
    if(SW2_wasPushed) {
      if((millis() - SW2_lastPush) > debounceTime){
        Serial.print("SWITCH2_OFF"); 
        digitalWrite(LED2_PIN, LOW);
        SW2_wasPushed = false;
      }
    }
  }


  
}
