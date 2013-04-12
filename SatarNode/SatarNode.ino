//         __                                            
//   ___  / /____  ____ ___  __  __  _________  ____ ___ CC
//  / _ \/ __/ _ \/ __ `__ \/ / / / / ___/ __ \/ __ `__ \
// /  __/ /_/  __/ / / / / / /_/ /_/ /__/ /_/ / / / / / /
// \___/\__/\___/_/ /_/ /_/\__,_/(_)___/\____/_/ /_/ /_/ 
//
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
//
////////////////////////////////// BEGIN Config

#define DEBUG 1 // debug mode with verbose output over serial at 115200 bps
#define nodeID 11 // Unique Node Identifier (1...254) - also the last byte of the IPv4 adress
#define REQUEST_RATE 30000L // request rate of webpage query in ms, for debugging
#define KEEPALIVE_RATE 32000L // request rate of sendKeepalive in ms

#define W5100
//#define USE_SD // only together with W5100
//#define ETHERCARD

// #include <EtherCard.h> // 
#include <SPI.h>
//#include <Timer.h> //Timer lib for non blocking delay

#ifdef USE_SD
#include <SD.h> // library to interface the Micro-SD card
#endif

#ifdef W5100
#include <Ethernet.h> // library to interface W5100 with integrated TCP stack
#endif

#ifdef ETHERCARD
#include <EtherCard.h> // library to interface ENC28J60 with integrated TCP stack
#endif
// ** MOSI - pin 11
// ** MISO - pin 12
// ** CLK - pin 13
// ** CS - pin 4 for SD card interface
// ** CS - pin 10 for ethernet interface

#ifdef W5100
const short CS_SD = 4; // ** CS - pin 4 for SD card
const short CS_ETH = 10; // ** CS - pin 10 for ethernet
#endif

// #define EthernetType 1 // type of ethernet hardware: 0=Microchip ENC28J60, 1=Wiznet W5100
#define DHCP 0 //disable or enable DHCP client NOTE: DHCP not yet implemented

static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x08 }; // ethernet interface mac address
static char website[] = "etemu.com"; // remote server, TLD/vHost

#ifdef W5100 //if using W5100

// IPAddress ip(192,168,8,nodeID); // static IP if DHCP is disabled
IPAddress ip(192,168,23,250); // static IP if DHCP is disabled
IPAddress gw(192,168,23, 1); // static gateway IP if DHCP is disabled
IPAddress subnet(255, 255, 255, 0); // static subnet if DHCP is disabled
#endif

#ifdef ETHERCARD //if using ENC28J60
static byte ip[] = { 192,168,8,nodeID }; // static IP if DHCP is disabled
static byte gw[] = { 192,168,8,1 }; // static gateway IP if DHCP is disabled
byte Ethernet::buffer[400];   // tcp/ip buffer for ENC28J60
#endif

////////////////////////////////// END Config

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that we want to connect to (port 80 = default for HTTP):

#ifdef W5100
EthernetClient client;
//EthernetServer server;
#endif

volatile boolean startTriggered=0; // flag which will be set to 1 in an ISR if the interrupt triggers
volatile boolean finishTriggered=0; // same, but for second portpin (e.g. finish line)
boolean trigger_start_armed=1; // 0 = not armed, 1 = trigger is armed and listens at input
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

static long timer_ms;
static long timer_us;
static boolean cardLog=0;
boolean LEDstate=LOW;

// Timer t; // Timer lib for non-blocking periodic calls

void printRAM(){
  Serial.print("RAM: ");
  #ifdef USE_SD
  Serial.print(FreeRam());
  #else
  Serial.print("?"); //FreeRam() is included in the SD card lib - we can't use it if we don't use the lib.
  #endif
  Serial.println(" bytes free.");
}

void sendStatus(int nodeStatus=2){ // keepalive packet to the server, doubles as a status packet if the inputs are armed
  #ifdef DEBUG
  Serial.println("DEB: Emit heartbeat <3.");
  #endif
  unsigned int armedID = 0;
  armedID = trigger_start_armed; // B0000000X
  armedID = trigger_finish_armed << 1;  //B000000X0
  forgePacket(millis(),nodeStatus,armedID);
}

