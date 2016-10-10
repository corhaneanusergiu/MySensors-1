/*
  REVISION HISTORY
  Created by Mark Swift
  V1.0 - Initial release.
*/

//*** MY SENSORS ******************************************

// Enable debug prints to serial monitor
#define MY_DEBUG

#define MY_NODE_ID 99
#define MY_PARENT_NODE_ID AUTO // AUTO
// #define MY_PARENT_NODE_IS_STATIC
// #define MY_BAUD_RATE 9600 // For us with 1Mhz modules

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Set RF24L01+ channel number
// #define MY_RF24_CHANNEL 125

// For crappy PA+LNA module
// #define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enable repeater functionality for this node
// #define MY_REPEATER_FEATURE

// Enables OTA firmware updates
// #define MY_OTA_FIRMWARE_FEATURE

#include <MySensors.h>

//*********************************************************

void setup()
{
}

void loop()
{

  // *** RELAY TRIGGER TEST **********************************************

  send(MyMessage(1, V_LIGHT).setDestination(4).set(true));
  delay(2000);
  send(MyMessage(1, V_LIGHT).setDestination(4).set(false));
  delay(2000);

  Serial.print("Node ID: ");
  Serial.println(getNodeId());

}
