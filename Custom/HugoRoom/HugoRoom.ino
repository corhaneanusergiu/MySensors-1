/*
  REVISION HISTORY
  Created by Mark Swift
  V1.1 - Cleaned up code
  V1.2 - Added PIR sensor
  V1.3 - Corrected sensor child ID bug
  V1.4 - Removed OTA lines (Not needed) + slight code cleanup
  V1.5 - Refactor code and introduce timer logic
  V1.6 - Added resend function
*/

//*** EXTERNAL LIBRARIES **********************************

#include <DHT.h>
#include <elapsedMillis.h>

//*** MYSENSORS *******************************************

// Enable MY_SPECIAL_DEBUG in sketch to activate I_DEBUG messages if MY_DEBUG is disabled
// I_DEBUG requests are:
// R: routing info (only repeaters): received msg XXYY (as stream), where XX is the node and YY the routing node
// V: CPU voltage
// F: CPU frequency
// M: free memory
// E: clear MySensors EEPROM area and reboot (i.e. "factory" reset)

// Enable debug prints
// #define MY_DEBUG

// Lower serial speed if using 1Mhz clock
// #define MY_BAUD_RATE 9600

#define MY_NODE_ID 1
#define MY_PARENT_NODE_ID 0
#define MY_PARENT_NODE_IS_STATIC

// Time to wait for messages default is 500ms
// #define MY_SMART_SLEEP_WAIT_DURATION_MS (1000ul)

// Transport ready boot timeout default is 0 meaning no timeout
// Set to 30 seconds on battery nodes to avoid excess drainage
// #define MY_TRANSPORT_WAIT_READY_MS (60*1000UL)

// Transport ready loop timeout default is 10 seconds
// Usually left at default but can be extended if required
// #define MY_SLEEP_TRANSPORT_RECONNECT_TIMEOUT_MS (10*1000UL)

// Enable and select radio type attached
#define MY_RADIO_NRF24
// #define MY_RADIO_RFM69

// Override RF24L01 channel number default is 125
// #define MY_RF24_CHANNEL 125

// Override RF24L01 module PA level default is max
// #define MY_RF24_PA_LEVEL RF24_PA_LOW

// Override RF24L01 datarate default is 250Kbps
// #define MY_RF24_DATARATE RF24_250KBPS

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <MySensors.h>

// *** SKETCH CONFIG **************************************

#define SKETCH_NAME "Hugo Bedroom"
#define SKETCH_MAJOR_VER "1"

// Define the sensor child IDs
#define CHILD_ID1 1 // Temperature
#define CHILD_ID2 2 // Humidity
#define CHILD_ID3 3 // Motion

// Define the message formats
MyMessage msg1(CHILD_ID1, V_TEMP);
MyMessage msg2(CHILD_ID2, V_HUM);
MyMessage msg3(CHILD_ID3, V_TRIPPED);

// *** SENSORS CONFIG *************************************

// Motion sensor pin
#define PIR_DIGITAL_PIN 2
// Store last motion status for comparison
boolean last_motion = 0;

// DHT sensor pin
#define DHT_DIGITAL_PIN 3
// Set DHT process name
DHT dht;

// Define timers as global so they will not reset each loop

// Force send updates of temperature and humidity after x milliseconds

// Change to trigger reading update
#define THRESHOLD_TEMPERATURE 2
// Change to trigger reading update
#define THRESHOLD_HUMIDITY 10

// Store last temperature for comparison
float last_temperature;
// Store last humidity for comparison
float last_humidity;

// *** BEGIN **********************************************

void setup()
{
  // Set DHT pin
  dht.setup(DHT_DIGITAL_PIN);
  // Set PIR pin to input
  pinMode(PIR_DIGITAL_PIN, INPUT);
}

void presentation()
{
  // Send the sketch version information to the gateway and controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);
  // Register all sensors to the gateway
  present(CHILD_ID1, S_TEMP);
  present(CHILD_ID2, S_HUM);
  present(CHILD_ID3, S_MOTION);
}

void loop()
{
  // *** DHT SENSOR ***************************************

  wait(dht.getMinimumSamplingPeriod());
  // Fetch temperature from DHT sensor
  float temperature = dht.getTemperature();
  // The absolute difference
  float temperature_difference = temperature - last_temperature;
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
  Serial.print("Temperature timer: ");
#endif
  if (isnan(temperature)) {
    Serial.println("Failed to read temperature");
  }
    last_temperature = temperature;
  }

  // *** DHT SENSOR ****************************************

  // Fetch humidity from DHT sensor
  float humidity = dht.getHumidity();
  // The absolute difference
  float humidity_Difference = humidity - last_humidity;
  // 'abs' is the absolute value, the result is always positive
  humidity_Difference = abs(humidity_Difference);
#ifdef MY_DEBUG
  Serial.print("Humidity: ");
  Serial.println(humidity);
#endif
#ifdef MY_DEBUG
  Serial.print("Humidity difference: ");
  Serial.println(humidity_Difference);
#endif
#ifdef MY_DEBUG
  Serial.print("Humidity timer : ");
#endif
  if (isnan(temperature)) {
    Serial.println("Failed to read humidity");
  }
    last_humidity = humidity;
  }

  // *** PIR SENSOR ****************************************

  // Read digital motion value
  boolean motion = digitalRead(PIR_DIGITAL_PIN) == HIGH;
  if (last_motion != motion) {
#ifdef MY_DEBUG
    Serial.print("Motion: ");
    Serial.println(motion);
#endif
    last_motion = motion;
    // Send motion value value to gateway
    send(msg3.set(motion ? "1" : "0"));
  }
}
