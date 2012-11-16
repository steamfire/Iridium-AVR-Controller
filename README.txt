This is the White Star Balloon Comm Controller Software for Arduino. 

REQUIRES the custom library WSWire: http://github.com/whitestarballoon/WSWireLib
Do not use with the stock Arduino 1.0 Wire Library, or it will freeze in some I2C situations.

Current hardware supported:
Arduino MEGA 2560

Iridium 9602 modem should be connected as follows:

Arduino Pin / Connection
0  Serial ASCII Status Monitor Terminal RX Input  (115200 Baud 8N1)
1  Serial ASCII Status Monitor Terminal TX Output (115200 Baud 8N1)
2  IRIDIUM Ring Indicator (RI)
3  IRIDIUM Network Available
4  IRIDIUM Power Enable (On/Off)
5  IRIDIUM 3.3V Supply Enable
6  IRIDIUM DSR (rises on boot complete)
7  

8  
9  
10 
11 
12 
13 

14  
15  
16 
17 
18 IRIDIUM Serial TXO
19 IRIDIUM Serial RXI
20 I2C SCL - To EEPROM and User Microcontroller
21 I2C SDA - To EEPROM and User Microcontroller

This software is licensed under the MIT license.  See LICENSE.txt for details.