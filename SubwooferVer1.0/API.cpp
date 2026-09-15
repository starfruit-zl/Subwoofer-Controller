#include "API.h"

API::API()
{
}

void API::handleUnsupportedMethod()
{
  client.println("HTTP/1.1 405 Method Not Allowed");
  client.print("Allow: ");
  client.println(allowedMethodsString());
  client.println("Content-Type: text/plain");
  client.println("Content-Length: 26");
  client.println("Connection: close");
  client.println();
  client.println("Unsupported Request Method");
  client.stop();
  Serial.println("Responded with 405");
  return;
}

void API::handleBadRequest(const char* message)
{
  client.println("HTTP/1.1 400 Bad Request");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println(message);
  client.stop();
  Serial.println("Responded with 400");
  return;
}

void WebAPI::handleGET(const String& body)
{
  //html and script code return for website.
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();//to add: status dashboard.
  client.println(R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
      <title>Subwoofer WebUI</title>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <style>
      body {background-color: #121212; font-family: Tahoma, sans-serif; margin-top: 40px;}
      h1, h2   {color: #E0E0E0; text-align: center}
      h3   {font-size: 20px; font-weight: 600; color: #E6E6E6; margin: 30px 0 12px 20px;}
      h4   {font-size: 14px;font-weight: 500;color: #B8B8B8;margin: 16px 0 6px 20px;text-transform: uppercase;letter-spacing: 0.5px;}
      p    {color: #B0B0B0; margin-left: 20px}
      button {font-size: 15px; padding: 10px 10px; margin: 20px;}
      .timer-label {color: #FFFFC5; font-size: 40px; margin: 20px 20px; line-height: 1.2;}
      .timer-input {color: #B0B0B0; margin-left: 20px}
      #status {margin-top: 20px; font-weight: bold;}
      </style>
      </head>

      <body>
      <h1>
      Subwoofer Web Interface
      </h1>
      <p>See below the status and the available API interactions for the Subwoofer's Microcontroller.</p>

      <h2>API Call Interfaces</h2>
      <br>
      <h3>Power Control</h3>
      <h4>Power On/Off</h4>
      <button id="powerBtn" onclick = togglePower()>loading...</button>
      <h4>Power Sleep Timer</h4>
      <p class="timer-label">Power off in <span id ="timerDisplay">--:--</span></p>
      <label for="timerInput" class="timer-input">Set timer (minutes):</label>
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
      let timerEnd = 0;

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
          remainingSeconds = seconds;
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
            remainingSeconds = seconds;
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
      setInterval(() => {
        if (remainingSeconds > 0) {
          remainingSeconds--;
          updateTimerDisplay(remainingSeconds);
        } else if (remainingSeconds === 0) {
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

void PowerAPI::handleGET(const String& body) override
{
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

void PowerAPI::handlePOST(const String& body) override
{
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
    handleBadRequest("Invalid Body Format");
  }
}

void PowerTimerAPI::handleGET(const String& body) override
{
  //need to get now-time
  unsigned long now = timeClient.getEpochTime();
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

void PowerTimerAPI::handlePOST(const String& body) override
{
  if(body.startsWith("sleep=")){ //if body has sleep= as preceeding term, i.e. valid content.
      int splitIndex = body.indexOf('=');
      long contentLong = body.substring(splitIndex + 1).toInt();
      //created if not already existing? (201 code?)
      if (contentLong >= 0 && powerOn){ //only allow to set the power if its on.
        setSleepTimer(contentLong);          
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.print("sleep=");
        client.println(contentLong);
        Serial.println("Responded with 200");
        client.stop();
        return;
      }
      else if(!powerOn){
        client.println("HTTP/1.1 409 Conflict");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Device already powered off");
        Serial.println("Responded with 409");
        client.stop();
      }
      else
      {
        handleBadRequest("Timer duration must be non-negative");
      }
  }
  else{
    handleBadRequest("Invalid Body Format");
  }
}