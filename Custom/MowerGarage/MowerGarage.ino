/*
  REVISION HISTORY
  Created by Mark Swift
  V1.4 - Reversed waiting LED logic
  V1.5 - Added analog moisture option
  V1.6 - Added analog moisture current_smoothing_reading gateway message
  V1.7 - Added analog smoothing
  V1.8 - Shortened Sketch Narme (Prevent send presentation errors)
  V1.9 - Moved LED to a seperate function
  V2.0 - Corrected presentation function
  V2.1 - Preventing moisture current_smoothing_readings being set or sent until
  analog smoothing in place
  V2.2 - Added mower relay activate / deactivate logic
  V2.3 - Added message failure retry function
  V2.4 - Optimised resend, also changed delay to wait
  V2.5 - Small changes to formatting and layout
*/

//*** EXTERNAL LIBRARIES **********************************

#include <BH1750.h>
#include <NewPing.h>
#include <Adafruit_NeoPixel.h>
#include <elapsedMillis.h>

//*** MYSENSORS *******************************************

// Enable debug prints
// #define MY_DEBUG

// Lower serial speed if using 1Mhz clock
// #define MY_BAUD_RATE 9600

#define MY_NODE_ID 3
#define MY_PARENT_NODE_ID AUTO // AUTO
// #define MY_PARENT_NODE_IS_STATIC

// Enable and select radio type attached
#define MY_RADIO_NRF24
// #define MY_RADIO_RFM69

// Override RF24L01 channel number
// #define MY_RF24_CHANNEL 125

// Override RF24L01 module PA level
// #define MY_RF24_PA_LEVEL RF24_PA_HIGH

// Override RF24L01 datarate
// #define MY_RF24_DATARATE RF24_250KBPS

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <MySensors.h>

// *** SKETCH CONFIG **************************************

#define SKETCH_NAME "Landroid Garage"
#define SKETCH_MAJOR_VER "2"
#define SKETCH_MINOR_VER "5"

// *** SENSORS CONFIG *************************************

// Radio wait between sends allows the radio to settle, ideally 0...
#define RADIO_PAUSE 50

// Radio retries upon failure
int radio_retries = 10;

// End of loop pause time
#define LOOP_PAUSE 60000

// Time between sensor blocks, allows sensor VCC to settle, ideally 0...
#define SENSOR_DELAY 100

// NeoPixel settings
#define NEO_PIN 2
#define NEO_LEDS 8
Adafruit_NeoPixel strip_1 = Adafruit_NeoPixel(NEO_LEDS, NEO_PIN, NEO_GRB + NEO_KHZ800);

// Set moisture mode to either digital (D) or analog (A)
#define MOISTURE_MODE_A

#ifdef MOISTURE_MODE_D
// Digital input pin moisture sensor
#define DIGITAL_INPUT_MOISTURE 6
#endif

#ifdef MOISTURE_MODE_A
// Analog input pin moisture sensor
#define ANALOG_INPUT_MOISTURE A0
// Moisture upper limit
int moisture_limit = 850;
// Moisture current_smoothing_reading derived from analog value
int moisture_value = -1;
// Analog smoothing samples
const int smoothing_readings = 10;
// Analog smoothing array setup
int current_smoothing_reading[smoothing_readings];
// Index of current smoothing reading
int smoothing_read_index = 0;
// Smoothing running total
int smoothing_total = 0;
// Soothing average
int smoothing_average = 0;
// Smoothing readings ready
boolean smoothing_readings_ready = false;
#endif

// Power pin moisture sensor
#define MOISTURE_POWER_PIN 5
// Send only if changed? 1 = Yes 0 = No
#define COMPARE_MOISTURE 0
// Store last moisture current_smoothing_reading for comparison
int last_moisture_value = -1;

// Power pin rain sensor
#define RAIN_POWER_PIN 7
// Digital input pin rain sensor
#define DIGITAL_INPUT_RAIN 8
// Send only if changed? 1 = Yes 0 = No
#define COMPARE_RAIN 0
// Store last rain current_smoothing_reading for comparison
int last_rain_value = -1;

