void trigger_start(){ 
  startTriggered=1; //set flag to 1 when interrupt is triggered by a falling edge
} 
void trigger_finish(){ 
  finishTriggered=1; //set flag to 1 when interrupt is triggered by a falling edge
} 

//void send_event_start()
//{
//  if (millis() - lastReadStart >= triggerIntervalStart) //minimum delay between two events which will actually get logged: triggerInterval*
//  {
//    byte typeEvent=10; //set the event type to 10 = Start triggered
//    lastReadStart=millis();
//  }
//  startTriggered=0;
//}
//
//void send_event_finish()
//{
//  if (millis() - lastReadFinish >= triggerIntervalFinish) //minimum delay between two events which will actually get logged: triggerInterval*
//  {
//    byte typeEvent=11; //set the event type to 11 = Finish triggered
//    lastReadFinish=millis();
//  }
//  finishTriggered=0;
//}

void checkTriggers(){

  if (startTriggered){
    detachInterrupt(0); // disable the interrupt, so it won't bother us
    startTriggeredMillis=millis(); // this is our time which will be used as the event time
    forgePacket(startTriggeredMillis,10,0); // send the acquired data
    startTriggered=0; // reset the trigger flag
    trigger_start_armed=0; // set the helper flag, so we now that the trigger is not armed for now
  }

  if (!trigger_start_armed){ //if the trigger is not armed then check if we should arm it again:
    if (millis() - startTriggeredMillis >= triggerIntervalStart) //minimum delay between two events which will actually get logged: triggerInterval*
    {
      attachInterrupt(0, trigger_start, FALLING); //let's arm the trigger again
      trigger_start_armed=1; //set the helper flag, so we now that the trigger is armed
    }; 
  }
}
