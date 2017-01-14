/*
  REVISION HISTORY
  Created by Mark Swift
  V1.4 - Reversed waiting LED logic
  V1.5 - Added analog moisture option
  V1.6 - Added analog moisture reading gateway message
  V1.7 - Added analog smoothing
  V1.8 - Shortened Sketch Narme (Prevent send presentation errors)
  V1.9 - Moved LED to a seperate function
  V2.0 - Corrected presentation function
  V2.1 - Preventing moisture readings being set or sent until analog smoothing in place
  V2.2 - Added mower relay activate / deactivate logic
  V2.3 - Added message failure retry function
*/

#include <BH1750.h>
#include <NewPing.h>
#include <Adafruit_NeoPixel.h>
#include <elapsedMillis.h>

//*** MY SENSORS ******************************************

// Enable debug prints
// #define MY_DEBUG

#define MY_NODE_ID 3
#define MY_PARENT_NODE_ID AUTO // AUTO
// #define MY_PARENT_NODE_IS_STATIC
// #define MY_BAUD_RATE 9600 // For use with 1Mhz modules

// Enable and select radio type attached
#define MY_RADIO_NRF24
// #define MY_RADIO_RFM69

// Set RF24L01+ channel number
// #define MY_RF24_CHANNEL 125

// Manually define the module PA level
// #define MY_RF24_PA_LEVEL RF24_PA_HIGH

// Manually define the datarate
// #define MY_RF24_DATARATE RF24_250KBPS

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <MySensors.h>

//*** CONFIG **********************************************

// Define radio wait time between sends
#define RADIO_PAUSE 50 // This allows the radio to settle between sends, ideally 0...

// Define radio retries upon failure
int radioRetries = 10;

// Define end of loop pause time
#define LOOP_PAUSE 60000

// Define time between sensors blocks
#define SENSORS_DELAY 100  // This allows sensor VCC to settle between readings, ideally 0...

// Define NeoPixel settings
#define NEO_PIN 2
#define NUM_LEDS 8
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUM_LEDS, NEO_PIN, NEO_GRB + NEO_KHZ800);

// Set moisture mode to either digital (D) or analog (A)
#define MOISTURE_MODE_A

#ifdef MOISTURE_MODE_D
// Digital input pin moisture sensor
#define DIGITAL_INPUT_MOISTURE 6
#endif

#ifdef MOISTURE_MODE_A
// Analog input pin moisture sensor
#define ANALOG_INPUT_MOISTURE A0
// Define moisture upper limit
int moistureLimit = 850;
// Moisture reading derived from analog value
int moistureValue = -1;
// Analog smoothing
const int numReadings = 10; // The number of samples to keep track of
int reading[numReadings]; // The readings from the analog input
int readIndex = 0; // The index of the current reading
int total = 0; // The running total reading
int average = 0; // The average reading
boolean readingsReady = false; // Only start sending readings once smoothing is complete
#endif

// Power pin moisture sensor
#define MOISTURE_POWER_PIN 5
// Send only if changed? 1 = Yes 0 = No
#define COMPARE_MOISTURE 0
// Store last moisture reading for comparison
int lastMoistureValue = -1;

// Power pin rain sensor
#define RAIN_POWER_PIN 7
// Digital input pin rain sensor
#define DIGITAL_INPUT_RAIN 8
// Send only if changed? 1 = Yes 0 = No
#define COMPARE_RAIN 0
// Store last rain reading for comparison
int lastRainValue = -1;

// Ultrasonic trigger pin
#define TRIGGER_PIN 4
// Ultrasonic echo pin
#define ECHO_PIN 3
// Maximum distance we want to ping for (in cms), maximum sensor distance is rated at 400-500cm
#define MAX_DISTANCE 300
// NewPing setup of pins and maximum distance
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
// Send only if changed? 1 = Yes 0 = No
#define COMPARE_DIST 0
// Store last distance reading for comparison
int lastDist = -1;

// Set BH1750 name
BH1750 lightSensor;
// Send only if changed? 1 = Yes 0 = No
#define COMPARE_LUX 0
// Store last LUX reading for comparison
uint16_t lastlux = -1;

// Landroid settings
boolean landroidWaiting = false;
boolean landroidWaitingTriggered = false;
boolean landroidHome = false;
elapsedMillis timeElapsed;

// Landroid timers
#define TIMER1 3200000 // 60 minutes * 60 seconds * 1000 millis = 3200000
#define TIMER2 7200000 // 120 minutes * 60 seconds * 1000 millis = 7200000
#define TIMER3 10800000 // 180 minutes * 60 seconds * 1000 millis = 10800000
#define TIMER4 14400000 // 240 minutes * 60 seconds * 1000 millis = 14400000

