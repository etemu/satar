void trigger_one(){ 
  detachInterrupt(0);          // disable the interrupt, so it won't bother us while we process the event
  oneTriggered=1;              //set flag to 1 when interrupt is triggered
  oneTriggeredMicros=millis(); // this is our time which will be used as the event time
} 

void trigger_two(){ 
  detachInterrupt(1);          // disable the interrupt, so it won't bother us while we process the event
  twoTriggered=1;      //set flag to 1 when interrupt is triggered
  twoTriggeredMicros=millis(); // this is our time which will be used as the event time
} 

void checkTriggerOne(){

  if (oneTriggered){
    //oneTriggeredMicros;           // this is our time which will be used as the event time
    oneTriggeredMillis=millis();           // this is our time which will be used as the event time
    Serial.println(F("ISR: 1 fired."));
    forgePacket(oneTriggeredMicros,101,0);  // send the acquired data: ms, eventID, riderID
    oneTriggered=0;                        // reset the trigger flag
    trigger_one_armed=0;                   // set the helper flag, so we now that the trigger is not armed for now
  }
  if (!trigger_one_armed){                 //if the trigger is not armed then check if we should arm it again:
    if (millis() - oneTriggeredMillis >= triggerIntervalOne) //minimum delay between two events which will actually get logged: triggerInterval*
    {
      trigger_one_armed=1;                 //set the helper flag, so we now that the trigger is armed
      Serial.println(F("ISR: 1 armed."));
      attachInterrupt(0, trigger_one, LOW); //let's arm the trigger again      
    }; 
  }
}

void checkTriggerTwo(){

  if (twoTriggered){
    //twoTriggeredMicros;           // this is our time which will be used as the event time
    twoTriggeredMillis=millis();           // this is our time which will be used as the event time
    Serial.println(F("ISR: 2 fired."));
    forgePacket(twoTriggeredMicros,102,0);  // send the acquired data: ms, eventID, riderID
    twoTriggered=0;                        // reset the trigger flag
    trigger_two_armed=0;                   // set the helper flag, so we now that the trigger is not armed for now
  }
  if (!trigger_two_armed){                 //if the trigger is not armed then check if we should arm it again:
    if (millis() - twoTriggeredMillis >= triggerIntervalTwo) //minimum delay between two events which will actually get logged: triggerInterval*
    {
      trigger_two_armed=1;                 //set the helper flag, so we now that the trigger is armed
      Serial.println(F("ISR: 2 armed."));
      attachInterrupt(0, trigger_two, LOW); //let's arm the trigger again      
    }; 
  }
}

