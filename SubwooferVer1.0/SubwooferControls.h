#ifndef SUBWOOFERCONTROLS_H
#define SUBWOOFERCONTROLS_H

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

//include relevant user libraries
#include "globals.h"

/*Time System Handlers*/
void setSleepTimer(long seconds);

//EEPROM Save Handler
void saveTimesToEEPROM();

//Power controls
void turnAmpOn();
void turnAmpOff();
void powerAmpSwitch();

#endif