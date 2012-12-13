#include <WSWire.h> // Modified Wire library to add timeouts for I2C bus problems.  http://github.com/whitestarballoon/WSWireLib
#include <avr/io.h>
#include "I2CCommMgr.h"
#include "CommCtrlrConfig.h"
#include "DebugMsg.h"
#include "I2CQueue.h"
#include "CutDown.h"
#include "Iridium9602.h"
#include "SatCommMgr.h"
#include <EEPROMex.h>

#define i2cDebug

extern struct StoreStruct prefs;
extern void savePrefsIfChanged();
/* 
 I2CXmit - Program for communicating via I2C Bus
 Created by White Star Balloon, December 11, 2010.
 Last Revised December 13, 2010
 */

I2CCommMgr::I2CCommMgr(SatCommMgr& satCommMgr):
_satCommMgr(satCommMgr)
{
}

//Check the I2C pins for specific freeze/lockup pattern of constant low SDA.  SDA should float high.
// Returns true when frozen
boolean I2CCommMgr::CheckForI2CFreeze(){
//FIXME in all branches this is not portable!
	boolean sdaValue;
	unsigned int icheck;
	sdaValue = ((PIND & 0b10) >> 1);
	
	for (icheck=0;icheck<1000;icheck++){
		// Look for sda to remain low all the time by ORring the bits of the last and current value.  
		// This should show a 1 after repeated comparisons if there was ever a 1 present in that cyle.
		sdaValue = ((PIND & 0b10) >> 1);
		if (1 == sdaValue) {
			//There's data action or the bus is idle and not frozen, we can leave now.
			return false;
		}
		//Continue checking the bus if it's low, to see if it ever goes high.
		wdtrst();
		delay(10);
		}
	//If we get here, we've got a stuck bus
	Serial.println(F("I2C SDA haz become stuck down."));
	return true;

}


void I2CCommMgr::I2CAliveCheck(){
	 //byte rebootCount=0; 
	 unsigned int countdown=TIMEOUTPERIODs;
	 //rebootCount = EEPROM.read(EPLOCI2CRebootCount);
	 if (CheckForI2CFreeze()) {//If I2C is frozen:
	 	if (prefs.rebootCount < i2cRebootCountMax) {  //and we haven't rebooted too many times yet, reboot:
			
			Serial.print(F("Going down for reboot number "));
			Serial.print(prefs.rebootCount++);  //print and increment the rebootcount
			savePrefsIfChanged();//Write new rebootcount to eeprom for next boot cycle to read
			//EEPROM.write(EPLOCI2CRebootCount,rebootCount);  //Write new rebootcount to eeprom for next boot cycle to read
			Serial.println(F(" in... "));
			wdtrst();
			while (1){
				Serial.println(countdown--);
				delay(1000);
				//Watchdog timer will reboot us in this infinite while loop in about 8 seconds.
			} 
	 	
		} 
		//Have rebooted too many times! Continue checking, but not rebooting.
		if (prefs.rebootCount==i2cRebootCountMax) {
			//SEND A MESSAGE HOME SAYING I2C IS BORKED.
			prefs.rebootCount++;
			//EEPROM.write(EPLOCI2CRebootCount,rebootCount);  //Write new rebootcount to eeprom for next boot cycle to read
			savePrefsIfChanged(); //Write new rebootcount to eeprom for next boot cycle to read
		}
	 } else if (prefs.rebootCount != 0) {
	 	//If it's just recovered, then reset the reboot counter.
	 	prefs.rebootCount = 0;
	 	//EEPROM.write(EPLOCI2CRebootCount,rebootCount);  //Write new rebootcount to eeprom for next boot cycle to read
	 	savePrefsIfChanged();
	 	Serial.println(F("I2C is alive once again, carry on."));
	 	//SEND MESSAGE HOME SAYING I2C ISN'T BORKED ANYMORE.
	 }
}



