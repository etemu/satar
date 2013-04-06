void logPacketToCard(String dataString){
digitalWrite(CS_ETH, HIGH); // CS ethernet
digitalWrite(CS_SD, LOW); // CS SD

File dataFile = SD.open("satar.csv", FILE_WRITE);

  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println("Save to SD card successful.");
    // print to the serial port too:
    Serial.println(dataString);
  }  

  else {
    Serial.println("! Save to SD card failed.");  // if the file isn't accessible, send out an error
  } 
  
digitalWrite(CS_SD, HIGH); // CS SD
digitalWrite(CS_ETH, LOW); // CS ethernet  
}