// Set default gateway value for metric or imperial
boolean metric = true;

// Define sensor children IDs for MySensors
#define CHILD_ID1 1 // Moisture
#ifdef MOISTURE_MODE_A
#define CHILD_ID2 2 // Moisture Analog Reading
#endif
#define CHILD_ID3 3 // Rain
#define CHILD_ID4 4 // Light
#define CHILD_ID5 5 // Distance
#define CHILD_ID10 10 // Landroid home boolean
#define CHILD_ID11 11 // Landroid waiting boolean
#define CHILD_ID12 12 // Landroid time elapsed

// Define MySensors message types
MyMessage msg1(CHILD_ID1, V_TRIPPED);
#ifdef MOISTURE_MODE_A
MyMessage msg2(CHILD_ID2, V_CUSTOM);
#endif
MyMessage msg3(CHILD_ID3, V_TRIPPED);
MyMessage msg4(CHILD_ID4, V_LEVEL);
MyMessage msg5(CHILD_ID5, V_DISTANCE);
MyMessage msg10(CHILD_ID10, V_TRIPPED);
MyMessage msg11(CHILD_ID11, V_TRIPPED);
MyMessage msg12(CHILD_ID12, V_CUSTOM);

//*********************************************************

void setup()
{
#ifdef MOISTURE_MODE_D
  // Set the moisture sensor digital pin as input
  pinMode(DIGITAL_INPUT_MOISTURE, INPUT);
#endif
  // Set the rain sensor digital pin as input
  pinMode(DIGITAL_INPUT_RAIN, INPUT);
  // Set the moisture sensor power pin as output
  pinMode(MOISTURE_POWER_PIN, OUTPUT);
  // Set the rain sensor power pin as output
  pinMode(RAIN_POWER_PIN, OUTPUT);
  // Set to LOW so no power is flowing through the moisture sensor
  digitalWrite(MOISTURE_POWER_PIN, LOW);
  // Set to LOW so no power is flowing through the rain sensor
  digitalWrite(RAIN_POWER_PIN, LOW);
  // Check gateway for metric setting
  boolean metric = getConfig().isMetric;
  // Start BH1750 light sensor
  lightSensor.begin();
  // Start NeoPixel LED strip
  strip1.begin();
  // Initialise all Neopixel LEDs off
  strip1.show();
}

void presentation()
{
  // Send the sketch version information to the gateway and controller
  sendSketchInfo("Mower Garage", "2.3");
  wait(RADIO_PAUSE);
  // Register all sensors to the gateway (they will be created as child devices)
  present(CHILD_ID1, S_MOTION);
  wait(RADIO_PAUSE);
#ifdef MOISTURE_MODE_A
  present(CHILD_ID2, S_CUSTOM);
  wait(RADIO_PAUSE);
#endif
  present(CHILD_ID3, S_MOTION);
  wait(RADIO_PAUSE);
  present(CHILD_ID4, S_LIGHT_LEVEL);
  wait(RADIO_PAUSE);
  present(CHILD_ID5, S_DISTANCE);
  wait(RADIO_PAUSE);
  present(CHILD_ID10, S_MOTION);
  wait(RADIO_PAUSE);
  present(CHILD_ID11, S_MOTION);
  wait(RADIO_PAUSE);
  present(CHILD_ID12, S_CUSTOM);
  wait(RADIO_PAUSE);
}

