/*
  REVISION HISTORY
  Created by Mark Swift
  V1.1 - Cleaned up code.
  V1.2 - Added PIR sensor.
  V1.3 - Corrected sensor child ID bug.
*/

#include <SPI.h>
#include <DHT.h>

//*** MY SENSORS ******************************************

// Enable debug prints
// #define MY_DEBUG

#define MY_NODE_ID 1
// #define MY_PARENT_NODE_ID 0 // AUTO

// Enable and select radio type attached
#define MY_RADIO_NRF24
// #define MY_RADIO_RFM69

// Set RF24L01 channel number
// #define MY_RF24_CHANNEL 125

// For crappy PA+LNA module
// #define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <MySensor.h>

//*** CONFIG **********************************************

// Define DHT22 pin
#define DHT_DIGITAL_PIN 3
// Send temperature only if changed? 1 = Yes 0 = No
#define COMPARE_TEMP 0
// Send humidity only if changed? 1 = Yes 0 = No
#define COMPARE_HUM 0
// Set DHT process name
DHT dht;
// Store last temperature for comparisons
float lastTemp;
// Store last humidity for comparisons
float lastHum;
// Set default setting for reading formats
boolean metric = true;

// PIR sensor
#define PIR_DIGITAL_PIN 2 // The digital input you attached your motion sensor

// Define the loop delay
#define LOOP_TIME 30000 // Sleep time between reads (in milliseconds)

// Define the sensor child IDs
#define CHILD_ID1 1 // Temperature
#define CHILD_ID2 2 // Humidity
#define CHILD_ID3 3 // PIR

// Define the message formats
MyMessage msg1(CHILD_ID1, V_TEMP);
MyMessage msg2(CHILD_ID2, V_HUM);
MyMessage msg3(CHILD_ID3, V_TRIPPED);

//*********************************************************

void setup()
{
  dht.setup(DHT_DIGITAL_PIN);
  metric = getConfig().isMetric;
  pinMode(PIR_DIGITAL_PIN, INPUT);
}

void presentation()
{
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Hugo's Room", "1.3");
  // Register all sensors to the gateway (they will be created as child devices)
  present(CHILD_ID1, S_TEMP);
  present(CHILD_ID2, S_HUM);
  present(CHILD_ID3, S_MOTION);
}

void loop()
{

  //*** DHT SENSOR ****************************************

  wait(dht.getMinimumSamplingPeriod()); // Delay or wait (repeater)

  // Fetch temperatures from DHT sensor
  float temperature = dht.getTemperature();

  if (isnan(temperature))
  {
    Serial.println("Failed reading temperature from DHT");
  }
#if COMPARE_TEMP == 1
  else if (temperature != lastTemp)
#endif
  {
    lastTemp = temperature;
    if (!metric)
    {
      temperature = dht.toFahrenheit(temperature);
    }
    send(msg1.set(temperature, 1));
#ifdef MY_DEBUG
    Serial.print("T: ");
    Serial.println(temperature);
#endif
  }

  // Fetch humidity from DHT sensor
  float humidity = dht.getHumidity();

  if (isnan(humidity))
  {
#ifdef MY_DEBUG
    Serial.println("Failed reading humidity from DHT");
#endif
  }
#if COMPARE_HUM == 1
  else if (humidity != lastHum)
#endif
  {
    lastHum = humidity;
    send(msg2.set(humidity, 1));
#ifdef MY_DEBUG
    Serial.print("H: ");
    Serial.println(humidity);
#endif
  }

  //*** PIR SENSOR ****************************************

  // Read digital motion value
  boolean tripped = digitalRead(PIR_DIGITAL_PIN) == HIGH;

#ifdef MY_DEBUG
  Serial.print("PIR: ");
  Serial.println(tripped);
#endif
  send(msg3.set(tripped ? "1" : "0")); // Send tripped value to gateway

  wait(LOOP_TIME); // Sleep or wait (repeater)
}
