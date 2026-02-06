#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
//user libraries
#include "keys.h"
/*Defines key values in the following format:
#ifndef KEYS_H
#define KEYS_H

#define STASSID "WIFI NAME"
#define STAPSK "WIFI PASSWORD"
#define AUTH "Basic username:password" //encoded in base64

#endif
*/

const char* ssid = STASSID;
const char* password = STAPSK;

WiFiServer espServer(80); /* Instance of WiFiServer with port number 80 */
/* 80 is the Port Number for HTTP Web Server */
WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp, "pool.ntp.org");

struct headersHttp{
  int contentLength = 0;
  char contentType[34] = {0};
  bool loginSuccess = false;
};

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

/*Important values that change depending on server capabilities*/
/*API Web Server Handlers, could be its own library*/
const int numPaths = 3;
const char validPaths[numPaths][16] = {"/", "/power", "/power/timer"};
/*To add, "/power/schedule"*/

headersHttp handleHeaders(WiFiClient &client){
  
  const char* expectedAuth = AUTH;//basic auth
  
  headersHttp retHeaders;
  String line;
  unsigned long start = millis();

  while(true){
    if (millis() - start > 2000) break;
    line = client.readStringUntil('\r');
    client.readStringUntil('\n');

    if (line.length() == 0) break; /*end of headers*/

    if (line.startsWith("Content-Length:"))
      retHeaders.contentLength = line.substring(16).toInt();

    if (line.startsWith("Content-Type:")){
      strncpy(retHeaders.contentType, line.substring(14).c_str(), sizeof(retHeaders.contentType)-1);
      retHeaders.contentType[sizeof(retHeaders.contentType) - 1] = '\0';
    }
    if (line.startsWith("Authorization:")){
      String auth = line.substring(15);
      auth.trim();

      if (auth==expectedAuth) 
        retHeaders.loginSuccess = true;
    }
  }

  return retHeaders;
}

bool hasBody(const String& methodHttp){
  return methodHttp == "POST" || methodHttp == "PUT" || methodHttp == "PATCH";
}

bool pathExists(const String& pathHttp){
  for (int i = 0; i < numPaths; i++)
    if (pathHttp == validPaths[i]) return true;

  return false;
}

/*Time System Handlers*/
void setSleepTimer(long seconds){
  if (seconds == 0) timerEnd = 0;
  else timerEnd = timeClient.getEpochTime() + (unsigned long)seconds;
  //we know that timer for sure is changed.
  saveTimesToEEPROM();
}

//remoteIR will have a different handler/system based off incremental presses where if not pressed within a certain amount of time will not update.
//first press ignored, then subsequent within 5 seconds are counted. Can be updated and added to current timer in same fashion. Only update eeprom when time expires.
void saveTimesToEEPROM(){
  bool update = false;
  if (timerEnd != timerEndAtLastUpdate){
    EEPROM.put(0, timerEnd);
    timerEndAtLastUpdate = timerEnd;
    update = true;
  }
  if (scheduleOffTime != scheduleOffTimeAtLastUpdate){
    EEPROM.put(4, scheduleOffTime);
    scheduleOffTimeAtLastUpdate = scheduleOffTime;
    update = true;
  }
  if (scheduleOnTime != scheduleOnTimeAtLastUpdate){
    EEPROM.put(8, scheduleOnTime);
    scheduleOnTimeAtLastUpdate = scheduleOnTime;
    update = true;
  }

  if(update)
    EEPROM.commit();
}

/*Internal system controls*/

void turnAmpOn(){
  digitalWrite(5, HIGH);
  powerOn = true;
  //if a timer is set
  if (timerEnd != 0){
    timerEnd = 0;
    saveTimesToEEPROM();
  }
}

void turnAmpOff(){
  digitalWrite(5, LOW);
  powerOn = false;
  //if a timer is set
  if (timerEnd != 0){
    timerEnd = 0;
    saveTimesToEEPROM();
  }
}

