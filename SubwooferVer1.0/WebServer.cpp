#include "WebServer.h"

WebServer::WebServer()
{
  //declare each API
  ServerApis[0] = &webAPI;
  ServerApis[1] = &powerAPI;
  ServerApis[2] = &powerTimerAPI;
  //add more once implemented
}

headersHttp WebServer::handleHeaders(){
  
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
//sort of useless
bool WebServer::pathExists(const String& pathHttp){
  for (int i = 0; i < numApis; i++)
    if (pathHttp == ServerApis[i]->getPath()) return true;

  return false;
}

void WebServer::handleRequest()
{
  //first, parse request for data
  /* Read the first line of the request from client */
  String request = client.readStringUntil('\r'); 
  client.readStringUntil('\n');
  Serial.println(request);
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
  //define variables of method and path:
  String methodHttp = request.substring(0, splitIndex);
  String pathHttp = request.substring(splitIndex + 1, endPath);
  Serial.println("The request method is " + methodHttp + " with path of " + pathHttp);

  /*handle header data line by line, important parameters defined below:*/
  headersHttp requestHeaders = handleHeaders();

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

  //validate login info
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

  //call relevant API (if exists)
  //Linear complexity.
  for (int i = 0; i < numApis; i++)
    if(pathHttp == ServerApis[i]->getPath())
    {
      ServerApis[i]->updateClient(client);
      ServerApis[i]->callPathway(methodHttp, body);
      client.stop();
      return;
    }
  //if no matches found, 404 case.
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.stop();
  Serial.println("Responded with 404");
  return;
}