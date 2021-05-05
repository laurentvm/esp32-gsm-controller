#include <Arduino.h>
#include <TaskScheduler.h>
#define SIM800L_IP5306_VERSION_20190610

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

TinyGsmClient client(modem);

/*
  Semaphore
*/
SemaphoreHandle_t semaphoreNetworkOn; // to signalize upload is in progress

void initSemaphores()
{ // changed configSUPPORT_STATIC_ALLOCATION to 1 in FreeRTOSConfig.h to get static version of xSemCreateBin
  semaphoreNetworkOn = xSemaphoreCreateBinary();
  /*
  semaphoreEndOfTrip = xSemaphoreCreateBinary(); // !!! TODO check this semaphore with semaphoreGetCount instead of polling using take and 0 blocking time - should not increment it -> TEST
  semaphoreSPISync = xSemaphoreCreateBinary();
  semBufferSwitch = xSemaphoreCreateBinary();
  sdSpiMutex = xSemaphoreCreateMutex();

  xSemaphoreGive(semaphoreUpload); // once created with xSemaphoreCreateBinary, bin semaphores are in state "taken"
  xSemaphoreGive(semaphoreSPISync);
  xSemaphoreGive(semBufferSwitch);
  // note: semaphoreEndOfTrip is in state "taken", i.e. 0, will be set to 1 ("given") by stateChart to signalize IORoutine end of trip
  */
}

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
  //pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, LED_OFF);

  xSemaphoreGive(semaphoreNetworkOn);
  network_is_on
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
int network_ok = 0;

void ledStatus();

Task ledTask(3000, TASK_FOREVER, &ledStatus);

void ledStatus()
{
  Serial.println(F("in ledStatus"));
  digitalWrite(LED_GPIO, LED_ON);
  delay(50);
  digitalWrite(LED_GPIO, LED_OFF);
}

/*
Scheduler runner;
void setup()
{
  // Set console baud rate
  SerialMon.begin(115200);

  delay(10);

  // Start power management
  if (setupPMU() == false)
  {
    Serial.println("Setting power error");
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  }
  digitalWrite(LED_GPIO, LED_ON);
  
  // Some start operations
  setupModem();

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  
runner.init();
Serial.println("Initialized scheduler");
runner.addTask(ledTask);
Serial.println("added ledTask");
}

void loop()
{
  runner.execute();
}
*/
// Callback methods prototypes
void t1Callback();
void t2Callback();
void t3Callback();

//Tasks
Task t4();
Task t1(2000, 10, &t1Callback);
Task t2(3000, TASK_FOREVER, &t2Callback);

Scheduler runner;

int network_is_on = 0;

void t1Callback()
{
  Serial.print("t1: ");
  Serial.println(millis());

  if (t1.isFirstIteration())
  {
    runner.addTask(t3);
    t3.enable();
    Serial.println("t1: enabled t3 and added to the chain");
  }

  if (t1.isLastIteration())
  {
    t3.disable();
    runner.deleteTask(t3);
    //t2.setInterval(500);
    Serial.println("t1: disable t3 and delete it from the chain. t2 interval set to 500");
  }
}

void t2Callback()
{
  Serial.print("t2: ");
  Serial.println(millis());

  if (network_is_on)
  {
    int i = 10;
    while (i--)
    {
      Serial.println(i);
      digitalWrite(LED_GPIO, LED_ON);
      delay(250);
      digitalWrite(LED_GPIO, LED_OFF);
      delay(250);
    }
  }
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
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  }
  initSemaphores();
  // turn on LED for 1 sec
  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, HIGH);
  delay(1000);
  digitalWrite(LED_GPIO, LOW);
  Serial.println("Scheduler TEST");

  runner.init();
  Serial.println("Initialized scheduler");

  //runner.addTask(t1);
  //Serial.println("added t1");

  runner.addTask(t2);
  Serial.println("added t2");

  delay(5000);

  t1.enable();
  Serial.println("Enabled t1");
  t2.enable();
  Serial.println("Enabled t2");
  //setupModem
  delay(10000);
  setupModem();
}

void loop()
{
  runner.execute();
}