// DHT Temperature & Humidity Sensor
// Unified Sensor Library Example
// Written by Tony DiCola for Adafruit Industries
// Released under an MIT license.

// Depends on the following Arduino libraries:
// - Adafruit Unified Sensor Library: https://github.com/adafruit/Adafruit_Sensor
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN      6         // Pin which is connected to the DHT sensor.

// Uncomment the type of sensor in use:
#define DHTTYPE     DHT11     // DHT 11
//#define DHTTYPE   DHT22     // DHT 22 (AM2302)
//#define DHTTYPE   DHT21     // DHT 21 (AM2301)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview
//  NOTE: For working with a faster chip, like an Arduino Due or Teensy, you might need to increase the threshold for cycle counts considered a 1 or 0.
//  You can do this by passing a 3rd parameter for this threshold.
//  It's a bit of fiddling to find the right value, but in general the faster the CPU the higher the value.
//  The default for a 16mhz AVR is a value of 6.

//  For an Arduino Due that runs at 84mhz a value of 30 works. Example to initialize DHT sensor for Arduino Due:
//  DHT dht(DHTPIN, DHTTYPE, 30);
DHT_Unified dht(DHTPIN, DHTTYPE);

//  OUTPUT SAMPLE
//Hum%: 30.60%  TempF: 71.42*F  HeatIndexF: 69.70*F DewPointF: 38.92*F  TempC: 21.90*C  HeatIndexC: 20.94*C DewPointC: 3.84*C
//Hum%: 30.50%  TempF: 71.24*F  HeatIndexF: 69.50*F DewPointF: 38.68*F  TempC: 21.80*C  HeatIndexC: 20.83*C DewPointC: 3.71*C

float t, h, f;
uint32_t delayMS;

void setup() {

  Serial.begin(115200);

  // Initialize device.
  dht.begin();
  Serial.println("DHTxx Unified Sensor Example w/ Heat Index and Dew Point");

  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");
  Serial.println("------------------------------------");
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

void loop() {

  // Delay between measurements.
  delay(delayMS);

  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
  }
  else {
    t = event.temperature;
  }

  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    h = event.relative_humidity;
  }

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  } else {
    f = TempCToF(t);
    // Compute heat index
    float hi = heatIndex(f, h);
    float hiDegC = TempFToC(hi);
    float dewC = dewPoint(t, h);
    float dewF = TempCToF(dewC);

    Serial.print("Hum%: ");   Serial.print(h); Serial.print("%\t");
    Serial.print(" | ");
    Serial.print("TempF: ");  Serial.print(f); Serial.print("*F\t");
    Serial.print("HeatIndexF: "); Serial.print(hi); Serial.print("*F\t");
    Serial.print("DewPointF: ");  Serial.print(dewF); Serial.print("*F\t");
    Serial.print(" | ");
    Serial.print("TempC: ");  Serial.print(t); Serial.print("*C\t");
    Serial.print("HeatIndexC: "); Serial.print(hiDegC); Serial.print("*C\t");
    Serial.print("DewPointC: ");  Serial.print(dewC); Serial.println("*C");

  }
}

double TempCToF(double celsius)
{
  float f = (((9.0 / 5.0) * celsius + 32.0));
  return f;
}

double TempFToC(double fahrenheit)
{
  float c = ((double)5 / 9) * (fahrenheit - 32);
  return c;
}

// John Main added dewpoint code from : http://playground.arduino.cc/main/DHT11Lib
// Also added DegC output for Heat Index.
// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);

  // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;

  // (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP / 0.61078); // temp var
  return (241.88 * T) / (17.558 - T);
}

// Heat Index Function
//  Convert from Temp F to Heat Index F
double heatIndex(double t, double h)
{
  float offset;
  float hiF = -42.379 + 2.04901523 * t + 10.14333127 * h - .22475541 * t * h - .00683783 * t * t - .05481717 * h * h + .00122874 * t * t * h + .00085282 * t * h * h - .00000199 * t * t * h * h;
  if ((h < 13) && (t >= 80 && t <= 112)) {
    offset = ((13 - h) / 4) * sqrt((17 - abs(t - 95.)) / 17);
  }
  if ((h > 85) && (t >= 80 && t <= 87)) {
    offset = ((h - 85) / 10) * ((87 - t) / 5);
  }
  hiF = hiF + offset;
  if (hiF < 80) {
    //The Rothfusz regression is not appropriate when conditions of temperature and humidity warrant a heat index value below about 80 degrees F. In those cases, a //simpler formula is applied to calculate values consistent with Steadman's results:
    hiF = 0.5 * (t + 61.0 + ((t - 68.0) * 1.2) + (h * 0.094));
  }
  return hiF;
}


