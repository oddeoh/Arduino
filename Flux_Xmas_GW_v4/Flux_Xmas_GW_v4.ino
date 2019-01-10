//  Vixen To Serial, to NeoPixel Matrix, to UDP Broadcast
//  HeppXmas_Gateway v4 (171206)

//BN: Arduino/Genuino Mega or Mega 2560
//VID: 2341
//PID: 0042
//SN: 85632313039351617182

//  IMPORTS
#include <SPI.h>
#include <Ethernet2.h>
#include <EthernetUdp2.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include <Adafruit_NeoPixel.h>


//  STATE MACHINE
#define MAX_CHANNELS      40
#define MAX_RELAYS        4
#define MAX_DEVICES       10

enum states
{
  IDLE, DELIM, READ, DISP
};

struct channel {
  int       deviceId;         //  Reference number of the parent device this channel is on
  boolean   inUse;            //  Is this channel deployed/usable
  uint32_t  rgbColor;         //  Color for this channel
  boolean   isOn;             //  Is this channel currently ON/OFF
};

struct device {
  int       startPos    ;
  byte      ipOctet     ;
  long      ipPort      ;
  IPAddress targetIP    ;
  String    macAddress  ;
  long      perf_TAT    ;
  String    firmware    ;
  long      lastAck     ;
  byte      maxChannels ;
};

device  devices[MAX_DEVICES];
channel channels[MAX_CHANNELS];
byte channelVal[MAX_CHANNELS] = {0};


//  DISPLAY
#define NEOPIX_PIN        6
#define MAX_WIDTH         8
#define MAX_HEIGHT        5
#define MAX_PIXELS        MAX_WIDTH * MAX_HEIGHT
#define MAX_BRIGHT        128
#define MAX_COLORS        7

enum ColorIdType
{
  black = 0,
  red = 1,
  green = 2,
  blue = 3,
  white = 4,
  yellow = 5,
  purple = 6
};

enum ColorType
{
  rgb_black = 0,
  rgb_red = 8388608,
  rgb_green = 32768,
  rgb_blue = 128,
  rgb_white = 8421504,
  rgb_yellow = 8421376,
  rgb_purple = 8437963
};

Adafruit_NeoPixel matrix = Adafruit_NeoPixel(MAX_PIXELS, NEOPIX_PIN, NEO_GRB + NEO_KHZ800);

const uint32_t RGB_BLACK = matrix.Color(0, 0, 0);
const uint32_t RGB_RED = matrix.Color(MAX_BRIGHT, 0, 0);
const uint32_t RGB_GREEN = matrix.Color(0, MAX_BRIGHT, 0);
const uint32_t RGB_BLUE = matrix.Color(0, 0, MAX_BRIGHT);
const uint32_t RGB_WHITE = matrix.Color(MAX_BRIGHT, MAX_BRIGHT, MAX_BRIGHT);
const uint32_t RGB_YELLOW = matrix.Color(MAX_BRIGHT, MAX_BRIGHT, 0);
const uint32_t RGB_PURPLE = matrix.Color(MAX_BRIGHT, 0, MAX_BRIGHT);

uint32_t colors[] = { RGB_BLACK, RGB_RED, RGB_GREEN, RGB_BLUE, RGB_WHITE, RGB_YELLOW, RGB_PURPLE };


//  NETWORK
//  Instance the UDP output sockets
#define MAX_SOCKETS       4 //  Number of UDP socket objects available
#define MAX_RETRIES       2 //  Count of duplicate UDP packets to blindly send
#define UDP_PACKET_FRAME  8 //  NOTE: UDP library default = 24
#define FRAME_HEADER      2 //  Count of leading frame characters for remote parser

EthernetUDP UDP1;
EthernetUDP UDP2;
EthernetUDP UDP3;
EthernetUDP UDP4;

//  CONFIG - Network
unsigned int destPort = 1225;
byte mac[] = {
  0x2C, 0xF7, 0xF1, 0x08, 0x0D, 0xA0
};


//  GENERAL
int gatewayState = 0;
int deviceId = 0;
int channelPtr = 0;
int ch = 0;
int loopCnt = 0;                //  Testing


