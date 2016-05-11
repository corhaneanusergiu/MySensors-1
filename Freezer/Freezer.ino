/*
  REVISION HISTORY
  Created by Mark Swift
  V1.1 - Started code clean up.
  V1.2 - Cleaned up.
*/

#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>

//*** MY SENSORS ******************************************

// Enable debug prints to serial monitor
// #define MY_DEBUG

#define MY_NODE_ID 2
#define MY_PARENT_NODE_ID 1 // AUTO

// Enable and select radio type attached
#define MY_RADIO_NRF24
// #define MY_RADIO_RFM69

// Channel furthest away from Wifi
// #define MY_RF24_CHANNEL 125

// For crappy PA+LNA module
// #define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enabled repeater feature for this node
// #define MY_REPEATER_FEATURE

#include <MySensor.h>

//*** CONFIG **********************************************

// Send temperature only if changed? 1 = Yes 0 = No
#define COMPARE_TEMP 0
// Pin where dallas sensor is connected
#define ONE_WIRE_BUS 3
// The maximum amount of dallas temperatures connected
#define MAX_ATTACHED_DS18B20 16
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass the oneWire reference to Dallas Temperature
DallasTemperature sensors(&oneWire);

float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors = 0;

// Define loop time
#define LOOP_TIME 60000 // Sleep time between reads (in milliseconds)

// Define sensor children
#define CHILD_ID1 1 // Dallas temperature sensor

// Initialise messages
MyMessage msg(CHILD_ID1, V_TEMP);

//*********************************************************

void setup()
{
  sensors.begin(); // Startup up the OneWire library
  sensors.setWaitForConversion(false); // RequestTemperatures() will not block current thread
}

void presentation()
{
  sendSketchInfo("Garage Freezer", "1.1"); // Send the sketch version information to the gateway and Controller
  numSensors = sensors.getDeviceCount(); // Fetch the number of attached temperature sensors
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) // Present all sensors to controller
  {
    present(i, S_TEMP);
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
      send(msg.setSensor(i).set(temperature, 1)); // Send in the new temperature
      wait(500); // If set to sleeping, will still have time to wait for OTA messages...
      lastTemperature[i] = temperature; // Save new temperatures for next compare
    }
    }

sleep(LOOP_TIME); // Sleep or wait (repeater)
}
