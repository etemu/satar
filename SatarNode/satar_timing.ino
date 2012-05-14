void trigger_start(){ 
  startTriggered=1; //set flag to 1 when interrupt is triggered by a falling edge
} 
void trigger_finish(){ 
  finishTriggered=1; //set flag to 1 when interrupt is triggered by a falling edge
} 

void send_event_start()
{
  if (millis() - lastReadStart >= triggerIntervalStart) //minimum delay between two events which will actually get logged: triggerInterval*
  {
    byte typeEvent=10; //set the event type to 10 = Start triggered
    lastReadStart=millis();
  }
  startTriggered=0;
}

void send_event_finish()
{
  if (millis() - lastReadFinish >= triggerIntervalFinish) //minimum delay between two events which will actually get logged: triggerInterval*
  {
    byte typeEvent=11; //set the event type to 11 = Finish triggered
    lastReadFinish=millis();
  }
  finishTriggered=0;
}


