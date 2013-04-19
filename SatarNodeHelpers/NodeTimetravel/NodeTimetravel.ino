#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from bjoern@cs.stanford.edu

// PROC_DELAY: Minimum reaction time to note an 'R' or 'T'-type message: 309 us.
// Minimum time it takes to send an 'R'-type message response: 1136 us + PROC_DELAY.
// Minimum time it takes to send an 'T'-type message (after micros1 is read): 980 us.

// #define DEBUG 1

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
unsigned long micros2 = 0;
unsigned long micros3 = 0;
unsigned long timer_ms = - (SYNC_RATE / 8L); // start timing out faster in the beginning
unsigned long sentTime = 0;
byte nodes[4]={
  7,8,9,10}; //TODO: read node network topology setup from EEPROM
byte currentNode=0;
byte nodeIDindex=0;
const byte amountNodes=4;
unsigned long nodeStamps[4]; //holds timestamps from the nodes
char packetBuffer[8]; //buffer to hold incoming packet, standard: 24 chars/bytes
byte nodeID=0; // initialize global var with '0', should be overriden by EEPROM config
EthernetUDP Udp; // An Ethernet UDP instance to let us send and receive packets over UDP

unsigned long btol (byte* _b, int _pointer = 0){ //pointer is the startpointer in the array
  unsigned long _l=0;
  _l=long (_b[_pointer+3]);
  _l+=long (_b[_pointer+2]*256L);
  _l+=long (_b[_pointer+1]*65536L);
  _l+=long (_b[_pointer]*16777216L);
  return _l;  
}

void sendT(byte _nodeID = 0)
{ 
  if (_nodeID == nodeID) return; // I don't want to talk to myself..
  #ifdef DEBUG
  Serial.print("UDP:-> sendT to ");
  Serial.println(_nodeID);
  #endif  
  IPAddress rip(192,168,8,_nodeID);
  byte _m1[4];
  micros1=micros()+T_DELAY;
  //  Serial.println(micros1); // ^delta t start
  ltob(micros1,_m1);
  Udp.beginPacket(rip,8888);
  Udp.write(nodeID);
  Udp.write('T');  
  Udp.write(_m1[0]); // bit-banging is faster than looping
  Udp.write(_m1[1]);
  Udp.write(_m1[2]);
  Udp.write(_m1[3]);  
  Udp.endPacket(); // 980us since last micros1 set. 1364us delta t end
  sentTime=micros1;
}

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

void timeTravel(){
  if (nodes[currentNode]==nodeID) currentNode++;
//  if (currentNode>=amountNodes) currentNode=0;
  if (currentNode>=3) currentNode=0;
  currentNode=3;
  sendT(nodes[3]);
//  currentNode++;
  timer_ms = millis();
}

void recvUdp(){
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    micros1=micros();
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE); // 224us? - read the packet into packetBufffer
    if (packetBuffer[1]=='T'){
      sendR(); // 208us from micros1 until here
    }
    if (packetBuffer[1]=='R'){
      handlePacket_R();
    }
  }
}

void handlePacket_R(){
  micros1=micros1-PROC_DELAY;
  unsigned long replyTime=micros1-sentTime-PROC_DELAY;
#ifdef DEBUG
  IPAddress remote = Udp.remoteIP();
  Serial.print("UDP: <- recv in ");
  Serial.print(replyTime);
  Serial.print(" us from ");
  for (int i =0; i < 4; i++)
  {
    Serial.print(remote[i], DEC);
    if (i < 3)
    {
      Serial.print(".");
    }
  }
  Serial.print(":");
  Serial.println(Udp.remotePort());
#endif
  byte recvB[6];
  ctob(packetBuffer,recvB,6);
  byte r_nodeID=packetBuffer[0]; // remote nodeID
  unsigned long remoteT=btol(recvB,2);
  unsigned long hopTime=replyTime/2;
  nodeStamps[nodeIDindex]=sentTime;
  nodeStamps[currentNode]=remoteT-hopTime;
  #ifdef DEBUG
  Serial.print("UDP:    TripTime: ");
  Serial.print(replyTime);
  Serial.println(" us.");
  Serial.print("UDP:   TSN");
  Serial.print(nodeID);
  Serial.print(": ");
  Serial.println(nodeStamps[nodeIDindex]);
  Serial.print("UDP: <-TSN");
  Serial.print(nodes[currentNode]);
  Serial.print(": ");  
  Serial.println(nodeStamps[currentNode]);
  #endif
//  Serial.print("UDP:    TSdelta: ");
  if (nodeStamps[currentNode]>=nodeStamps[nodeIDindex]) Serial.println(nodeStamps[currentNode]-nodeStamps[nodeIDindex]);
  if (nodeStamps[currentNode]<nodeStamps[nodeIDindex]) Serial.println(nodeStamps[nodeIDindex]-nodeStamps[currentNode]);
}