void loop()
{
  //*** MOISTURE SENSOR DIGITAL *****************************

#ifdef MOISTURE_MODE_D
  digitalWrite(MOISTURE_POWER_PIN, HIGH); // Turn moisture power pin on
  wait(200); // Set a delay to ensure the moisture sensor has powered up
  int moistureValue = digitalRead(DIGITAL_INPUT_MOISTURE); // Read digital moisture value
  digitalWrite(MOISTURE_POWER_PIN, LOW); // Turn moisture power pin off
#if COMPARE_MOISTURE == 1
  if (moistureValue != lastMoistureValue)
#endif
  {
#ifdef MY_DEBUG
    Serial.print("Moisture: ");
    Serial.println(moistureValue == 0 ? 1 : 0);
#endif
    resend(msg1.set(moistureValue == 0 ? 1 : 0), radioRetries); // Send the inverse
    // wait(RADIO_PAUSE);
    lastMoistureValue = moistureValue; // For testing can be 0 or 1 or back to moistureValue
  }
#endif

  //*** MOISTURE SENSOR ANALOG ******************************

#ifdef MOISTURE_MODE_A
  total = total - reading[readIndex]; // Subtract the last smoothing reading
  digitalWrite(MOISTURE_POWER_PIN, HIGH); // Turn moisture power pin on
  wait(200); // Set a delay to ensure the moisture sensor has powered up
  reading[readIndex] = analogRead(ANALOG_INPUT_MOISTURE); // Read analog moisture value
  digitalWrite(MOISTURE_POWER_PIN, LOW); // Turn moisture power pin off
#ifdef MY_DEBUG
  Serial.print("Moisture: ");
  Serial.println(reading[readIndex]);
#endif
  total = total + reading[readIndex]; // Add the reading to the smoothing total
  readIndex = readIndex + 1; // Advance to the next position in the smoothing array
  if (readIndex >= numReadings) // If we're at the end of the array...
  {
    readingsReady = true;
    readIndex = 0; // Wrap around to the beginning
  }
  if (readingsReady)
  {
    average = total / numReadings;
    average = map(average, 0, 1023, 1023, 0);
    if (average <= moistureLimit)
    {
      moistureValue = 1;
    }
    else
    {
      moistureValue = 0;
    }
  }
#if COMPARE_MOISTURE == 1
  if (moistureValue != lastMoistureValue)
#endif
  {
#ifdef MY_DEBUG
    Serial.print("Moisture: ");
    Serial.println(moistureValue);
    Serial.print("Moisture Avg: ");
    Serial.println(average);
#endif
    {
      resend(msg1.set(moistureValue == 0 ? 1 : 0), radioRetries); // Send the inverse
      // wait(RADIO_PAUSE);
      resend(msg2.set(average), radioRetries);
      // wait(RADIO_PAUSE);
      lastMoistureValue = moistureValue; // For testing can be 0 or 1 or back to moistureValue
    }
  }
#endif

  wait(SENSORS_DELAY); // Wait after sensor readings

  //*** RAIN SENSOR *****************************************

  digitalWrite(RAIN_POWER_PIN, HIGH); // Turn rain power pin on
  wait(200); // Set a delay to ensure the moisture sensor has powered up
  int rainValue = digitalRead(DIGITAL_INPUT_RAIN); // Read digital rain value
  digitalWrite(RAIN_POWER_PIN, LOW); // Turn rain power pin off
#if COMPARE_RAIN == 1
  if (rainValue != lastRainValue) // Check value against saved value
#endif
  {
#ifdef MY_DEBUG
    Serial.print("Rain: ");
    Serial.println(rainValue == 0 ? 1 : 0); // Print the inverse
#endif
    resend(msg3.set(rainValue == 0 ? 1 : 0), radioRetries); // Send the inverse
    // wait(RADIO_PAUSE);
    lastRainValue = rainValue; // For testing can be 0 or 1 or back to rainValue
  }

  wait(SENSORS_DELAY); // Wait after sensor readings

  //*** LUX SENSOR ******************************************

  uint16_t lux = lightSensor.readLightLevel(); // Get Lux value

#if COMPARE_LUX == 1
  if (lux != lastlux)
#endif
  {
#ifdef MY_DEBUG
    Serial.print("LUX: ");
    Serial.println(lux);
#endif
    resend(msg4.set(lux), radioRetries);
    // wait(RADIO_PAUSE);
    lastlux = lux;
  }

  wait(SENSORS_DELAY); // Wait between sensor readings

  //*** DISTANCE SENSOR *************************************

  int dist = metric ? sonar.ping_cm() : sonar.ping_in();
#if COMPARE_DIST == 1
  if (dist != lastDist)
#endif
  {
#ifdef MY_DEBUG
    Serial.print("Distance: ");
    Serial.print(dist); // Convert ping time to distance in cm and print result (0 = outside set distance range)
    Serial.println(metric ? " cm" : " in");
#endif
    resend(msg5.set(dist), radioRetries);
    // wait(RADIO_PAUSE);
    lastDist = dist;
  }

  //*** ANALYSE, SET, SEND TO GATEWAY ***********************

  if (lastMoistureValue == 0 || lastRainValue == 0)
  {
    // resend(MyMessage(1, V_LIGHT).setDestination(4).set(true),radioRetries); // Send message to mower node to activate relay
    // wait(RADIO_PAUSE);
    // resend(MyMessage(1, V_LIGHT).setDestination(4).set(false),radioRetries); // Send message to mower node to deactivate relay, as the timer will now be running
    // wait(RADIO_PAUSE);
    landroidWaiting = true;
    landroidWaitingTriggered = true;
    timeElapsed = 0;
    setWaitingLights(strip1, 0, 4);
    // strip1.setPixelColor(0, 255, 0, 0);
    // strip1.setPixelColor(1, 255, 0, 0);
    // strip1.setPixelColor(2, 255, 0, 0);
    // strip1.setPixelColor(3, 255, 0, 0);
    // strip1.show();
#ifdef MY_DEBUG
    Serial.print("Rain or moisture detected, waiting: ");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroidWaiting);
    Serial.println(")");
#endif
  }

  else
  {
    landroidWaiting = false;
#ifdef MY_DEBUG
    Serial.print("No rain or moisture detected, not waiting: ");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroidWaiting);
    Serial.println(")");
