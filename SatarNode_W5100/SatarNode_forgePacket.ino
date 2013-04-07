boolean lastConnected = false;

void forgePacket(unsigned long timeStampEvent, unsigned int typeEvent, unsigned int ID) {    
  if (DEBUG) {
    unsigned long timeStampEvent = timer; //DEBUG
  }
  unsigned long timer_micros1= micros();

  // constructing the packet which will be transmitted to the remote server:
  String payloadString; // this is the whole payload, the package that will
  // be transmitted in the query. We will build a long string and then 
  // transform it toa char.
  //payload mask  : 123456789012345678901234567890123456789012 (size= 42)
  //payload packet: T=4294967295&E=255&I=4294967295&N=255      (size=<42)
  payloadString+="T="; // append the TSN string to form the GET request
  payloadString+=timeStampEvent; // append the (long) time variable to the payload string
  payloadString+="&E="; 
  payloadString+=typeEvent; // 0=debug, 10=int_trigger(0), 11=int_trigger(1)
  payloadString+="&I="; 
  payloadString+=ID; // unsigned long, rider ID
  payloadString+="&N="; 
  payloadString+=nodeID; // hardcoded with #DEFINE, TODO: implement hardware Jumper setting or EEPROM readout

if (cardLog){
    logPacketToCard(payloadString); // Log the forged payload string to the SD card.
  }

  char payload[42]; // this is the whole payload as a char array.
  payloadString.toCharArray(payload, 42); // convert String into char* and fill the buffer

  if (DEBUG) {
    Serial.println("###: timer, timeStampEvent, payloadString, payload");
    Serial.println(timer);
    Serial.println(timeStampEvent);
    Serial.println(payloadString);
    Serial.println(payload);
  }

  Serial.println("ETH: initiating connection.");


  if (client.connect(website, 80)) { //W5100
    Serial.println("ETH: connected to server.");
    client.print("GET /lab1/satar.php?"); 
    client.print(payload);
    client.println(" HTTP/1.0");
    client.print("Host: ");
    client.println(website);    
    client.println();
  } 
  else {
    Serial.println("ETH: connection failed");
  }

  // client.println();  ether.browseUrl(PSTR("/lab1/satar.php?"), payload, website, eth_reply); // ENC

  if (DEBUG) {
    unsigned long timer_micros_diff = micros()-timer_micros1;
    Serial.print("###: forgePacket executed in ");
    Serial.print(timer_micros_diff);
    Serial.println(" us");
  }
}

// function eth_reply_w5100 is called when the client request is complete
static void eth_reply_w5100()
{
  boolean printingPacket = false;
  char lastEthPacket;
  
  while (client.available()) {
    char c = client.read();
    if (c == '\r' && lastEthPacket == '\n') { // leave out the header, just print the content
        printingPacket=1; }
    if (printingPacket){ Serial.print(c); }
    lastEthPacket=c;
  }

 if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("ETH: closing connection...");
    client.stop();
    Serial.println("ETH: connection closed.");
  }
 lastConnected = client.connected();
}


