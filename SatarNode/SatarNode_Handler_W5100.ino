#ifdef W5100

boolean lastConnected = false;

void sendPacket(char* payload){

  Serial.print("ETH: Connect SatarServer... ");
  
  // if (client.connect(website, 80)) {
    if (client.connect(website_ip, 80)) {
    Serial.println("Done!");
    client.print("GET ");
    client.print(website_url);
    client.print("?");
    client.print(payload);
    client.println(" HTTP/1.0");
    client.print("Host: ");
    client.println(website);    
    client.println();
  } 
  else {
    Serial.println("\nETH:!Connect to SatarServer failed.");
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
    Serial.print("ETH: Reply in ");
    Serial.print(millis()-timer_ms);
    Serial.print(" us, stopping client... ");
    client.stop();
    Serial.println("Done!");
  }
 lastConnected = client.connected();
}
#endif
