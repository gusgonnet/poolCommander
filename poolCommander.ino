//The MIT License (MIT)
//Copyright (c) 2016 Gustavo Gonnet
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all copies
// or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// github: https://github.com/gusgonnet/poolCommander
// hackster: https://www.hackster.io/gusgonnet

#include "application.h"
#include "elapsedMillis.h"
#include "blynk.h"
#include "blynkAuthToken.h"
#include "NCD4Relay.h"
#include "FiniteStateMachine.h"
#include "lib.h"

#define APP_NAME "Pool Commander"
String VERSION = "Version 0.01";
/*******************************************************************************
 * changes in version 0.01:
       * initial version

TODO:
  * so many things

*******************************************************************************/
const int TIME_ZONE = -4;
const bool ON = true;
const bool OFF = false;

//minimum number of milliseconds to turn on the pump
// this is here to protect the pump and the relay
#define MINIMUM_PUMP_ON_TIMEOUT 60000
elapsedMillis minimumPumpOnTimer;

float targetTemp = 29.0;
float currentTemp = 20.0;

/*******************************************************************************
 initialize FSM states with proper enter, update and exit functions
*******************************************************************************/
State autoIdleState = State( autoIdleEnterFunction, autoIdleUpdateFunction, autoIdleExitFunction );
State autoHeatingState = State( autoHeatingEnterFunction, autoHeatingUpdateFunction, autoHeatingExitFunction );
State autoFilteringState = State( autoFilteringEnterFunction, autoFilteringUpdateFunction, autoFilteringExitFunction );
State manualState = State( manualEnterFunction, manualUpdateFunction, manualExitFunction );

//initialize state machine, start in state: autoIdleState
FSM poolStateMachine = FSM(autoIdleState);

//here are the possible states of the pool system
#define STATE_AUTO_IDLE "AUTO-IDLE"
#define STATE_AUTO_HEATING "AUTO-HEATING"
#define STATE_AUTO_FILTERING "AUTO-FILTERING"
#define STATE_MANUAL "MANUAL"
String state = STATE_AUTO_IDLE;

/*******************************************************************************
 Here you decide if you want to use Blynk or not
 Your blynk token goes in another file to avoid sharing it by mistake
 The file containing your blynk auth token has to be named blynkAuthToken.h and it should contain
 something like this:
  #define BLYNK_AUTH_TOKEN "1234567890123456789012345678901234567890"
 replace with your project auth token (the blynk app will give you one)
*******************************************************************************/
#define USE_BLYNK "yes"
char auth[] = BLYNK_AUTH_TOKEN;

//definitions for the blynk interface
#define BLYNK_BUTTON_RELAY01 V0
#define BLYNK_BUTTON_RELAY23 V1


NCD4Relay relayController;

/*******************************************************************************
 * Function Name  : setup
 * Description    : this function runs once at system boot
 *******************************************************************************/
void setup() {

  //publish startup message with firmware version
  Particle.publish(APP_NAME, VERSION, 60, PRIVATE);

  if (USE_BLYNK == "yes") {
    Blynk.begin(auth);
  }

  Time.zone(TIME_ZONE);

  relayController.setAddress(0, 0, 0);

}

/*******************************************************************************
 * Function Name  : loop
 * Description    : this function runs continuously while the project is running
 *******************************************************************************/
void loop() {

  //this function updates the FSM
  // the FSM is the heart of the pool Commander - all actions are defined by its states
  poolStateMachine.update();

  // relayController.turnOnAllRelays();
  // delay(500);
  // relayController.turnOffAllRelays();
  // delay(500);
  // relayController.turnOnAllRelays(1);
  // delay(500);
  // relayController.turnOnRelay(1);
  // delay(500);
  // relayController.turnOffRelay(1);
  // delay(500);

  if (USE_BLYNK == "yes") {
    Blynk.run();
  }

}

/*******************************************************************************
 * Function Name  : setState
 * Description    : sets the state of the system
 * Return         : none
 *******************************************************************************/
void setState(String newState) {
  state = newState;
//  Blynk.virtualWrite(BLYNK_DISPLAY_STATE, state);
}

/*******************************************************************************
 * Function Name  : setPump
 * Description    : turns on or off the pump
 * Return         : none
 *******************************************************************************/
