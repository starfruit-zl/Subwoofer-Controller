#include "IRInstructionHandler.h"

void handleCode(uint32_t data){
  switch (data){
    case 0x17: //power command (Subwoofer and Unit)
      Serial.print("Received IR code ");
      Serial.println(data);
      Serial.println("Swapping power");
      powerAmpSwitch(); //swap power
      break;
    case 0x0B: //reprofiled power command (Subwoofer only)
      Serial.print("Received IR code ");
      Serial.println(data);
      Serial.println("Swapping power");
      powerAmpSwitch(); //swap power on subwoofer only (special function?)
      break;
    case 0x1C: //mute button on receiver. This is an edge case as its not dependent on the devices current state.
      //to be used for muting function on input that uses MOSFET.
      break;
    case 0x58: //surround on
      break; 
    case 0x05: //dsp on
      break; 
    case 0x03: //sleep button command
      Serial.print("Received IR code ");
      Serial.println(data);
      Serial.println("Incrementing timer");
      timeSleepButton = timeClient.getEpochTime();
      ++numSleepButtonPresses;
      break;
    case 0x1E: //volume up
      break;
    case 0x1F: //volume down
      break;
    //for macros, log inputs and if within acceptable time, handle command.
    case 0x79: //press "1"
      break;
    case 0x7A: //press "2"
      break;
    case 0x7B: //press "3"
      break;
    case 0x7C: //press "4"
      break;
    case 0x7D: //press "5"
      break;
    case 0x7E: //press "6"
      break;
    case 0x78: //press "7"
      break;
    default:
      break;
  }
}