/*
   FourButtSound
   Reads transmitted RO2A Momentary RF Buttons to pins A0-A03 and plays associated .WAV file for that button press
   Pressing another button interrupts current file playing with the new one.
   A=0, B=1, C=2, D=3
   
   Startup Sound  0
   ButtonA        1
   ButtonB        2
   ButtonC        4
   ButtonD        8
   ButtonA/B      3
   ButtonC/D     12
   
   * Button Presses play sounds, interrupts last sound immediately.
   * Create Sub-folder "A_S" with sounds for a button to play sequentially on each press (loops through in fixed order).
   * Create Sub-folder "A_R" with sounds for a button to play randomly on each press, but will not repeat a sound in the last n-2 plays (>3 sounds, plays.
   * The "Sequential" Sounds folder (A_S) overrides what is in the "Random" (A_R) folder.
   * 
   * TODO - Log (ms) for presses, triggers and plays to SD Card
   * TODO - Config file on startup with 
   *  A_S over A_R, or A_R over A_S
   *  Set minimum play time for button overrides (ms)
   *  Set minimum silence time between sounds (ms)
   *  Set debounce time in (ms) to accept presses
   *  Set Enforce Non-Repeat count (n-2, n-3, etc.)
   *  Set (ms) to trigger IR signals to play sounds
   *  Map Button letter to IR signals (A_S, A_R, NonRepeat)
   
*/

#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"

SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the filesystem on the card
FatReader f;      // This holds the information for the file we're play

WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

#define DEBOUNCE 5  // button debouncer

// here is where we define the buttons that we'll use. button "1" is the first, button "6" is the 6th, etc
byte buttons[] = {6, 7, 8, 9};

// This handy macro lets us determine how big the array up above is, by checking the size
#define NUMBUTTONS sizeof(buttons)

// we will track if a button is just pressed, just released, or 'pressed' (the current state
volatile byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];

void setup() {

  byte i;

  // set up serial port
  Serial.begin(9600);
  putstring_nl("WaveHC with ");
  Serial.print(NUMBUTTONS, DEC);
  putstring_nl("buttons");

  putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!

  // Set the output pins for the DAC control. This pins are defined in the library
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);

  // pin13 LED
  //pinMode(13, OUTPUT);

  if (!card.init()) {         //play with 8 MHz spi (default faster!)
    putstring_nl("Card init. failed!");  // Something went wrong, lets print out why
    sdErrorCheck();
    while (1);                           // then 'halt' - do nothing!
  }

  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);

  // Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {     // we have up to 5 slots to look in
    if (vol.init(card, part))
      break;                             // we found one, lets bail
  }
  if (part == 5) {                       // if we ended up not finding one  :(
    putstring_nl("No valid FAT partition!");
    sdErrorCheck();      // Something went wrong, lets print out why
    while (1);                           // then 'halt' - do nothing!
  }

  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(), DEC);    // FAT16 or FAT32?

  // Try to open the root directory
  if (!root.openRoot(vol)) {
    putstring_nl("Can't open root dir!"); // Something went wrong,
    while (1);                            // then 'halt' - do nothing!
  }

  // Whew! We got past the tough parts.
  putstring_nl("Ready!");

  TCCR2A = 0;
  TCCR2B = 1 << CS22 | 1 << CS21 | 1 << CS20;

  //Timer2 Overflow Interrupt Enable
  TIMSK2 |= 1 << TOIE2;
}

SIGNAL(TIMER2_OVF_vect) {
  check_switches();
}

void check_switches()
{

  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];
  byte index;

  for (index = 0; index < NUMBUTTONS; index++) {
    currentstate[index] = digitalRead(buttons[index]);   // read the button

    //Serial.print(index, DEC);
    //Serial.print(": cstate=");
    //Serial.print(currentstate[index], DEC);
    //Serial.print(", pstate=");
    //Serial.print(previousstate[index], DEC);
    //Serial.print(", press=");

    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
        // just pressed
        justpressed[index] = 1;
      }
      else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
        // just released
        justreleased[index] = 1;
      }
      pressed[index] = !currentstate[index];  // remember, digital HIGH means NOT pressed
    }
    //Serial.println(pressed[index], DEC);
    previousstate[index] = currentstate[index];   // keep a running tally of the buttons
  }
  justpressed[0] = LOW;
}


void loop() {
  byte i;

  if (justpressed[0]) {
    justpressed[0] = 0;
    playfile("TRK_1.WAV");
  }
  else if (justpressed[1]) {
    justpressed[1] = 0;
    playfile("TRK_2.WAV");
  }
  else if (justpressed[2]) {
    justpressed[2] = 0;
    playfile("TRK_3.WAV");
  }
  else if (justpressed[3]) {
    justpressed[3] = 0;
    playfile("TRK_4.WAV");
  }

}


// Plays a full file from beginning to end with no pause.
void playcomplete(char *name) {
  // call our helper to find and play this name
  playfile(name);
  while (wave.isplaying) {
    // do nothing while its playing
  }
  // now its done playing
}


void playfile(char *name) {
  // see if the wave object is currently doing something
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it

  }
  // look in the root directory and open the file
  if (!f.open(root, name)) {
    putstring("Couldn't open file "); Serial.print(name); return;
  }
  // OK read the file and turn it into a wave object
  if (!wave.create(f)) {
    putstring_nl("Not a valid WAV"); return;
  }
  // ok time to play! start playback
  wave.play();
}


// this handy function will return the number of bytes currently free in RAM, great for debugging!
int freeRam(void)
{
  extern int  __bss_end;
  extern int  *__brkval;
  int free_memory;
  if ((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }
  return free_memory;
}

void sdErrorCheck(void)
{
  if (!card.errorCode()) return;
  putstring("\n\rSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  putstring(", ");
  Serial.println(card.errorData(), HEX);
  while (1);
}

