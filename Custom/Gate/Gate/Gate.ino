/**
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.

   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2015 Sensnology AB
   Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors

   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

 *******************************

   DESCRIPTION

   Interrupt driven binary switch example with dual interrupts
   Author: Patrick 'Anticimex' Fallberg
   Connect one button or door/window reed switch between
   digitial I/O pin 3 (BUTTON_PIN below) and GND and the other
   one in similar fashion on digital I/O pin 2.
   This example is designed to fit Arduino Nano/Pro Mini

*/

// Enable debug prints to serial monitor
#define MY_DEBUG

#define MY_NODE_ID 4
#define MY_PARENT_NODE_ID 1 // 1 / AUTO
#define MY_PARENT_NODE_IS_STATIC
// #define MY_BAUD_RATE 9600 // For us with 1Mhz modules

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>
#include <Vcc.h>

#define SKETCH_NAME "Door Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "1"

#define CHILD_ID1 1
#define CHILD_ID2 2
#define CHILD_ID3 3

#define PRIMARY_BUTTON_PIN 3   // Arduino Digital I/O pin for button / reed switch

MyMessage msg(CHILD_ID1, V_TRIPPED);
MyMessage msg2(CHILD_ID2, V_CUSTOM);
MyMessage msg3(CHILD_ID3, V_CUSTOM);

const float VccMin        = 2.0 * 0.9; // Minimum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VccMax        = 2.0 * 1.5; // Maximum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VccCorrection = 2.88 / 2.82; // Measured Vcc by multimeter divided by reported Vcc
Vcc vcc(VccCorrection);

uint8_t value;

void setup()
{
  // Setup the contact sensor
  pinMode(PRIMARY_BUTTON_PIN, INPUT);

  // Activate internal pull-ups
  // digitalWrite(PRIMARY_BUTTON_PIN, HIGH);
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
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
  // uint8_t value;
  // static uint8_t sentValue = 2;

  // Short delay to allow buttons to properly settle
  sleep(5);

  value = digitalRead(PRIMARY_BUTTON_PIN);

  // if (value != sentValue) {
  // Value has changed from last transmission, send the updated value
  send(msg.set(value == HIGH ? 0 : 1 ));
  // sentValue = value;
  //}

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

  // Sleep until something happens with the sensor

#ifdef MY_DEBUG
  Serial.print("Sleeping");
#endif
  smartSleep(PRIMARY_BUTTON_PIN - 2, CHANGE, 0);
}
