#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#ifndef STASSID
#define STASSID "blank"
#define STAPSK "blank"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

WiFiServer espServer(80); /* Instance of WiFiServer with port number 80 */
/* 80 is the Port Number for HTTP Web Server */

struct headersHttp{
  int contentLength = 0;
  char contentType[33] = {0};
  bool loginSuccess = false;
};

headersHttp handleHeaders(WifiClient &client){
  
  const char* expectedAuth = "Basic blank";
  
  headersHttp retHeaders;
  String line;
  /*maybe make function that returns a headers struct?*/
  while(true){
    line = client.readStringUntil('\r');
    client.readStringUntil('\n');

    if (line.length() == 0) break; /*end of headers*/

    if (line.startsWith("Content-Length:"))
      retHeaders.contentLength = line.substring(15).toInt();

    if (line.startsWith("Content-Type:"))
      retHeaders.contentType = line.substring(13);

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

void setup() {
  pinMode(5, OUTPUT);
  pinMode(0, INPUT);
  digitalWrite(5, LOW); 
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
  ArduinoOTA.handle();
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
  int endPath = request.indexOf(' ', splitIndex);
  /*error handling for null string?*/
  String methodHttp = request.substring(0, splitIndex);
  String pathHttp = request.substring(splitIndex + 1, endPath);
  /*handle header data line by line, important parameters defined below:*/
  headersHttp requestHeaders = handleHeaders(client);

  String body;
  if (hasBody(methodHttp) && requestHeaders.contentLength > 0)
    while(body.length() < requestHeaders.contentLength)
      if (client.available())
        body += char(client.read());
  
  client.flush();
  
  /* Extract the URL of the request */
  /* We have two URLs. If IP Address is 192.168.1.6 (for example),
   * then URLs are: 
   * 192.168.1.6/ and its requests are: 
   *        GET / HTTP/1.1, where it will populate the client an HTML UI interface
   * 192.168.1.6/LED and its requests are: 
   *        GET /LED HTTP/1.1 , where it will post to the server the status of the light (on or off)
   *        POST /LED HTTP/1.1 , where it will update the light to the status in the content of the request(on or off)
   */
   char validPaths[2][16] = {"/", "/LED"};
   /*Verify login info*/
   if(!requestHeaders.LoginSuccess){
    client.println("HTTP/1.1 401 Unauthorized");
    client.println("WWW-Authenticate: Basic realm=\"ESP\"");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Login failed");
    client.stop();
    return;
   }
   /*individual functions for each path?*/
   if (pathHttp.indexOf("/LED") != -1) 
   {
     if(methodHttp == GET){
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: keep-alive");
        client.println();
        if(digitalRead(5))
          client.println("Device Powered");
        else
          client.println("Device OFF");
      }

      if(methodHttp == "POST"){
        if(body == "on=1"){
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: keep-alive");
          client.println();
          digitalWrite(5, HIGH);
          client.println("Device Powered");
        }
        else if (body == "on=0"){
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: keep-alive");
          client.println();
          digitalWrite(5, LOW);
          client.println("Device OFF");
        }
        else{
          client.println("HTTP/1.1 400 Bad Request");
          client.println("Content-Type: text/plain");
          client.println("Connection: keep-alive");
          client.println();
          client.println("Invalid Body Format");
        }
      }
   } 
  
}
