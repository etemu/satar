
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

if (cardLog){
    logPacketToCard(payloadString); // Log the forged payload string to the SD card.
  }

  char payload[48]; // this is the whole payload as a char array.
  payloadString.toCharArray(payload, 48); // convert String into char* and fill the buffer

  if (DEBUG) {
    Serial.println("# debug information: timer, timeStampEvent, payloadString, payload");
    Serial.println(timer);
    Serial.println(timeStampEvent);
    Serial.println(payloadString);
    Serial.println(payload);
  }

  Serial.println("\n>>> sending GET request.");


  if (client.connect(website, 80)) { //W5100
    client.print("GET /lab1/satar.php?"); 
    client.print(payload);
    client.println(" HTTP/1.0");
    client.println();
  } 
  else {
    Serial.println("connection failed");
  }
  eth_reply_w5100();
  // client.println();  ether.browseUrl(PSTR("/lab1/satar.php?"), payload, website, eth_reply); // ENC

  if (DEBUG) {
    unsigned long timer_micros_diff = micros()-timer_micros1;
    Serial.print("# function forgePacket executed in ");
    Serial.print(timer_micros_diff);
    Serial.println(" microseconds");
  }
}

// function eth_reply_w5100 is called when the client request is complete
static void eth_reply_w5100()

{
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  if (!client.connected()) {
    Serial.println();
    Serial.println(" Closing connection.");
    client.stop();
    Serial.println(" -> Connection closed.");
  }
}


