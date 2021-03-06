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

// Enable MY_SPECIAL_DEBUG in sketch to activate I_DEBUG messages if MY_DEBUG is disabled
// I_DEBUG requests are:
// R: routing info (only repeaters): received msg XXYY (as stream), where XX is the node and YY the routing node
// V: CPU voltage
// F: CPU frequency
// M: free memory
// E: clear MySensors EEPROM area and reboot (i.e. "factory" reset)
// #define MY_SPECIAL_DEBUG

// Enable debug serial prints
// #define MY_DEBUG

// Lower serial speed if using 1Mhz clock
// #define MY_BAUD_RATE 9600

#define MY_NODE_ID 4
// #define MY_PARENT_NODE_ID 1
// #define MY_PARENT_NODE_IS_STATIC

// Time to wait for messages default is 500ms
// #define MY_SMART_SLEEP_WAIT_DURATION_MS (1000ul)

// Transport ready boot timeout default is 0 meaning no timeout
// Set to 30 seconds on battery nodes to avoid excess drainage
#define MY_TRANSPORT_WAIT_READY_MS (30*1000UL)

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
// #define MY_REPEATER_FEATURE

#include <MySensors.h>

// *** SKETCH CONFIG **************************************

#define SKETCH_NAME "Door Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "5"

// Define the sensor child IDs
#define CHILD_ID1 1 // Switch
#define CHILD_ID2 2 // Battery voltage
#define CHILD_ID3 3 // Battery percent

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
#define SLEEP_IN_MS 86400000UL

// Store the old value for comparison
int old_value = -1;

// Minimum expected vcc level, in volts (2xAA alkaline)
const float vcc_min = 2.0 * 0.9;
// Maximum expected vcc level, in volts (2xAA alkaline)
const float vcc_max = 2.0 * 1.5;
// Measured vcc by multimeter divided by reported vcc
const float vcc_correction = 2.88 / 2.82;
Vcc vcc(vcc_correction);

// Define radio retries upon failure
int radio_retries = 10;


// *** BEGIN **********************************************

void setup()
{
  // Setup the contact sensor
  pinMode(PRIMARY_BUTTON_PIN, INPUT);
  // Activate or deactivate the internal pull-up high if using internal resistor or low if using external
  digitalWrite(PRIMARY_BUTTON_PIN, LOW);
}

void presentation()
{
  // Send the sketch version information to the gateway and controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);
  // Register all sensors to the gateway
  present(CHILD_ID1, S_DOOR);
  present(CHILD_ID2, S_CUSTOM);
  present(CHILD_ID3, S_CUSTOM);
}

void loop()
{
  // Debouce using wait or sleep
  wait(DEBOUNCE_SLEEP);
  int value = digitalRead(PRIMARY_BUTTON_PIN);
  // If value has changed send the updated value
  if (value != old_value) {
    resend(msg.set(value == HIGH ? 0 : 1), radio_retries);
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
  resend(msg2.set(battery_voltage, 2), radio_retries);
  resend(msg3.set(battery_percent, 2), radio_retries);
  sendBatteryLevel(battery_percent);
  // Sleep until either interupt change or timer reaches zero
  smartSleep(PRIMARY_BUTTON_PIN - 2, CHANGE, SLEEP_IN_MS);
}

void resend(MyMessage &msg, int repeats)
{
  int repeat = 1;
  int repeatDelay = 0;
  while ((!send(msg)) and (repeat < repeats)) {
    repeatDelay += 100;
    repeat++;
    wait(repeatDelay);
  }
}
