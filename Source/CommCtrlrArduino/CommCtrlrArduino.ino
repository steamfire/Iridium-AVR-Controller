/*
Hardware required: 
	Arduino(or clone) MEGA 1280 or MEGA 2560
	I2C EEPROM
	Iridium 9602 Modem
	Appropriate power circuitry for 9602 (seriously rigid power requirements)
	
Arduino IDE Compatibility:
	<1.0 - very unlikely
	1.0 - Probably
	1.0.2 - YES
	
Arduino Libraries required:
    WSWire v1.0:               https://github.com/whitestarballoon/WSWireLib
    CmdArduino Dec 28, 2011:   https://github.com/fakufaku/CmdArduino
    EEPROMex 0.8.2:            http://thijs.elenbaas.net/2012/07/extended-eeprom-library-for-arduino/
	
User interface: Debug output available on Serial/USB from Mega board, 115000 Bps
Messaging interface: I2C bus - connect another arduino or other MCU via I2C

This software is licensed under the terms of the MIT license.  See LICENSE.txt for details.
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
#include <EEPROMex.h>
#include <Cmd.h>

unsigned long wdResetTime = 0;

//********
//EEPROM config variables
//********
volatile bool EPconfigOK  = true;
volatile int EPconfigAddress=0;

/*
struct StoreStruct {
	char version[4];  //Config version number
	volatile unsigned int satDesiredSBDSessionInterval;
	byte debuglevel;
	bool pinExistsDSR, pinExistsRI, pinExistsNA, pinExistsPWR_EN, 
	useInterruptPinRI, useInterruptPinNA;
	byte i2cmyaddr, i2cuseraddr, operationMode, rebootCount, satMinimumSignalRequired;
};
*/
struct StoreStruct prefs = {
	EPCONFIG_VERSION,
	1800,
	9,
	false, false, false, false, 
	false, false,
	0x08,0x05,1,0,2
};


//These things should be later integrated properly VVVVV.
volatile int NetworkAvailableJustChanged = 0;
volatile int SatelliteNetworkAvailable = 0;
 // (ms) Force initiate SBD session after period of no activity, in minutes, the first number in parenthesis
 // Defaults to 15 minutes
unsigned long satForceSBDSessionInterval;
volatile bool _DEBUG_MSG_ASCII = true;
	
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


  //Set up sat modem capacitor voltage measure pin as input
   pinMode(A0, INPUT);
   digitalWrite(A0, LOW);
  
   //Turn Iridium 9602 modem off
   pinMode(pinModemPowerSwitch, OUTPUT);
   digitalWrite(pinModemPowerSwitch, LOW);

wdtrst();	
   IRIDIUM_SERIAL_PORT.begin(19200);  //Sat Modem Port
   Serial.begin(115200);	//Debug-Programming Port
   cmdInit(115200);  //Initialize the Serial Commandline User Interface from cmdArduino library
   //Serial.print("Booting...\n");
   
   

 
 
  DebugMsg::msg_P("CC",'I',PSTR("**** White Star Iridium 9602 Modem Controller **** \n\n"));
  DebugMsg::msg_P("CC",'I',PSTR("Distributed under MIT License"));
  DebugMsg::msg_P("CC",'I',PSTR("Originally a White Star Balloon project by Dan Bowen, Bill Peipmeyer, and Chorgy"));
  DebugMsg::msg_P("CC",'I',PSTR("Currently maintained by Dan Bowen, dan@balloonconsulting.com"));
  cmdLineSetup();
  EPsettingsInit(); //Initialize Preference Settings
//  DebugMsg::msg("CC",'I',"MSG %02d %02d %08d",30,40, 50);
//  DebugMsg::msg("CC",'I',"MSG %02d",10);
#if (__CUTDOWNENABLED__ == true)
     Serial2.begin(1200);
  CutDown::initCutdown(&CUTDOWN_SERIAL_PORT);
#endif

wdtrst();  
  i2cCommMgr.i2cInit();
wdtrst();  
  DebugMsg::setI2CCommMgr( &i2cCommMgr );  
	if  (UseI2CForDebugMsgs) {
		DebugMsg::msg_P("CC",'I',PSTR("Will Send Debug out I2C"));
	}
wdtrst();  
  satCommMgr.satCommInit(  &i2cCommMgr );
wdtrst();

  DebugMsg::msg_P("CC",'I',PSTR("CommCtrlr Boot Finished."));
  

	cmdLine_help(0,0);
    

}
int firstTime = true; //Watchdog Var


