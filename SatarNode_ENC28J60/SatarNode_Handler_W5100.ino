#ifdef W5100

boolean lastConnected = false;

void sendPacket(char* payload){

  Serial.println("ETH: initiating connection.");
  if (client.connect(website, 80)) { //W5100
    Serial.println("ETH: W5100 connected to server.");
    client.print("GET /lab1/satar.php?"); 
    client.print(payload);
    client.println(" HTTP/1.0");
    client.print("Host: ");
    client.println(website);    
    client.println();
  } 
  else {
    Serial.println("ETH: connection failed.");
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
#endif
