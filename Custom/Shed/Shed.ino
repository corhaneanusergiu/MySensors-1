/*
  REVISION HISTORY
  Created by Mark Swift
  V1.1 - Started code clean up
  V1.2 - Cleaned up
  V1.3 - Changed sleep to smartSleep to allow for future developments // Cancelled for now as doesn't work so well!
  V2.0 - Integrated moisture sensor and optimised code
  V2.1 - Minor formatting changes
  V3.0 - Refactor and added timer logic
  V3.1 - Updated gateway metric logic
*/

//*** EXTERNAL LIBRARIES **********************************

#include <DallasTemperature.h>
#include <OneWire.h>
#include <elapsedMillis.h>

//*** MY SENSORS ******************************************

// Enable debug prints
// #define MY_DEBUG

// Lower serial speed if using 1Mhz clock
// #define MY_BAUD_RATE 9600

#define MY_NODE_ID 2
#define MY_PARENT_NODE_ID AUTO // AUTO
// #define MY_PARENT_NODE_IS_STATIC

// Enable and select radio type attached
#define MY_RADIO_NRF24
// #define MY_RADIO_RFM69

// Override RF24L01 channel number
// #define MY_RF24_CHANNEL 125

// Override RF24L01 module PA level
// #define MY_RF24_PA_LEVEL RF24_PA_LOW

// Override RF24L01 datarate
// #define MY_RF24_DATARATE RF24_250KBPS

// Enabled repeater feature for this node
// #define MY_REPEATER_FEATURE

#include <MySensors.h>

// *** SKETCH CONFIG **************************************

#define SKETCH_NAME "Shed"
#define SKETCH_MAJOR_VER "3"
#define SKETCH_MINOR_VER "1"

// Define the sensor child IDs
#define CHILD_ID1 1 // Temperature
#define CHILD_ID2 2 // Moisture

// Define the message formats
MyMessage msg1(CHILD_ID1, V_TEMP);
MyMessage msg2(CHILD_ID2, V_LEVEL);

// Set default gateway value for metric or imperial
boolean metric = true;

// *** SENSORS CONFIG *************************************

// Pin where dallas sensor is connected
#define ONE_WIRE_BUS 3
// The maximum amount of dallas temperatures connected
#define MAX_ATTACHED_DS18B20 16
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass the oneWire reference to Dallas Temperature
DallasTemperature sensors(&oneWire);
// Set default sensors attached
int num_sensors = 1;

// Analog input pin moisture sensor
#define ANALOG_INPUT_MOISTURE A0
// Moisture sensor power pin
#define MOISTURE_POWER_PIN 8
// Store last moisture reading for comparison
int last_moisture_value = -1;
// Anolog smoothing, the number of samples to keep track of
const int num_readings = 10;
// The readings from the analog input
int reading[num_readings];
// The index of the current reading
int read_index = 0;
// The running total reading
int total = 0;
// The average reading
int average = 0;
// Only start sending readings once smoothing is complete
boolean readings_ready = false;

// Define timers as global so they will not reset each loop
elapsedMillis time_elapsed_temperature;
elapsedMillis time_elapsed_moisture;

// Force send updates of temperature and humidity after x milliseconds
#define TIMER_TEMPERATURE 600000
#define TIMER_MOISTURE 600000

// Change to trigger reading update
#define THRESHOLD_TEMPERATURE 2
// Change to trigger reading update
#define THRESHOLD_MOISTURE 10

// Store last temperature for comparison
float last_temperature[MAX_ATTACHED_DS18B20];
// Store last humidity for comparison
float last_moisture;

// Define radio retries upon failure
int radio_retries = 10;

//*********************************************************

void before()
{
  // Startup up the OneWire library
  sensors.begin();
}

void setup()
{
  // RequestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
  // Set the moisture sensor power pin as output
  pinMode(MOISTURE_POWER_PIN, OUTPUT);
  // Set to LOW so no power is flowing through the moisture sensor
  digitalWrite(MOISTURE_POWER_PIN, LOW);
  // Check gateway for metric setting
  boolean metric = getConfig().isMetric;
}

void presentation()
{
  // Send the sketch version information to the gateway and controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);
  // Fetch the number of attached temperature sensors
  num_sensors = sensors.getDeviceCount();
  // Present all sensors to controller
  for (int i = 0; i < num_sensors && i < MAX_ATTACHED_DS18B20; i++) {
    present(i, S_TEMP);
    present(CHILD_ID2, S_MOISTURE);
  }
}

void loop()
{
  //*** TEMP SENSOR ***************************************

  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();
  // Query conversion time and sleep until conversion completed
  int16_t conversion_time = sensors.millisToWaitForConversion(sensors.getResolution());
  // Sleep can be replaced by wait if node needs to process incoming messages or if node is repeater
  wait(conversion_time);
  // Read temperatures and send them to gateway and controller
  for (int i = 0; i < num_sensors && i < MAX_ATTACHED_DS18B20; i++) {
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((metric ? sensors.getTempCByIndex(i) : sensors.getTempFByIndex(i)) * 10.)) / 10.;
    // The absolute difference
    float temperature_difference = temperature - last_temperature[i];
    // 'abs' is the absolute value, the result is always positive
    temperature_difference = abs(temperature_difference);
#ifdef MY_DEBUG
    Serial.print("Temperature: ");
    Serial.println(temperature);
#endif
#ifdef MY_DEBUG
    Serial.print("Temperature difference: ");
    Serial.println(temperature_difference);
#endif
#ifdef MY_DEBUG
    Serial.print("Temperature timer : ");
    Serial.println(time_elapsed_temperature / 1000);
#endif
    if (temperature_difference > THRESHOLD_TEMPERATURE && temperature != -127.00 && temperature != 85.00 || time_elapsed_temperature > TIMER_TEMPERATURE) {
      last_temperature[i] = temperature;
      time_elapsed_temperature = 0;
      resend(msg1.setSensor(i).set(temperature, 1), radio_retries);
    }
  }

  //*** MOISTURE SENSOR ***********************************

  // Subtract the last smoothing reading
  total = total - reading[read_index];
  // Turn moisture power pin on
  digitalWrite(MOISTURE_POWER_PIN, HIGH);
  // Set a delay to ensure the moisture sensor has settled
  wait(200);
  // Read analog moisture value
  reading[read_index] = analogRead(ANALOG_INPUT_MOISTURE);
  // Turn moisture power pin off to prevent oxidation
  digitalWrite(MOISTURE_POWER_PIN, LOW);
#ifdef MY_DEBUG
  Serial.print("Moisture Now: ");
  Serial.println(reading[read_index]);
#endif
  // Add the reading to the smoothing total
  total = total + reading[read_index];
  // Advance to the next position in the smoothing array
  read_index = read_index + 1;
  // If we're at the end of the array...
  if (read_index >= num_readings) {
    readings_ready = true;
    // Wrap around to the beginning
    read_index = 0;
  }
  if (readings_ready) {
    average = total / num_readings;
  }
  // The absolute difference
  int moisture_difference = average - last_moisture_value;
  // 'abs' is the absolute value, the result is always positive
  moisture_difference = abs(moisture_difference);
#ifdef MY_DEBUG
  Serial.print("Moisture Avg: ");
  Serial.println(average);
#endif
  if (moisture_difference > THRESHOLD_MOISTURE || time_elapsed_moisture > TIMER_MOISTURE) {
    last_moisture_value = average;
    time_elapsed_moisture = 0;
    resend(msg2.set(average), radio_retries);
  }
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
