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
////////////////////////////////// BEGIN Config

#define DEBUG 1 // debug mode with verbose output over serial at 115200 bps
// v2.3: 20834 vs 19786 debug mode vs non-debug mode
byte nodeID = 11; // Unique Node Identifier (2...254) - also the last byte of the IPv4 adress, not used if USE_EEPROM is set
#define USE_EEPROM // read nodeID and network settings from EEPROM at bootup, overwrites nodeID and MAC.
#define REQUEST_RATE 30000L // request rate of webpage query in ms, for debugging
#define KEEPALIVE_RATE 32768L // request rate of sendKeepalive in ms

// begin timetravel config
#include <EthernetUdp.h>         // UDP library from bjoern@cs.stanford.edu
#define SYNC_RATE 16000L // update interval for time exchange UDP packets between nodes
#define PROCTIME 1912L // 201304162000 aShure, 16Mhz Atmega 328
#define T_DELAY 980L // minimum processing time until T packet is sent out
#define PROC_DELAY 309L; // minimum reaction time to any packet type
#define R_DELAY 1136L // minimum processing time until R packet can be replied
#define R_ADVANCE 414L // Anticipated propagation time, shifts the 'R' time into the middle of the round trip.
// (PROC_DELAY + R_DELAY ) DIV 2 - PROC_DELAY
unsigned int localPort = 8888;      // local UDP port to listen on
unsigned long micros1 = 0;
unsigned long sentTime = 0;
byte nodes[]={
  7,9}; //TODO: read node network topology setup from EEPROM
byte currentNode=0;
byte nodeIDindex=0;
const byte amountNodes=2;
unsigned long nodeStamps[2]; //holds timestamps from the nodes
char packetBuffer[8]; //buffer to hold incoming packet, standard: 24 chars/bytes
EthernetUDP Udp; // An Ethernet UDP instance to let us send and receive packets over UDP
// end timetravel config

#define W5100 // use the Wiznet W5100 ethernet controller
//#define USE_SD // only together with W5100
//#define ETHERCARD // use the Microchip ENC28J60 controller
#include <EEPROM.h>
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

#ifdef W5100
const short CS_SD = 4; // ** CS - pin 4 for SD card
const short CS_ETH = 10; // ** CS - pin 10 for ethernet
#endif

// #define EthernetType 1 // type of ethernet hardware: 0=Microchip ENC28J60, 1=Wiznet W5100

static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x14 }; // ethernet interface mac address, not used if USE_EEPROM is set
static char website[] = "etemu.com"; // remote server, TLD/vHost
IPAddress website_ip(192,168,8,113); // the IP from the website - optional, used for debugging purposes

static char website_url[] = "/satar/SatarServer/satar.php"; // URL pointing to SatarServer's satar.php, leading slash, no trailing slash.
// vetu_anna: http://192.168.8.113/satar/SatarServer/satar.php
// etemu.com lab1: http://etemu.com/lab1/satar.php

#ifdef W5100 //if using W5100

IPAddress ip(192,168,8,nodeID); // static IP if DHCP is disabled
IPAddress gw(192,168,8, 1); // static gateway IP if DHCP is disabled
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

volatile boolean oneTriggered=0; // flag which will be set to 1 in an ISR if the interrupt triggers
volatile boolean twoTriggered=0; // same, but for second portpin (e.g. finish line)
boolean trigger_one_armed=0; // 0 = not armed, 1 = trigger is armed and listens at input
boolean trigger_two_armed=0; // 0 = not armed, 1 = trigger is armed and listens at input
unsigned int debounceCountsOne=0;
unsigned int debounceCountsTwo=0;
unsigned long oneTriggeredMillis=0; 
unsigned long twoTriggeredMillis=0; 
volatile unsigned long oneTriggeredMicros=0;
volatile unsigned long twoTriggeredMicros=0;
byte typeEvent=2;

const unsigned int triggerIntervalOne=1000; //minimum time between two occuring start trigger events in ms
const unsigned int triggerIntervalTwo=1000; 
const int onePin = 2;	// the number of the input pin, ISR only at portpins 2 and 3 (AtMega328)
const int twoPin = 3; 

unsigned long timer_ms = 0;
unsigned long timer_us = 0;
static boolean cardLog = 0;

// Timer t; // Timer lib for non-blocking periodic calls

void printRAM(){
  #ifdef USE_SD
  Serial.print("RAM: ");
  Serial.print(FreeRam());
  Serial.println(" bytes free.");
  #endif
}

void sendStatus(int nodeStatus=1){ // keepalive packet to the server, doubles as a status packet if the inputs are armed
  #ifdef DEBUG
  Serial.println("DEB: Emit heartbeat <3:");
  #endif
  byte armedID = 1;
//  armedID = trigger_two_armed << 1;  //B000000X0
//  armedID = armedID + trigger_one_armed; // B0000000X  
  forgePacket(millis(),nodeStatus,armedID);
}

