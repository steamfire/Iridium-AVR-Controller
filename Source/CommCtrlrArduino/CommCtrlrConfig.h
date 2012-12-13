#ifndef CommCtrlrConfig_h
#define CommCtrlrConfig_h


#define __WHITESTARBALLOON__
/* define to enable debug messages */
#define _WS_DEBUG 1
//#define _txtmsgworking

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

//************* Watchdog Setup ****************
extern unsigned long wdResetTime;
#define TIMEOUTPERIODs 120
#define TIMEOUTPERIOD (TIMEOUTPERIODs* 1000UL)             // You can make this time as long as you want,
                                       // it's not limited to 8 seconds like the normal
                                       // watchdog
#define wdtrst() wdResetTime = millis();  // This macro will reset the timer
/*
Global Configuration Information
*/
#define IRIDIUM_SERIAL_PORT Serial1
#if (__CUTDOWNENABLED__ == true)
	#define CUTDOWN_SERIAL_PORT Serial2
#endif


/*******************************
 *    Pin declarations         *
 *******************************/
//#define pinRI 2  // Not used in code yet
//#define pinNA 3  
//#define pinDSR 6  
//#define pinModemPowerSwitch 4
#define pinBrownout 5
//define pinVoltageMeasure A0


//Cutdown enable/disable

#define __CUTDOWNENABLED__ false

/*******************************
 *    Address declarations     *
 * These are software variable  *
 *******************************/
const byte i2cFlightComputerAddr  = 0x05;           // Flight Computer I2C Address
const byte i2cCutDownAddr         = 0x06;           // Cut Down Module I2C Address
const byte i2cGroundSupportAddr   = 0x07;           // Ground Support Board I2C Address
const byte i2cCommCtrlAddr        = 0x08;           // Communication Computer I2C Address
const byte i2cBallastCtrlAddr     = 0x09;           // Ballast Computer I2C Address
const byte i2ceePROMAddr          = 0x50;           // EEPROM I2C Address

/*******************************
 *     Constants for Sat Modem *
 *******************************/
const unsigned int satdesiredRandomRangeSecs = 60;  //default number of seconds to randomize the message check delay by
const byte satMinimumSignalRequired = 2;			//Minimum signal bars number required to do a normal SBD session - a Forced session will ignore this.
const byte satIncomingPackLenLimit = 70;             //Used to define length of buffer arrays for packet data
const unsigned int maxTelemLenConst = 340;          //Maximum acceptable length of telemetry packet FROM EEPROM, set by Iridium MO max message size
#define satNetworkNotAvailable = 255;
const int LongMsgQueueLen = 20;             // Number of messages that can be in the queue to send out sat modem
const unsigned long satResponseTimeout = 1 * 60UL * 1000UL;       // (ms) Timeout used when waiting for response timeouts
const unsigned int satSBDIXResponseLost = 30000;     // (ms) How much time to wait before assuming SBDIX command failed
const unsigned long satSBDIntervalRandomizePlusMinusMillis = 60000UL;


/*******************************
 *   I2C Incoming Queue
********************************/
#define I2C_MSG_QUEUE_LEN 4
const byte I2CMsgQueueLen = I2C_MSG_QUEUE_LEN;      // Number of messages that can be in the queue of received I2C msgs
const byte i2cMaxDataLen = 15;                      //I2C buffer length for cmd+data
const byte UseI2CForDebugMsgs = 0; 					// 0 == NO, !0 == Yes

/*******************************
 *   Constants for message payload processing (set these based on headers of sat modem provider)
********************************/
const byte packetPayloadStartIndex = 0;  // Message content starts here in a received packet from sat modem 6 for orbcomm, may be 0 for Iridium
const byte satIncomingMessageHeaderLength = 0;  //Length of inbound message headers, 15 for orbcomm, may be 0 for Iridium
const byte i2cRetryLimit = 10;
const unsigned int satMessageCharBufferSize = 340;  //Char array size for loading messages from eeprom into


/*******************************
 *     Constants for I2C Commands to Comm Controller  *
 *******************************/
const byte i2cCmdSATTXATCRpt = 0x00;
const byte i2cCmdSATTxFrmEEPROM = 0x01;
const byte i2cCmdCDNHeartBeat = 0x06;
const byte i2cCmdCDNSetTimerAndReset = 0x07;
const byte i2cCmdUpdateCurrentEpochTime = 0x08;
const byte i2cCmdUpdateThreeNinersValue = 0x09;
const byte i2cCmdCDNCUTDOWNNOW = 0x99;
const byte i2cCmdSATPowerOn = 0xBB;
const byte i2cCmdSATPowerOff = 0xAA;
const byte i2cCmdForceSatSession = 0x20;

const byte i2cRebootCountMax = 5;

/*******************************
 *   Internal EEPROM Allocation       *
 *******************************/
const int EPloadDefaultsFlagAddr = 1;  //If content at this byte is non-zero it will load defaults into eeprom
#define EPCONFIG_VERSION "aam"      // ID of the settings block
#define memoryBase 10		       // Tell it where to store your config data in EEPROM

struct StoreStruct {
	char version[4];  //Config version number
	volatile unsigned int satDesiredSBDSessionInterval;
	byte debuglevel, 
	pinDSR, pinRI, pinNA, pinModemPowerSwitch, 
	i2cmyaddr, i2cuseraddr, operationMode, rebootCount, satMinimumSignalRequired;
};

#endif


