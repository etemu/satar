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

#define nodeID 20

#define EthernetType 0 // type of ethernet hardware: 0=Microchip ENC28J60, 1=Wiznet W5100
#include <EtherCard.h> // library with simple IP + TCP stack 

#define DEBUG 1 // debug mode with verbose output over serial at 115200 
#define DHCP 0 //disable or enable DHCP client
#define REQUEST_RATE 5000 // request rate of webpage query in ms, for keepalive or debugging

////////////////////////////////// BEGIN Ethernet Type: ENC


static byte myip[] = { 
  192,168,8,nodeID }; // static IP if DHCP is disabled
static byte gwip[] = { 
  192,168,8,1 }; // static gateway IP if DHCP is disabled
static byte dnsip[] = { 
  192,168,8,1 }; // static DNS server IP if DHCP is disabled
static byte hisip[] = { 
  83,169,41,76 }; // static Server IP if DHCP is disabled
static char website[] PROGMEM = "etemu.com"; // remote server, TLD

static byte mac[] = { 
  0x01,0x01,0x01,0x01,0x32,nodeID }; // ethernet interface mac address

byte Ethernet::buffer[512]; // Buffer for ethernet frames

////////////////////////////////// END Ethernet Type: ENC

volatile boolean startTriggered=0; // flag which will be set to 1 if the interrupt triggers
volatile boolean finishTriggered=0; // flag which will be set to 1 if the interrupt triggers
boolean trigger_start_armed=0; // 0 = not armed, 1 = trigger is armed and listens at input
unsigned int debounceCountsStart=0;
unsigned int debounceCountsFinish=0;
unsigned long startTriggeredMillis=0; 

byte typeEvent=2;
unsigned long ID=nodeID;

const unsigned int triggerIntervalStart=700; //minimum time between two occuring start trigger events in ms
const unsigned int triggerIntervalFinish=700; //minimum time between two occuring finish trigger events in ms
const int startPin = 2;	// the number of the input pin, ISR only at portpins 2 and 4
const int finishPin = 4; // the number of the input pin, ISR only at portpins 2 and 4
const int debounceDelayTrigger = 0;  // milliseconds to wait until stable recovery from the falling edge event

static long timer;
static long timer_micros;

void setup () {
  Serial.begin(115200); // init serial connection for debugging
  Serial.println("\n=== [SATAR Controller]");
  Serial.println("=== System for Advanced Timekeeping and Amateur Racing");
  Serial.print("\n nodeID: ");
  Serial.print(nodeID);
  Serial.print("\n Booting system and initializing peripherals.");
  if (EthernetType==0){ 
    Serial.println("\n Ethernet Hardware: (SPI) Microchip ENC28J60 (10-baseT)");
  }
  if (EthernetType==1){ 
    Serial.println("\n Ethernet Hardware: (SPI) Wiznet W5100 (100-baseT)");
  }

  ////////////////////////////////// BEGIN Ethernet Type: ENC
  if (ether.begin(sizeof Ethernet::buffer, mac,10) == 0) //never forget the chip select portpin
    Serial.println( "ERR: Failed to access ENC ethernet controller.");

  if (DHCP){
    Serial.println(" Aquiring DHCP lease.");
    if (EthernetType==0){ 
      if (!ether.dhcpSetup())
        Serial.println("ERR: ENC DHCP failed.");
    }
  }
  if (DHCP==0){
    Serial.println(" Setting up static IP and routing table.");
    if (EthernetType==0){ 
      ether.staticSetup(myip,gwip,dnsip);
      ether.copyIp(ether.hisip, hisip);
    }
  }
  ether.printIp("eth0 IP:  ", ether.myip);
//  ether.printIp("Netmask:  ", ether.mymask);
  ether.printIp("eth0 GW:  ", ether.gwip);
  ether.printIp("eth0 DNS: ", ether.dnsip);

  if (EthernetType==0){
    if (!ether.dnsLookup(website))
      Serial.println("ERR: DNS resolution of server failed.");
  }
  ether.printIp("Server IP: ", ether.hisip);

  while (ether.clientWaitingGw())
    ether.packetLoop(ether.packetReceive());
  Serial.println("Gateway responded, link established.");
  ////////////////////////////////// END Ethernet Type: ENC

  timer = - REQUEST_RATE; // start timing out right away

  pinMode(startPin, INPUT); //ISR only at portpins 2 and 4
  pinMode(finishPin, INPUT); //ISR only at portpins 2 and 4
  digitalWrite(startPin, HIGH); //20kOhm internal pull-up enable 
  digitalWrite(finishPin, HIGH); //20kOhm internal pull-up enable 
  attachInterrupt(0, trigger_start, FALLING); //0 = portpin 2
  attachInterrupt(1, trigger_finish, FALLING); //1 = portpin 4
}



void loop () {
  // DHCP expiration is a bit brutal, because all other ethernet activity and
  // incoming packets will be ignored until a new lease has been acquired
  if (ether.dhcpExpired() && !ether.dhcpSetup())
    Serial.println("ERR: DHCP renewal failed.");

  ether.packetLoop(ether.packetReceive());

  if (millis() > timer + REQUEST_RATE) {
    timer = millis();
    forgePacket(123,123,123);
  }
}


