#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

//include relevant user libraries
#include "globals.h"
#include "SubwooferControls.h"
#include "API.h"
#include "HttpTypes.h"

class WebServer{
  private:
    WebAPI webAPI;
    PowerAPI powerAPI;
    PowerTimerAPI powerTimerAPI;

    const int numApis = 3;
    API *ServerApis[numApis];
    WiFiClient client;
  public:
    //constructor
    WebServer();
    void updateClient(WiFiClient &Client){
      client = Client;
    }
    headersHttp handleHeaders();
    bool hasBody(const String& methodHttp){
      return methodHttp == "POST" || methodHttp == "PUT" || methodHttp == "PATCH";
    }
    bool pathExists(const String& pathHttp);
    void handleRequest();
};
#endif