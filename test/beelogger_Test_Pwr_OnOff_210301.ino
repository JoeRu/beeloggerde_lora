/*
   (C) 2020 R.Schick / Thorsten Gurzan - beelogger.de

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

// beelogger.de - Arduino Datenlogger für Imker
// Erläuterungen dieses Programmcodes unter http://beelogger.de


//----------------------------------------------------------------
// Konfiguration
//----------------------------------------------------------------

// Kalibrierung, bitte Werte eintragen.
//----------------------------------
const long Kalib_Spannung =  5500;    // Hier ist der Wert aus der Kalibrierung einzutragen
const long Kalib_Bitwert  =  1114;    // Hier ist der Wert aus der Kalibrierung einzutragen


//----------------------------------

//----------------------------------------------------------------
// Ende Konfiguration
//----------------------------------------------------------------



#include <Sodaq_DS3231.h>
#include <Wire.h>
#include <LowPower.h>

// Intervalle
//----------------------------------
byte WeckIntervallMinuten = 1;
byte AlternativIntervallMinuten = 3;
float VAlternativ = 1.0;             // Minimale Spannung ab der automatisch das alternative Intervall aktiviert wird
float VMinimum = 1.0;                 // Minimale Spannung, Fehlermeldung
//----------------------------------


float Batteriespannung = -1.0;
float Solarspannung = -1.0;
float RTCTemp = 99.9;

volatile bool ok_sleep = false;

#define USB_RX      0          // Pin 0 RX-USB
#define USB_TX      1          // Pin 1 TX-USB
#define DS3231_Interrupt_Pin  2

#define Power_Pin  4

#define Batterie_messen    A6
#define Solarzelle_messen  A7

DateTime aktuell;
int count = 2;
//--------------------------------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial) {};
  Serial.println(F("beelogger Power-On-Off-Test 01.03.2021 "));
  Serial.println(" ");
#if defined(ARDUINO_AVR_NANO)
  Serial.println(F(" beelogger-Universal mit Arduino Nano  "));
#else
  Serial.println(F(" beelogger mit Arduino Pro-Mini  "));
#endif

  Serial.println(" ");
  Serial.println(F("... teste Uhr(DS3231), ADC-Kalibrierwerte und Power-ON-OFF "));

  pinMode(Power_Pin, OUTPUT);
  digitalWrite(Power_Pin, HIGH);

  if ( Kalib_Bitwert == 100) {
    Serial.println("\n");
    Serial.println(F("Bitte Kalibrierwerte eintragen! "));
    Serial.println(F("Bitte Kalibrierwerte eintragen! "));
    Serial.println(F("Bitte Kalibrierwerte eintragen! "));
    Serial.println("\n");
    WeckIntervallMinuten = 10;
  }
  Spannungen_messen();
  delay(5);


  rtc.begin();

  DateTime pc_tim = DateTime(__DATE__, __TIME__);
  long l_pczeit = pc_tim.get();
  DateTime aktuell = rtc.now();
  long l_zeit = aktuell.get();
  if (l_pczeit > l_zeit)  rtc.setDateTime(pc_tim.get());

  rtc.clearINTStatus(); 

  delay(100);
  pinMode(DS3231_Interrupt_Pin, INPUT_PULLUP);
  delay(100);

  check_D2();  // check Interrupt Pin

  pinMode(DS3231_Interrupt_Pin, INPUT_PULLUP);
  ok_sleep = true;

  delay(5);

  if (display_time() == 0) {
    Serial.println(F("\n\nSketch STOP"));
    delay(200);
    while (true) {};
  }

  Alarm_konfigurieren();
  SleepNow(false);
}
//--------------------------------------------------


//##################################################
void loop() {
  check_intervall();
  Spannungen_messen();
  check_D2();  // check Interrupt Pin
  display_time();
  display_temp();

  Alarm_konfigurieren();
  SleepNow(true);
}
//##################################################

void check_D2() {
  int val = digitalRead(DS3231_Interrupt_Pin);
  if ( val == 0) {
    Serial.println("\n");
    Serial.println(F("  PIN D2  'ist auf' 0 Volt, eventuell: "));
    Serial.println(F("   - Modifikation DS3231-Modul nicht erfolgt oder fehlerhaft? "));
    Serial.println(F("   - Kurzschluß an D2?  Schalter an D2 ein? "));
    Serial.println(F("  Sketch nicht funktionsfähig! "));
    Serial.println(" ");
    Serial.println(F("  Sketch Neustart versuchen! "));
    Serial.println("\n");
    while (digitalRead(DS3231_Interrupt_Pin) == 0) {
      delay (5000);
      Serial.print(".");
    };
  }
}
//--------------------------------------------------
uint8_t  display_time() {
  uint8_t ok_val = 0;
  char buf[8];
  DateTime aktuell = rtc.now();
  if ( aktuell.year() > 2100) {
    Serial.print(F("\nDS3231 Uhrbaustein nicht erkannt oder nicht vorhanden \n"));
  }
  else {
    ok_val = 1;
    Serial.print(F("beelogger-System Datum und Uhrzeit: "));
    Serial.print(aktuell.date());
    Serial.print('.');
    Serial.print(aktuell.month());
    Serial.print('.');
    Serial.print(aktuell.year());
    byte h = aktuell.hour();
    if (h < 10) {
      Serial.print("  ");
    }
    else Serial.print(" ");
    Serial.print(h);
    Serial.print(':');
    sprintf(buf, "%02d", aktuell.minute());
    Serial.print(buf);
    Serial.print(':');
    sprintf(buf, "%02d", aktuell.second());
    Serial.print(buf);
  }
  Serial.println(" ");
  Serial.flush();
  return (ok_val);
}
//--------------------------------------------------

//--------------------------------------------------
void check_intervall() {
  long l_akt = aktuell.get() - 60; //min. 1 minute
  DateTime jetzt = rtc.now();
  long l_jetzt = jetzt.get();
  long df = (l_jetzt - l_akt);
  if (df < 55) {
    Serial.println(F("********************************************"));
    Serial.println(F("Fehler Sleepzeit nicht erreicht !"));
    Serial.println(F(" Test wiederholen, wenn Fehler nochmal erfolgt:"));
    Serial.println(F("  DS3231-Modul  Modifikation nicht erfolgt oder fehlerhaft? "));
    Serial.println(F("********************************************"));
    Serial.println();
    Serial.flush();
  }
}
//--------------------------------------------------

//--------------------------------------------------
void display_temp() {
  RTCTemp = rtc.getTemperature();
  Serial.print(F("Temperatur ueber Sensor in RTC: "));
  Serial.println(RTCTemp);
  Serial.println();
  Serial.flush();
}
//--------------------------------------------------



//--------------------------------------------------
void Spannungen_messen() {
  Batteriespannung = Messe_Spannung(Batterie_messen);
  Serial.print(F(" Batterie [V]: "));
  Serial.println(Batteriespannung);
  Serial.println();
  Serial.flush();
  if (Batteriespannung > VMinimum) {
    Solarspannung = Messe_Spannung(Solarzelle_messen);
    Serial.print(F(" Solarspannung [V]: "));
    Serial.println(Solarspannung);
    Serial.println();
    Serial.flush();
  }
}

float Messe_Spannung (byte Pin) {
  int Messung_Spannung;
  float Spannung;
  Messung_Spannung = analogRead(Pin);
  Messung_Spannung = 0;
  for (byte j = 0 ; j < 16; j++) {
    Messung_Spannung += analogRead(Pin);
  }
  Messung_Spannung = Messung_Spannung >> 2;
  Serial.print(F("\n  ADC-Bitwert= "));  Serial.print(Messung_Spannung); Serial.println();
  Spannung = (float)map(Messung_Spannung, 0, Kalib_Bitwert, 0, Kalib_Spannung) / 1000.0;
  return (Spannung);
}
//--------------------------------------------------


//--------------------------------------------------
void Alarm_konfigurieren() {
  byte IntervallMinuten;

  if (Batteriespannung > VAlternativ) {
    IntervallMinuten = WeckIntervallMinuten;
  } else {
    Serial.println();
    Serial.println(F("******************************************************************"));
    Serial.println(F("Batteriespannung unter Grenzwert, alternatives Intervall aktiviert"));
    Serial.println(F("******************************************************************"));
    Serial.println();
    IntervallMinuten = AlternativIntervallMinuten;
  }
  Serial.println();
  Serial.print(F("Weckintervall: "));
  Serial.print(IntervallMinuten);
  Serial.println(F(" Minute(n)"));
  Serial.flush();

  aktuell = rtc.now();
  long timestamp = aktuell.get();
  aktuell = timestamp + IntervallMinuten * 60;
  rtc.enableInterrupts(aktuell.hour(), aktuell.minute(), aktuell.second());

  Serial.print(F("Wakeup at: "));
  Serial.print(aktuell.hour());
  Serial.print(":");
  if (aktuell.minute() < 10) Serial.print("0");
  Serial.println(aktuell.minute());
  Serial.println();
  Serial.flush();
}
//--------------------------------------------------


//--------------------------------------------------
void WakeUp() {
  detachInterrupt(0);
  ok_sleep = false;
}
//--------------------------------------------------


//--------------------------------------------------
void SleepNow(byte power_off) {
  delay(100);
  if (power_off) {
    Serial.println(F("Sleep-Modus mit Power-OFF ist aktiviert, bitte warten ... "));
    Serial.flush();
    digitalWrite(Power_Pin, LOW);
    delay(2000);
    check_D2();  // check Interrupt Pin
  }
  else {
    Serial.println(F("Sleep-Modus mit Power-ON gestartet, bitte warten ... "));
  }
  Serial.flush();
  Serial.end();        // Serial aus
  delay(50);
  digitalWrite(USB_RX, LOW);   // Port aus
  pinMode(USB_TX, INPUT);
  digitalWrite(USB_TX, LOW);   // Port aus

  TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));
  digitalWrite (A4, LOW);
  digitalWrite (A5, LOW);

  delay(1);
  attachInterrupt(0, WakeUp, LOW);
  if (ok_sleep) {
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
  else {
    Serial.begin(9600);
    Serial.println(F("********************************************"));
    Serial.println(F("Sleep-Modus konnte nicht aktiviert werden"));
    Serial.println(F("********************************************"));
  }


  digitalWrite(Power_Pin, HIGH);
  delay (5);
  rtc.begin();
  rtc.clearINTStatus();
  ok_sleep = true;

  Serial.begin(9600);
  if (power_off) {
    Serial.println(F("Sleep-Modus mit Power-OFF beendet "));
    Serial.println();
    Serial.println(F("Power-On-OFF Test erfolgreich!"));
    Serial.println();
    count --;
  }
  else {
    Serial.println(F("Power-ON Sleep wurde beendet."));
    Serial.println();
  }
  Serial.println();
  Serial.flush();
  delay(5);
  // Power off test counter
  if (count == 0) {
    Serial.println(F("------------------- Test beendet"));

    Serial.flush();
    Serial.end();        // Serial aus
    delay(50);
    digitalWrite(USB_RX, LOW);   // Port aus
    pinMode(USB_TX, INPUT);
    digitalWrite(USB_TX, LOW);   // Port aus
    digitalWrite(Power_Pin, LOW);

    TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));
    digitalWrite (A4, LOW);
    digitalWrite (A5, LOW);

    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    Serial.begin(9600);
    while (1 == 1) {
      delay (5000);
      Serial.print(".");
    }
  }
}
//--------------------------------------------------
