
void forgePacket(unsigned long timeStampEvent, unsigned int typeEvent, unsigned int ID) {    
  if (DEBUG) {
      unsigned long timeStampEvent = timer; //DEBUG
  }
  unsigned long timer_micros1= micros();

  // constructing the packet which will be transmitted to the remote server:
  String payloadString; // this is the whole payload, the package that will
  // be transmitted in the query. We will build a long string and then 
  // transform it toa char.
  //payload mask  : 1234567890123456789012345678901234567890123456789012345678 (size= 48)
  //payload packet: TSN=4294967295&EVENT=255&ID=4294967295&NODE=255            (size=<48)
  payloadString+="TSN="; // append the TSN string to form the GET request
  payloadString+=timeStampEvent; // append the (long) time variable to the payload string
  payloadString+="&EVENT="; 
  payloadString+=typeEvent; // 0=debug, 10=int_trigger(0), 11=int_trigger(1)
  payloadString+="&ID="; 
  payloadString+=ID; // unsigned long, rider ID
  payloadString+="&NODE="; 
  payloadString+=nodeID; // hardcoded with #DEFINE, TODO: implement hardware Jumper setting

  char payload[64]; // this is the whole payload as a char array.
  payloadString.toCharArray(payload, 64); // convert String into char* and fill the buffer

  if (DEBUG) {
    Serial.println("#DEBUG mode: timer, timeStampEvent, payload");
    Serial.println(timer);
    Serial.println(timeStampEvent);
    Serial.println(payloadString);
    Serial.println(payload);
  }
  
  Serial.println("\n>>> request");

  ether.browseUrl(PSTR("/lab1/satar.php?"), payload, website, eth_reply);

  if (DEBUG) {
    unsigned long timer_micros_diff = micros()-timer_micros1;
    Serial.println("#DEBUG mode: packet forging and transaction time in microseconds");
    Serial.println(timer_micros_diff);
  }
}

// function eth_reply is called when the client request is complete
static void eth_reply (byte status, word off, word len) {
  Serial.print("<<< reply ");
  Serial.print(millis() - timer);
  Serial.println(" ms");
  if (EthernetType==0){
    Serial.println((const char*) Ethernet::buffer + off);
  } // ENC
  //   if (EthernetType==1){char c = client.read(); Serial.print(c);} // W5100
}


