
/*
   (C) 2020 R.Schick / Thorsten Gurzan - beelogger.de

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// beelogger.de - Arduino LoRaWAN Datenlogger für Imker
// Version 2021
// compile error fix oslmic.h line 230 static inline type table_get ## postfix#
// Migrated from .ino to PlatformIO .v01

char ID[] = "MULTI_LORA_"; //nur Grossbuchstaben,Zahlen, keine Blanks
long l_ID = 210310;

#define CFG_noassert 1

//bitte Multi_LORA_config.h nach my_Multi_LORA_config.h kopieren und entsprechend anpassen
#include "my_Multi_LORA_config.h" // Konfiguration und Kalibrierung eintragen
#include "beelogger_config.h"

//----------------------------------------------------------------
// Variablen
//----------------------------------------------------------------
const float No_Val = 99.9;  // Vorbelegung, Wert nicht gemessen
const float No_Value = 0.0; // Vorbelegung, Wert nicht gemessen

//                     KeinSensor DHT1, DHT1,   Si7021, SHT31a, SHT31b, BMEa,   BMEb,  DS18B20a,DS18B20b
float SensorTemp[12] = {No_Val, No_Val, No_Val, No_Val, No_Val, No_Val, No_Val, No_Val, No_Val, No_Val, No_Val, No_Val};

//                        KeinSensor  DHT1,    DHT2,     Si7021,   SHT31a,   SHT31b,   BMEa,     BMEb.   KeinSensor KeinSensor
float SensorFeuchte[10] = {No_Value, No_Value, No_Value, No_Value, No_Value, No_Value, No_Value, No_Value, No_Value, No_Value};

float Aux[4] = {No_Value, No_Value};

float DS_Temp = No_Val;
float Licht = No_Value;
float Gewicht[4] = {0.01, 0.01, 0.01, 0.01};
float LetztesGewicht[4] = {No_Value, No_Value, No_Value, No_Value};

float Batteriespannung = No_Val;
float Solarspannung = No_Val;

float Service = No_Value;

uint8_t WeckIntervallMinuten = WeckIntervall_default;

uint32_t time_on = 0;

uint8_t report_info = 0; // 0 = time-On

volatile bool ok_sleep = true;

// andere ehemalige INO Files als includes.
#include "Multi_LORA.h"
#include "beelogger_sensors.h"
#include "beelogger_utility_lora.h"
//-------------------------

void measure_and_send()
{
  Sensor_Temp_Zelle(false);
  Sensor_DHT();
  Sensor_DS18B20();
  Sensor_Si7021();
  Sensor_SHT31();
  Sensor_BME280();
  Sensor_Licht();
  Sensor_Temp_Zelle(true);
  Sensor_Gewicht(false);
  Daten_Senden();
}
//----------------------------------------------------------------
void setup()
{

  Serial.begin(Serial_Baudrate);
  Serial.print(ID);
  Serial.println(l_ID);

  //----------------------------------------------------------------
  //  System On
  //----------------------------------------------------------------
  //Spannungen_messen();

  while (Batteriespannung < VMinimum)
  {
    LowPower.powerStandby(SLEEP_8S, ADC_OFF, BOD_OFF);
    Spannungen_messen();
  }

  init_LoRaSeq(); // init LoRa sequence number

  digitalWrite(Power_Pin, HIGH);
  pinMode(Power_Pin, OUTPUT);
  delay(5);

  setup_hx711();
  System_On();

  //----------------------------------------------------------------
  //  Setup RTC
  //----------------------------------------------------------------
  DateTime pc_tim = DateTime(__DATE__, __TIME__);
  long l_pczeit = pc_tim.get();
  DateTime aktuell = rtc.now();
  long l_zeit = aktuell.get();
  if (l_pczeit > l_zeit)
  {
    rtc.setDateTime(l_pczeit);
  }

  //----------------------------------------------------------------
  //  Sleep Mode & Send data Interrupt
  //----------------------------------------------------------------
  pinMode(DS3231_Interrupt_Pin, INPUT_PULLUP);
  delay(5);
  //----------------------------------------------------------------

  //----------------------------------------------------------------
  // Setup Gewicht
  //----------------------------------------------------------------
  Sensor_Gewicht(true); // Startwert Gewicht holen
  //----------------------------------------------------------------

  Serial.flush();
  Serial.end();
  Serial_rxtx_off();
}

//##################################################################
void loop()
{

  debugbegin(Serial_Baudrate);
  debugprintlnF("Loop");
  debugflush();

  Spannungen_messen();
  if (Batteriespannung > VMinimum)
  {
    measure_and_send();
    if (User_Int() == 1)
    {
      measure_and_send();
    }
  }
  Alarm_konfigurieren();

  debugflush();
  debugend();
  Serial_rxtx_off();

  System_Off();
  SleepNow();
  System_On();
}
//##################################################################