void setup() {

  //  SERIAL
  Serial.begin(115200);   //  VIXEN INP
  Serial1.begin(115200);  //  DEBUG OUT

  //  DISPLAY
  Serial1.println("Clear the display");
  matrix.begin();
  matrix.setBrightness(MAX_BRIGHT);
  matrix.clear();
  matrix.show();

  //  NETWORK
  Serial1.println("Initializing LAN NIC..");
  IPAddress ip(192, 168, 1, 5);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  Ethernet.begin(mac, ip, gateway, subnet);
  //  Setup multiple UDP sockets for output
  Serial1.println("Enable UDP ports on Socket 1 to 4 ");
  UDP1.begin(destPort);
  UDP2.begin(destPort + 1);
  UDP3.begin(destPort + 2);
  UDP4.begin(destPort + 3);

  //  CONFIGURE
  Config_Channels();
  Config_Devices();
  Config_Report();

  //  TEST DISPLAY
  Display_Test();
  Display_Channels();
  matrix.clear();
  matrix.show();

  //  TEST I/O
  Serial1.println("Sending Channel Stream to clear tbe intial remote devices  ");
  SendChannelStream();

}


void loop() {

  //  Scrape the Serial data stream for proper values
  if (Serial.available() > 0) {

    switch (gatewayState)
    {
      case IDLE:
        ch = 0;
        if (Serial.read() == '+')
        {
          gatewayState = DELIM;
        }
        else
        {
          gatewayState = IDLE;
        }
        break;

      case DELIM:
        ch = 0;
        if (Serial.read() == '>')
        {
          gatewayState = READ;
        }
        else
        {
          gatewayState = IDLE;
        }
        break;

      case READ:
        //  Wait until the entire string has been received
        if (Serial.available() >= MAX_CHANNELS) {
          ch = Serial.readBytes(channelVal, MAX_CHANNELS);
          if (ch = MAX_CHANNELS)
          {
            gatewayState = DISP;

            //  DEBUG console output of the received string of data from VIXEN
            for (ch = 0; ch < MAX_CHANNELS; ch++) {
              Serial1.print("\t"); Serial1.print(channelVal[ch], DEC);
            }
            Serial1.println();
            //  DEBUG
          }
        }
        break;

      case DISP:
        gatewayState = IDLE;
        //  We have all of the data to transmit, send it and show it
        SendChannelStream();
        break;
    }
  }

}



void SendChannelStream() {

  //  Respond to the deviceId being referenced and transmit the entire data stream to the devices
  //  Clear the output buffer and stuff the header into the frame for each device
  int ch = 0;
  Serial1.print(millis()); Serial1.println("\tSendChannelStream ...");

  //  Iterate through each device and send the target IP the relay packet
  for (deviceId = 0; deviceId < MAX_DEVICES; deviceId++) {
    //deviceId = 0;

    //  Transmit to each device the 4-channels it manages
    Serial1.print("Xmit to Octet\t");
    Serial1.println(devices[deviceId].ipOctet, DEC);

    //  Setup blank buffer packet for this device (with just the state header)
    byte  outputBuffer[6] = {'+', '>', 0, 0, 0, 0};
    //  Move the channel values into the buffer (from this device pass)
    for (int ptr = 0; ptr < MAX_RELAYS; ptr++) {
      outputBuffer[FRAME_HEADER + ptr] = channelVal[ch];
      //  Sync the pixel display to the channel values
      if (channelVal[ch] > 0) {
        PixelOn(ch);
      } else {
        PixelOff(ch);
      }
      //  Track the channel pointer for each Remote Device Send iteration
      ch++;
    }
    
    //  DEBUG Show the output buffer contents for this UDP packet
    Serial1.print(millis()); Serial1.print("\tOutput buffer ..."); Serial1.print("Xmit to Octet\t"); Serial1.println(devices[deviceId].ipOctet, DEC);
    for (int ptr = 0; ptr < FRAME_HEADER + MAX_RELAYS; ptr++) {
      Serial1.print("\t"); Serial1.print(outputBuffer[ptr], DEC);
    }
    Serial1.println();
    //  DEBUG
    
    //  Enable the channel display before UDP sending to see Real-Time visual representation
    matrix.show();

    //  TODO  Remove this single Device Send constraint, or create toggle between:
    //        a) Vixen COM I/O debugging, and b) Remote Wifi UDP debugging
    if (deviceId == 0) {
      //  Packet Copy 1
      for (int ptr=0; ptr < MAX_RETRIES; ptr++) {
        UDP1.beginPacket(devices[deviceId].targetIP, destPort);
        UDP1.write(outputBuffer, FRAME_HEADER + MAX_RELAYS);
        UDP1.endPacket();  
      }
    }
  }

}


