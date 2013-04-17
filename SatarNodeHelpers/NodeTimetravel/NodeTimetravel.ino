#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from bjoern@cs.stanford.edu

// PROC_DELAY: Minimum reaction time to note an 'R' or 'T'-type message: 309 us.
// Minimum time it takes to send an 'R'-type message response: 1136 us + PROC_DELAY.
// Minimum time it takes to send an 'T'-type message (after micros1 is read): 980 us.

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
unsigned long timer_ms = -SYNC_RATE;
byte nodes[8];
unsigned long nodeStamps[2]; //holds timestamps from the nodes
// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet, standard: 24
//T4294967296 
//T1234567890
byte nodeID=11;
// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

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
  Serial.println("UDP: ->sendT. ");
  IPAddress rip(192,168,8,_nodeID);
  byte _m1[4];
  micros1=micros()+T_DELAY;
//  Serial.println(micros1); // ^delta t start
  ltob(micros1,_m1);
  Udp.beginPacket(rip,8888);
  Udp.write(nodeID);
  Udp.write('T');  
  Udp.write(_m1[0]);
  Udp.write(_m1[1]);
  Udp.write(_m1[2]);
  Udp.write(_m1[3]);  
  Udp.endPacket(); // 980us since last micros1 set. 1364us delta t end
}

void setup() {
  // start the Ethernet and UDP:
  Serial.begin(115200);
  Serial.print("STR: Node ID: ");
  nodeID=EEPROM.read(0);
  Serial.println(nodeID);
  static byte mac[] = { 
    EEPROM.read(1), EEPROM.read(2), EEPROM.read(3), EEPROM.read(4), EEPROM.read(5), EEPROM.read(6)       }; // ethernet interface mac address
  IPAddress ip(192,168,8,nodeID); // static IP
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);
  nodes[0]=nodeID;
  Serial.println("init successful");
}

void loop() {
  // if there's data available, read a packet
  recvUdp();

  if (millis() > timer_ms + SYNC_RATE) {
    timer_ms = millis();
    sendT(90);
//    sendT(90);
//    sendT(91);    
//    sendT(92);
//    sendT(93);
//    sendT(94);
//    sendT(95);    
  }

}

void recvUdp(){
  int packetSize = Udp.parsePacket(); // ^delta t start 
  if (packetSize)
  {
    unsigned long replyTime=micros()-micros1; // delta t end -> 305+4us
    micros1=micros(); //-PROC_DELAY
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE); // 224us - read the packet into packetBufffer
    if (packetBuffer[1]=='T'){
      sendR(); // from micros1 until here 208us
    }
    Serial.print("UDP: <-recv in ");
    Serial.print(replyTime);
    Serial.print(" us, len ");
    Serial.print(packetSize);
    Serial.print(" from ");
    IPAddress remote = Udp.remoteIP();
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
    byte recvB[6];
    ctob(packetBuffer,recvB,6);
    unsigned long remoteT=btol(recvB,2);
    if (packetBuffer[1]=='R'){
      long hopTime = (replyTime)/2L;
      nodeStamps[0]=micros1;
      nodeStamps[1]=remoteT-hopTime;
      Serial.print("UDP:   hopTime: ");
      Serial.println(hopTime);
      Serial.print("UDP:   TS0: ");
      Serial.println(nodeStamps[0]);
      Serial.print("UDP: <-TS1: ");
      Serial.println(nodeStamps[1]);
      Serial.print("UDP:   TSdelta: ");
      if (nodeStamps[1]>=nodeStamps[0]) Serial.println(nodeStamps[1]-nodeStamps[0]-hopTime);
      if (nodeStamps[1]<nodeStamps[0]) Serial.println(nodeStamps[0]-nodeStamps[1]-hopTime);
    }


  }
  // delay(10);
}



void sendR()
{ 
  byte _m1[4];
//  Serial.println(micros1); // ^delta t start
  ltob(micros1,_m1);
//  micros2=micros();
  Udp.beginPacket(Udp.remoteIP(),Udp.remotePort()); // ^726us until Udp.endpacket, 924us if dynamic IP
  Udp.write(nodeID);
  Udp.write('R');  
  Udp.write(_m1[0]);
  Udp.write(_m1[1]);
  Udp.write(_m1[2]);
  Udp.write(_m1[3]);
  Udp.endPacket(); // 52us
  //1136 us from last micros1 update
  Serial.print("UDP:<->sentResponse to nodeID ");
  Serial.println(packetBuffer[0],DEC);
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
    "h"    };    // init char array
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


