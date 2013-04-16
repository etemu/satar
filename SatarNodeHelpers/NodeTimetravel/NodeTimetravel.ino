#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from bjoern@cs.stanford.edu

#define TIME_RATE 10000L //
#define PROCTIME 1L

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network: (Not used here - config read out from EEPROM)
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFA, 0xED };
//IPAddress ip(192, 168, 8, 177);

unsigned int localPort = 8888;      // local port to listen on
unsigned long micros1 = 0;
unsigned long timer_ms = -8000;
unsigned long timeStamps[2];
// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet, standard: 24
char  ReplyBuffer[] = "T";       // a string to send back
//T4294967296 
//T1234567890

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  // start the Ethernet and UDP:
  Serial.begin(115200);
  Serial.print("STR: Node ID: ");
  byte nodeID=EEPROM.read(0);
  Serial.println(nodeID);
  static byte mac[] = { 
    EEPROM.read(1), EEPROM.read(2), EEPROM.read(3), EEPROM.read(4), EEPROM.read(5), EEPROM.read(6)       }; // ethernet interface mac address
  IPAddress ip(192,168,8,nodeID); // static IP
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);

//  unsigned long longInt=4294967294;
  unsigned long longInt=240;
  byte byteArray[4];
  //  ltob(longInt,byteArray);
  unsigned char charArray[4];
  ltoc(longInt,charArray);
  ltob(longInt,byteArray);
  Serial.println("long");
  Serial.println(longInt);
  Serial.println(longInt,BIN);
  Serial.print(charArray[0],BIN);
  Serial.print("-");
  Serial.print(charArray[1],BIN);
  Serial.print("-");
  Serial.print(charArray[2],BIN);
  Serial.print("-");
  Serial.println(charArray[3],BIN);
  Serial.println("=====================");
  char c[4];
  c[0]=charArray[0]-128;
  c[1]=charArray[1]-128;
  c[2]=charArray[2]-128;
  c[3]=charArray[3]-128;
  unsigned long temp=c[3];
  int temp2=c[3];
  Serial.println(c[3],BIN);
   Serial.println(temp);
    Serial.println(temp,BIN);
          Serial.println(temp2);
      Serial.println(temp2,BIN);
  unsigned long longInt2=ctol(charArray);
  unsigned long longInt3=ctol2(c);
  Serial.println("convert back to long:");    
  Serial.println(longInt);
  Serial.println(longInt2);
  Serial.println(longInt3);
  Serial.println("init successful");
}

void loop() {
  // if there's data available, read a packet
  recvUdp();

  if (millis() > timer_ms + TIME_RATE) {
    timer_ms = millis();
    sendT(); 
  }

}

void recvUdp(){
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    unsigned long replyTime=micros()-micros1;
    micros1=micros();
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE); // read the packet into packetBufffer
    if (packetBuffer[0]=='T'){
      sendR();
    }     
    Serial.print("UDP: <-recv in ");
    Serial.print(replyTime);
    Serial.print(" len: ");
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
    Serial.print("UDP: <-recvT: ");
    Serial.println(packetBuffer);

    if (packetBuffer[0]=='R'){
      unsigned long hopTime = (replyTime-PROCTIME)/2L;
      timeStamps[0]=micros1;
      unsigned int len=strlen(packetBuffer);
      Serial.println(len);
      /*
      memmove(packetBuffer,packetBuffer+1,len);
       timeStamps[1]=atoi(packetBuffer);
       */
      //timeStamps[1]=AToLong(packetBuffer,len);
      timeStamps[1]=
        Serial.print("UDP: <-TS0: ");
      Serial.println(timeStamps[0]);
      Serial.print("UDP: <-TS1: ");
      Serial.println(timeStamps[1]);
      Serial.print("UDP: <-TSdelta: ");
      if (timeStamps[1]>=timeStamps[0]) Serial.println(timeStamps[1]-timeStamps[0]-hopTime);
      if (timeStamps[1]<timeStamps[0]) Serial.println(timeStamps[0]-timeStamps[1]-hopTime);
    }

    if (packetBuffer[0]=='R'){
      unsigned long hopTime = (replyTime-PROCTIME)/2;
      timeStamps[0]=micros1;
      unsigned int len=strlen(packetBuffer);
      Serial.println(len);
      /*
      memmove(packetBuffer,packetBuffer+1,len);
       timeStamps[1]=atoi(packetBuffer);
       */
      timeStamps[1]=AToLong(packetBuffer,len);

      Serial.print("UDP: <-TS0: ");
      Serial.println(timeStamps[0]);
      Serial.print("UDP: <-TS1: ");
      Serial.println(timeStamps[1]);
      Serial.print("UDP: <-TSdelta: ");
      if (timeStamps[1]>=timeStamps[0]) Serial.println(timeStamps[1]-timeStamps[0]-hopTime);
      if (timeStamps[1]<timeStamps[0]) Serial.println(timeStamps[0]-timeStamps[1]-hopTime);
    }


  }
  // delay(10);
}

void sendT()
{ 
  micros1=micros();
  Serial.print("UDP: ->sendT: ");
  String micros1String;
  micros1String +="T";
  micros1String += micros1;
  Serial.println(micros1String);
  char micros1Char[5];
  micros1String.toCharArray(micros1Char,11);
  ltob(micros1,

  IPAddress rip(192,168,8,90);
  Udp.beginPacket(rip,6000);
  //Udp.remoteIP(), Udp.remotePort());
  //    Udp.write(ReplyBuffer);

 byte b1=65;
 byte b2=66;
 byte b3=67;

  Udp.write(b1);
  Udp.write(b2);
  Udp.write(b3);
    
//   Udp.write(micros1Char);
  //4294967296
  //1234567890
  Udp.endPacket();
  micros1=micros();
}

void sendR()
{ 
  Serial.print("UDP: ->sendR: ");
  String micros1String;
  micros1String +="R";
  micros1String += micros1;
  Serial.println(micros1String);
  char micros1Char[11];
  micros1String.toCharArray(micros1Char,11);
  IPAddress rip(Udp.remoteIP());
  Udp.beginPacket(rip,Udp.remotePort());
  //Udp.remoteIP(), Udp.remotePort());
  //    Udp.write(ReplyBuffer);
  Udp.write(micros1Char);
  //4294967296
  //1234567890
  Udp.endPacket();
  micros1=micros();
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

unsigned long btol (byte* _b){
  unsigned long _l=0;
  _l=long (_b[3]);
  _l+=long (_b[2]*256L);
  _l+=long (_b[1]*65536L);
  _l+=long (_b[0]*16777216L);
  return _l;  
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

void ltob(unsigned long _longInt,byte _b[]){   // convert from an unsigned long int to a 4-byte array
  _b[0] = ((_longInt >> 24) & 0xFF);
  _b[1] = ((_longInt >> 16) & 0xFF);
  _b[2] = ((_longInt >> 8) & 0XFF);
  _b[3] = ((_longInt & 0XFF));
}

void ltoc(unsigned long _longInt, unsigned char _b[]){   // convert from an unsigned long int to a 4-byte array
  _b[0] = ((_longInt >> 24) & 0xFF);
  _b[1] = ((_longInt >> 16) & 0xFF);
  _b[2] = ((_longInt >> 8) & 0XFF);
  _b[3] = ((_longInt & 0XFF));
}