//I2CMaxRetries should probably be no more than 10 as each retry delay will increase exponentially.
int I2CCommMgr::I2CXmit(byte device, byte command, byte* data, int length)
{
  
  int sentStatus;

  wdtrst();
  // Transmit over I2C
  for (unsigned int i = 1; i < i2cRetryLimit; i++) 
  {
    Wire.beginTransmission(device);                      // Begin Transmission to (address)
#if (ARDUINO >= 100)
    Wire.write(command);                                   // Put (command) on queue
#else
    Wire.send(command);                                   // Put (command) on queue
#endif
    for (int k = 0;k < length; k++)                       // Loop to put all (data) on queue
    {
#if (ARDUINO >= 100)
      Wire.write(data[k]);                                   // Put (command) on queue
#else
      Wire.send(data[k]);                                   // Put (command) on queue
#endif
    }
    sentStatus = Wire.endTransmission();              // Send queue, will return status
    if (sentStatus == 0) 
    {
      break;
    }
    Serial.println(F("I2CTXERR"));
    Serial.flush();
    //If it didn't' suceed for  any reason, try again
    //delay(random(((i-1)*(i-1)*100),(i*i*100)));  //Delay for a random time between (i-1)^2*100 millis i^2*100 millis
    delay(random(100, 5*1000));
  }
  return sentStatus;                                   // Return Status of Transmission	
}

int I2CCommMgr::I2CXmitMsg(byte device, byte* data, int length)
{

  int sentStatus;
  wdtrst();
  // Transmit over I2C
  for (unsigned int i = 1; i < i2cRetryLimit; i++) 
  {
    // Transmit over I2C
    Wire.beginTransmission(device);                      // Begin Transmission to (address)
    for (int k = 0;k < length; k++)                       // Loop to put all (data) on queue
    {
#if (ARDUINO >= 100)
      Wire.write(data[k]);                                   // Put (command) on queue
#else
      Wire.send(data[k]);                                   // Put (command) on queue
#endif
    }
    sentStatus = Wire.endTransmission();              // Send queue, will return status
    if (sentStatus == 0) 
    {
      break;
    }
    Serial.println(F("I2CTXERR"));
    Serial.flush();
    //If it didn't' suceed for  any reason, try again
    delay(random(((i-1)*(i-1)*100),(i*i*100)));  //Delay for a random time between (i-1)^2*100 millis i^2*100 millis
  }
  return sentStatus;                                   // Return Status of Transmission
}

/*
void I2Csend(byte length) {
 do                                                       // Sends data out on the I2C Bus
 { 
 i2csentStatus = I2CXmit(i2caddress, i2ccommand, i2cdata, length);     // Gets return value from endTransmission
 delay(random(1,200));                                   // Random delay in case of delivary failure
 } 
 while (i2csentStatus != 0);                              // Continue until data arrives
 }
 */


void I2CCommMgr::i2cInit()
{
  wdtrst();
  Wire.begin(prefs.i2cmyaddr);                                   // Join I2C Bus as slave
  //FIXME remove this stuff vvvv
#define TWI_FREQ_WSBFAST 400000UL
#ifndef CPU_FREQ
#define CPU_FREQ = 16000000UL
#endif
  //TWBR = ((CPU_FREQ / TWI_FREQ_WSBFAST) - 16) / 2;  // Make I2C FASTER! to 400KHz

  Wire.onReceive(I2CCommMgr::i2cReceiveData);                            // Set On Receive Handler
  DebugMsg::msg_P("I2C",'I',PSTR("I2C Init Done. Addr %x"),prefs.i2cmyaddr);

}



//Wire library interrupt will pass number of bytes to receive
void I2CCommMgr::i2cReceiveData(int wireDataLen) 
{
  int i=0;
  I2CMsg i2cMsg;
	wdtrst();

  // Check to see if the I2C messsage is too big to handle if so throw it away
  if ((wireDataLen -1) > i2cMaxDataLen) {
    Serial.println("2B");
    DebugMsg::msg_P("I2C",'I',PSTR("\ni2cRx: data too big!"));
    while(Wire.available() > 0)           // Loop to receive the Data from the I2C Bus
    {
#if (ARDUINO >= 100)
      Wire.read();   // Finish receiving data and do not store.
#else
      Wire.receive();   // Finish receiving data and do not store.
#endif
    }
  }
  else
  {
    i2cMsg.i2cDataLen = wireDataLen - 1;
#if (ARDUINO >= 100)
    i2cMsg.i2cRxCommand = Wire.read();          // Receive the Command as the first byte from the I2C Bus
#else
    i2cMsg.i2cRxCommand = Wire.receive();          // Receive the Command as the first byte from the I2C Bus
#endif

    while(Wire.available() > 0)           // Loop to receive the Data from the I2C Bus
    {
#if (ARDUINO >= 100)
      i2cMsg.i2cData[i] = Wire.read();
#else
      i2sMsg.i2cData[i] = Wire.receive();
#endif
      i++;
    }
  }
  //  Serial.print("Cmd:");Serial.print(i2cMsg.i2cRxCommand);
#ifdef i2cDebug_DONT_ENABLE  // This makes the serial out too long and causes lockup
  // Printing code for debug usage                                                 
  DebugMsg::msg("I2C",'I',"i2c Packet Rx'd. Cmd: %x Data: ",i2cMsg.i2cRxCommand);
  for (int i = 0;i < i2cMsg.i2cDataLen-1; i++)
  {
    Serial.print(i2cMsg.i2cData[i],HEX); 
    Serial.print(" ")
      //DebugMsg::msg("I2C",'I'," %0x",i2cMsg.i2cData[i]);
    }
    Serial.println();
#endif
  // Store the Command
  I2CQueue::getInstance().write(i2cMsg);
}