// Ultrasonic trigger pin
#define TRIGGER_PIN 4
// Ultrasonic echo pin
#define ECHO_PIN 3
// Maximum distance to ping in cms (maximum sensor distance is rated at
// 400-500cm)
#define MAX_DISTANCE 300
// NewPing setup of pins and maximum distance
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
// Send only if changed? 1 = Yes 0 = No
#define COMPARE_DIST 0
// Store last distance current_smoothing_reading for comparison
int last_distance_value = -1;

// Set BH1750 name
BH1750 light_sensor;
// Send only if changed? 1 = Yes 0 = No
#define COMPARE_LUX 0
// Store last LUX current_smoothing_reading for comparison
uint16_t last_lux_value = -1;

// Landroid settings
boolean landroid_waiting = false;
boolean landroid_waiting_triggered = false;
boolean landroid_home = false;
elapsedMillis time_elapsed;

// Landroid timers
#define TIMER1 3200000 // 60 minutes * 60 seconds * 1000 millis = 3200000
#define TIMER2 7200000 // 120 minutes * 60 seconds * 1000 millis = 7200000
#define TIMER3 10800000 // 180 minutes * 60 seconds * 1000 millis = 10800000
#define TIMER4 14400000 // 240 minutes * 60 seconds * 1000 millis = 14400000

// Set default gateway value for metric or imperial
boolean metric = true;

// Define the sensor child IDs
// Moisture boolean
#define CHILD_ID1 1
#ifdef MOISTURE_MODE_A
// Moisture analog current_smoothing_reading
#define CHILD_ID2 2
#endif
// Rain boolean
#define CHILD_ID3 3
// Light LUX
#define CHILD_ID4 4
// Distance
#define CHILD_ID5 5
// Landroid home boolean
#define CHILD_ID10 10
// Landroid waiting boolean
#define CHILD_ID11 11
// Landroid timer
#define CHILD_ID12 12

// Define the message formats
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
  light_sensor.begin();
  // Start NeoPixel LED strip
  strip_1.begin();
  // Initialise all Neopixel LEDs off
  strip_1.show();
}

