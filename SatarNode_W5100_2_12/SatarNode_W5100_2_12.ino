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

// #include <EtherCard.h> // library with simple IP + TCP stack 
#include <SPI.h>
#include <Ethernet.h> // library to interface W5100 with integrated TCP stack
#include <SD.h> // library to interface the Micro-SD card,
// ** MOSI - pin 11
// ** MISO - pin 12
// ** CLK - pin 13
// ** CS - pin 4 for SD card

////////////////////////////////// BEGIN Config

const int CS_SD = 4; // ** CS - pin 4 for SD card
#define DEBUG 1 // debug mode with verbose output over serial at 115200 
#define nodeID 20
#define EthernetType 1 // type of ethernet hardware: 0=Microchip ENC28J60, 1=Wiznet W5100
#define DHCP 0 //disable or enable DHCP client
#define REQUEST_RATE 5000 // request rate of webpage query in ms, for keepalive or debugging

IPAddress ip(192,168,8,nodeID); // static IP if DHCP is disabled
IPAddress gw(192,168,8, 1); // static gateway IP if DHCP is disabled
IPAddress subnet(255, 255, 255, 0); // static gateway IP if DHCP is disabled
static byte mac[] = { 
  0x01,0x01,0x01,0x01,0x32,nodeID }; // ethernet interface mac address
static char website[] PROGMEM = "www.etemu.com"; // remote server, TLD/vHost
IPAddress server(83,169,41,76); // remote server, IP, comma separated

////////////////////////////////// END Config

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that we want to connect to (port 80 is default for HTTP):
EthernetClient client;
volatile boolean startTriggered=0; // flag which will be set to 1 if the interrupt triggers
volatile boolean finishTriggered=0; // flag which will be set to 1 if the interrupt triggers
boolean trigger_start_armed=0; // 0 = not armed, 1 = trigger is armed and listens at input
boolean trigger_finish_armed=0; // 0 = not armed, 1 = trigger is armed and listens at input
unsigned int debounceCountsStart=0;
unsigned int debounceCountsFinish=0;
unsigned long startTriggeredMillis=0; 
unsigned long finishTriggeredMillis=0; 
byte typeEvent=2;
unsigned long ID=nodeID;

const unsigned int triggerIntervalStart=700; //minimum time between two occuring start trigger events in ms
const unsigned int triggerIntervalFinish=700; //minimum time between two occuring finish trigger events in ms
const int startPin = 2;	// the number of the input pin, ISR only at portpins 2 and 4
const int finishPin = 4; // the number of the input pin, ISR only at portpins 2 and 4

static long timer;
static long timer_micros;
static boolean cardLog=0;

void setup () {
  Serial.begin(115200); // init serial connection for debugging
  pinMode(4, OUTPUT); // CS SD card
  pinMode(10, OUTPUT); // CS hardware SPI, e.g. ethernet
  pinMode(startPin, INPUT); //ISR only at portpins 2 and 4
//  pinMode(finishPin, INPUT); //ISR only at portpins 2 and 4
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Arduino Leonardo < only
  }
  
printRAM();

  Serial.println("\n======================");
  Serial.println("=== [SATAR Controller]");
  Serial.println("=== CC-BY-SA-NC 3.0");
  Serial.print("=== nodeID: ");
  Serial.print(nodeID);
  if (EthernetType==0){ 
    Serial.println("\n=== Eth Connection: (SPI) Microchip ENC28J60 (10-baseT)");
  }
  if (EthernetType==1){ 
    Serial.println("\n=== Eth Connection: (SPI) Wiznet W5100 (100-baseT)");
  }
  Ethernet.begin(mac, ip, gw, subnet);
  Serial.print("=== SATAR node IP: ");
  Serial.println(Ethernet.localIP());
  timer = - REQUEST_RATE; // start timing out right away
  // digitalWrite(startPin, HIGH); //20kOhm internal pull-up enable 
  // digitalWrite(finishPin, HIGH); //20kOhm internal pull-up enable
  // attachInterrupt(0, trigger_start, RISING); //0 = portpin 2
  // attachInterrupt(1, trigger_finish, RISING); //1 = portpin 4
printRAM();
digitalWrite(10, HIGH); // CS ethernet
digitalWrite(CS_SD, LOW); // CS SD
  if (!SD.begin(CS_SD)) { 
    Serial.println("! SD card init failed or no card present.");
    static boolean cardLog=0;
  }
  else {
    Serial.println("=== SD card initialized.");
    static boolean cardLog=1;
    logPacketToCard("REBOOT"); //
  }

digitalWrite(CS_SD, HIGH); // CS SD
digitalWrite(10, LOW); // CS ethernet
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
      forgePacket(12346,4,13423); //send a packet for testing purposes
    }
  }

}

void printRAM(){
  Serial.print("RAM free: ");
  Serial.println(FreeRam());
}