#endif
  }

  if ( landroidWaiting == false && landroidWaitingTriggered == false )
  {
    setWaitingLights(strip1, 4, 4);
    // strip1.setPixelColor(0, 0, 127, 0);
    // strip1.setPixelColor(1, 0, 127, 0);
    // strip1.setPixelColor(2, 0, 127, 0);
    // strip1.setPixelColor(3, 0, 127, 0);
    // strip1.show();
#ifdef MY_DEBUG
    Serial.print("Waiting on normal schedule: ");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroidWaiting);
    Serial.println(")");
#endif
  }

  if ( landroidWaiting == false && landroidWaitingTriggered == true && timeElapsed < TIMER1 ) // Logic for timer trigger
  {
    setWaitingLights(strip1, 0, 4);
    // strip1.setPixelColor(0, 255, 0, 0);
    // strip1.setPixelColor(1, 255, 0, 0);
    // strip1.setPixelColor(2, 255, 0, 0);
    // strip1.setPixelColor(3, 255, 0, 0);
    // strip1.show();
#ifdef MY_DEBUG
    Serial.print("4 Hours Left: ");
    Serial.println("4 LEDs");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroidWaiting);
    Serial.println(")");
    Serial.print("Time elapsed: ");
    Serial.println(timeElapsed / 1000);
#endif
  }

  if ( landroidWaiting == false && landroidWaitingTriggered == true && timeElapsed > TIMER1 ) // Logic for timer trigger
  {
    setWaitingLights(strip1, 1, 4);
    // strip1.setPixelColor(0, 0, 0, 0);
    // strip1.setPixelColor(1, 255, 0, 0);
    // strip1.setPixelColor(2, 255, 0, 0);
    // strip1.setPixelColor(3, 255, 0, 0);
    // strip1.show();
#ifdef MY_DEBUG
    Serial.print("3 Hours Left: ");
    Serial.println("3 LEDs");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroidWaiting);
    Serial.println(")");
    Serial.print("Time elapsed: ");
    Serial.println(timeElapsed / 1000);
#endif
  }

  if ( landroidWaiting == false && landroidWaitingTriggered == true && timeElapsed > TIMER2 ) // Logic for timer trigger
  {
    setWaitingLights(strip1, 2, 4);
    // strip1.setPixelColor(0, 0, 0, 0);
    // strip1.setPixelColor(1, 0, 0, 0);
    // strip1.setPixelColor(2, 255, 0, 0);
    // strip1.setPixelColor(3, 255, 0, 0);
    // strip1.show();
#ifdef MY_DEBUG
    Serial.print("2 Hours Left: ");
    Serial.println("2 LEDs");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroidWaiting);
    Serial.println(")");
    Serial.print("Time elapsed: ");
    Serial.println(timeElapsed / 1000);
#endif
  }

  if ( landroidWaiting == false && landroidWaitingTriggered == true && timeElapsed > TIMER3 ) // Logic for timer trigger
  {
    setWaitingLights(strip1, 3, 4);
    // strip1.setPixelColor(0, 0, 0, 0);
    // strip1.setPixelColor(1, 0, 0, 0);
    // strip1.setPixelColor(2, 0, 0, 0);
    // strip1.setPixelColor(3, 255, 0, 0);
    //strip1.show();
#ifdef MY_DEBUG
    Serial.print("1 Hour Left: ");
    Serial.println("1 LEDs");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroidWaiting);
    Serial.println(")");
    Serial.print("Time elapsed: ");
    Serial.println(timeElapsed / 1000);