void presentation()
{
  // Send the sketch version information to the gateway and controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);
  wait(RADIO_PAUSE);
  // Present all sensors to controller
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
  // Turn moisture power pin on
  digitalWrite(MOISTURE_POWER_PIN, HIGH);
  // Set a delay to ensure the moisture sensor has settled
  wait(200);
  // Read digital moisture value
  int moisture_value = digitalRead(DIGITAL_INPUT_MOISTURE);
  // Turn moisture power pin off to prevent oxidation
  digitalWrite(MOISTURE_POWER_PIN, LOW);
#if COMPARE_MOISTURE == 1
  if (moisture_value != last_moisture_value)
#endif
  {
#ifdef MY_DEBUG
    Serial.print("Moisture: ");
    // Print the inverse
    Serial.println(moisture_value == 0 ? 1 : 0);
#endif
    // Send the inverse
    resend(msg1.set(moisture_value == 0 ? 1 : 0), radio_retries);
    wait(RADIO_PAUSE);
    // For testing can be 0 or 1 or back to moisture_value
    last_moisture_value = moisture_value;
  }
#endif

  //*** MOISTURE SENSOR ANALOG ******************************

#ifdef MOISTURE_MODE_A
  // Subtract the last smoothing current_smoothing_reading
  smoothing_total = smoothing_total - current_smoothing_reading[smoothing_read_index];
  // Turn moisture power pin on
  digitalWrite(MOISTURE_POWER_PIN, HIGH);
  // Set a delay to ensure the moisture sensor has settled
  wait(200);
  // Read analog moisture value
  current_smoothing_reading[smoothing_read_index] = analogRead(ANALOG_INPUT_MOISTURE);
  // Turn moisture power pin off to prevent oxidation
  digitalWrite(MOISTURE_POWER_PIN, LOW);
#ifdef MY_DEBUG
  Serial.print("Moisture: ");
  Serial.println(current_smoothing_reading[smoothing_read_index]);
#endif
  // Add the current_smoothing_reading to the smoothing smoothing_total
  smoothing_total = smoothing_total + current_smoothing_reading[smoothing_read_index];
  // Advance to the next position in the smoothing array
  smoothing_read_index = smoothing_read_index + 1;
  // If we're at the end of the array...
  if (smoothing_read_index >= smoothing_readings) {
    smoothing_readings_ready = true;
    // Wrap around to the beginning
    smoothing_read_index = 0;
  }
  if (smoothing_readings_ready) {
    smoothing_average = smoothing_total / smoothing_readings;
    smoothing_average = map(smoothing_average, 0, 1023, 1023, 0);
    if (smoothing_average <= moisture_limit) {
      moisture_value = 1;
    }
    else {
      moisture_value = 0;
    }
  }
#if COMPARE_MOISTURE == 1
  if (moisture_value != last_moisture_value)
#endif
  {
#ifdef MY_DEBUG
    Serial.print("Moisture: ");
    Serial.println(moisture_value);
    Serial.print("Moisture average: ");
    Serial.println(smoothing_average);
#endif
    {
      // Send the inverse
      resend(msg1.set(moisture_value == 0 ? 1 : 0), radio_retries);
      wait(RADIO_PAUSE);
      resend(msg2.set(smoothing_average), radio_retries);
      wait(RADIO_PAUSE);
      // For testing can be 0 or 1 or back to moisture_value
      last_moisture_value = moisture_value;
    }
  }
#endif

  wait(SENSOR_DELAY);

  //*** RAIN SENSOR *****************************************

  // Turn rain power pin on
  digitalWrite(RAIN_POWER_PIN, HIGH);
  // Set a delay to ensure the rain sensor has settled
  wait(200);
  // Read digital rain value
  int rainValue = digitalRead(DIGITAL_INPUT_RAIN);
  // Turn moisture power pin off to prevent oxidation
  digitalWrite(RAIN_POWER_PIN, LOW);
  // Check value against saved value
#if COMPARE_RAIN == 1
  if (rainValue != last_rain_value)
#endif
  {
#ifdef MY_DEBUG
    Serial.print("Rain: ");
    // Print the inverse
    Serial.println(rainValue == 0 ? 1 : 0);
#endif
    // Send the inverse
    resend(msg3.set(rainValue == 0 ? 1 : 0), radio_retries);
    wait(RADIO_PAUSE);
    // For testing can be 0 or 1 or back to rainValue
    last_rain_value = rainValue;
  }

  wait(SENSOR_DELAY);

  //*** LUX SENSOR ******************************************

  // Get Lux value
  uint16_t lux = light_sensor.readLightLevel();
#if COMPARE_LUX == 1
  if (lux != last_lux_value)
#endif
  {
#ifdef MY_DEBUG
    Serial.print("LUX: ");
    Serial.println(lux);
#endif
    resend(msg4.set(lux), radio_retries);
    wait(RADIO_PAUSE);
    last_lux_value = lux;
  }

  wait(SENSOR_DELAY);

  //*** DISTANCE SENSOR *************************************

  // Get distance based on metric boolean
  int dist = metric ? sonar.ping_cm() : sonar.ping_in();
#if COMPARE_DIST == 1
  if (dist != last_distance_value)
#endif
  {
#ifdef MY_DEBUG
    Serial.print("Distance: ");
    Serial.print(dist);
    // Display unit of measure based on metric boolean
    Serial.println(metric ? " cm" : " in");
#endif
    resend(msg5.set(dist), radio_retries);
    wait(RADIO_PAUSE);
    last_distance_value = dist;
  }

  //*** ANALYSE, SET, SEND TO GATEWAY ***********************

  if (last_moisture_value == 0 || last_rain_value == 0) {
    // Landroid relay tests
    // resend(MyMessage(1, V_LIGHT).setDestination(4).set(true),radio_retries);
    // // Send message to mower node to activate relay
    // wait(RADIO_PAUSE);
    // resend(MyMessage(1, V_LIGHT).setDestination(4).set(false),radio_retries);
    // // Send message to mower node to deactivate relay, as the timer will now
    // be running
    // wait(RADIO_PAUSE);
    landroid_waiting = true;
    landroid_waiting_triggered = true;
    time_elapsed = 0;
    setWaitingLights(strip_1, 0, 4);
    // strip_1.setPixelColor(0, 255, 0, 0);
    // strip_1.setPixelColor(1, 255, 0, 0);
    // strip_1.setPixelColor(2, 255, 0, 0);
    // strip_1.setPixelColor(3, 255, 0, 0);
    // strip_1.show();
#ifdef MY_DEBUG
    Serial.print("Rain or moisture detected, waiting: ");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroid_waiting);
    Serial.println(")");
#endif
  }

  else {
    landroid_waiting = false;
#ifdef MY_DEBUG
    Serial.print("No rain or moisture detected, not waiting: ");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroid_waiting);
    Serial.println(")");