void setup () {
  Serial.begin(115200); // init serial connection for debugging
  #ifdef USE_SD
  pinMode(CS_SD, OUTPUT); // CS SD card
  pinMode(CS_ETH, OUTPUT); // CS hardware SPI, e.g. ethernet
  #endif
  pinMode(onePin, INPUT); //ISR only at portpins 2 and 3
  pinMode(twoPin, INPUT); //ISR only at portpins 2 and 3 TODO: fix the wiring
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Arduino Leonardo < only
  }
  Serial.println("\n_____________________");
  Serial.println("== [SATAR Controller]");
  Serial.println("== CC-BY-SA-NC 3.0");
  Serial.print("STR: Node ID: ");
  #ifdef USE_EEPROM
  nodeID=EEPROM.read(0);
  Serial.print("EEPROM, ");
  #endif
  Serial.println(nodeID);
  #ifdef W5100
  Serial.println("ETH: (SPI) Wiznet W5100 (100-baseT).");
  #ifdef USE_EEPROM
  static byte mac[] = { EEPROM.read(1), EEPROM.read(2), EEPROM.read(3), EEPROM.read(4), EEPROM.read(5), EEPROM.read(6) }; // ethernet interface mac address
  IPAddress ip(EEPROM.read(7), EEPROM.read(8), EEPROM.read(9), EEPROM.read(10)); // static IP of the node
  IPAddress gw(EEPROM.read(11), EEPROM.read(12), EEPROM.read(13), EEPROM.read(14)); // static IP of the gateway
  #endif
  Ethernet.begin(mac, ip, gw, gw, subnet);
  Udp.begin(localPort);
  Serial.print("ETH: Node IP: ");
  Serial.println(Ethernet.localIP());
  #endif
  
  #ifdef ETHERCARD
  Serial.println("ETH: (SPI) Microchip ENC28J60 (10-baseT).");
  if (ether.begin(sizeof Ethernet::buffer, mac) == 0){ 
  Serial.println( "ETH:!Failed to access ENC28J60 controller.");}
  ether.staticSetup(ip, gw);
  Serial.print("ETH: Satar Node IP: ");
  ether.printIp("ETH: IP: ", ether.myip);
  ether.printIp("ETH: GW: ", ether.gwip);
  ether.printIp("ETH: DNS: ", ether.dnsip);
  //Serial.println(Ethernet.localIP());
  
   if (!ether.dnsLookup(website))
    Serial.println("ETH: DNS lookup failed.");
  ether.printIp("ETH: Server IP: ", ether.hisip);
  #endif
  
  // digitalWrite(onePin, HIGH); //20kOhm internal pull-up enable 
  // digitalWrite(twoPin, HIGH); //20kOhm internal pull-up enable
  // Serial.println("ISR: 1 UP.");
  // attachInterrupt(0, trigger_one, FALLING); //0 = portpin 2
  // attachInterrupt(1, trigger_two, FALLING); //1 = portpin 4
  
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
    logPacketToCard("BOOT"); //
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
pinMode(5, OUTPUT); // LED at pin 5, on when busy with sending and receiving in subfunction.

  // enumerate the nodes and check which index we have
 for (byte i = 0; (i+1)<=amountNodes ; i++) { //check which array index is our nodeID
    if(nodes[i]==nodeID) nodeIDindex=i;
  }
  Serial.print("nodeIDindex: ");
  Serial.print(nodeIDindex);
  Serial.print(", nodes[nodeIDindex]: ");
  Serial.println(nodes[nodeIDindex]);

  sendStatus(0); // send the initial status packet, 0 = bootup successful
}



void loop () {

  checkTriggerOne();
  // checkTriggerTwo();
  //  timer_us=micros();
   
  if (millis() > timer_ms + KEEPALIVE_RATE) {
      sendStatus(1); //send a heartbeat packet to the server and signal our health, also used for time sync
      timer_ms = millis();
    }
  #ifdef ETHERCARD
    ether.packetLoop(ether.packetReceive()); // read out the ethernet buffer frequently.
  #endif
  
  #ifdef W5100
  if (millis() > timer_ms + SYNC_RATE) {
    timeTravel(); // send out a TimeTravel packet to a node from the node list array 'nodes[]'.
  }
  recvUdp();  // call the UDP handler - if there's data available, read a packet and act accordingly.
  digitalWrite(5, HIGH); //LED at pin 3 as a status indicator, high when busy.
  eth_reply_w5100(); // read out the ethernet buffer frequently.
  #endif
 
  digitalWrite(5, LOW); //LED at pin 3 as status-indicator
}

