void thread_flex()
{
  int flex_value = analogRead(A0);

  while(1)
  {
     double flex_voltage = ((double)(flex_value*1023))/((double)5);

  Serial.println(flex_voltage); 
  }
}

void setup()
{
  ThreadController controller = ThreadController();
  
  Thread flexThread = Thread();
  flexThread.setInterval(10);
//  flexThread.ThreadName = "flexThread";
  flexThread.onRun(thread_flex);

//  Thread mpuThread = Thread();
//  mpuThread.setInterval(10);
//  mpuThread.ThreadName = "mpuThread";
//  mpuThread.onRun(thread_mpu);

  controller.add(&flexThread);
//  controller.add(&mpuThread);

  controller.run();
}

void loop()
{
  
}