void sendR()
{ 
  byte _m1[4];
  //  Serial.println(micros1); // ^delta t start
  ltob(micros1+R_ADVANCE,_m1);
  //  micros2=micros();
  Udp.beginPacket(Udp.remoteIP(),Udp.remotePort()); // ^726us until Udp.endpacket, 924us if dynamic IP
  Udp.write(nodeID);
  Udp.write('R');  
  Udp.write(_m1[0]);
  Udp.write(_m1[1]);
  Udp.write(_m1[2]);
  Udp.write(_m1[3]);
  Udp.endPacket(); // 52us
  sentTime=micros()-52L;
  //1136 us from last micros1 update
  #ifdef DEBUG
  Serial.print("UDP:<-> sentR to node ");
  Serial.println(packetBuffer[0],DEC);
  #endif
}


unsigned long timesBaseTen(int _move){ // the
  unsigned long place = 1; //base 10
  unsigned int n = 0;
  if( _move < 1){ 
    return 1;
  }//ones place no change
  else{
    for( n= 0; n<_move; n++){ 
      place = place*10;
    }
  }
  return place;
} 


unsigned long AToLong(char *_input, unsigned int _arrayLength){ // converts a (_arrayLength) digit ASCII to long int
  unsigned long number=0;
  unsigned long total=0;
  unsigned int arrayLook=0;
  unsigned int i = 0;
  char s[]={
    "h"          };    // init char array
  // call invert string for proper string power order
  for(i =_arrayLength; i >= 0;i--){
    // picks out ASCII numbers for error check
    if ((_input[i] >= '0') && (_input[i]<= '9')){
      s[0] = _input[i];// work around to get string arg
      number = atoi(s);// ASCII to int
      number =  (number * (timesBaseTen(arrayLook))); // puts number in its place
      arrayLook++;
      total = number+total;
    }
  }
  return total; 
}



unsigned long ctol ( unsigned char* _b){
  unsigned long _l=0;
  _l=long (_b[3]);
  _l+=long (_b[2]*256L);
  _l+=long (_b[1]*65536L);
  _l+=long (_b[0]*16777216L);
  return _l;  
}
unsigned long ctol2 ( char* _b){
  unsigned long _l=0;
  _l=long (_b[3]);
  _l+=long (_b[2]*256L);
  _l+=long (_b[1]*65536L);
  _l+=long (_b[0]*16777216L);
  return _l;  
}

void ltoc(unsigned long _longInt, unsigned char _b[]){   // convert from an unsigned long int to a 4-byte array
  _b[0] = ((_longInt >> 24) & 0xFF);
  _b[1] = ((_longInt >> 16) & 0xFF);
  _b[2] = ((_longInt >> 8) & 0XFF);
  _b[3] = ((_longInt & 0XFF));
}

void ctob(char* _chars, byte* _bytes, unsigned int _count){
  for(unsigned int i = 0; i < _count; i++)
    _bytes[i] = (byte)_chars[i];
}

void ltob(unsigned long _longInt,byte _b[]){   // convert from an unsigned long int to a 4-byte array
  _b[0] = ((_longInt >> 24) & 0xFF);
  _b[1] = ((_longInt >> 16) & 0xFF);
  _b[2] = ((_longInt >> 8) & 0XFF);
  _b[3] = ((_longInt & 0XFF));
}





