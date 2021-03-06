// Copyright 2015, Matthijs Kooijman <matthijs@stdin.nl>
//
// Permission is hereby granted, free of charge, to anyone
// obtaining a copy of this document and accompanying files, to do
// whatever they want with them without any restriction, including, but
// not limited to, copying, modification and redistribution.
//
// NO WARRANTY OF ANY KIND IS PROVIDED.
//
//
// This example shows how the coordinator Arduino can receive sensor
// data through the XBee radio, and store that data for later processing
// and visualization.
//
// This version of this sketch sends the data to the Beebotte internet
// service, using the MQTT protocol. With some minimal changes to the
// settings and the data format used in publish(), this example can be
// changed to support Adafruit IO as well.
//
// For internet connectivity, the Ethernet shield (or an Arduino
// Ethernet) is used, together with the Ethernet library supplied along
// with the Arduino IDE.
//
// Don't forget to enter the MAC address (printed on a sticker on the
// shield) in the code below.
//
// Any other hardware whose Arduino library implements the "Client"
// interface can be used as well with no changes to the MQTT part.

#include <XBee.h>
#include <Printers.h>
#include <AltSoftSerial.h>
#include "binary.h"
#include <Ethernet.h>
#include <SPI.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

// XBee object
XBeeWithCallbacks xbee;

AltSoftSerial SoftSerial;
#define DebugSerial Serial
#define XBeeSerial SoftSerial

// Enter a MAC address for your controller below. Newer Ethernet
// shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0x00, 0x00, 0x5E, 0x00, 0x53, 0x00 };

// Ethernet client object, handling a single TCP connection
EthernetClient client;

// Store the MQTT server, client ID, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = "mqtt.beebotte.com";
const char MQTT_CLIENTID[] PROGMEM  = "";
const char MQTT_USERNAME[] PROGMEM  = "your_key_here";
const char MQTT_PASSWORD[] PROGMEM  = "";
const int MQTT_PORT = 1883;

// MQTT object using the CC3000 Client object
// Note that this does not use the Adafruit_MQTTO_CC3000 object, which
// pulls in some watchdog timer dependency we don't really need. It
// should be optimized to work with the Adafruit CC3000 library, but
// in practice, the advantages seem to be negligable.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_CLIENTID,
                          MQTT_USERNAME, MQTT_PASSWORD);

// For MQTT debugging: Enable MQTT_DEBUG in Adafruit_MQTT.h

void publish(const __FlashStringHelper *resource, float value) {
  // Use JSON to wrap the data, so Beebotte will remember the data
  // (instead of just publishing it to whoever is currently listening).
  String data;
  data += "{\"data\": ";
  data += value;
  data += ", \"write\": true}";

  DebugSerial.print(F("Publishing "));
  DebugSerial.print(data);
  DebugSerial.print(F( " to "));
  DebugSerial.println(resource);

  // Publish data and try to reconnect when publishing data fails
  if (!mqtt.publish(resource, data.c_str())) {
    DebugSerial.println(F("Failed to publish, trying reconnect..."));
    connect();

    if (!mqtt.publish(resource, data.c_str()))
      DebugSerial.println(F("Still failed to publish data"));
  }
}

void processRxPacket(ZBRxResponse& rx, uintptr_t) {
  Buffer b(rx.getData(), rx.getDataLength());
  uint8_t type = b.remove<uint8_t>();
  XBeeAddress64 addr = rx.getRemoteAddress64();

  if (addr == 0x0013A20040DADEE0 && type == 1 && b.len() == 8) {
      publish(F("Livingroom/Temperature"), b.remove<float>());
      publish(F("Livingroom/Humidity"), b.remove<float>());
      return;
  }

  if (addr == 0x0013A20040E2C832 && type == 1 && b.len() == 8) {
      publish(F("Study/Temperature"), b.remove<float>());
      publish(F("Study/Humidity"), b.remove<float>());
      return;
  }

  DebugSerial.println(F("Unknown or invalid packet"));
  printResponse(rx, DebugSerial);
}

void halt(Print& p, const __FlashStringHelper *s) {
  p.println(s);
  while(true);
}

void connect() {
  client.stop(); // Ensure any old connection is closed
  uint8_t ret = mqtt.connect();
  if (ret == 0)
    DebugSerial.println(F("MQTT connected"));
  else
    DebugSerial.println(mqtt.connectErrorString(ret));
}
void setup() {
  // Setup debug output through USB
  DebugSerial.begin(115200);
  DebugSerial.println(F("Starting..."));

  // Setup XBee through the hardware serial port
  XBeeSerial.begin(9600);
  xbee.begin(XBeeSerial);

  // Set up the ethernet module, using DHCP
  if (Ethernet.begin(mac) == 0)
    halt(DebugSerial, F("Ethernet failed to init"));

  DebugSerial.println(F("Ethernet initialized"));

  connect();
  xbee.onZBRxResponse(processRxPacket);
  xbee.onPacketError(printErrorCb, (uintptr_t)(Print*)&Serial);
}

void loop() {

  // Check the serial port to see if there is a new packet available
  xbee.loop();

  // Keep the Ethernet DHCP lease current, if needed
  Ethernet.maintain();
}
