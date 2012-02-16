#ifndef I2CCommMgr_h
#define I2CCommMgr_h

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <string.h>
#include "CommCtrlrConfig.h"

#include "I2CMsg.h"


class I2CCommMgr
{

public:
        I2CCommMgr();
        void i2cInit(void);

        /*
	static I2CCommMgr& getInstance() 
        {
          static I2CCommMgr instance;
          return instance;
        }
        */
        
        int I2CXmit(byte device, byte command, byte* data, int length);
        void update();
        void I2CParse(I2CMsg i2cMsg);
        
private:
        static void i2cReceiveData(int wireDataLen);

};

#endif



