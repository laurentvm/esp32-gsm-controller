/*
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();

  // Turn off network status lights to reduce current consumption
  turnOnNetlight();

  // The status light cannot be turned off, only physically removed
  //turnOffStatuslight();

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(240000L))
  {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");

  // When the network connection is successful, turn on the indicator
  digitalWrite(LED_GPIO, LED_ON);

  if (modem.isNetworkConnected())
  {
    SerialMon.println("Network connected");
  }

  // Shutdown
  client.stop();
  SerialMon.println(F("Client disconnected"));

  // Make the LED blink three times before going to sleep
  int i = 10;
  while (i--)
  {
    digitalWrite(LED_GPIO, LED_ON);
    modem.sendAT("+SPWM=0,1000,80");
    delay(50);
    digitalWrite(LED_GPIO, LED_OFF);
    modem.sendAT("+SPWM=0,1000,0");
    delay(50);
  }

  //After all off
  modem.poweroff();

  SerialMon.println(F("Poweroff"));

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  esp_deep_sleep_start();
  */
/*
    The sleep current using AXP192 power management is about 500uA,
    and the IP5306 consumes about 1mA
    */