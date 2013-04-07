//         __                                            
//   ___  / /____  ____ ___  __  __  _________  ____ ___ CC
//  / _ \/ __/ _ \/ __ `__ \/ / / / / ___/ __ \/ __ `__ \
// /  __/ /_/  __/ / / / / / /_/ /_/ /__/ /_/ / / / / / /
// \___/\__/\___/_/ /_/ /_/\__,_/(_)___/\____/_/ /_/ /_/ 
//
// SATAR controller
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
//      ____      ____    _____    ____      ____ CC 
//  ___(_ (_`____/ () \__|_   _|__/ () \____| () )_____   
//    .__)__)   /__/\__\   |_|   /__/\__\   |_|\_\
//  System for Advanced Timekeeping and Amateur Racing.
//
// This firmware is meant for a SATAR controller node, which
// communicates to to a remote server over TCP/IP.
// Hardware: Atmel AtMega 328 or higher, Microchip ENC28J60
// Uses hardware SPI enabled with eth0 SPI CSN at port 10.
// AtMega clocked at 16Mhz, Microchip clocked at 25Mhz.
// CC-BY-SA 3.0: This work is Open Source and licensed under
// the Creative Commons Attribution-ShareAlike 3.0 License.

// #include <EtherCard.h> // library with simple IP + TCP stack for ENC28J60
#include <SPI.h>
#include <Ethernet.h> // library to interface W5100 with integrated TCP stack
#include <SD.h> // library to interface the Micro-SD card
// ** MOSI - pin 11
// ** MISO - pin 12
// ** CLK - pin 13
// ** CS - pin 4 for SD card interface
// ** CS - pin 10 for ethernet interface

////////////////////////////////// BEGIN Config

#define nodeID 42 // Unique Node Identifier (1...254) - also the last byte of the IPv4 adress

const short CS_SD = 4; // ** CS - pin 4 for SD card
const short CS_ETH = 10; // ** CS - pin 10 for ethernet
#define DEBUG 1 // debug mode with verbose output over serial at 115200 bps
#define EthernetType 1 // type of ethernet hardware: 0=Microchip ENC28J60, 1=Wiznet W5100
#define DHCP 0 //disable or enable DHCP client NOTE: DHCP not yet implemented
#define REQUEST_RATE 30000 // request rate of webpage query in ms, for keepalive or debugging

IPAddress ip(192,168,8,nodeID); // static IP if DHCP is disabled
IPAddress gw(192,168,8, 1); // static gateway IP if DHCP is disabled
IPAddress subnet(255, 255, 255, 0); // static subnet if DHCP is disabled
static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x05 }; // ethernet interface mac address
static char website[] = "etemu.com"; // remote server, TLD/vHost

////////////////////////////////// END Config

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that we want to connect to (port 80 = default for HTTP):
EthernetClient client;
volatile boolean startTriggered=0; // flag which will be set to 1 in an ISR if the interrupt triggers
volatile boolean finishTriggered=0; // same, but for second portpin (e.g. finish line)
boolean trigger_start_armed=0; // 0 = not armed, 1 = trigger is armed and listens at input
boolean trigger_finish_armed=0; // 0 = not armed, 1 = trigger is armed and listens at input
unsigned int debounceCountsStart=0;
unsigned int debounceCountsFinish=0;
unsigned long startTriggeredMillis=0; 
unsigned long finishTriggeredMillis=0; 
byte typeEvent=2;
unsigned long ID=nodeID;

const unsigned int triggerIntervalStart=700; //minimum time between two occuring start trigger events in ms
const unsigned int triggerIntervalFinish=700; 
const int startPin = 2;	// the number of the input pin, ISR only at portpins 2 and 4 (AtMega328)
const int finishPin = 4; 

static long timer;
static long timer_micros;
static boolean cardLog=0;

void setup () {
  Serial.begin(115200); // init serial connection for debugging
  pinMode(CS_SD, OUTPUT); // CS SD card
  pinMode(CS_ETH, OUTPUT); // CS hardware SPI, e.g. ethernet
  pinMode(startPin, INPUT); //ISR only at portpins 2 and 4
//  pinMode(finishPin, INPUT); //ISR only at portpins 2 and 4 TODO: fix the wiring and reroute CS_SD!
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Arduino Leonardo < only
  }

//============= BEGIN SD CARD INIT

printRAM();

digitalWrite(CS_ETH, HIGH); // CS ethernet
digitalWrite(CS_SD, LOW); // CS SD
  if (!SD.begin(CS_SD)) { 
    Serial.println("! SD card init failed or no card present.");
    static boolean cardLog=0;
  }
  else {
    Serial.println("== SD card initialized.");
    static boolean cardLog=1;
    logPacketToCard("REBOOT"); //
    logPacketToCard("REBOOTREBOOT"); //    
    // logPacketToCard("DEBUGDEBUG"); // debug
    // logPacketToCard("TripleDebug"); // debug
  }
digitalWrite(CS_SD, HIGH); // CS SD
digitalWrite(CS_ETH, LOW); // CS ethernet

//============= END SD CARD INIT
  
printRAM();

  Serial.println("\n=====================");
  Serial.println("== [SATAR Controller]");
  Serial.println("== CC-BY-SA-NC 3.0");
  Serial.print("== nodeID: ");
  Serial.print(nodeID);
  Serial.println("\n== Eth Connection: (SPI) Wiznet W5100 (100-baseT)");
  Ethernet.begin(mac, ip, gw, gw, subnet);
  Serial.print("== SATAR node IP: ");
  Serial.println(Ethernet.localIP());
  timer = - REQUEST_RATE; // start timing out right away
  // digitalWrite(startPin, HIGH); //20kOhm internal pull-up enable 
  // digitalWrite(finishPin, HIGH); //20kOhm internal pull-up enable
  // attachInterrupt(0, trigger_start, RISING); //0 = portpin 2
  // attachInterrupt(1, trigger_finish, RISING); //1 = portpin 4

printRAM();

}

void loop () {
  // DHCP expiration is a bit brutal, because all other ethernet activity and
  // incoming packets will be ignored until a new lease has been acquired
  //  if (ether.dhcpExpired() && !ether.dhcpSetup()) {
  //    Serial.println("ERR: DHCP renewal failed.");
  //  }

  // checkTrigger0();
  // checkTrigger1();
  //  ether.packetLoop(ether.packetReceive());

  if (DEBUG) {
    if (millis() > timer + REQUEST_RATE) {
      timer = millis();
      forgePacket(timer,1,nodeID); //send a packet for testing purposes
    }
  }

  eth_reply_w5100(); // read out the ethernet buffer frequently.

}

void printRAM(){
  Serial.print("RAM available: ");
  Serial.print(FreeRam()*100/2048);
  Serial.println("%");
}

