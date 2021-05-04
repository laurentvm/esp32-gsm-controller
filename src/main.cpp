#include <Arduino.h>

// Please select the corresponding model

#define SIM800L_IP5306_VERSION_20190610
// #define SIM800L_AXP192_VERSION_20200327
// #define SIM800C_AXP192_VERSION_20200609
// #define SIM800L_IP5306_VERSION_20200811

// #define TEST_RING_RI_PIN            //Note will cancel the phone call test

// #define ENABLE_SPI_SDCARD   //Uncomment will test external SD card

// Define the serial console for debug prints, if needed
//#define DUMP_AT_COMMANDS
#define TINY_GSM_DEBUG SerialMon

#include "utilities.h"

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to the module)
#define SerialAT Serial1

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800   // Modem is SIM800
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60          /* Time ESP32 will go to sleep (in seconds) */

// Server details
const char server[] = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";

// Your GPRS credentials (leave empty, if missing)
const char apn[] = "";      // Your APN
const char gprsUser[] = ""; // User
const char gprsPass[] = ""; // Password
const char simPIN[] = "";   // SIM card PIN code, if any

TinyGsmClient client(modem);
const int port = 80;

void setupModem()
{
#ifdef MODEM_RST
  // Keep reset high
  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, HIGH);
#endif

  pinMode(MODEM_PWRKEY, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  // Turn on the Modem power first
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Pull down PWRKEY for more than 1 second according to manual requirements
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);

  // Initialize the indicator as an output
  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, LED_OFF);
}

void turnOffNetlight()
{
  SerialMon.println("Turning off SIM800 Red LED...");
  modem.sendAT("+CNETLIGHT=0");
}

void turnOnNetlight()
{
  SerialMon.println("Turning on SIM800 Red LED...");
  modem.sendAT("+CNETLIGHT=1");
}

void setup()
{
  // Set console baud rate
  SerialMon.begin(115200);

  delay(10);

  // Start power management
  if (setupPMU() == false)
  {
    Serial.println("Setting power error");
  }

  // Some start operations
  setupModem();

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
}

void loop()
{
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

  //modem.gprsDisconnect();
  //SerialMon.println(F("GPRS disconnected"));

  // DTR is used to wake up the sleeping Modem
  // DTR is used to wake up the sleeping Modem
  // DTR is used to wake up the sleeping Modem
#ifdef MODEM_DTR
  bool res;

  modem.sleepEnable();

  delay(100);

  // test modem response , res == 0 , modem is sleep
  res = modem.testAT();
  Serial.print("SIM800 Test AT result -> ");
  Serial.println(res);

  delay(1000);

  Serial.println("Use DTR Pin Wakeup");
  pinMode(MODEM_DTR, OUTPUT);
  //Set DTR Pin low , wakeup modem .
  digitalWrite(MODEM_DTR, LOW);

  // test modem response , res == 1 , modem is wakeup
  res = modem.testAT();
  Serial.print("SIM800 Test AT result -> ");
  Serial.println(res);

#endif

#ifdef TEST_RING_RI_PIN
#ifdef MODEM_RI
  // Swap the audio channels
  SerialAT.print("AT+CHFA=1\r\n");
  delay(2);

  //Set ringer sound level
  SerialAT.print("AT+CRSL=100\r\n");
  delay(2);

  //Set loud speaker volume level
  SerialAT.print("AT+CLVL=100\r\n");
  delay(2);

  // Calling line identification presentation
  SerialAT.print("AT+CLIP=1\r\n");
  delay(2);

  //Set RI Pin input
  pinMode(MODEM_RI, INPUT);

  Serial.println("Wait for call in");
  //When is no calling ,RI pin is high level
  while (digitalRead(MODEM_RI))
  {
    Serial.print('.');
    delay(500);
  }
  Serial.println("call in ");

  //Wait 10 seconds for the bell to ring
  delay(10000);

  //Accept call
  SerialAT.println("ATA");

  delay(10000);

  // Wait ten seconds, then hang up the call
  SerialAT.println("ATH");
#endif //MODEM_RI
#endif //TEST_RING_RI_PIN

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

  /*
    The sleep current using AXP192 power management is about 500uA,
    and the IP5306 consumes about 1mA
    */
}