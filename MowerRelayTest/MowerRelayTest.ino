/*
  REVISION HISTORY
  Created by Mark Swift
  V1.0 - Initial release.
*/

#include <SPI.h>
#include <MySensor.h>

//*** CONFIG **********************************************

// Enable debug prints to serial monitor
#define MY_DEBUG

#define MY_NODE_ID 99
// #define MY_PARENT_NODE_ID 0 // AUTO

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Set RF24L01+ channel number
// #define MY_RF24_CHANNEL 125

// For crappy PA+LNA module
// #define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enable repeater functionality for this node
// #define MY_REPEATER_FEATURE

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