void setup () {
  Serial.begin(115200); // init serial connection for debugging
  #ifdef USE_SD
  pinMode(CS_SD, OUTPUT); // CS SD card
  pinMode(CS_ETH, OUTPUT); // CS hardware SPI, e.g. ethernet
  #endif
  pinMode(startPin, INPUT); //ISR only at portpins 2 and 4
//  pinMode(finishPin, INPUT); //ISR only at portpins 2 and 4 TODO: fix the wiring and reroute CS_SD!
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Arduino Leonardo < only
  }

  Serial.println("\n=====================");
  Serial.println("== [SATAR Controller]");
  Serial.println("== CC-BY-SA-NC 3.0");
  Serial.print("== nodeID: ");
  Serial.println(nodeID);
  #ifdef W5100
  Serial.println("== ETH: (SPI) Wiznet W5100 (100-baseT).");
  Ethernet.begin(mac, ip, gw, gw, subnet);
  Serial.print("== SATAR node IP: ");
  Serial.println(Ethernet.localIP());
  #endif
  
  #ifdef ETHERCARD
  Serial.println("== ETH: (SPI) Microchip ENC28J60 (10-baseT).");
  if (ether.begin(sizeof Ethernet::buffer, mac) == 0){ 
  Serial.println( "== ETH: Failed to access ENC28J60 controller.");}
  ether.staticSetup(ip, gw);
  Serial.print("== SATAR node IP: ");
  ether.printIp("ETH: IP: ", ether.myip);
  ether.printIp("ETH: GW: ", ether.gwip);
  ether.printIp("ETH: DNS: ", ether.dnsip);
  //Serial.println(Ethernet.localIP());
  
   if (!ether.dnsLookup(website))
    Serial.println("ETH: DNS lookup failed.");
  ether.printIp("ETH: Server IP: ", ether.hisip);
  #endif
  
  timer_ms = - REQUEST_RATE; // start timing out right away
  // digitalWrite(startPin, HIGH); //20kOhm internal pull-up enable 
  // digitalWrite(finishPin, HIGH); //20kOhm internal pull-up enable
  // attachInterrupt(0, trigger_start, RISING); //0 = portpin 2
  // attachInterrupt(1, trigger_finish, RISING); //1 = portpin 4
  
//============= BEGIN SD CARD INIT
#ifdef USE_SD
#ifdef W5100
digitalWrite(CS_ETH, HIGH); // CS ethernet
#endif W5100
digitalWrite(CS_SD, LOW); // CS SD
  if (!SD.begin(CS_SD)) { 
    Serial.println(" SD: Init failed or no card present.");
    static boolean cardLog=0;
  }
  else {
    Serial.println(" SD: Card initialized.");
    static boolean cardLog=1;
    logPacketToCard("REBOOT"); //
    // logPacketToCard("DEBUGDEBUG"); // debug
    // logPacketToCard("TripleDebug"); // debug
  }
digitalWrite(CS_SD, HIGH); // CS SD
#ifdef W5100
digitalWrite(CS_ETH, LOW); // CS ethernet
#endif W5100
#else
    Serial.println(" SD: Interface disabled.");
#endif
//============= END SD CARD INIT

printRAM();

if (DEBUG) {
  pinMode(3, OUTPUT); //LED at pin 3
  
/*  int tkeepalive = t.every(KEEPALIVE_RATE, sendHeartbeat); // create a thread to send a heartbeat to the server
  Serial.print(" PS: Heartbeat thread: ");
  Serial.println(tkeepalive);
  */
  


/*

  int ledEvent = t.oscillate(3, 25, HIGH);
  Serial.print(" PS: LED thread: ");
  Serial.println(ledEvent);
  */
  
      #ifdef W5100
      sendStatus();
      //forgePacket(timer_ms,1,nodeID); //send a packet for testing purposes
      #endif
 }
}



void loop () {

  delay(10);
  
  // t.update(); //check for active timer threads //Timer lib for non blocking delay
  // checkTrigger0();
  // checkTrigger1();
  
//  timer_ms=millis();
//  timer_us=micros();
  digitalWrite(3, HIGH); //LED at pin 3 as freeze-indicator  
  
  #ifdef ETHERCARD
    ether.packetLoop(ether.packetReceive()); //pump the network frequently to handle all incoming packets.
  #endif
  
  #ifdef W5100
  eth_reply_w5100(); // read out the ethernet buffer frequently.
  #endif
  
  /* if (millis() > timer_ms + KEEPALIVE_RATE) {
      timer_ms = millis();
      #ifdef W5100
      sendStatus(1); //send a packet for testing purposes
      #endif
    }
  */
  digitalWrite(3, LOW); //LED at pin 3 as freeze-indicator
}

