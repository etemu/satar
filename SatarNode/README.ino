// This firmware is meant for a SATAR controller node, which
// communicates to to a remote server over TCP/IP.
// Hardware: Atmel AtMega 328 or higher, Microchip ENC28J60
// Uses hardware SPI enabled with eth0 SPI CSN at port 10.
// AtMega clocked at 16Mhz, Microchip clocked at 25Mhz.
// CC-BY-SA 3.0: This work is Open Source and licensed under
// the Creative Commons Attribution-ShareAlike 3.0 License.
//
// SATAR node controller
// 2.1b 201205061419 Shure: port to Microchip ENC28J60
// 2.2b 201205091412 Shure: added dhcp functionality
// 2.3b 201205091919 Shure: port to WizNet W5100
// 2.6b 201205100356 Shure: building the TCP/IP payload
// 2.7b 201205101733 Shure: bugfixing, TSN did not rise
// 2.8b 201205121620 Shure: outsourced the W5100 code
// 2.9b 201205150149 Shure: merge satar_timing from satar6
// 2.10 201205150332 Shure: interrupt service routine redesign
// 2.11 201206071741 Shure: fork to W5100 due to buggy ISR+ENC28J60
// 2.12 201209010523 Shure: SD card logging (out of RAM :| )
// 2.13 201304060158 Shure: decrease payload buffer 48->42
// 2.14 201304062342 Shure: fix MAC + http request forging
// 2.15 201304070211 Shure: strip out the http reply answer's header
// 2.16 201304070317 Shure: no consistent connectivity (WIP)
// 2.17 201304070400 Shure: outsource chip dependent code
// 2.18 201304071818 Shure: implement code for ENC28J60
// 2.19 201304081924 Shure: keepalive packet forging
// 2.20 201304131925 Shure: read out MAC+nodeID from EEPROM
// 2.21 201304200212 Shure: Fix IP compilation and read out Gateway - 18576 bytes
// 2.30 201304200319 Shure: merge TimeTravel into SatarNode - 20834 bytes
// 2.31 201304201645 Shure: 20432 bytes
// 
// ** MOSI - pin 11
// ** MISO - pin 12
// ** CLK - pin 13
// ** CS - pin 4 for SD card interface
// ** CS - pin 10 for W5100 ethernet interface
// ** CS - pin 8 for ENC28J60 ethernet interface
//