void loop()
{
  if (firstTime){
    firstTime = false;
	DebugMsg::msg_P("CC",'D',PSTR("Starting main loop."));
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
  cmdPoll();
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

/////////////////////////////////////////////////////////////////////////////////////////

void cmdLineSetup() {  // setup the available commandline commands
	cmdAdd("help", cmdLine_help);
	cmdAdd("h", cmdLine_help);
	cmdAdd("?", cmdLine_help);
	cmdAdd("ccc", cmdLine_commControlCommand);
	cmdAdd("off", cmdLine_modemOff);
	cmdAdd("on", cmdLine_modemOn);
	cmdAdd("ir", cmdLine_iridiumATCommand);
//	cmdAdd("msg", cmdLine_msgSendText);
	cmdAdd("msgb", cmdLine_msgSendBinary);
	cmdAdd("rst", cmdLine_Reset);
	cmdAdd("settings", cmdLine_settings);
    DebugMsg::msg_P("CM",'I',PSTR("Interactive. Use CR ONLY for line ends. ? for help.\n\n\n"));
    
}

void cmdLine_help(int arg_count, char **args)  {
	Serial.println(F("Available commands, end each with a CR:"));
	Serial.println(F("  help"));
	Serial.println(F("  h"));
	Serial.println(F("  ?"));
	Serial.println(F("  rst  Resets the arduino"));

	Serial.println(F("  off  Turns the Iridium modem off"));

	Serial.println(F("  on   Turns the Iridium modem on"));
//	Serial.println(F("  msg	[message text here up to 100 chars]"));
//	Serial.println(F("       Loads specified text message into Iridium modem for transmit as plain text email."));
//	Serial.println(F("       If no data is provided, it returns an error."));
	Serial.println(F("  msgb [message binary here up to 100 bytes]"));
	Serial.println(F("       Loads specified binary (or ASCII) data into Iridium modem for transmit as binary email attachment."));
	Serial.println(F("       If no message provided, sends 'Hello World'"));
	Serial.println(F("  settings                         Display/edit preferences"));
	Serial.println(F("  ir [AT command here]"));
	Serial.println(F("       Issues direct command to Iridium modem, freeform text. Debug must be on to see responses"));
	Serial.println(F("       Useful AT commands:"));
	Serial.println(F("         AT+CSQ          Check signal strength"));
	Serial.println(F("         AT+SBDD         Delete the messages in the MT(inbound) and MO (outbound) message queues"));
	Serial.println(F("         AT+SBDRT        Read the text email in the MT(inbound) message queue"));
	Serial.println(F("         AT+CGMR         Display Modem Firmware Version Numbers"));
	Serial.println(F("         AT+SBDWT=[text] Load text-only email message in MO(outbound) message queue."));
	Serial.println(F("         AT+SBDSX        Messaging status, extended"));
	Serial.println(F("       NOTE: Please avoid using SBDIX, SBDREG, and other session and SBD ring registration related commands."));	
	Serial.println(F("             These functions are taken care of automatically by the WSB CommController code."));
/*
	Serial.println(F("  ccc [0x00-0xFF]                  Test comm control commands, as used on I2C bus or uplink messsages. Hex data format."));

*/

	

}

void cmdLine_msgSendBinary(int arg_count, char **args)  {

	char j = 0;
	char lstr[101];
	if (1 == arg_count) {  // 1 when there are no arguments
		DebugMsg::msg_P("UI",'E',PSTR("No binary message specified. Sending Hello World"));
		satCommMgr.sendBinaryMsg("Hello World ");
	} else {
   		j = sprintf(lstr, "%s ", args[1]);
   		for ( int b = 2; b<arg_count; b++) {
   			j += sprintf(lstr+j, "%s ", args[b]);
   		}
   		//Serial.print(F("Will send this: "));
   		//Serial.println(lstr);
		satCommMgr.sendBinaryMsg(lstr,sizeof(lstr));
	}
}

#ifdef _txtmsgworking
void cmdLine_msgSendText(int arg_count, char **args)  {
		char j = 0;
	String lstr;
	if (1 == arg_count) {  // 1 when there are no arguments
		satCommMgr.sendTextMsg("Hello World");
	} else {
		for ( int b = 2; b<arg_count; b++) {
   			lstr += args[b];
   			lstr += ' ';
   		}
		satCommMgr.sendTextMsg(lstr);
	}


}
#endif

void cmdLine_iridiumATCommand(int arg_count, char **args)  {
		char j = 0;
	char lstr[101];
	if (1 == arg_count) {  // 1 when there are no arguments
		satCommMgr.issueDirectCmd("AT");
	} else {
   		j = sprintf(lstr, "%s ", args[1]);
   		for ( int b = 2; b<arg_count; b++) {
   			j += sprintf(lstr+j, "%s ", args[b]);
   		}
   		//Serial.print(F("Will send this: "));
   		//Serial.println(lstr);
		satCommMgr.issueDirectCmd(lstr);
	}


}

void cmdLine_commControlCommand(int arg_count, char **args)  {
	if (1 == arg_count) {  // 1 when there are no arguments
		Serial.print(F("ERROR: No command specified."));
	} else {
		
		
	}


}

void cmdLine_modemOff(int arg_count, char **args)  {
	satCommMgr.turnModemOff();
}

void cmdLine_modemOn(int arg_count, char **args)  {
	satCommMgr.turnModemOn();
}



void cmdLine_Reset(int arg_count, char **args)  {
	resetFunc(); //call reset
	//Alternately let the watchdog timer timeout.
}

void cmdLine_settings(int arg_count, char **args)  {
	switch (arg_count) {
	case 1:  //only the 'settings' term used, no other args
			// Print settings help out
//	Serial.println(F("  settings debuglevel [0]          0 = no output 9 = lots of output"));
	Serial.println(F("  settings pinexists [DSR | RI | NA | PWR_EN] [true | false]"));
	Serial.println(F("		 Specify which pins on the Iridium Modem are connected to arduino.  true = connected, false = not connected"));	
	Serial.println(F("  settings idlecheck [0 | 60-2147483 seconds]"));           
	Serial.println(F("       How long to wait between checking Iridium gateway for messages"));
	Serial.println(F("       This time interval is slightly randomized every time to comply with Iridium traffic rules."));
	Serial.println(F("  settings i2cmyaddr [0x00-0xFF]     Set I2C self address of this arduino comm controller."));
	Serial.println(F("  settings i2cfcaddr [0x00-0xFF]     Set I2C  address of the flight computer.	"));
//	Serial.println(F("  settings deviceexists [CDN | FC] [true | false]"));
//	Serial.println(F("       Indicate which devices are connected.  It won't try to communicate with devices that aren't there."));	
	
  DebugMsg::msg_P("CM",'I',PSTR("     settings"));
  DebugMsg::msg_P("CM",'I',PSTR("              idlecheck %d"),prefs.satDesiredSBDSessionInterval); 
  DebugMsg::msg_P("CM",'I',PSTR("              pinexists")); 
  DebugMsg::msg_P("CM",'I',PSTR("                        DSR %s"),(prefs.pinExistsDSR)?"true":"false");     
  DebugMsg::msg_P("CM",'I',PSTR("                        RI %s"),(prefs.pinExistsRI)?"true":"false"); 
  DebugMsg::msg_P("CM",'I',PSTR("                        NA %s"),(prefs.pinExistsNA)?"true":"false"); 
  DebugMsg::msg_P("CM",'I',PSTR("                        PWR_EN %s"),(prefs.pinExistsPWR_EN)?"true":"false");
  DebugMsg::msg_P("CM",'I',PSTR("              i2cmyaddr %x"),prefs.i2cmyaddr);  
  DebugMsg::msg_P("CM",'I',PSTR("              i2cuseraddr %x"),prefs.i2cuseraddr);          
    
			break;
	case 2:
			break;
	case 3:
	
	
			break;
	default:
	;
	}


}

void EPsettingsInit() {
	wdtrst();
	byte LoadDefaultsFlag;
	LoadDefaultsFlag = EEPROM.read(EPloadDefaultsFlagAddr);
	wdtrst();	
	EEPROM.setMemPool(memoryBase, E2END); //  Set memory range start and end, E2END is a define in io.h for the last EEPROM address
	EPconfigAddress = EEPROM.getAddress(sizeof(StoreStruct));  //Size of prefs struct
	wdtrst(); 
	if (LoadDefaultsFlag) {  // Check to see if we need to initialize the EEPROM, set from prior boot
		DebugMsg::msg_P("EP", 'D', PSTR("Default prefs version: %s"), prefs.version);
		EEPROM.updateBlock(EPconfigAddress,prefs);
		wdtrst();
		while(!EEPROM.isReady());
		EEPROM.write(EPloadDefaultsFlagAddr, 0);  //Set flag for next boot to NOT load defaults
		while(!EEPROM.isReady());
     DebugMsg::msg_P("EP", 'I', PSTR("Pref defaults saved into EEPROM."));
	}
	EPconfigOK = EPsettingsLoadConfig();
	if (true == EPconfigOK){
     DebugMsg::msg_P("EP", 'I', PSTR("Prefs loaded."));
		wdtrst();
		}	else  {
		wdtrst();
     DebugMsg::msg_P("EP", 'I', PSTR("Prefs in RAM/EEPROM version mismatch, rebooting to set firmware defaults"));
		EEPROM.write(EPloadDefaultsFlagAddr, 1);  //Set flag for next boot to load defaults
		//while(!isReady());  //wait till the eeprom finishes.
		//LoadDefaultsFlag = EEPROM.read(EPloadDefaultsFlagAddr);
		while(!EEPROM.isReady());
		resetFunc();  //reboot the chip.
		}
		
		
	}
	
bool EPsettingsLoadConfig() {
	EEPROM.readBlock(EPconfigAddress, prefs);
	while(!EEPROM.isReady());
	bool result = ((prefs.version[0] == EPCONFIG_VERSION[0]) && (prefs.version[1] == EPCONFIG_VERSION[1] && (prefs.version[2] == EPCONFIG_VERSION[2])));
    if (!result) {
     DebugMsg::msg_P("EP", 'D', PSTR("EEPROM prefs version: %s, current firmware prefs version: %s"), prefs.version, EPCONFIG_VERSION);
	}
	return result;
}

// This function will read the eeprom settings and compare to ram settings
//     It will only write bytes to EEPROM when they differ from the RAM bytes.
void savePrefsIfChanged() {
	EEPROM.updateBlock(EPconfigAddress, prefs);
}