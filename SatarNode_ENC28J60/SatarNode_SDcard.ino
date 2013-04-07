void logPacketToCard(String dataString){
  #ifdef USE_SD
digitalWrite(CS_ETH, HIGH); // CS ethernet
digitalWrite(CS_SD, LOW); // CS SD

File dataFile = SD.open("satar.csv", FILE_WRITE);

  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(" SD: Write successful.");
    // print to the serial port too:
    Serial.println(dataString);
  }  

  else {
    Serial.println(" SD: ! Write failed.");  // if the file isn't accessible, send out an error
  } 
  
digitalWrite(CS_SD, HIGH); // CS SD
digitalWrite(CS_ETH, LOW); // CS ethernet  
#else
Serial.println(" SD: ! SD disabled.");
#endif
}

