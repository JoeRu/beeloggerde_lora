
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

int Kalib_Spannung = 5020; //Hier ist der mit Multimeter gemessene Wert der Akkuspannung in Millivolt einzutragen

//----------------------------------------------------------------
// Ende Konfiguration
//----------------------------------------------------------------


#include <Sodaq_DS3231.h>
#include <Wire.h>

int Kalib_Bitwert = 0;
float Batteriespannung = 999.99;
byte Power_Pin = 4;


void setup() {

  Serial.begin(9600);
  while (!Serial) {};
  Serial.println("beelogger Kalibrierung ADC 01.03.2021");
  Serial.println(" ");

#if defined(ARDUINO_AVR_NANO)
  Serial.println(" beelogger-Universal mit Arduino Nano  ");
#else
  Serial.println(" beelogger mit Arduino Pro-Mini  ");
#endif
  Serial.println(" ");
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  pinMode(Power_Pin, OUTPUT);
  digitalWrite(Power_Pin, HIGH);
  delay(5);
  Serial.println("Starte Uhrbaustein:");
  Serial.flush();
  rtc.begin();
  rtc.setDateTime(1); // start RTC
  delay(50);
  DateTime pc_tim = DateTime(__DATE__, __TIME__);
  long l_pczeit = pc_tim.get();
  DateTime aktuell = rtc.now();
  long l_zeit = aktuell.get();
  if (l_pczeit > l_zeit)  rtc.setDateTime(l_pczeit);
  delay(50);
  if (display_time()) Serial.println(F("Uhrbaustein initialisiert."));
  digitalWrite(13, LOW);
  digitalWrite(Power_Pin, LOW);
  Serial.flush();

  TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));
  digitalWrite (A4, LOW);
  digitalWrite (A5, LOW);
  if (Kalib_Spannung == 0) {
    Serial.println(F("Kein Wert fuer 'Kalib_Spannung' eingetragen."));
    Serial.println(F("Die Kalibrierung kann nicht durchgefuehrt werden."));
    while (true) {};
  }

  kalib_spg();
}


void loop() {
  led_blink(10, 500);
  delay(2000);
  kalib_spg();
}

//--------------------------------------------------
uint8_t display_time() {
  uint8_t ok_val = 0;
  DateTime aktuell = rtc.now();
  if ( aktuell.year() > 2100) {
    Serial.print(F("\nUhrbaustein nicht erkannt oder nicht vorhanden \n"));
  }
  else {
    ok_val = 1;
    Serial.print(F("\nDatum und Uhrzeit aktuell im Uhrbaustein: \n"));
    Serial.print(aktuell.date(), DEC);
    Serial.print('.');
    Serial.print(aktuell.month(), DEC);
    Serial.print('.');
    Serial.print(aktuell.year(), DEC);
    Serial.print(' ');
    Serial.print(aktuell.hour(), DEC);
    Serial.print(':');
    Serial.print(aktuell.minute(), DEC);
    Serial.print(':');
    Serial.print(aktuell.second(), DEC);
  }
  Serial.println(" ");
  Serial.flush();
  return (ok_val);
}
//--------------------------------------------------

void kalib_spg() {
  Kalib_Bitwert = analogRead(A6);
  Kalib_Bitwert = 0;
  for (byte j = 0 ; j < 16; j++) {
    Kalib_Bitwert += analogRead(A6);
  }
  Kalib_Bitwert = Kalib_Bitwert >> 2;
  Serial.println();  Serial.println();
  Serial.print(F("Hinterlegter Wert fuer 'Kalib_Spannung': "));
  Serial.println(Kalib_Spannung);

  Serial.print(F("Gemessener Wert fuer 'Kalib_Bitwert': "));
  Serial.println(Kalib_Bitwert);
  Serial.println(" ");

  if (Kalib_Bitwert > 3700) {
    Serial.println (F("################################"));
    Serial.println (F("Fehler in der Kalibrierung"));
    Serial.println(" ");
    Serial.println (F("'Kalib_Bitwert' zu hoch"));
    Serial.println(" ");
    Serial.println (F("Widerstand 1MOhm und 470/430kOhm kontrollieren."));
    Serial.println(" ");
    Serial.println (F("Spannung an Pin A6 darf nicht mehr sein als:"));
    Serial.println (F("     3V (beelogger-solar),"));
    Serial.println (F("   4,5V (beelogger-Universal)."));
    Serial.println (F("################################"));
  }
  else if (Kalib_Bitwert < 800) {
    Serial.println (F("################################"));
    Serial.println (F("Fehler in der Kalibrierung"));
    Serial.println(" ");
    Serial.println (F("'Kalib_Bitwert' zu klein"));
    Serial.println(" ");
    Serial.println (F("Widerstand 1MOhm und 470/430kOhm kontrollieren."));
    Serial.println(" ");
    Serial.println (F("################################"));
  }
  else {
    Batteriespannung = (map(Kalib_Bitwert, 0, Kalib_Bitwert, 0, Kalib_Spannung)) / 1000.0;

    Serial.print(F("Die mit dieser Kalibrierung ermittelte Akkuspannung betraegt:"));
    Serial.print(Batteriespannung);
    Serial.println(" V");
    Serial.println(" ");

    Serial.println (F(" Die Zeilen für die Konfiguration:"));
    Serial.println(" ");
    Serial.print (F("const long Kalib_Spannung =  "));
    Serial.print (Kalib_Spannung);
    Serial.println (F(";    // Hier ist der Wert aus der Kalibrierung einzutragen"));
    Serial.print (F("const long Kalib_Bitwert  =  "));
    Serial.print (Kalib_Bitwert);
    Serial.println (F(";    // Hier ist der Wert aus der Kalibrierung einzutragen"));
    Serial.println (" ");
  }
}
void led_blink(int cnt, int tm) {
  int x = 0;
  for (; x < cnt; x++) {
    digitalWrite(13, HIGH);
    delay(tm);
    digitalWrite(13, LOW);
    delay(tm);
  }
}