//  TEST SUBS
void TEST_loop() {

  Serial1.println("Start Stream Test Loop");
  //for (int deviceId; deviceId < MAX_DEVICES; deviceId++) {
  TEST_SendChannelStream(0);
  delay(200);

}

void TEST_SendChannelStream(int deviceId) {

  //  Respond to the deviceId being referenced and transmit the entire data stream to the devices
  //  Clear the output buffer and stuff the header into the frame for each device
  int ch = 0;
  Serial1.print(millis()); Serial1.println("\tSendChannelStream...");
  loopCnt++;

  //  Force Values to Device
  switch (loopCnt % 6)
  {
    case 0:
      channelVal[0] = 0;
      channelVal[1] = 0;
      channelVal[2] = 0;
      channelVal[3] = 0;
      break;

    case 1:
      channelVal[0] = 255;
      channelVal[1] = 0;
      channelVal[2] = 0;
      channelVal[3] = 0;
      break;

    case 2:
      channelVal[0] = 0;
      channelVal[1] = 255;
      channelVal[2] = 0;
      channelVal[3] = 0;
      break;

    case 3:
      channelVal[0] = 0;
      channelVal[1] = 0;
      channelVal[2] = 255;
      channelVal[3] = 0;
      break;

    case 4:
      channelVal[0] = 0;
      channelVal[1] = 0;
      channelVal[2] = 0;
      channelVal[3] = 255;
      break;

    case 5:
      channelVal[0] = 255;
      channelVal[1] = 255;
      channelVal[2] = 255;
      channelVal[3] = 255;
      break;
  }

  //  Build the "blank" starter packet for this device
  byte  outputBuffer[6] = {'+', '>', 0, 0, 0, 0};

  //  Assign the channel values to the output buffer for this device
  for (int ptr = 0; ptr < MAX_RELAYS; ptr++) {
    outputBuffer[FRAME_HEADER + ptr] = channelVal[ptr];
    if (channelVal[ptr] > 0) {
      PixelOn(ch);
    } else {
      PixelOff(ch);
    }
    ch++;
  }

  //  Send to the target device its RELAY frame
  Serial1.print(millis()); Serial1.print("\tOutput buffer ..."); Serial1.print("Xmit to Octet\t"); Serial1.println(devices[deviceId].ipOctet, DEC);
  for (int ptr = 0; ptr < FRAME_HEADER + MAX_RELAYS; ptr++) {
    Serial1.print("\t"); Serial1.print(outputBuffer[ptr], DEC);
  }
  Serial1.println();

  //  Toggle using different UDP sockets
  switch (loopCnt % 4)
  {
    case 0:
      UDP4.beginPacket(devices[deviceId].targetIP, destPort);
      UDP4.write(outputBuffer, FRAME_HEADER + MAX_RELAYS);
      UDP4.endPacket();

      UDP4.beginPacket(devices[deviceId].targetIP, destPort);
      UDP4.write(outputBuffer, FRAME_HEADER + MAX_RELAYS);
      UDP4.endPacket();
      break;

    case 1:
      UDP1.beginPacket(devices[deviceId].targetIP, destPort);
      UDP1.write(outputBuffer, FRAME_HEADER + MAX_RELAYS);
      UDP1.endPacket();

      UDP1.beginPacket(devices[deviceId].targetIP, destPort);
      UDP1.write(outputBuffer, FRAME_HEADER + MAX_RELAYS);
      UDP1.endPacket();
      break;

    case 2:
      UDP2.beginPacket(devices[deviceId].targetIP, destPort);
      UDP2.write(outputBuffer, FRAME_HEADER + MAX_RELAYS);
      UDP2.endPacket();

      UDP2.beginPacket(devices[deviceId].targetIP, destPort);
      UDP2.write(outputBuffer, FRAME_HEADER + MAX_RELAYS);
      UDP2.endPacket();
      break;

    case 3:
      UDP3.beginPacket(devices[deviceId].targetIP, destPort);
      UDP3.write(outputBuffer, FRAME_HEADER + MAX_RELAYS);
      UDP3.endPacket();

      UDP3.beginPacket(devices[deviceId].targetIP, destPort);
      UDP3.write(outputBuffer, FRAME_HEADER + MAX_RELAYS);
      UDP3.endPacket();
      break;
  }

  matrix.show();

}




