/*
  REVISION HISTORY
  Created by Mark Swift
  V1.2 - Added sleep time to force daily update
  V1.3 - Deactivate pullup on setup and added old_value logic
  V1.4 - Added daily wakeup to sleep
  V1.5 - Code cleanup
*/

//*** EXTERNAL LIBRARIES **********************************

#include <Vcc.h>

//*** MYSENSORS *******************************************

// Enable debug prints
// #define MY_DEBUG

// Lower serial speed if using 1Mhz clock
// #define MY_BAUD_RATE 9600

#define MY_NODE_ID 4
#define MY_PARENT_NODE_ID 1 // AUTO
#define MY_PARENT_NODE_IS_STATIC

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

#define SKETCH_NAME "Door Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "5"

// Define the sensor child IDs
#define CHILD_ID1 1
#define CHILD_ID2 2
#define CHILD_ID3 3

// Define the message formats
MyMessage msg(CHILD_ID1, V_TRIPPED);
MyMessage msg2(CHILD_ID2, V_CUSTOM);
MyMessage msg3(CHILD_ID3, V_CUSTOM);

// *** SENSORS CONFIG *************************************

// Digital I/O pin for button or reed switch
#define PRIMARY_BUTTON_PIN 3

// Debounce using a small sleep
#define DEBOUNCE_SLEEP 5

// Sleep timer in milliseconds
#define SLEEP_IN_MS 86400000

// Store the old value for comparison
int old_value = -1;

// Minimum expected vcc level, in volts (2xAA alkaline)
const float vcc_min = 2.0 * 0.9;
// Maximum expected vcc level, in volts (2xAA alkaline)
const float vcc_max = 2.0 * 1.5;
// Measured vcc by multimeter divided by reported vcc
const float vcc_correction = 2.88 / 2.82;
Vcc vcc(vcc_correction);

// *** BEGIN **********************************************

void setup() {
  // Setup the contact sensor
  pinMode(PRIMARY_BUTTON_PIN, INPUT);
  // Activate or deactivate the internal pull-up - high if using internal resistor or low if using external
  digitalWrite(PRIMARY_BUTTON_PIN, LOW);
}

void presentation() {
  // Send the sketch version information to the gateway and controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);
  // Register all sensors to the gateway
  present(CHILD_ID1, S_DOOR);
  present(CHILD_ID2, S_CUSTOM);
  present(CHILD_ID3, S_CUSTOM);
}

void loop() {
  // Debouce using sleep
  sleep(DEBOUNCE_SLEEP);
  int value = digitalRead(PRIMARY_BUTTON_PIN);
  // If value has changed send the updated value
  if (value != old_value) {
    send(msg.set(value == HIGH ? 0 : 1 ));
    old_value = value;
  }
  // Read battery voltage
  float battery_voltage = vcc.Read_Volts();
  // Calculate battery percentage
  float battery_percent = vcc.Read_Perc(vcc_min, vcc_max);
#ifdef MY_DEBUG
  Serial.print("Battery = ");
  Serial.print(battery_voltage);
  Serial.println(" Volts");
  Serial.print("Battery = ");
  Serial.print(battery_percent);
  Serial.println("%");
#endif
  // Send battery readings
  send(msg2.set(battery_voltage, 2));
  send(msg3.set(battery_percent, 1));
  sendBatteryLevel(battery_percent);
  // Sleep until either interupt change or timer reaches zero
  smartSleep(PRIMARY_BUTTON_PIN - 2, CHANGE, SLEEP_IN_MS);
}
