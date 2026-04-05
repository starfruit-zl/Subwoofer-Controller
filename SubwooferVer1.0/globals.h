#ifndef GLOBALS_H
#define GLOBALS_H
//external global variables
extern WiFiServer espServer; /* Instance of WiFiServer with port number 80 */
/* 80 is the Port Number for HTTP Web Server */
extern WiFiUDP ntpUdp;
extern NTPClient timeClient;
extern bool powerOn;

extern unsigned long timerEnd;
extern unsigned long scheduleOffTime;
extern unsigned long scheduleOnTime;

extern unsigned long timerEndAtLastUpdate;
extern unsigned long scheduleOffTimeAtLastUpdate;
extern unsigned long scheduleOnTimeAtLastUpdate;

//modified in handleCode(command) and in main loop.
extern unsigned long timeSleepButton;
extern unsigned int numSleepButtonPresses;
//http API globals
extern const size_t numPaths;
//redefine as needed
extern const char validPaths[][16];

#endif