void setPump(bool newStatus) {
  if (newStatus) {
    relayController.turnOnRelay(1);
    relayController.turnOnRelay(2);
  } else {
    relayController.turnOffRelay(1);
    relayController.turnOffRelay(2);
  }
}

/*******************************************************************************
 * Function Name  : setHeater
 * Description    : turns on or off the heater
 * Return         : none
 *******************************************************************************/
void setHeater(bool newStatus) {
  if (newStatus) {
    relayController.turnOnRelay(3);
    relayController.turnOnRelay(4);
  } else {
    relayController.turnOffRelay(3);
    relayController.turnOffRelay(4);
  }
}


/*******************************************************************************
********************************************************************************
********************  FINITE STATE MACHINE FUNCTIONS  **************************
********************************************************************************
*******************************************************************************/
//State manualState = State( manualEnterFunction, manualUpdateFunction, manualExitFunction );
void manualEnterFunction(){
  setState(STATE_MANUAL);
}
//in this state the user can control manually the pump and the heater
void manualUpdateFunction(){
  //user set auto mode? => switch to autoIdle state
}
//from manual going to auto-idle, maybe pump and heater should be turned off (if they are on)?
void manualExitFunction(){
}

/******************************************************************************/
//State autoIdleState = State( autoIdleEnterFunction, autoIdleUpdateFunction, autoIdleExitFunction );
void autoIdleEnterFunction(){
  setState(STATE_AUTO_IDLE);
}
void autoIdleUpdateFunction(){
  //user set manual mode? => switch to manual state
  //is it 6am? start heating
}
void autoIdleExitFunction(){
}

/******************************************************************************/
//State autoHeatingState = State( autoHeatingEnterFunction, autoHeatingUpdateFunction, autoHeatingExitFunction );
void autoHeatingEnterFunction(){
  setState(STATE_AUTO_HEATING);
  //turn on pump and heater
  setPump(ON);
  setHeater(ON);
}
void autoHeatingUpdateFunction(){

  //user set manual mode? => switch to manual state

  //is it 5pm? stop heating => switch to filtering state

  //temp of water > targeTemp? => stop heating, continue filtering
  if ( currentTemp >= (targetTemp) ) {
    logMessage("Desired temperature reached: " + float2string(currentTemp) + "°C" + getTime());
    poolStateMachine.transitionTo(autoFilteringState);
  }

}
void autoHeatingExitFunction(){
  //turn off heater
  setHeater(OFF);
}

/******************************************************************************/
//State autoFilteringState = State( autoFilteringEnterFunction, autoFilteringUpdateFunction, autoFilteringExitFunction );
void autoFilteringEnterFunction(){
  setState(STATE_AUTO_FILTERING);

  //start the minimum timer of this cycle
  minimumPumpOnTimer = 0;
}

void autoFilteringUpdateFunction(){
  //is minimum time up? not yet, so get out of here
  //this can be used as well as a delay between switching off the heater and the pump
  if (minimumPumpOnTimer < MINIMUM_PUMP_ON_TIMEOUT) {
    return;
  }

  //user set manual mode? => switch to manual state
  //is it 5pm? stop filtering => switch to auto idle state
  //filtered for 8 hours? => stop filtering
}

void autoFilteringExitFunction(){
  //turn off pump
  setPump(OFF);
}

/*******************************************************************************/
/*******************************************************************************/
/*******************          BLYNK FUNCTIONS         **************************/
/*******************************************************************************/
/*******************************************************************************/

/*******************************************************************************
 * Function Name  : BLYNK_WRITE
 * Description    : these functions are called by blynk when the blynk app wants
                     to write values to the photon
                    source: http://docs.blynk.cc/#blynk-main-operations-send-data-from-app-to-hardware
 *******************************************************************************/
BLYNK_WRITE(BLYNK_BUTTON_RELAY01) {
  //flip relay status, if it's on switch it off and viceversa
  // do this only when blynk sends a 1
  if ( param.asInt() == 1 ) {
    //flip relays 0 and 1
  }
}

BLYNK_WRITE(BLYNK_BUTTON_RELAY23) {
  if ( param.asInt() == 1 ) {
    //flip relays 2 and 3
  }
}
