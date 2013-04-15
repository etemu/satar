

void forgePacket(unsigned long timeStampEvent, unsigned int typeEvent, unsigned int ID) {    
  if (DEBUG) {
    unsigned long timeStampEvent = timer_ms; //DEBUG
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

  #ifdef DEBUG
//    Serial.print("DEB: Timer  : ");
//    Serial.println(timer_ms);
    Serial.print("DEB: Payload: ");
    Serial.print(payload);
    Serial.print(" forged in ");
    Serial.print(micros()-timer_micros1);
    Serial.println(" us.");
  #endif

sendPacket(payload); //send out the forged packet to the ethernet chip via SPI

  // client.println();  ether.browseUrl(PSTR("/lab1/satar.php?"), payload, website, eth_reply); // ENC

  if (DEBUG) {
    Serial.print("DEB: forge&sendPacket done in ");
    Serial.print(micros()-timer_micros1);
    Serial.println(" us.");
  }
}


