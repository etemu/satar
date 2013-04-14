#ifdef ETHERCARD

boolean lastConnected = false;

static void eth_reply_ethercard (byte status, word off, word len) { // called when the client request is complete
  Serial.print("ETH: Reply in ");
  Serial.print(millis() - timer_ms);
  Serial.println(" ms.");
  Serial.println((const char*) Ethernet::buffer + off);
}

void sendPacket(char* payload){

  Serial.println("ETH: initiating connection."); //ENC28J60
    ether.browseUrl(PSTR(website_url), payload, website, eth_reply_ethercard);
    Serial.println("ETH: ENC28J60 connected to server.");

//    Serial.println("ETH: connection failed.");

}

#endif
