/*
Hardware required: 
	Arduino MEGA 2560
	I2C EEPROM
	Iridium 9602 Modem
	
User interface: Debug output available on Serial/USB from Mega board, 115000 Bps
*/

#define __WHITESTARBALLOON__
#include <avr/wdt.h>
#include "CommCtrlrConfig.h"
#include <WSWire.h>  // Modified Wire library to add timeouts for I2C bus problems.  http://github.com/whitestarballoon/WSWireLib
#include "Iridium9602.h"
#include "TimeKeeper.h"
#include "BaseMsg.h"
#include "DebugMsg.h"
#include "SatQueue.h"
#include "CutDown.h"
#include "I2CCommMgr.h"
#include "SatCommMgr.h"
#include <EEPROM.h>

unsigned long wdResetTime = 0;
//These things should be later integrated properly VVVVV.
volatile int NetworkAvailableJustChanged = 0;
volatile int SatelliteNetworkAvailable = 0;
 // (ms) Force initiate SBD session after period of no activity, in minutes, the first number in parenthesis
 // Defaults to 15 minutes
volatile unsigned long satForceSBDSessionInterval = (15UL * 60UL * 1000UL);
	
// Declare an instance of the class
Iridium9602 iridium = Iridium9602(IRIDIUM_SERIAL_PORT);
SatCommMgr satCommMgr = SatCommMgr( iridium ); 

TimeKeeper myTimeKeeper = TimeKeeper::getInstance();

I2CCommMgr i2cCommMgr( satCommMgr );

void(* resetFunc) (void) = 0; //declare WDReset function @ address 0

void setup()
{
   wdt_reset();
       // Set all pins as input for bootup
   for(int i=2;i<53;i++){
    pinMode(i, INPUT);
   }
   wdt_reset();
     // Disable all pullup resistors
   for(int i=2;i<53;i++){ 
    digitalWrite(i, LOW);
   } 
   watchdogSetup();

  


   pinMode(A2,INPUT);
   pinMode(A3,INPUT);
   pinMode(A4,INPUT);
   pinMode(A5,INPUT);
   digitalWrite(A2,LOW);
   digitalWrite(A3,LOW);
   digitalWrite(A4,LOW);
   digitalWrite(A5,LOW);


   //Set I2C White Star Shield alternate pins to inputs, then disable pullup resistor
   pinMode(A8,INPUT);
   pinMode(A9,INPUT);
   pinMode(A10,INPUT);
   pinMode(A11,INPUT);
   digitalWrite(A8,LOW);
   digitalWrite(A9,LOW);
   digitalWrite(A10,LOW);
   digitalWrite(A11,LOW);

  //Set up sat modem capacitor voltage measure pin as input
   pinMode(A0, INPUT);
   digitalWrite(A0, LOW);
  
   //Turn Iridium 9602 modem off
   pinMode(pinModemPowerSwitch, OUTPUT);
   digitalWrite(pinModemPowerSwitch, LOW);

wdtrst();	
   IRIDIUM_SERIAL_PORT.begin(19200);  //Sat Modem Port
   Serial.begin(115200);	//Debug-Programming Port
   //Serial.print("Booting...\n");
   Serial2.begin(1200);
 
 
  DebugMsg::msg_P("CC",'I',PSTR("WSB Comm Controller Reporting for Duty!"));

  
//  DebugMsg::msg("CC",'I',"MSG %02d %02d %08d",30,40, 50);
//  DebugMsg::msg("CC",'I',"MSG %02d",10);

  CutDown::initCutdown(&CUTDOWN_SERIAL_PORT);
wdtrst();  
  i2cCommMgr.i2cInit();
wdtrst();  
  DebugMsg::setI2CCommMgr( &i2cCommMgr );  
	if  (UseI2CForDebugMsgs) {
		DebugMsg::msg_P("CC",'I',PSTR("Will Send Debug out I2C"));
	}
wdtrst();  
  satCommMgr.satCommInit(  &i2cCommMgr );
  			//Slightly randomize the delay between SBD checks
  			randomSeed(analogRead(10));
  			satForceSBDSessionInterval += (random(60000)-random(60000));
  			Serial.print(F("SetMaxSBDInterval to: "));
  			Serial.print(satForceSBDSessionInterval,DEC);
  			Serial.print(F(" (m:s): "));
			Serial.print((satForceSBDSessionInterval/60000),DEC);
			Serial.print(":");
			Serial.println((satForceSBDSessionInterval%60000)/1000,DEC);
wdtrst();
  DebugMsg::msg_P("CC",'I',PSTR("CommCtrlr Boot Finished."));

}
int firstTime = true; //Watchdog Var


void loop()
{
  if (firstTime){
    firstTime = false;
    Serial.println(F("Start main loop."));
  }
  if(millis() - wdResetTime > 2000){
    //wdtrst();  // if you uncomment this line, it will keep resetting the timer.
  }
  // Update the TimeKeeper Clock
  myTimeKeeper.update( );
  wdtrst();
  i2cCommMgr.update();
  wdtrst();
  satCommMgr.update();
  wdtrst();
  
  //i2cCommMgr.I2CAliveCheck();
}


void IridiumUpdateNetworkAvailable()
{
	NetworkAvailableJustChanged = true;
	SatelliteNetworkAvailable = true;
}

void initIridiumNetworkInterrupt()
{
	 attachInterrupt(1, IridiumUpdateNetworkAvailable, CHANGE);
}

void watchdogSetup()
{
cli();  // disable all interrupts
wdt_reset(); // reset the WDT timer
MCUSR &= ~(1<<WDRF);  // because the data sheet said to
/*
WDTCSR configuration:
WDIE = 1 :Interrupt Enable
WDE = 1  :Reset Enable - I won't be using this on the 2560
WDP3 = 0 :For 1000ms Time-out
WDP2 = 1 :bit pattern is
WDP1 = 1 :0110  change this for a different
WDP0 = 0 :timeout period.
*/
// Enter Watchdog Configuration mode:
WDTCSR = (1<<WDCE) | (1<<WDE);
// Set Watchdog settings: interrupte enable, 0110 for timer
WDTCSR = (1<<WDIE) | (0<<WDP3) | (1<<WDP2) | (1<<WDP1) | (0<<WDP0);
sei();
delay(100);
//Serial.println(F("finished watchdog setup"));  // just here for testing
}


ISR(WDT_vect) // Watchdog timer interrupt.
{
  if(millis() - wdResetTime > TIMEOUTPERIOD){
 // Serial.println(F("***** This is where it would have rebooted"));  // just here for testing
  //  wdtrst();                                         // take these lines out
  resetFunc();     // This will call location zero and cause a reboot.
  }
  //else                                                       // these lines should
    //Serial.println(F("Else Howdy"));                                 // be removed also
}



extern "C" {
int atexit(void (*func)(void))
{
        return 0;
}
};


