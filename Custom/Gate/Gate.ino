/*
  REVISION HISTORY
  Created by Mark Swift
  V1.2 - Added sleep time to force daily update
  V1.3 - Deactivate pullup on setup. Added oldValue logic.
*/

//*** EXTERNAL LIBRARIES **********************************

#include <Vcc.h>

//*** MY SENSORS ******************************************

// Enable debug prints to serial monitor
// #define MY_DEBUG

#define MY_NODE_ID 4
#define MY_PARENT_NODE_ID 1 // AUTO
#define MY_PARENT_NODE_IS_STATIC

// For use with 1Mhz modules
// #define MY_BAUD_RATE 9600

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

#define SKETCH_NAME "Door Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "3"

#define CHILD_ID1 1
#define CHILD_ID2 2
#define CHILD_ID3 3

MyMessage msg(CHILD_ID1, V_TRIPPED);
MyMessage msg2(CHILD_ID2, V_CUSTOM);
MyMessage msg3(CHILD_ID3, V_CUSTOM);

// Arduino Digital I/O pin for button or reed switch
#define PRIMARY_BUTTON_PIN 3

// Store the old value for comparison
int oldValue = -1;

// Sleep timer in milliseconds
#define SLEEP_IN_MS 86400000

// Calibrate and setup voltage parameters
const float VccMin = 2.0 * 0.9; // Minimum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VccMax = 2.0 * 1.5; // Maximum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VccCorrection = 2.88 / 2.82; // Measured Vcc by multimeter divided by reported Vcc
Vcc vcc(VccCorrection);

//*** BEGIN ***********************************************

void setup()
{
  // Setup the contact sensor
  pinMode(PRIMARY_BUTTON_PIN, INPUT);

  // Activate or deactivate the internal pull-up / High if using internal resistor, low is using external.
  digitalWrite(PRIMARY_BUTTON_PIN, LOW);
}

void presentation() {
  // Send the sketch version information to the gateway and controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices).
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage.
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  present(CHILD_ID1, S_DOOR);
  present(CHILD_ID2, S_CUSTOM);
  present(CHILD_ID3, S_CUSTOM);
}

// Loop will iterate on changes on the BUTTON_PINs
void loop()
{
#ifdef MY_DEBUG
  Serial.print("Waking");
#endif

  // Short delay to allow inputs to settle
  sleep(5);

  int value = digitalRead(PRIMARY_BUTTON_PIN);

  // If value has changed from last transmission, send the updated value
  if (value != oldValue) {
    send(msg.set(value == HIGH ? 0 : 1 ));
    oldValue = value;
  }

  float v = vcc.Read_Volts();
#ifdef MY_DEBUG
  Serial.print("VCC = ");
  Serial.print(v);
  Serial.println(" Volts");
#endif

  float p = vcc.Read_Perc(VccMin, VccMax);
#ifdef MY_DEBUG
  Serial.print("VCC = ");
  Serial.print(p);
  Serial.println("%");
#endif

  send(msg2.set(v, 2));
  send(msg3.set(p, 1));
  sendBatteryLevel(p);

  // Sleep until something either interupt or timer reaches zero

#ifdef MY_DEBUG
  Serial.print("Sleeping");
#endif
  smartSleep(PRIMARY_BUTTON_PIN - 2, CHANGE, SLEEP_IN_MS);
}