#endif
  }

  if (landroid_waiting == false && landroid_waiting_triggered == false) {
    setWaitingLights(strip_1, 4, 4);
    // strip_1.setPixelColor(0, 0, 127, 0);
    // strip_1.setPixelColor(1, 0, 127, 0);
    // strip_1.setPixelColor(2, 0, 127, 0);
    // strip_1.setPixelColor(3, 0, 127, 0);
    // strip_1.show();
#ifdef MY_DEBUG
    Serial.print("Waiting on normal schedule: ");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroid_waiting);
    Serial.println(")");
#endif
  }

  // Logic for timer
  if (landroid_waiting == false && landroid_waiting_triggered == true && time_elapsed < TIMER1) {
    // strip_1.setPixelColor(0, 255, 0, 0);
    // strip_1.setPixelColor(1, 255, 0, 0);
    // strip_1.setPixelColor(2, 255, 0, 0);
    // strip_1.setPixelColor(3, 255, 0, 0);
    // strip_1.show();
#ifdef MY_DEBUG
    Serial.print("4 Hours Left: ");
    Serial.println("4 LEDs");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroid_waiting);
    Serial.println(")");
    Serial.print("Time elapsed: ");
    Serial.println(time_elapsed / 1000);
#endif
  }

  // Logic for timer trigger
  if (landroid_waiting == false && landroid_waiting_triggered == true && time_elapsed > TIMER1) {
    setWaitingLights(strip_1, 1, 4);
    // strip_1.setPixelColor(0, 0, 0, 0);
    // strip_1.setPixelColor(1, 255, 0, 0);
    // strip_1.setPixelColor(2, 255, 0, 0);
    // strip_1.setPixelColor(3, 255, 0, 0);
    // strip_1.show();
#ifdef MY_DEBUG
    Serial.print("3 Hours Left: ");
    Serial.println("3 LEDs");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroid_waiting);
    Serial.println(")");
    Serial.print("Time elapsed: ");
    Serial.println(time_elapsed / 1000);
#endif
  }

  // Logic for timer trigger
  if (landroid_waiting == false && landroid_waiting_triggered == true && time_elapsed > TIMER2) {
    setWaitingLights(strip_1, 2, 4);
    // strip_1.setPixelColor(0, 0, 0, 0);
    // strip_1.setPixelColor(1, 0, 0, 0);
    // strip_1.setPixelColor(2, 255, 0, 0);
    // strip_1.setPixelColor(3, 255, 0, 0);
    // strip_1.show();
#ifdef MY_DEBUG
    Serial.print("2 Hours Left: ");
    Serial.println("2 LEDs");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroid_waiting);
    Serial.println(")");
    Serial.print("Time elapsed: ");
    Serial.println(time_elapsed / 1000);
#endif
  }

  // Logic for timer trigger
  if (landroid_waiting == false && landroid_waiting_triggered == true && time_elapsed > TIMER3) {
    setWaitingLights(strip_1, 3, 4);
    // strip_1.setPixelColor(0, 0, 0, 0);
    // strip_1.setPixelColor(1, 0, 0, 0);
    // strip_1.setPixelColor(2, 0, 0, 0);
    // strip_1.setPixelColor(3, 255, 0, 0);
    // strip_1.show();
#ifdef MY_DEBUG
    Serial.print("1 Hour Left: ");
    Serial.println("1 LEDs");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroid_waiting);
    Serial.println(")");
    Serial.print("Time elapsed: ");
    Serial.println(time_elapsed / 1000);
