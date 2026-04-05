#ifndef IRINSTRUCTIONHANDLER_H
#define IRINSTRUCTIONHANDLER_H

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

//include relevant user libraries
#include "globals.h"
#include "SubwooferControls.h"

void handleCode(uint32_t data);

#endif