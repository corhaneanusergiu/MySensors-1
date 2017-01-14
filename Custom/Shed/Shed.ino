/*
  REVISION HISTORY
  Created by Mark Swift
  V1.1 - Started code clean up.
  V1.2 - Cleaned up.
  V1.3 - Changed sleep to smartSleep to allow for future developments // Cancelled for now as doesn't work so well!
  V2.0 - Integrated moisture sensor and optimised code.
*/

#include <DallasTemperature.h>
#include <OneWire.h>

//*** MY SENSORS ******************************************

// Enable debug prints to serial monitor
// #define MY_DEBUG

#define MY_NODE_ID 2
// #define MY_PARENT_NODE_ID 0 // AUTO
// #define MY_PARENT_NODE_IS_STATIC
// #define MY_BAUD_RATE 9600 // For us with 1Mhz modules

// Enable and select radio type attached
#define MY_RADIO_NRF24
// #define MY_RADIO_RFM69

// Channel furthest away from Wifi
// #define MY_RF24_CHANNEL 125

// For crappy PA+LNA module
// #define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enabled repeater feature for this node
// #define MY_REPEATER_FEATURE

#include <MySensors.h>

//*** CONFIG **********************************************

// Define radio retries upon failure
int radioRetries = 10;

// DS18B20 sensor settings
#define COMPARE_TEMP 0 // Send temperature only if changed? 1 = Yes 0 = No
#define ONE_WIRE_BUS 3 // Pin where dallas sensor is connected
#define MAX_ATTACHED_DS18B20 16 // The maximum amount of dallas temperatures connected
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors = 1;

// Analog moisture settings
#define ANALOG_INPUT_MOISTURE A0 // Analog input pin moisture sensor
#define MOISTURE_POWER_PIN 8 // Moisture sensor power pin
#define COMPARE_MOISTURE 0 // Send only if changed? 1 = Yes 0 = No
int lastMoistureValue = -1; // Store last moisture reading for comparison
const int numReadings = 10; // Anolog smoothing, the number of samples to keep track of
int reading[numReadings]; // The readings from the analog input
int readIndex = 0; // The index of the current reading
int total = 0; // The running total reading
int average = 0; // The average reading
boolean readingsReady = false; // Only start sending readings once smoothing is complete

// Define loop time
#define LOOP_TIME 60000 // Sleep time between reads (in milliseconds)

// Define sensor children
#define CHILD_ID1 1 // Dallas temperature sensor
#define CHILD_ID2 2 // Moisture Analog Reading

// Initialise messages
MyMessage msg1(CHILD_ID1, V_TEMP);
MyMessage msg2(CHILD_ID2, V_LEVEL);

//*********************************************************

void before()
{
  // Startup up the OneWire library
  sensors.begin();
}

void setup()
{
  sensors.setWaitForConversion(false); // RequestTemperatures() will not block current thread
  // Set the moisture sensor power pin as output
  pinMode(MOISTURE_POWER_PIN, OUTPUT);
  // Set to LOW so no power is flowing through the moisture sensor
  digitalWrite(MOISTURE_POWER_PIN, LOW);
}

void presentation()
{
  sendSketchInfo("Shed", "2.0"); // Send the sketch version information to the gateway and Controller
  numSensors = sensors.getDeviceCount(); // Fetch the number of attached temperature sensors
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) // Present all sensors to controller
  {
    present(i, S_TEMP);
    present(CHILD_ID2, S_MOISTURE);
  }
}

void loop()
{

  //*** DS18B20 *******************************************

  sensors.requestTemperatures(); // Fetch temperatures from Dallas sensors
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution()); // Query conversion time and sleep until conversion completed
  wait(conversionTime); // Sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) // Read temperatures and send them to controller
  {
    float temperature = static_cast<float>(static_cast<int>((getConfig().isMetric ? sensors.getTempCByIndex(i) : sensors.getTempFByIndex(i)) * 10.)) / 10.; // Fetch and round temperature to one decimal
#if COMPARE_TEMP == 1 // Only send data if temperature has changed and no error
    if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00)
    {
#else
    if (temperature != -127.00 && temperature != 85.00)
    {
#endif
#ifdef MY_DEBUG
      Serial.print("Temperature: ");
      Serial.println(temperature);
#endif
      resend(msg1.setSensor(i).set(temperature, 1), radioRetries); // Send in the new temperature
      wait(500); // If set to sleeping, will still have time to wait for OTA messages...
      lastTemperature[i] = temperature; // Save new temperatures for next compare
    }
  }

  //*** MOISTURE SENSOR ANALOG ******************************

  total = total - reading[readIndex]; // Subtract the last smoothing reading
  digitalWrite(MOISTURE_POWER_PIN, HIGH); // Turn moisture power pin on
  wait(200); // Set a delay to ensure the moisture sensor has powered up
  reading[readIndex] = analogRead(ANALOG_INPUT_MOISTURE); // Read analog moisture value
  digitalWrite(MOISTURE_POWER_PIN, LOW); // Turn moisture power pin off
#ifdef MY_DEBUG
  Serial.print("Moisture Now: ");
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
  }
#if COMPARE_MOISTURE == 1
  if (average != lastMoistureValue)
#endif
  {
#ifdef MY_DEBUG
    Serial.print("Moisture Avg: ");
    Serial.println(average);
#endif
    {
      resend(msg2.set(average), radioRetries);
      // For testing can be 0 or 1 or back to moistureValue
      lastMoistureValue = average;
    }
  }

  wait(LOOP_TIME); // Sleep or wait (repeater)
}

void resend(MyMessage &msg, int repeats)
{
  int repeat = 0;
  int repeatdelay = 0;
  boolean sendOK = false;

  while ((sendOK == false) and (repeat < radioRetries))
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
      repeatdelay += random(50, 200);
    }
    repeat++;
    delay(repeatdelay);
  }
}
