#ifdef W5100

boolean lastConnected = false;

void sendPacket(char* payload){

  Serial.print(F("ETH: Contact SatarServer.. "));
  
  // if (client.connect(website, 80)) { // use DNS to lookup website. e.g. in WAN
    if (client.connect(website_ip, 80)) { // use IP, do not resolve DNS. e.g. in LAN
    Serial.println(F("ACK."));
    client.print("GET ");
    client.print(website_url);
    client.print("?");
    client.print(payload);
    client.println(" HTTP/1.0");
    client.print("Host: ");
    client.println(website);    
    client.println();
    timer_us = micros();
  } 
  else {
    Serial.println(F("\nETH:!Connect to SatarServer failed."));
  }
}

// function eth_reply_w5100 is called when the client request is complete
static void eth_reply_w5100()
{ 
  if (client.available()){
  unsigned long timer_us2 = micros()-timer_us;
  boolean printingPacket = false;
  char lastEthPacket;
  unsigned long timer_ms2 = millis();
  while (client.available()) {
    char c = client.read();
    if (c == '\r' && lastEthPacket == '\n') { // leave out the header, just print the content
        printingPacket=1; }
    if (printingPacket){ Serial.print(c); }
    lastEthPacket=c;
  
 if (!client.connected() && lastConnected) {
    Serial.print(F("ETH: Answer in "));
    Serial.print(timer_us2);
    Serial.println(F(" us"));
    Serial.print(F("ETH: Parsing time: "));
    Serial.print(millis()-timer_ms2);
    Serial.print(F(" ms, stopping client.. "));
    client.stop();
    Serial.println(F("ACK\n"));
  }
 lastConnected = client.connected();
 }
 }
}
#endif
