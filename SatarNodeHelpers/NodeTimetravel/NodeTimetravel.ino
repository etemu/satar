#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from bjoern@cs.stanford.edu

// PROC_DELAY: Minimum reaction time to note an 'R' or 'T'-type message: 309 us.
// Minimum time it takes to send an 'R'-type message response: 1136 us + PROC_DELAY.
// Minimum time it takes to send an 'T'-type message (after micros1 is read): 980 us.

#define DEBUG 1

#define SYNC_RATE 3000L // request rate
#define PROCTIME 1912L // 201304162000 aShure, 16Mhz Atmega 328
#define T_DELAY 980L // minimum processing time until T packet is sent out
#define PROC_DELAY 309L; // minimum reaction time to any packet type
#define R_DELAY 1136L // minimum processing time until R packet can be replied

#define R_ADVANCE 414L // Anticipated propagation time, shifts the 'R' time into the middle of the round trip.
//  PROC_DELAY + R_DELAY ) DIV 2 - PROC_DELAY

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network: (Not used here - config read out from EEPROM)
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFA, 0xED };
//IPAddress ip(192, 168, 8, 177);

unsigned int localPort = 8888;      // local port to listen on
unsigned long micros1 = 0;
unsigned long timer_ms = - (SYNC_RATE / 8L); // start timing out faster in the beginning
unsigned long sentTime = 0;
byte nodes[]={
  7,9}; //TODO: read node network topology setup from EEPROM
byte currentNode=0;
byte nodeIDindex=0;
const byte amountNodes=2;
unsigned long nodeStamps[2]; //holds timestamps from the nodes
char packetBuffer[8]; //buffer to hold incoming packet, standard: 24 chars/bytes
byte nodeID=0; // initialize global var with '0', should be overriden by EEPROM config
EthernetUDP Udp; // An Ethernet UDP instance to let us send and receive packets over UDP


void setup() {
  // start the Ethernet and UDP:
  Serial.begin(115200);
  Serial.print("STR: Node ID: ");
  nodeID=EEPROM.read(0);
  Serial.println(nodeID);
  static byte mac[] = { 
    EEPROM.read(1), EEPROM.read(2), EEPROM.read(3), EEPROM.read(4), EEPROM.read(5), EEPROM.read(6)  }; 
  IPAddress ip(EEPROM.read(7),EEPROM.read(8),EEPROM.read(9),EEPROM.read(10)); 
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);
  for (byte i = 0; (i+1)<=amountNodes ; i++) { //check which array index is our nodeID
    if(nodes[i]==nodeID) nodeIDindex=i;
  }
  Serial.print("nodeIDindex: ");
  Serial.print(nodeIDindex);
  Serial.print(", nodes[nodeIDindex]: ");
  Serial.println(nodes[nodeIDindex]);
  Serial.println("init successful");
}

void loop() {
  recvUdp();  // call the UDP handler - if there's data available, read a packet.

  if (millis() > timer_ms + SYNC_RATE) {
    timeTravel(); // send out a TimeTravel packet to a node from the node list array 'nodes[]'.
  }
}

