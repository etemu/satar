void logPacketToCard(String dataString){
  #ifdef USE_SD
digitalWrite(CS_ETH, HIGH); // CS ethernet
digitalWrite(CS_SD, LOW); // CS SD

// String logFile="Node";
// logFile+=nodeID;
// logFile+=".csv";
// char logFileCSV[13];
// logFile.toCharArray(logFileCSV, 13);

File dataFile = SD.open("satar.csv", FILE_WRITE);

  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(F(" SD: Write successful."));
    // print to the serial port too:
    Serial.println(dataString);
  }  

  else {
    Serial.println(F(" SD: ! Write failed."));  // if the file isn't accessible, send out an error
  } 
  
digitalWrite(CS_SD, HIGH); // CS SD
digitalWrite(CS_ETH, LOW); // CS ethernet  
#else
Serial.println(F(" SD: ! SD disabled."));
#endif
}

