/*
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

   REVISION HISTORY
   Version 1.0 - Henrik Ekblad

   DESCRIPTION
   Example sketch showing how to create a node thay repeates messages
   from nodes far from gateway back to gateway.
   It is important that nodes that has enabled repeater mode calls
   process() frequently. Repeaters should never sleep.
*/

#define MY_NODE_ID 3
// #define MY_PARENT_NODE_ID 0

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
// #define MY_RADIO_RFM69

// #define MY_RF24_PA_LEVEL RF24_PA_HIGH

// #define MY_RF24_CHANNEL 125

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <MySensor.h>

void setup()
{
}

void presentation()
{
  //Send the sensor node sketch version information to the gateway
  sendSketchInfo("R", "1.0");
}

void loop()
{
}