/*
  Look at the queue and see if there is anything that needs to be processed.
 */
void I2CCommMgr::update()
{
  wdtrst();
  if (  I2CQueue::getInstance().count() > 0)  // Got a message that needs processing
  {
    I2CParse( I2CQueue::getInstance().read());
    //Serial.print("vvvvvvv Q:"); Serial.println(I2CQueue::getInstance().count());
  }
}



void I2CCommMgr::I2CParse(I2CMsg i2cMsg)
{
 wdtrst();
#if 0
  //DebugMsg::msg_P("I2C",'I',PSTR("I2C Parse"));
  DebugMsg::msg("I2C",'I',("i2c Packet Rx'd. Cmd: %0x Data V "),i2cMsg.i2cRxCommand);
  Serial.flush();
  for (int i = 0;i < i2cMsg.i2cDataLen; i++)
  {
    Serial.print(i2cMsg.i2cData[i],HEX); 
    Serial.print(" ");
  }
  Serial.println();
  Serial.flush();
#endif
  switch(i2cMsg.i2cRxCommand){

  case i2cCmdSATTXATCRpt: 
    { 
      DebugMsg::msg_P("I2C",'I',PSTR("Store ATC Rpt - no function"));
   
      break;
    } 

  case i2cCmdSATTxFrmEEPROM: 
    { 
      //Check for duplicate start and end addresses
      if( (i2cMsg.i2cData[0] == i2cMsg.i2cData[2]) && (i2cMsg.i2cData[1] == i2cMsg.i2cData[3])) {
        DebugMsg::msg_P("I2C",'W',PSTR("LongMsg Start and End Addrs The Same.  No Send."));
      } 
      else {
        DebugMsg::msg_P("I2C",'I',PSTR("Store Long Message"));
        LongMsg msg(i2cMsg.i2cData[0],i2cMsg.i2cData[1],i2cMsg.i2cData[2],i2cMsg.i2cData[3]);

        if (SatQueue::getInstance().write(msg))
        {
          //DebugMsg::msg_P("I2C",'I',PSTR("Report Stored OK."));
        } 
        else {
          DebugMsg::msg_P("I2C",'W',PSTR("Report Store FAILED."));
        }
      }
      break;
    }


  case i2cCmdCDNHeartBeat: 
    { 
      DebugMsg::msg_P("I2C",'I',PSTR("CutDn Heart Beat"));
      CutDown::ResetTimer();
      break;
    }

  case i2cCmdCDNSetTimerAndReset: 
    { 
      DebugMsg::msg_P("I2C",'I',PSTR("CutDn Set Timer and Reset"));
      CutDown::CmdSet(i2cMsg.i2cData[0]);  //Take 1 byte 0-255 for minutes
      break;
    }


  case i2cCmdCDNCUTDOWNNOW: 
    { 
      DebugMsg::msg_P("I2C",'I',PSTR("CutDn Now"));
      CutDown::CutdownNOW();
      break;
    }

  case i2cCmdSATPowerOn: 
    { 
      DebugMsg::msg_P("I2C",'I',PSTR("SatModem ON"));
      _satCommMgr.turnModemOn();
      break;
    }

  case i2cCmdSATPowerOff: 
    {
      DebugMsg::msg_P("I2C",'I',PSTR("SatModem OFF"));
      _satCommMgr.turnModemOff();
      break;
    }
  case i2cCmdForceSatSession:
  	{
  	  DebugMsg::msg_P("I2C",'I',PSTR("Force Sat Session"));
  	  _satCommMgr.initiate_session = true;
      break;
    }

  default:                                               // Ignore any command that is not in the list
    {
      DebugMsg::msg_P("I2C",'E',PSTR("Unknown Command: %0x"), i2cMsg.i2cRxCommand);
    }
  }
}





