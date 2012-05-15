void trigger_start(){ 
  startTriggered=1;       //set flag to 1 when interrupt is triggered by a falling edge
} 
void trigger_finish(){ 
  finishTriggered=1;      //set flag to 1 when interrupt is triggered by a falling edge
} 

void checkTrigger0(){
  if (startTriggered){
    detachInterrupt(0);                      // disable the interrupt, so it won't bother us
    startTriggeredMillis=millis();           // this is our time which will be used as the event time
    forgePacket(startTriggeredMillis,10,0);  // send the acquired data: ms, eventID, riderID
    startTriggered=0;                        // reset the trigger flag
    trigger_start_armed=0;                   // set the helper flag, so we now that the trigger is not armed for now
  }
  if (!trigger_start_armed){                 //if the trigger is not armed then check if we should arm it again:
    if (millis() - startTriggeredMillis >= triggerIntervalStart) //minimum delay between two events which will actually get logged: triggerInterval*
    {
      attachInterrupt(0, trigger_start, FALLING); //let's arm the trigger again
      trigger_start_armed=1;                 //set the helper flag, so we now that the trigger is armed
    }; 
  }
}

void checkTrigger1(){
  if (finishTriggered){
    detachInterrupt(1);                      // disable the interrupt, so it won't bother us
    startTriggeredMillis=millis();           // this is our time which will be used as the event time
    forgePacket(startTriggeredMillis,11,0);  // send the acquired data: ms, eventID, riderID
    finishTriggered=0;                        // reset the trigger flag
    trigger_finish_armed=0;                   // set the helper flag, so we now that the trigger is not armed for now
  }
  if (!trigger_finish_armed){                 //if the trigger is not armed then check if we should arm it again:
    if (millis() - finishTriggeredMillis >= triggerIntervalFinish) //minimum delay between two events which will actually get logged: triggerInterval*
    {
      attachInterrupt(1, trigger_finish, FALLING); //let's arm the trigger again
      trigger_finish_armed=1;                 //set the helper flag, so we now that the trigger is armed
    }; 
  }
}