//  SETUP
void Device_Init(int deviceId, byte lastIpOctet, String macAddr, int totalChannels) {

  //  Setup the device and channel arrays with the initial values for DEVICE. Default values for CHANNELS
  devices[deviceId].ipOctet = lastIpOctet;

  devices[deviceId].macAddress = macAddr;
  devices[deviceId].maxChannels = totalChannels;
  devices[deviceId].ipPort = 1225;
  devices[deviceId].perf_TAT = 0;
  devices[deviceId].firmware = "1.0.1";
  devices[deviceId].targetIP = IPAddress(192, 168, 1, devices[deviceId].ipOctet);

  //  Link the channels to the devices
  //  Calculate the ordinal position of these channels within this device block and use default values
  channelPtr = deviceId * totalChannels;
  for (int ptr = channelPtr; ptr < (channelPtr + totalChannels); ptr++)
  {
    channels[ptr].deviceId = deviceId;
    channels[ptr].inUse = false;
    channels[ptr].isOn = false;
    channels[ptr].rgbColor = RGB_BLACK;
  }

}

void Channel_Init(int channelId, uint32_t rgbColor) {

  //  Setup the CHANNEL assignment values

  // Setup the channels with the programmed color values}
  if (channelId <= MAX_CHANNELS) {
    channels[channelId].rgbColor = rgbColor;
    //  Black means unassigned, check the passed color and mark this channel unused
    if (rgbColor = RGB_BLACK) {
      channels[channelId].inUse = false;
    } else {
      channels[channelId].inUse = true;
    }
  }
}


void Config_Report() {

  //  Output the device/channel configuration being used with VIXEN to DEBUG screen
  channelPtr = 0;
  for (int deviceId = 0; deviceId < MAX_DEVICES; deviceId++)
  {
    //  REMOTE DEVICES
    Serial1.println("");
    Serial1.print("REMOTE DEVICE "); Serial1.println(deviceId + 1, DEC);
    Serial1.print("mac");
    Serial1.print("\t"); Serial1.print("ip");
    Serial1.print("\t"); Serial1.print("port");
    Serial1.print("\t"); Serial1.println("channels"); ;

    Serial1.print(devices[deviceId].macAddress);
    Serial1.print("\t"); Serial1.print(devices[deviceId].ipOctet, DEC);
    Serial1.print("\t"); Serial1.print(devices[deviceId].ipPort, DEC);
    Serial1.print("\t"); Serial1.println(devices[deviceId].maxChannels, DEC);

    //  CHANNEL OUTPUT FOR THIS DEVICE
    Serial1.println("");
    Serial1.println("Channels:");
    Serial1.print("id");
    Serial1.print("\t"); Serial1.print("plug");
    Serial1.print("\t"); Serial1.print("use");
    Serial1.print("\t"); Serial1.println("color");

    for (int ptr = 0; ptr < devices[deviceId].maxChannels; ptr++)
    {
      Serial1.print(channelPtr);
      Serial1.print("\t"); Serial1.print(ptr + 1);
      Serial1.print("\t"); Serial1.print(channels[channelPtr].inUse);
      Serial1.print("\t");
      if (channels[channelPtr].inUse == true) {
        Serial1.print(ColorToName(channels[channelPtr].rgbColor)); Serial1.print(" ("); Serial1.print(channels[channelPtr].rgbColor, DEC); Serial1.print(")");
      }
      Serial1.println();
      channelPtr++;
    }

  }

}

void Display_Test() {

  //  Startup routine, confirms that the pixel display is working at each address
  //  and that the colors are appropriate.

  //  Grid Coordinate mapping test
  //  Output "special" color to the outside coordinates of the Matrix to validate dimensions
  matrix.clear();
  matrix.setPixelColor(0, RGB_YELLOW); matrix.show(); delay(100);
  matrix.setPixelColor(MAX_WIDTH - 1, RGB_YELLOW); matrix.show(); delay(100);
  matrix.setPixelColor(MAX_PIXELS - 1, RGB_YELLOW); matrix.show(); delay(100);
  matrix.setPixelColor(MAX_PIXELS - MAX_WIDTH, RGB_YELLOW); matrix.show(); delay(100);
  matrix.clear();

  //  Grid Pixel Color Test
  //  Iterate through all pixels with each color
  for (int colorPtr = 0; colorPtr < MAX_COLORS; ++colorPtr) {
    for (int ptr = 0; ptr < MAX_CHANNELS; ++ptr) {
      matrix.setPixelColor(ptr, colors[colorPtr]);
      matrix.show();
      delay(25);
      matrix.clear();
    }
  }
  delay(500);
  matrix.clear();

}

