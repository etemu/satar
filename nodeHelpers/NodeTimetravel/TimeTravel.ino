
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

unsigned long btol (byte* _b, int _pointer = 0){ //pointer is the startpointer in the array
  unsigned long _l=0;
  _l=long (_b[_pointer+3]);
  _l+=long (_b[_pointer+2]*256L);
  _l+=long (_b[_pointer+1]*65536L);
  _l+=long (_b[_pointer]*16777216L);
  return _l;  
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
  Serial.print("UDP:    TSdelta: ");
  if (nodeStamps[currentNode]>=nodeStamps[nodeIDindex]) Serial.println(nodeStamps[currentNode]-nodeStamps[nodeIDindex]);
  if (nodeStamps[currentNode]<nodeStamps[nodeIDindex]) Serial.println(nodeStamps[nodeIDindex]-nodeStamps[currentNode]);
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

