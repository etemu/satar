

void forgePacket(unsigned long timeStampEvent, unsigned int typeEvent, unsigned int ID) {    

  unsigned long timer_micros1=micros();

  // constructing the packet which will be transmitted to the remote server:
  String payloadString; // this is the whole payload, the package that will
  // be transmitted in the query. We will build a long string and then 
  // transform it toa char.
  
  // TODO: break the payload down into at least <16b fragments.
  
  //payload mask  : 123456789012345678901234567890123456789012 (size= 42)
  //payload packet: T=4294967295&E=255&I=4294967295&N=255      (size=<42)

  //payload mask  : 1234567890123456789012345678901234567890123456789012345678901234567890123456789012 (size= 82)
  //payload packet: t=255&T=4294967295&u=255&U=4294967295&v=255&V=4294967295&E=255&I=4294967295&N=255 (size=<82)  
  payloadString+="t=";
  payloadString+=nodes[0];
/*  payloadString+="T="; // append the TS_t string to form the GET request
  payloadString+=nodeStamps[0]+timeStampEvent-nodeStamps[nodeIDindex]; // should work with an overflow as we are unsigned
  payloadString+="u=";
  payloadString+=nodes[1];
  payloadString+="U="; // append the TS_u string to form the GET request
  payloadString+=nodeStamps[1]+timeStampEvent-nodeStamps[nodeIDindex]; // should work with an overflow as we are unsigned
//  payloadString+="v=";
//  payloadString+=nodes[2];
//  payloadString+="V="; // append the TS_v string to form the GET request
//  payloadString+=nodeStamps[2]+timeStampEvent-nodeStamps[nodeIDindex];
  payloadString+="&E="; 
  payloadString+=typeEvent; // 0=bootup 1=heartbeat, 101=int_trigger(0), 102=int_trigger(1)
  payloadString+="&I="; 
  payloadString+=ID; // unsigned long, rider ID
  payloadString+="&N="; 
  payloadString+=nodeID; */

if (cardLog){
    logPacketToCard(payloadString); // Log the forged payload string to the SD card.
  }
  char payload[82]; // this is the whole payload as a char array.
  payloadString.toCharArray(payload, 82); // convert String into char* and fill the buffer 

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
#ifdef DEBUG
    Serial.print("DEB: forge&sendPacket done in ");
    Serial.print(micros()-timer_micros1);
    Serial.println(" us.");
#endif
}