#endif
  }

  // Logic for timer trigger
  if (landroid_waiting == false && landroid_waiting_triggered == true && time_elapsed > TIMER4) {
    landroid_waiting_triggered = false;
    setWaitingLights(strip_1, 4, 4);
    // strip_1.setPixelColor(0, 0, 127, 0);
    // strip_1.setPixelColor(1, 0, 127, 0);
    // strip_1.setPixelColor(2, 0, 127, 0);
    // strip_1.setPixelColor(3, 0, 127, 0);
    // strip_1.show();
#ifdef MY_DEBUG
    Serial.print("4 Hours Passed: ");
    Serial.println("It's no longer wet! Waiting on normal schedule");
    Serial.print("Status");
    Serial.print("(");
    Serial.print(landroid_waiting);
    Serial.println(")");
    Serial.print("Time elapsed: ");
    Serial.println(time_elapsed / 1000);
#endif
  }

  // Logic for Landroid home logic
  if (last_distance_value < 30) {
    landroid_home = true;
    setHomeLights(strip_1, true);
    // strip_1.setPixelColor(4, 0, 127, 0);
    // strip_1.setPixelColor(5, 0, 127, 0);
    // strip_1.setPixelColor(6, 0, 127, 0);
    // strip_1.setPixelColor(7, 0, 127, 0);
    // strip_1.show();
#ifdef MY_DEBUG
    Serial.println("Home charging");
#endif
  }

  else {
    landroid_home = false;
    setHomeLights(strip_1, false);
    // strip_1.setPixelColor(4, 255, 0, 0);
    // strip_1.setPixelColor(5, 255, 0, 0);
    // strip_1.setPixelColor(6, 255, 0, 0);
    // strip_1.setPixelColor(7, 255, 0, 0);
    // strip_1.show();
#ifdef MY_DEBUG
    Serial.println("Cutting the grass!");
#endif
  }

  // Send Landroid home status to gateway
#ifdef MY_DEBUG
  Serial.print("Sending landroid_home ");
  Serial.print("(");
  Serial.print(landroid_home);
  Serial.print(")");
  Serial.print(" status: ");
#endif
  resend(msg10.set(landroid_home), radio_retries);
  wait(RADIO_PAUSE);

  // Send Landroid waiting status to gateway
#ifdef MY_DEBUG
  Serial.print("Sending landroid_waiting_triggered ");
  Serial.print("(");
  Serial.print(landroid_waiting_triggered);
  Serial.print(")");
  Serial.print(" status: ");
#endif
  resend(msg11.set(landroid_waiting_triggered), radio_retries);
  wait(RADIO_PAUSE);

  // Send Landroid waiting timer status to gateway
  if (landroid_waiting_triggered == true && time_elapsed > LOOP_PAUSE && time_elapsed < TIMER4) {
#ifdef MY_DEBUG
    Serial.print("Sending time_elapsed ");
    Serial.print("(");
    Serial.print(time_elapsed / 1000);
    Serial.print(")");
    Serial.print(" status: ");
#endif
    resend(msg12.set(time_elapsed / 1000), radio_retries);
    wait(RADIO_PAUSE);
  }
  else {
#ifdef MY_DEBUG
    Serial.print("Sending time_elapsed ");
    Serial.print("(");
    Serial.print(0);
    Serial.print(")");
    Serial.print(" status: ");
#endif
    resend(msg12.set(0), radio_retries);
    wait(RADIO_PAUSE);
  }

  wait(LOOP_PAUSE);
}

void setHomeLights(Adafruit_NeoPixel& strip_1, boolean is_green)
{
  for (int i = 4; i < 8; ++i) {
    if (is_green) {
      strip_1.setPixelColor(i, 0, 10, 0);
    }
    else {
      strip_1.setPixelColor(i, 50, 0, 0);
    }
  }
  strip_1.show();
}

void setWaitingLights(Adafruit_NeoPixel& strip_1, int enabled_leds, int total_leds)
{
  for (int i = 0; i < enabled_leds; ++i) {
    strip_1.setPixelColor(i, 0, 10, 0);
  }
  for (int i = enabled_leds; i < total_leds; ++i) {
    strip_1.setPixelColor(i, 50, 0, 0);
  }
  strip_1.show();
}

void resend(MyMessage& msg, int repeats)
{
  int repeat = 0;
  int repeat_delay = 0;
  boolean send_ok = false;
  while ((send_ok == false) and (repeat < radio_retries)) {
    if (send(msg)) {
      send_ok = true;
    }
    else {
      send_ok = false;
      repeat_delay += random(50, 200);
#ifdef MY_DEBUG
      Serial.print(F("Send ERROR "));
      Serial.println(repeat);
#endif
    }
    repeat++;
    wait(repeat_delay);
  }
}
