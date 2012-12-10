
#ifndef SatCommMgr_h
#define SatCommMgr_h

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <string.h>
#include "CommCtrlrConfig.h"
#include "LongMsg.h"
#include "SatQueue.h"

class I2CCommMgr;

class SatCommMgr
{

public:
        SatCommMgr(Iridium9602& satModem);
        
        void satCommInit(I2CCommMgr * i2cCommMgr);
        void issueDirectCmd(char *str);
        void sendBinaryMsg(char *str, ...);
        #ifdef _txtmsgworking
		void sendTextMsg(String);
		void sendTextMsg(const char*);
		#endif
        void update(void);
        void turnModemOn();
        void turnModemOff();
        bool initiate_session;
        bool satModemAlive;
        void randomizeMessageCheckInterval(unsigned long);

        
		
private:
        Iridium9602& _satModem;
        void sendShortMsg(ShortMsg sm);
        void sendLongMsg(unsigned char * mstr, int len);

        unsigned long _last_millis;
        //SatQueue& satQueue;
        I2CCommMgr * _i2cCommMgr;
        /* last time a session was know to be initiated */
        unsigned long _lastSessionTime;
        /* last time we tried to initiate a session */
        unsigned long _lastActivityTime;
        int _retryTimeIdx;
        unsigned long  _randomizedRetryTime;
		byte packetBufferA[i2cMaxDataLen]; 
        void parseIncommingMsg(unsigned char* packetBufferLocal,unsigned int packLen);
        void satCommCommandProc(unsigned char * packetBufferLocal);
        //void satConfirmTimerCHG(byte timerValue);
		
};

#endif