#endif
  }

  if ( landroidWaiting == false && landroidWaitingTriggered == true && timeElapsed > TIMER4 ) // Logic for timer trigger
  {
    landroidWaitingTriggered = false;
    setWaitingLights(strip1, 4, 4);
    // strip1.setPixelColor(0, 0, 127, 0);
    // strip1.setPixelColor(1, 0, 127, 0);
    // strip1.setPixelColor(2, 0, 127, 0);
    // strip1.setPixelColor(3, 0, 127, 0);
    // strip1.show();
#ifdef MY_DEBUG
    Serial.print("4 Hours Passed: ");
    Serial.println("It's no longer wet! Waiting on normal schedule");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroidWaiting);
    Serial.println(")");
    Serial.print("Time elapsed: ");
    Serial.println(timeElapsed / 1000);
#endif
  }

  if (lastDist < 30)
  {
    landroidHome = true;
    setHomeLights(strip1, true);
    // strip1.setPixelColor(4, 0, 127, 0);
    // strip1.setPixelColor(5, 0, 127, 0);
    // strip1.setPixelColor(6, 0, 127, 0);
    // strip1.setPixelColor(7, 0, 127, 0);
    // strip1.show();
#ifdef MY_DEBUG
    Serial.println("Home charging");
#endif
  }

  else
  {
    landroidHome = false;
    setHomeLights(strip1, false);
    // strip1.setPixelColor(4, 255, 0, 0);
    // strip1.setPixelColor(5, 255, 0, 0);
    // strip1.setPixelColor(6, 255, 0, 0);
    // strip1.setPixelColor(7, 255, 0, 0);
    // strip1.show();
#ifdef MY_DEBUG
    Serial.println("Cutting the grass!");
#endif
  }

  // Send Landroid home status to gateway
#ifdef MY_DEBUG
  Serial.print("Sending landroidHome ");
  Serial.print("(");
  Serial.print(landroidHome);
  Serial.print(")");
  Serial.print(" status: ");
#endif
  resend(msg10.set(landroidHome), radioRetries);
  // wait(RADIO_PAUSE);

  // Send Landroid waiting status to gateway
#ifdef MY_DEBUG
  Serial.print("Sending landroidWaitingTriggered ");
  Serial.print("(");
  Serial.print(landroidWaitingTriggered);
  Serial.print(")");
  Serial.print(" status: ");
#endif
  resend(msg11.set(landroidWaitingTriggered), radioRetries);
  // wait(RADIO_PAUSE);

  // Send Landroid waiting timer status to gateway
  if ( landroidWaitingTriggered == true && timeElapsed > LOOP_PAUSE && timeElapsed < TIMER4)
  {
#ifdef MY_DEBUG
    Serial.print("Sending timeElapsed ");
    Serial.print("(");
    Serial.print(timeElapsed / 1000);
    Serial.print(")");
    Serial.print(" status: ");
#endif
    resend(msg12.set(timeElapsed / 1000), radioRetries);
    // wait(RADIO_PAUSE);
  }

  else
  {
#ifdef MY_DEBUG
    Serial.print("Sending timeElapsed ");
    Serial.print("(");
    Serial.print(0);
    Serial.print(")");
    Serial.print(" status: ");
#endif
    resend(msg12.set(0), radioRetries);
    // wait(RADIO_PAUSE);
  }

  wait(LOOP_PAUSE); // Sleep or wait (repeater)
}

void setHomeLights(Adafruit_NeoPixel & strip1, boolean isGreen)
{
  for (int i = 4; i < 8; ++i)
  {
    if (isGreen)
    {
      strip1.setPixelColor(i, 0, 10, 0);
    }
    else
    {
      strip1.setPixelColor(i, 50, 0, 0);
    }
  }
  strip1.show();
}

void setWaitingLights(Adafruit_NeoPixel & strip1, int enabledLeds, int totalLeds)
{
  for (int i = 0; i < enabledLeds; ++i)
  {
    strip1.setPixelColor(i, 0, 10, 0);
  }
  for (int i = enabledLeds; i < totalLeds; ++i)
  {
    strip1.setPixelColor(i, 50, 0, 0);
  }
  strip1.show();
}

void resend(MyMessage &msg, int repeats)
{
  int repeat = 0;
  int repeatdelay = 200;
  boolean sendOK = false;

  while ((sendOK == false) and (repeat < repeats))
  {
    if (send(msg))
    {
      sendOK = true;
    }
    else
    {
      sendOK = false;
#ifdef MY_DEBUG
      Serial.print(F("Send ERROR "));
      Serial.println(repeat);
#endif
    }
    repeat++;
    wait(repeatdelay);
  }
}

