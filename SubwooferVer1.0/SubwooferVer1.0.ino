#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
//user libraries
#include "SubwooferControls.h"
#include "IRInstructionHandler.h"
#include "WebServer.h"
#include "keys.h"
/*Defines key values in the following format:
#ifndef KEYS_H
#define KEYS_H

#define STASSID "WIFI NAME"
#define STAPSK "WIFI PASSWORD"
#define AUTH "Basic username:password" //encoded in base64

#endif
*/
/*options*/
#define _IR_ENABLE_DEFAULT_ false
#define DECODE_JVC true

/*global objects*/
WiFiServer espServer(80); /* Instance of WiFiServer with port number 80 */
/* 80 is the Port Number for HTTP Web Server */
WebServer webServer;
WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp, "pool.ntp.org");

/*irrecv declaration*/
IRrecv irrecv(D5);
decode_results results;

/*global values for information*/
bool powerOn = false;
unsigned long timerEnd = 0;
//could be dynamic allocations to improve options, just only let there be a max of 10 or something.
unsigned long scheduleOffTime = 0;
unsigned long scheduleOnTime = 0;

unsigned long timerEndAtLastUpdate = 0;
//could be dynamic allocations to improve options, just only let there be a max of 10 or something.
unsigned long scheduleOffTimeAtLastUpdate = 0;
unsigned long scheduleOnTimeAtLastUpdate = 0;

//IR sleep button handler
unsigned long timeSleepButton = 0;
unsigned int numSleepButtonPresses = 0;

const char* ssid = STASSID;
const char* password = STAPSK;

void setup() {
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D5, INPUT);
  digitalWrite(D1, LOW); 
  digitalWrite(D2, LOW);
  EEPROM.begin(512);
  //
  EEPROM.get(0, timerEnd);
  EEPROM.get(4, scheduleOffTime);
  EEPROM.get(8, scheduleOnTime);
  timerEndAtLastUpdate = timerEnd;
  scheduleOffTimeAtLastUpdate = scheduleOffTime;
  scheduleOnTimeAtLastUpdate = scheduleOnTime;
  Serial.begin(115200);
  irrecv.enableIRIn();
  delay(5000);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("OTA Service Ready");
  Serial.print("\n");
  Serial.println("Starting Time Client...");
  timeClient.begin();
  Serial.println("NTP Time Client Started");
  Serial.print("\n");
  Serial.println("Starting ESP8266 Web Server...");
  espServer.begin(); /* Start the HTTP web Server */
  Serial.println("ESP8266 Web Server Started");
  Serial.print("\n");
  Serial.print("The URL of ESP8266 Web Server is: ");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.print("\n");
  Serial.println("Use the above URL in your Browser to access ESP8266 Web Server, or to process HTTP requests\n");
}

void loop() {
  /*handle potential OTA updates*/
  ArduinoOTA.handle();
  /*handle timer constraints*/
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  if (timerEnd != 0 && now >= timerEnd)
    turnAmpOff();
  //IR is good for now, but COMPU-LINK compatibility is the gold standard
  //Will allow the device to move in lock-step with the receiver, which is the ultimate goal.
  /*same for any schedule evaluations*/
  /*first, decode any ir inputs*/
  if (irrecv.decode(&results)) {
    handleCode(results.command);
    irrecv.resume();
  }

  //compare time to last button press time, if 5 seconds have elapsed set time to time recieved.
  if ((now - timeSleepButton) > 5 && timeSleepButton != 0){
    unsigned long timeUntilEnd = 0;
    timeSleepButton = 0;
    if (numSleepButtonPresses == 1){ //only to display time on receiver, so just put back current time.
      //i.e. do nothing
    }
    else if (timerEnd == 0){ //timer initialization step
      unsigned int totalSteps = numSleepButtonPresses-1;
      setSleepTimer((totalSteps)*600);
    }
    else{ //timer is initialized, so work from there
      timeUntilEnd = timerEnd - now;
      unsigned int remainingSteps = (timeUntilEnd + 599) / 600;
      //second press consumed to inputing "round-up" time.
      unsigned int totalSteps = numSleepButtonPresses-2 + remainingSteps;
      setSleepTimer((totalSteps)*600);
    }
    //if 1 do nothing (good)
    //if 2 round up to nearest 10
    //beyond handle normally (adding 10 each time)
    numSleepButtonPresses = 0;
  }

  /*handle any API calls (to do: make this a class or seperate function structure)*/
  WiFiClient client = espServer.available(); /* Check if a client is available */
  if(!client)
  {
    return;
  }
  Serial.println("New Client!!!");
  /*WebServer class handles request parsing, and dispatches relevant API pathway and method*/
  webServer.updateClient(client);
  webServer.handleRequest();
}
