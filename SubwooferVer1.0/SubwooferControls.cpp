#include "SubwooferControls.h"
//EEPROM Save Handler
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

/*Power controls*/

void turnAmpOn(){
  digitalWrite(D1, HIGH);
  digitalWrite(D2, HIGH);
  powerOn = true;
  //if a timer is set
  if (timerEnd != 0){
    timerEnd = 0;
    saveTimesToEEPROM();
  }
}

void turnAmpOff(){
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  powerOn = false;
  //if a timer is set
  if (timerEnd != 0){
    timerEnd = 0;
    saveTimesToEEPROM();
  }
}

//special control for swapping power (if off, turn on; vice-versa)
void powerAmpSwitch(){
  if (powerOn) turnAmpOff();
  else turnAmpOn();
}

/*Time System Handlers*/
//API Call version.

void setSleepTimer(long seconds){
  if (seconds == 0) timerEnd = 0;
  else timerEnd = timeClient.getEpochTime() + (unsigned long)seconds;
  //we know that timer for sure is changed.
  saveTimesToEEPROM();
}

//remoteIR has a different set handler based off incremental presses where if not pressed within a certain amount of time will not update.
//first press ignored, then subsequent within 5 seconds are counted. Can be updated and added to current timer in same fashion. Only update eeprom when time expires.
//Once inputs handled from decoding angle, will use same setSleepTimer underneath.

