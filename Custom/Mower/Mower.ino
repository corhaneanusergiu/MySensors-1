/*
  REVISION HISTORY
  Created by Mark Swift
  V1.0 - Initial release.
  V1.1 - Messages only for the correct sensor ID are processed.
*/

//*** MY SENSORS ******************************************

// Enable debug prints to serial monitor
#define MY_DEBUG

#define MY_NODE_ID 5
#define MY_PARENT_NODE_ID AUTO // AUTO
// #define MY_PARENT_NODE_IS_STATIC
// #define MY_BAUD_RATE 9600 // For us with 1Mhz modules

// Enable and select radio type attached
#define MY_RADIO_NRF24
// #define MY_RADIO_RFM69

// Set RF24L01+ channel number
// #define MY_RF24_CHANNEL 125

// For crappy PA+LNA module
// #define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enable repeater functionality for this node
// #define MY_REPEATER_FEATURE

// Enables OTA firmware updates
// #define MY_OTA_FIRMWARE_FEATURE

#include <MySensors.h>

//*** CONFIG **********************************************

// Define relay settings
#define RELAY_PIN 3 // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define RELAY_ON LOW // GPIO value to write to turn on attached relay
#define RELAY_OFF HIGH // GPIO value to write to turn off attached relay

// Define sensor children IDs for MySensors
#define CHILD_ID1 1 // Relay

// Define MySensors message types
MyMessage msg1(CHILD_ID1, V_LIGHT);

// EPPROM variable storage
bool lastState;

//*********************************************************

void setup()
{
  // Make sure relays are off when starting up
  digitalWrite(RELAY_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_PIN, OUTPUT);

  // Set relay to last known state (using eeprom storage)
  lastState = loadState(CHILD_ID1);
  digitalWrite(RELAY_PIN, lastState ? RELAY_ON : RELAY_OFF);

  // Send message to gateway to sync status (to be tested!)
  send(msg1.set(lastState ? 1 : 0));
#ifdef MY_DEBUG
  Serial.print("Sending status to controller");
#endif
}

void presentation()
{
  // Send the sketch version information to the gateway and controller
  sendSketchInfo("Mower", "1.1");
  // Register all sensors to the gateway (they will be created as child devices)
  present(CHILD_ID1, S_LIGHT);
}

void loop()
{
}

void receive(const MyMessage &message)
{
  // We only expect one type of message from controller, but we better check anyway
  if (message.type == V_LIGHT)
    // if (message.type == V_LIGHT && message.sensor == 1)
  {
    // Change relay state
    lastState = message.getBool();
    digitalWrite(RELAY_PIN, lastState ? RELAY_ON : RELAY_OFF);
    // Store state in eeprom
    saveState(CHILD_ID1, lastState);
  }

  // Write some debug info
#ifdef MY_DEBUG
  Serial.print("Incoming change for sensor:");
  Serial.print(message.sensor);
  Serial.print(", New status: ");
  Serial.println(message.getBool());
#endif
}
