#include "beelogger_config.h"
//14.01.2020 Lora-Version
//25.04.2020 User Service
//03.05.2020 User Service Schalter
//14.07.2020 Servicewert minimal 1
//13.08.2020 clear rtc-int in User int
//----------------------------------------------------------------
// Funktion Spannungen messen
//----------------------------------------------------------------
float Messe_Spannung(uint8_t Pin)
{
  int Messung_Spannung;
  float Spannung;
  Messung_Spannung = analogRead(Pin);
  Messung_Spannung = 0;
  for (uint8_t j = 0; j < 16; j++)
  {
    Messung_Spannung += analogRead(Pin);
  }
  Messung_Spannung = Messung_Spannung >> 2;
  Spannung = (float)map(Messung_Spannung, 0, Kalib_Bitwert, 0, Kalib_Spannung) / 1000.0;
  return (Spannung);
}

void Spannungen_messen()
{
  Batteriespannung = Messe_Spannung(Batterie_messen);

  debugprintF("Bat.[V]:");
  debugprintln(Batteriespannung);
  debugflush();

  if (Batteriespannung > VMinimum)
  {
    Solarspannung = Messe_Spannung(Solarzelle_messen);

    debugprintF("Sol.[V]:");
    debugprintln(Solarspannung);
    debugflush();
  }
}

//----------------------------------------------------------------

//----------------------------------------------------------------
// Funktion hole Zeitstempel von RTC
//----------------------------------------------------------------
uint32_t Time_from_RTC()
{
  DateTime aktuell = rtc.now();
  uint32_t l_tm = aktuell.get();
  return (l_tm);
}
//----------------------------------------------------------------

//----------------------------------------------------------------
// Funktion Alarm konfigurieren LORA VERSION
//----------------------------------------------------------------
void Alarm_konfigurieren()
{
  uint8_t IntervallMinuten;
  uint8_t hnow;

  DateTime aktuell = rtc.now();
  hnow = aktuell.hour();

  if (Batteriespannung > VAlternativ)
  {
    IntervallMinuten = WeckIntervall_default;
    // Lora spezial Intervall konfig
    if (WeckIntervall_aktiv)
    { // Manuelle Intervalsteuerung aktiviert
      if ((hnow >= WeckIntervall_Nacht_Anfang) || (hnow < WeckIntervall_Nacht_Ende))
      {
        IntervallMinuten = WeckIntervall_Nacht;
      }
      else
      {
        IntervallMinuten = WeckIntervall_Tag;
      }
      if (WeckIntervall_Winter_aktiv == 1)
      { // Winterintervall aktiviert
        if ((aktuell.month() > WeckIntervall_Winter_Anfang) || (aktuell.month() < WeckIntervall_Winter_Ende))
        {
          IntervallMinuten = WeckIntervall_Winter;
        }
      }
    }
  }
  else
  {
    IntervallMinuten = AlternativIntervallMinuten;
  }
  if (IntervallMinuten < 5)
    IntervallMinuten = 5; // LoRaWan usage restrictions

  uint32_t l_tm = Time_from_RTC();
  uint32_t tt = l_tm - time_on;
  if (tt == 0)
    tt = 1;     // minimum 1 -> not a reset
  time_on = tt; // store total TimeOn

  aktuell = (DateTime)(l_tm + (uint32_t)IntervallMinuten * 60); // xx Minuten später

  rtc.enableInterrupts(aktuell.hour(), aktuell.minute(), 0); //interrupt at  hour, minute, second

  debugprintF("Wakeup:");
  debugprint(aktuell.hour());
  debugprint(":");
#if myDEBUG
  if (aktuell.minute() < 10)
    debugprint("0");
#endif
  debugprintln(aktuell.minute());
  debugflush();
}
//----------------------------------------------------------------

//----------------------------------------------------------------
// Funktion WakeUp
//----------------------------------------------------------------
void WakeUp()
{
  ok_sleep = false;
  detachInterrupt(0);
}

//----------------------------------------------------------------
// Funktion SleepNow
//----------------------------------------------------------------
void SleepNow()
{

  attachInterrupt(0, WakeUp, LOW); // D2 = Interrupt
  delay(1);

  while (ok_sleep)
  {
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    delay(100); // Schalter entprellen.
  }

  ok_sleep = true;
  delay(5);
}
//----------------------------------------------------------------

//----------------------------------------------------------------

//----------------------------------------------------------------
// Funktion freeRam
//----------------------------------------------------------------
int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
//----------------------------------------------------------------

//----------------------------------------------------------------
// Funktion System_Off
//----------------------------------------------------------------
void System_Off()
{
  hx711_SCK_Low(); // code order, important!
  digitalWrite(Power_Pin, LOW);
  delay(5);

  TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));
  digitalWrite(A4, LOW);
  digitalWrite(A5, LOW);

  digitalWrite(ONE_WIRE_BUS, LOW);
  digitalWrite(DHT_Sensor_Pin[0], LOW);
  digitalWrite(DHT_Sensor_Pin[1], LOW);
  peripherial_off(); // Lora SPI
}
//----------------------------------------------------------------

//----------------------------------------------------------------
// Funktion System_On
//----------------------------------------------------------------
void System_On()
{
  peripherial_on(); // Lora SPI
  hx711_SCK_High();
  digitalWrite(DHT_Sensor_Pin[0], HIGH);
  digitalWrite(DHT_Sensor_Pin[1], HIGH);

  digitalWrite(Power_Pin, HIGH);
  delay(5); // vcc stable

  rtc.begin();
  rtc.clearINTStatus();
  delay(5);                  // wait clear Int
  Service = (float)time_on;  // Save TimeOn
  time_on = Time_from_RTC(); // Setze Time_on
}
//----------------------------------------------------------------
//----------------------------------------------------------------
// Funktion User_Int
//   teste ob Schalter an D2 hat Interrupt gesetzt hat
//----------------------------------------------------------------
uint8_t User_Int()
{ // User forced Interrupt
  uint8_t ret_val = 0;
  int i = 0;
  uint32_t time_on_sav = time_on;

  rtc.clearINTStatus();
  delay(5); // wait clear Int
  if (digitalRead(DS3231_Interrupt_Pin) == false)
  { // Schalter "ein" ?
    debugprintlnF("User!");
    debugflush();
    System_Off();
    debugend();
    Serial_rxtx_off();
    do
    {
      i++;
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); // Warte auf Freigabe
    } while ((digitalRead(DS3231_Interrupt_Pin) == false) && (i < 256));
    if (i > 40)
      ret_val = 1; // LORA: send new data only if at least 5 minutes
    System_On();   // this sets time_on, therefore

    time_on = Time_from_RTC() - time_on_sav; // calculate TimeOn from saved value
    System_On();                             // saving TimeOn in Service and set time_on again
    debugbegin(Serial_Baudrate);
  }
  return (ret_val);
}
//----------------------------------------------------------------