void Display_Channels() {
  //  Output the visual display of each devices's channel configuration
  matrix.clear();

  //  SLOW WALK
  for (int ptr = 0; ptr < MAX_CHANNELS; ++ptr) {
    matrix.setPixelColor(ptr, channels[ptr].rgbColor);
    matrix.show();
    delay(25);
    matrix.clear();
  }
  delay(500);

  //  REAL-TIME, All Channels Configured!
  for (int ptr = 0; ptr < MAX_CHANNELS; ++ptr) {
    matrix.setPixelColor(ptr, channels[ptr].rgbColor);
  }
  matrix.show();
  delay(500);

  matrix.clear();

}


void PixelOn(int channelId) {
  //  Broncos Colors: ORA (252, 76, 2), BLU (12,35,64)
  matrix.setPixelColor(channelId, channels[channelId].rgbColor);
}

void PixelOff(int channelId) {
  matrix.setPixelColor(channelId, RGB_BLACK);
}


String ColorToName(uint32_t rgbColorValue) {

  switch (rgbColorValue) {

    case 0:
      return "Black";
      break;
    case 8388608:
      return "Red";
      break;
    case 32768:
      return "Green";
      break;
    case 128:
      return "Blue";
      break;
    case 8421504:
      return "White";
      break;
    case 8421376:
      return "Yellow";
      break;
    case 8388736:
      return "Purple";
      break;

    default:
      return "INVALID";
      break;
  }
}

void Config_Devices() {

  //  Device Block Data
  Serial1.println("Setting up the Device values");
  //  IP and MAC and Matrix Display R/C start (x/y)
  Device_Init(0, 50, "9837", 4);
  Device_Init(1, 51, "0B3B", 4);
  Device_Init(2, 52, "A475", 4);
  Device_Init(3, 53, "9A99", 4);
  Device_Init(4, 54, "54EE", 4);
  Device_Init(5, 55, "5B87", 4);
  Device_Init(6, 56, "A5B2", 4);
  Device_Init(7, 57, "961B", 4);
  Device_Init(8, 58, "9BF1", 4);
  Device_Init(9, 59, "FEE2", 4);
}

void Config_Channels() {

  //  Channel Values
  Serial1.println("Setting up the Channel values");
  // 1  9837 (proto)  => 9AD8 (PROD)
  Channel_Init(0, RGB_GREEN  );
  Channel_Init(1, RGB_WHITE  );
  Channel_Init(2, RGB_BLUE   );
  Channel_Init(3, RGB_YELLOW );
  // 2  0B3B
  Channel_Init(4, RGB_RED    );
  Channel_Init(5, RGB_WHITE  );
  Channel_Init(6, RGB_GREEN  );
  Channel_Init(7, RGB_WHITE  );
  // 3  A475
  Channel_Init(8, RGB_RED    );
  Channel_Init(9, RGB_GREEN  );
  Channel_Init(10, RGB_GREEN );
  Channel_Init(11, RGB_GREEN );
  // 4  9A99
  Channel_Init(12, RGB_BLUE  );
  Channel_Init(13, RGB_RED   );
  Channel_Init(14, RGB_BLUE  );
  Channel_Init(15, RGB_RED   );
  // 5  54EE
  Channel_Init(16, RGB_RED   );
  Channel_Init(17, RGB_BLUE  );
  Channel_Init(18, RGB_WHITE );
  Channel_Init(19, RGB_YELLOW);
  // 6  5B87
  Channel_Init(20, RGB_WHITE );
  Channel_Init(21, RGB_RED   );
  Channel_Init(22, RGB_BLUE  );
  Channel_Init(23, RGB_GREEN );
  // 7  A5B2
  Channel_Init(24, RGB_RED   );
  Channel_Init(25, RGB_YELLOW);
  Channel_Init(26, RGB_GREEN );
  Channel_Init(27, RGB_BLUE  );
  //  8 961B
  Channel_Init(28, RGB_RED   );
  Channel_Init(29, RGB_WHITE );
  Channel_Init(30, RGB_BLUE  );
  Channel_Init(31, RGB_GREEN );
  //  9 9BF1
  Channel_Init(32, RGB_YELLOW);
  Channel_Init(33, RGB_WHITE );
  Channel_Init(34, RGB_WHITE );
  //Channel_Init(35, RGB_PURPLE);
  //  10  FEE2
  Channel_Init(36, RGB_RED   );
  Channel_Init(37, RGB_BLUE  );
  Channel_Init(38, RGB_WHITE );
  Channel_Init(39, RGB_BLUE  );

}


