Dan's Iridium Controller 
Email: dan@balloonconsulting.com

REQUIRES the custom library WSWire: http://github.com/whitestarballoon/WSWireLib
Do not use with the stock Arduino 1.0 Wire Library, or it will freeze in some I2C situations.

Current hardware supported:
Arduino MEGA 2560

Iridium 9602 modem should be connected as follows:
Asterisks indicate the minimum required connections, the rest are optional, but recommended.
Arduino Mega Pin - Modem Connection
0 - *SERIAL TERMINAL ASCII Status Monitor Terminal RX Input  (115200 Baud 8N1)
1 - *SERIAL TERMINAL ASCII Status Monitor Terminal TX Output (115200 Baud 8N1)
2 - IRIDIUM Ring Indicator (RI)              [ROCKBLOCK PIN 8 RING]
3 - IRIDIUM Network Available                [ROCKBLOCK PIN 9 NET]
4 - *IRIDIUM Power Enable (On/Off)           [ROCKBLOCK PIN 7 SLEEP]
5 - IRIDIUM 3.3V Supply Enable
6 - IRIDIUM DSR (rises on boot complete)
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
18 - *IRIDIUM Serial TXO
19 - *IRIDIUM Serial RXI
20 - I2C SCL: To EEPROM and User Microcontroller
21 - I2C SDA: To EEPROM and User Microcontroller

This software is licensed under the MIT license.  See LICENSE.txt for details.