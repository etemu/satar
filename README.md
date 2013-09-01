![IMG](ui/public/images/satarserver_header.png)

System for Advanced Timekeeping and Amateur Racing.
See LICENSE file for licensing information.

This software package is meant for a SATAR controller
 node, which communicates to to a remote server over
 TCP/IP, and a LAMP/Ruby/Redis server.
 Node hardware: Atmel AtMega 328/1280 or higher,
 Microchip ENC28J60 or Wiznet W5100 via SPI,
 optional SD card at CSN4.
 Uses hardware SPI enabled with eth0 SPI CSN at port 10.
 AtMega clocked at 16Mhz, Microchip clocked at 25Mhz.
 
 Node firmware compatible with the the Arduino platform.

 I tried to have full compatibility with the Arduino Bootloader,
 the official Arduino Ethernet Shield and the Arduino 1.5.3 IDE
 and up until now, everything compiles fine on Arduino 1.0!

 2013 etemu.com, Alex Shure, Leon Rische


```
         __                                            
   ___  / /____  ____ ___  __  __  _________  ____ ___ CC
  / _ \/ __/ _ \/ __ `__ \/ / / / / ___/ __ \/ __ `__ \
 /  __/ /_/  __/ / / / / / /_/ /_/ /__/ /_/ / / / / / /
 \___/\__/\___/_/ /_/ /_/\__,_/(_)___/\____/_/ /_/ /_/ 

  
      ____      ____    _____    ____      ____ CC 
  ___(_ (_`____/ () \__|_   _|__/ () \____| () )_____   
    .__)__)   /__/\__\   |_|   /__/\__\   |_|\_\
  System for Advanced Timekeeping and Amateur Racing.
 
         ___      __    __                 
        /\_ \   /'__`\ /\ \                
        \//\ \ /\_\L\ \\ \ \/'\     ___    
          \ \ \\/_/_\_<_\ \ , <   /' _ `\  
           \_\ \_/\ \L\ \\ \ \\`\ /\ \/\ \ 
           /\____\ \____/ \ \_\ \_\ \_\ \_\
           \/____/\/___/   \/_/\/_/\/_/\/_/
```