void setup() {
  pinMode(5, OUTPUT);
  pinMode(0, INPUT);
  digitalWrite(5, LOW); 
  EEPROM.begin(512);
  //
  EEPROM.get(0, timerEnd);
  EEPROM.get(4, scheduleOffTime);
  EEPROM.get(8, scheduleOnTime);
  timerEndAtLastUpdate = timerEnd;
  scheduleOffTimeAtLastUpdate = scheduleOffTime;
  scheduleOnTimeAtLastUpdate = scheduleOnTime;
  Serial.begin(115200);
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
  /*same for any schedule evaluations*/
  /*handle any API calls*/
  WiFiClient client = espServer.available(); /* Check if a client is available */
  if(!client)
  {
    return;
  }
  
  Serial.println("New Client!!!");
  /*Example requests:
   * GET:
  GET /GPIO4ON HTTP/1.1
  Host: 192.168.1.6
  Connection: keep-alive
  *  POST:
  POST /gpio HTTP/1.1
  Host: 192.168.1.6
  Content-Type: application/x-www-form-urlencoded
  Content-Length: 13

  pin=4&on=1
  */
  String request = client.readStringUntil('\r'); /* Read the first line of the request from client */
  client.readStringUntil('\n');
  Serial.println(request); /* Print the request on the Serial monitor */
  /*Now handle parsing. First line will contain method and path. Depending on method, handle content.*/
  int splitIndex = request.indexOf(' ');
  int endPath = request.indexOf(' ', splitIndex + 1);
  Serial.print("DEBUG: index of first split at ");
  Serial.print(splitIndex);
  Serial.print(" and end of path at ");
  Serial.println(endPath);
  /*error handling for null string?*/
  if (splitIndex == -1 || endPath == -1) {
    client.println("HTTP/1.1 400 Bad Request");
    client.println("Connection: close");
    client.stop();
    return;
  }
  String methodHttp = request.substring(0, splitIndex);
  String pathHttp = request.substring(splitIndex + 1, endPath);
  Serial.println("The request method is " + methodHttp + " with path of " + pathHttp);
  /*handle header data line by line, important parameters defined below:*/
  headersHttp requestHeaders = handleHeaders(client);

  String body;
  if (hasBody(methodHttp) && requestHeaders.contentLength > 0){
    unsigned long start = millis();
    while(body.length() < requestHeaders.contentLength){
      if (client.available())
        body += char(client.read());
      //timeout for request:
      if (millis() - start > 2000) break;
    }
  }
  Serial.println("The content body is " + body);
  /*consider removing:*/
  /*useful terms for storing content*/
  int contentInt = 0;
  long contentLong = 0;
  String contentString = "";
  /* Extract the URL of the request */
  /* We have two URLs. If IP Address is 192.168.1.6 (for example),
   * then URLs are: 
   * 192.168.1.6/ and its requests are: 
   *        GET / HTTP/1.1, where it will populate the client an HTML UI interface
   * 192.168.1.6/LED and its requests are: 
   *        GET /LED HTTP/1.1 , where it will post to the server the status of the light (on or off)
   *        POST /LED HTTP/1.1 , where it will update the light to the status in the content of the request(on or off)
   */
  
  if(!pathExists(pathHttp)){
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.stop();
    Serial.println("Responded with 404");
    return;
  }
  /*Verify login info*/
  if(!requestHeaders.loginSuccess){
    client.println("HTTP/1.1 401 Unauthorized");
    client.println("WWW-Authenticate: Basic realm=\"ESP\"");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Login required to proceed, please visit https://users.encs.concordia.ca/~za_lord/ for more information.");
    client.stop();
    Serial.println("Responded with 401");
    return;
  }

  if (pathHttp == "/"){
    /*populate website with UI, with login box into retrieval at top. GET is only method*/
    if (methodHttp != "GET"){
      client.println("HTTP/1.1 405 Method Not Allowed");
      client.println("Allow: GET");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("Unsupported Request Method");
      client.stop();
      Serial.println("Responded with 405");
      return;
    }
    else{
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println(R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
      <title>Subwoofer WebUI</title>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <meta http-equiv="refresh" content="30">
      <style>
      body {background-color: #121212; font-family: Tahoma, sans-serif; margin-top: 40px;}
      h1, h2   {color: #E0E0E0; text-align: center}
      h3, h4   {color: #E0E0E0; text-align: left; margin-left: 20px}
      p    {color: #B0B0B0; margin-left: 10px}
      button {font-size: 20px; padding: 10px 20px; margin: 10px;}
      .timer-label {color: #B0B0B0; margin-left: 10px}
      #status {margin-top: 20px; font-weight: bold;}
      </style>
      </head>

      <body>
      <h1>
      Subwoofer Web Interface
      </h1>
      <p>See below the available API interactions for the Subwoofer's Microcontroller.</p>

      <h2>API Call Interfaces</h2>
      <br>
      <h3>Power Control</h3>
      <h4>Power On/Off</h4>
      <button id="powerBtn" onclick = togglePower()>loading...</button>
      <h4>Power Sleep Timer</h4>
      <p style="color:#FFFFC5;font-size:40px;">Power off in <span id ="timerDisplay">--:--</span> seconds</p> <br>
      <label for="timerInput" class="timer-label">Set timer (minutes):</label>
      <input type="number" id="timerInput" min="0">
      <button onclick="startTimerFromInput()">Start Timer</button>
 
      <h2>About This Page:</h2>
      <p>The subwoofer is driven by a TPA3116 Audio Mono Amplifier in an old <a href="https://www.cnet.com/reviews/jvc-th-c30-review/">JVC-TH-C30</a>.<br>
      This website and its IOT interactions are powered by an ESP8266 microcontroller.<br>
      For more info about the project, please visit <a href="https://users.encs.concordia.ca/~za_lord/">my website.</a></p>
      </body>

      <script>
      let powerStatus = false;
      let remainingSeconds = 0;

      function updateButton() {
        const button = document.getElementById("powerBtn")
        button.innerText = powerStatus ? "Power OFF" : "Power ON";
      }

      function updateTimerDisplay(seconds) {
        const minutes = Math.floor(seconds / 60);
        const secs = seconds % 60;
        // pad single digits with a leading 0
        document.getElementById("timerDisplay").innerText = `${minutes}:${secs.toString().padStart(2,'0')}`;
      }

      function togglePower(){
        let content = "";
        content = powerStatus? "on=0" : "on=1";
        fetch('/power', {
          method: 'POST',
          headers: {'Content-Type': 'application/x-www-form-urlencoded'},
          body: content,
          credentials: 'same-origin'
        })
        .then(r => {
          if (!r.ok) throw new Error("Request failed");
          powerStatus = !powerStatus;
          updateButton();
        })
        .catch(err => console.error(err));
      }

      function setTimer(seconds) {
        let content = `sleep=${seconds}`
        fetch("/power/timer", {
          method: 'POST',
          headers: {'Content-Type': 'application/x-www-form-urlencoded'},
          body: content,
          credentials: 'same-origin'
        })
        .then(response => response.text())
        .then(text => {
          timerEnd = Math.floor(Date.now() / 1000) + seconds;
          updateTimerDisplay(remainingSeconds);
        })
        .catch(err => console.error(err));
      }

      function startTimerFromInput() {
        const minutes = parseInt(document.getElementById("timerInput").value);
        if (!isNaN(minutes) && minutes > 0) {
          setTimer(minutes * 60); // convert minutes to seconds
        }
      }

      function getTimer(){
        fetch('/power/timer', {method: 'GET', credentials: 'same-origin'})
        .then(response => response.text())
        .then(text =>{
          const parts = text.trim().split("=");
          if (parts.length === 2){
            const seconds = parseInt(parts[1]);
            const now = Math.floor(Date.now() / 1000);
            timerEnd = now + seconds;
            updateTimerDisplay(remainingSeconds);
          }
        });
      }

      function fetchInitialState(){
        fetch('/power', {method: 'GET', credentials: 'same-origin'})
        .then(response => response.text())
        .then(text =>{
          powerStatus = text.trim().includes("ON");
          updateButton();
        });

        getTimer();
      }
      const intervalUpdateTimer = setInterval(() => {
        if (remainingSeconds > 0) {
          remainingSeconds--;
          updateTimerDisplay(remainingSeconds);
        } else {
          clearInterval(intervalUpdateTimer);
          document.getElementById("timerDisplay").innerText = "--:--";
        }
      }, 1000);

      const intervalFetchTimer = setInterval(() => {getTimer();}, 10000);

      window.onload = fetchInitialState;
      </script>
      
      </html> 
      )rawliteral");

      client.stop();
      Serial.println("Responded with 200");
      return;
    }
  }
   /*individual functions for each path?*/
   /*move towards JSON instead of plaintext in next version?*/
  if (pathHttp.indexOf("/power") != -1) 
  {
    if(pathHttp == "/power/timer"){
      if(methodHttp == "GET"){
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if (timerEnd == 0 || now >= timerEnd)
          client.println("sleep=0");
        else{
          client.print("sleep=");
          client.println(timerEnd - now);
        }
        client.stop();
        Serial.println("Responded with 200");
        return;
      }
      else if(methodHttp == "POST"){
        if(body.startsWith("sleep=")){ //if body has power= as preceeding term, i.e. valid content.
          splitIndex = body.indexOf('=');
          contentLong = body.substring(splitIndex + 1).toInt();
          if (contentLong >= 0){
            setSleepTimer(contentLong);          
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.print("sleep=");
            client.println(contentLong);
            Serial.println("Responded with 200");
          }
          else{
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("Invalid Body Format");
            Serial.println("Responded with 400");
          }
          client.stop();
          return;
        }
      }
      else{
        client.println("HTTP/1.1 405 Method Not Allowed");
        client.println("Allow: GET, POST");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Unsupported Request Method");
        client.stop();
        Serial.println("Responded with 405");
        return;
      }
    }
    else if (pathHttp == "/power"){
     if(methodHttp == "GET"){
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(powerOn)
          client.println("Device ON");
        else
          client.println("Device OFF");
        client.stop();
        Serial.println("Responded with 200");
        return;
      }
      else if(methodHttp == "POST"){
        if(body == "on=1"){
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          turnAmpOn();
          client.println("Device ON");
          client.stop();
          Serial.println("Responded with 200");
          return;
        }
        else if (body == "on=0"){
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          turnAmpOff();
          client.println("Device OFF");
          client.stop();
          Serial.println("Responded with 200");
          return;
        }
        else{
          client.println("HTTP/1.1 400 Bad Request");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("Invalid Body Format");
          client.stop();
          Serial.println("Responded with 400");
          return;
        }
      }
      else{
        client.println("HTTP/1.1 405 Method Not Allowed");
        client.println("Allow: GET, POST");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Unsupported Request Method");
        client.stop();
        Serial.println("Responded with 405");
        return;
      }
    }
  } 
  
}
