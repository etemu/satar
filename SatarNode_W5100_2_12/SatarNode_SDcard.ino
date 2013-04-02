void logPacketToCard(String dataString){
digitalWrite(10, HIGH); // CS ethernet
digitalWrite(CS_SD, LOW); // CS SD
File dataFile = SD.open("satar.csv", FILE_WRITE);

  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println("Save to SD card successful.");
    // print to the serial port too:
    // Serial.println(dataString);
 dataFile.close();      
  }  

  else {
    Serial.println("! Save to SD card failed.");  // if the file isn't open, send out an error
  } 
digitalWrite(CS_SD, HIGH); // CS SD
digitalWrite(10, LOW); // CS ethernet
}
