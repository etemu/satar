![IMG](Graphics/SatarServerRuby_gui/SatarServer_header.png)

System for Advanced Timekeeping and Amateur Racing.
Compatible with the the Arduino platform. 
```
         __                                            
   ___  / /____  ____ ___  __  __  _________  ____ ___ CC
  / _ \/ __/ _ \/ __ `__ \/ / / / / ___/ __ \/ __ `__ \
 /  __/ /_/  __/ / / / / / /_/ /_/ /__/ /_/ / / / / / /
 \___/\__/\___/_/ /_/ /_/\__,_/(_)___/\____/_/ /_/ /_/ 

 SATAR controller
 2.1b 201205061419 Shure: port to Microchip ENC28J60
 2.2b 201205091412 Shure: added dhcp functionality
 2.3b 201205091919 Shure: port to WizNet W5100
 2.6b 201205100356 Shure: building the TCP/IP payload
 2.7b 201205101733 Shure: bugfixing, TSN did not rise
 (...) further changes are documented via the git repo.
 
      ____      ____    _____    ____      ____ CC 
  ___(_ (_`____/ () \__|_   _|__/ () \____| () )_____   
    .__)__)   /__/\__\   |_|   /__/\__\   |_|\_\
  System for Advanced Timekeeping and Amateur Racing.
```
 This firmware is meant for a SATAR controller node, which
 communicates to to a remote server over TCP/IP.
 Hardware: Atmel AtMega 328 or higher, Microchip ENC28J60
 Uses hardware SPI enabled with eth0 SPI CSN at port 10.
 AtMega clocked at 16Mhz, Microchip clocked at 25Mhz.
 CC-BY-SA 3.0: This work is Open Source and licensed under
 the Creative Commons Attribution-ShareAlike 3.0 License.
 
 Compatible with the the Arduino platform.

 I tried to have full compatibility with the Arduino Bootloader,
 the official Arduino Ethernet Shield and the Arduino 1.0 IDE
 and up until now, everything compiles fine on Arduino 1.0!

 Alex Shure, 2012-05 

 ___      __    __                 
/\_ \   /'__`\ /\ \                
\//\ \ /\_\L\ \\ \ \/'\     ___    
  \ \ \\/_/_\_<_\ \ , <   /' _ `\  
   \_\ \_/\ \L\ \\ \ \\`\ /\ \/\ \ 
   /\____\ \____/ \ \_\ \_\ \_\ \_\
   \/____/\/___/   \/_/\/_/\/_/\/_/

Even more ASCII-Art

Leon Rische, 2013-04
