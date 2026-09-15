#ifndef API_H
#define API_H

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

//include relevant user libraries
#include "globals.h"
#include "SubwooferControls.h"
#include "HttpTypes.h"

//base/abstract class:

class API
{
  protected:
  WiFiClient client;
  //eventually, add option to handle headers for formatting?
  //all endpoints are virtual, and default to unsupported method implementation
  virtual void handleGET(const String& body) { handleUnsupportedMethod(); }
  virtual void handleHEAD(const String& body){ handleUnsupportedMethod(); }
  virtual void handlePOST(const String& body){ handleUnsupportedMethod(); }
  virtual void handlePUT(const String& body){ handleUnsupportedMethod(); }
  virtual void handleDELETE(const String& body){ handleUnsupportedMethod(); }
  virtual void handleCONNECT(const String& body){ handleUnsupportedMethod(); }
  virtual void handleOPTIONS(const String& body){ handleUnsupportedMethod(); }
  virtual void handleTRACE(const String& body){ handleUnsupportedMethod(); }
  virtual void handlePATCH(const String& body){ handleUnsupportedMethod(); }
  void handleUnsupportedMethod();
  void handleBadRequest(const char* message);

  public:
  //constructor
  API();
  //destructor
  virtual ~API() = default;
  //update to most recent client
  void updateClient(WiFiClient &Client){
    client = Client;
  }
  //get path (Atmost 16 long, so a consistent return)
  virtual const char* getPath() = 0;
  //string containing allowed methods
  virtual const char* allowedMethodsString()
  {
    //always seperated by space and comma between each method.
    //default should actually be empty
    return ""; //default
  }
  //function that takes method as input, and then sends response.
  void callPathway(const String& methodHttp, const String& body)
  {
    //verify that method is supported:
    //if statement using isMethodSupportedBool is stupid. Just define supported methods and will be caught by switch???
    if(methodHttp == "GET")
      handleGET(body);
    else if(methodHttp == "HEAD")
      handleHEAD(body);
    else if(methodHttp == "POST")
      handlePOST(body);
    else if(methodHttp == "PUT")
      handlePUT(body);
    else if(methodHttp == "DELETE")
      handleDELETE(body);
    else if(methodHttp == "CONNECT")
      handleCONNECT(body);
    else if(methodHttp == "OPTIONS")
      handleOPTIONS(body);
    else if(methodHttp == "TRACE")
      handleTRACE(body);
    else if(methodHttp == "PATCH")
      handlePATCH(body);
    else
      handleUnsupportedMethod();
  }
};

//inheriting/implementing classes

class WebAPI: public API
{
  //hard-coded API's that use inheritance for structure
  protected:
  void handleGET(const String& body) override;

  public:
  const char* getPath() override
  {
    return "/";
  }
  const char* allowedMethodsString() override
  {
    return "GET";
  }
};

//could eventually merge power and power timer, have timer be a second plaintext input? Or timers be Json values?
//If so, post will require all be defined, while put will preserve unchanged values
class PowerAPI : public API
{
  protected:
  void handleGET(const String& body) override;
  void handlePOST(const String& body) override;

  public:
  const char* getPath() override
  {
    return "/power";
  }
  const char* allowedMethodsString() override
  {
    return "GET, POST";
  }
};

class PowerTimerAPI : public API
{
  protected:
  void handleGET(const String& body) override;
  void handlePOST(const String& body) override;

  public:
  const char* getPath() override
  {
    return "/power/timer";
  }
  const char* allowedMethodsString() override
  {
    return "GET, POST";
  }
};
#endif