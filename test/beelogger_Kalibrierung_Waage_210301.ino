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
//

//----------------------------------------------------------------
// Allgemeine Konfiguration
//----------------------------------------------------------------

// hier kann das Kalibriergewicht in _Gramm_ eingetragen werden
long KalibrierGewicht = 2043;  // 10000 = 10kG Referenzgewicht

//Sensorkonfiguration
#define Anzahl_Sensoren_Gewicht  1  // Mögliche Werte: '0','1','2','3','4','5','6'

// hier können bekannte Kalibrierwerte für Test-/Vergleichsmessung eingetragen werden
//long Taragewicht[6] = { 41263 , 5367,  10,  10,  10,  10};
//float Skalierung[6] = {20487.95 , 5367.77, 1.0, 1.0, 1.0, 1.0};

 long Taragewicht[6] = { 546762 , 5367 , 10 , 10}; // Hier ist der Wert aus der Kalibrierung einzutragen
 float Skalierung[6] = { 22899.75 , 5367.77 , 1.00 , 1.00}; // Hier ist der Wert aus der Kalibrierung einzutragen


// Anschluss / Konfiguration Wägezellen
// mit Anzahl_Sensoren_Gewicht 1
//   HX711(1) Kanal A = Wägeelement(e) Waage1
// mit Anzahl_Sensoren_Gewicht 2
//   HX711(1) Kanal B = Wägeelement(e) Waage2

// mit Anzahl_Sensoren_Gewicht 3
//   HX711(2) Kanal A = Wägeelement(e) Waage3
// mit Anzahl_Sensoren_Gewicht 4
//   HX711(2) Kanal B = Wägeelement(e) Waage4

// mit Anzahl_Sensoren_Gewicht 5
//   HX711(3) Kanal A = Wägeelement(e) Waage5
// mit Anzahl_Sensoren_Gewicht 6
//   HX711(3) Kanal B = Wägeelement(e) Waage6

// hier die Adressen für die/den HX711 eintragen
//             Nr.1: A,  B  Nr.2: A,  B   Nr.3: A, B
uint8_t HX711_SCK[6] = {A0, A0,      10, 10,        6, 6}; //  S-Clock
uint8_t HX711_DT[6] =  {A1, A1,      11, 11,        7, 7}; //  Data
//----------------------------------------------------------------


//----------------------------------------------------------------
// Ende Konfiguration

//----------------------------------------------------------------



//----------------------------------------------------------------
// Stromversorgung für Sensoren, Module und DS3231
// WLAN und GSM Modul stopp
//----------------------------------------------------------------
#define Power_Pin 4
#define Stop_GSM_WLAN A2
//----------------------------------------------------------------


#include <LowPower.h>
#include <HX711.h>

//----------------------------------------------------------------
// Variablen
//----------------------------------------------------------------
HX711 scale;
const float No_Val =  -1.0;  // Vorbelegung, Wert nicht gemessen
float Gewicht[6] = {No_Val, No_Val, No_Val, No_Val, No_Val, No_Val};
float LetztesGewicht[6] = {0, 0, 0, 0, 0, 0};
float DS_Temp = No_Val;
//----------------------------------------------------------------


void setup() {

  Serial.begin(9600);
  while (!Serial) {};
  Serial.println("Waage Kalibrierung 01.03.2021");
  Serial.println(" ");

  kalibrieren();

  Serial.println(F("Kalibriervorgang abgeschlossen. "));
  Serial.println(" ");
  Serial.flush();

  werte_anzeigen();

  Serial.println(" ");
  Serial.println(F( " Weiter mit w"));
  Serial.println(" ");
  char c = '0';
  while (c != 'w') {
    c = Serial.read();
  };
  Serial.println(F("Starte Testwiegen. "));
  Serial.println(F("x = Kalibrierung wiederholen. "));
  Serial.println(" ");
  Serial.flush();
  delay(200);
}

//######################################################
#include <avr/wdt.h>
void loop() {
  char buf[16];
  Sensor_Gewicht(1);

  for (int i = 0; i < Anzahl_Sensoren_Gewicht; i++) {
    if (LetztesGewicht[i] == -1000.0) {
      Serial.println(F(" --- ignoriert"));
      continue;  // keine Messung
    }
    Serial.print(F("  Waage "));
    Serial.print(i + 1);
    Serial.print(F(":        Gewicht: "));
    dtostrf(Gewicht[i], 4, 3, buf);
    Serial.print(buf);
    Serial.print(" kg");
    delay(500);
    Serial.print(F("   Skalierung: "));
    Serial.print(Skalierung[i]);
    Serial.print(F("   Taragewicht: "));
    Serial.println(Taragewicht[i]);
    Serial.flush();
    delay(1000);
  }

  Serial.print(F("\n\n"));
  Serial.flush();

  char c = Serial.read();
  if (c == 'x') {
    Serial.print(F("\n\n\n\n\n"));
    Serial.flush();
    wdt_enable(WDTO_30MS); // turn on the WatchDog and don't stroke it.
    for (;;) {}       // do nothing and wait for the eventual...
  }

  delay(5000);
}
//######################################################


//----------------------------------------------------------------
void kalibrieren() {
  char c;
  char buf[16];
  float Kal_Gew = ((float)KalibrierGewicht) / 1000.0;
  float wert;

  digitalWrite(Stop_GSM_WLAN, LOW);
  pinMode(Stop_GSM_WLAN, OUTPUT);
  digitalWrite(Stop_GSM_WLAN, LOW);
  digitalWrite(Power_Pin, HIGH);
  pinMode(Power_Pin, OUTPUT);
  digitalWrite(Power_Pin, HIGH);
  Serial.println(F("Zur Kalibrierung der Stockwaagen bitte den Anweisungen folgen!"));
  Serial.println(F("Fehlerhafte und nicht angeschlossene Waagen werden auch angezeigt!"));
  Serial.println(F("Eine Waage, die nicht kalibriert werden soll, kann ausgelassen werden."));
  Serial.println(" ");
  Serial.print(F("... konfiguriert:  "));
  Serial.print(Anzahl_Sensoren_Gewicht);
  Serial.println(F("  Waage(n)!"));
  Serial.println(" ");
  Serial.println(F("  Alle Waagen ohne Gewicht!"));
  Serial.println(" ");
  delay(5000);

  for (int i = 0; i < Anzahl_Sensoren_Gewicht; i++) {

    c = '#';
    if ((i == 1) || (i == 3) || ( i == 5)) {
      scale.set_gain(32);  // Nr.1,2,3 Kanal B
    }
    else {
      scale.begin(HX711_DT[i], HX711_SCK[i]); // Nr. 1, 2, 3 Kanal A
      scale.set_gain(128);
    }
    scale.power_up();

    if (scale.wait_ready_retry(10, 500)) {
      Serial.println(F("HX711 o.k. \n"));
    } else {
      Serial.println(F(" HX711 nicht gefunden. HX711 SCK/DT Konfiguration und/oder Verdrahtung prüfen!"));
      Serial.println(F( " uint8_t HX711_SCK[6] = {  ??  }"));
      Serial.println(F( " uint8_t HX711_DT[6]  = {  ??  }"));
    }
    scale.read();

    Serial.print(F("Waage Nummer: "));
    Serial.println(i + 1);
    delay(500);
    Serial.println(F( " Kalibrierung der Null-Lage ohne Gewicht mit '1' und 'Enter' starten!"));
    Serial.println(F( " Eingabe von 'x': Waage wird nicht kalibriert."));
    Serial.flush();
    while ((c != '1') && (c != 'x')) {
      c = Serial.read();
    };
    if (c == 'x') {
      Serial.println(" ");
      if (Taragewicht[i] == 10 ) {
        LetztesGewicht[i] = -1000.0;
        Serial.println(F(" keine Messung "));
        Serial.println(" ");
      }
      else {
        Serial.println(F(" verwende vorgegebene Daten für das Testwiegen"));
      }
      Serial.println(" ");
      continue;
    }
    c = '#';
    Serial.println(" ");
    Serial.print(F("Null-Lage ... "));
    scale.set_scale(1.0);
    scale.read_average(50);

    Serial.print(F("  ...  "));
    Serial.flush();
    scale.read_average(50);

    Serial.println(F("  ...  "));
    Serial.flush();
    Taragewicht[i] = scale.read_average(50);
    Serial.print(F(" Tara:  "));    Serial.println(Taragewicht[i]);
    Serial.println(" ");

    if ((Taragewicht[i] > 8000000 ) || (Taragewicht[i] < -8000000)) {
      Serial.println(F("  Fehler in der Verdrahtung oder Wägezelle defekt!"));
      c = 'm';
      while (c != 'w') {
        c = Serial.read();
      }
      Serial.print(F("\n\n\n\n\n"));
      Serial.flush();
      wdt_enable(WDTO_30MS); // turn on the WatchDog and don't stroke it.
      for (;;) {}       // do nothing and wait for the eventual...
    }
    if ((Taragewicht[i] == 0)) {
      Serial.println(F("  Eventuell HX711 nicht gefunden. HX711 SCK/DT Konfiguration und/oder Verdrahtung prüfen!"));
      delay(20000);
    }

    Serial.print(F("Waage Nummer: "));
    Serial.println(i + 1);
    Serial.print(F("mit genau  "));
    dtostrf(Kal_Gew, 4, 3, buf);
    Serial.print(buf);
    Serial.println(F("  Kilogramm beschweren - Kalibrierung mit '2' und 'Enter' starten!"));
    Serial.flush();

    while (c != '2') {
      c = Serial.read();
    };
    Serial.println(" ");
    Serial.print(F("Kalibriere Waage: "));
    Serial.print(i + 1);
    Serial.print(F("  ...  "));
    Serial.flush();
    scale.read_average(20);

    Serial.println(F("  ...  "));
    Serial.flush();
    wert = (float)(scale.read_average(32) - Taragewicht[i]);
    Skalierung[i] = wert / Kal_Gew;
    Serial.print(F("Taragewicht "));
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(Taragewicht[i]);

    Serial.print(F("Skalierung  "));
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(Skalierung[i]);
    Serial.println(" ");
    Serial.flush();
    if (Skalierung[i] < 0.0) {
      Serial.print(F("\n Skalierwert negativ. Anschluss Wägezelle an HX711 prüfen oder Belastungsrichtung Wägezelle falsch! \n"));
      Serial.flush();
      c = 'm';
      while (c != 'w') {
        c = Serial.read();
      };
    }
    if (Skalierung[i] == 0.0) {
      Serial.println(F("\n Fehler in Messwerten:"));
      Serial.println(F(" - Verbindung HX711 zu ATmega prüfen."));
      Serial.println(F(" - eventuell Wägezelle mit anderen Leiterfarben?"));
      Serial.println(F(" - eventuell Wägezelle defekt?"));
      Serial.println(F(" - HX711 defekt?"));
      Serial.println(F(" - ausschließlich HX711- Bibliothek aus beelogger- Lib-Paket verwenden."));
      Serial.flush();
      c = 'm';
      while (c != 'w') {
        c = Serial.read();
      };
    }
    Serial.print(F("Pruefe Gewicht: "));
    Serial.flush();
    long l_gew = scale.read_average(32) - Taragewicht[i];
    float gew = ((float) l_gew) / Skalierung[i];

    if (isnan(gew)) {
      Serial.print  (F("\n Verbindung HX711 zu ATmega prüfen oder HX711 defekt! \n"));
      Serial.flush();
    }
    dtostrf(gew, 4, 3, buf);
    Serial.println(buf);
    Serial.println(" ");
    Serial.flush();
  }
  Serial.println(" ");
}
//----------------------------------------------------------------

//----------------------------------------------------------------
void werte_anzeigen() {
  Serial.println (F(" Die Zeile für die Konfiguration:"));
  Serial.println(" ");
  if (Anzahl_Sensoren_Gewicht > 4) {
    Serial.print (F("const long Taragewicht[6] = { "));
  } else {
    Serial.print (F("const long Taragewicht[4] = { "));
  }
  Serial.print (Taragewicht[0]);  Serial.print (" , ");
  Serial.print (Taragewicht[1]);  Serial.print (" , ");
  Serial.print (Taragewicht[2]);  Serial.print (" , ");
  Serial.print (Taragewicht[3]);
  if (Anzahl_Sensoren_Gewicht > 4) {
    Serial.print (" , ");
    Serial.print (Taragewicht[4]);
    Serial.print (" , ");
    Serial.print (Taragewicht[5]);
  }
  Serial.println (F("}; // Hier ist der Wert aus der Kalibrierung einzutragen"));
  if (Anzahl_Sensoren_Gewicht > 4) {
    Serial.print (F("const float Skalierung[6] = { "));
  } else {
    Serial.print (F("const float Skalierung[4] = { "));
  }
  Serial.print (Skalierung[0]); Serial.print (" , ");
  Serial.print (Skalierung[1]); Serial.print (" , ");
  Serial.print (Skalierung[2]); Serial.print (" , ");
  Serial.print (Skalierung[3]);
  if (Anzahl_Sensoren_Gewicht > 4) {
    Serial.print (" , ");
    Serial.print (Skalierung[4]);
    Serial.print (" , ");
    Serial.print (Skalierung[5]);
  }
  Serial.println (F("}; // Hier ist der Wert aus der Kalibrierung einzutragen"));
  Serial.println (" ");
}
//----------------------------------------------------------------



//----------------------------------------------------------------
// Funktion Gewicht
//----------------------------------------------------------------
void Sensor_Gewicht(boolean quick) {
  if ((Anzahl_Sensoren_Gewicht > 0) && (Anzahl_Sensoren_Gewicht < 7)) {
    const float Diff_Gewicht = 0.5;
    //HX711 scale;
    for (int i = 0; i < Anzahl_Sensoren_Gewicht; i++) {
      if (LetztesGewicht[i] == -1000.0) continue;   // keine Messung
      Serial.print(" - wiegen mit Waage ");
      Serial.print(i + 1);
      Serial.print(" ");
      Serial.flush();
      if ((i == 0) || (i == 2) || (i == 4)) {
        scale.begin(HX711_DT[i], HX711_SCK[i], 128); // Nr.1,2,3 Kanal A
      }
      else {
        scale.set_gain(32);  // Nr.1,2,3 Kanal B
      }
      scale.power_up();
      scale.read();
      LowPower.powerStandby(SLEEP_500MS, ADC_OFF, BOD_OFF);

      for (uint8_t j = 0 ; j < 2; j++) { // Anzahl der Widerholungen, wenn Abweichung zum letzten Gewicht zu hoch
        long l_gew = scale.read_average(10) - Taragewicht[i];
        Gewicht[i] = ((float) l_gew) / Skalierung[i];
        if (quick) {
          LetztesGewicht[i] = Gewicht[i];
          break;
        }
        if (fabs(Gewicht[i] - LetztesGewicht[i]) < Diff_Gewicht) break; // Abweichung für Fehlererkennung
        LowPower.powerStandby(SLEEP_1S, ADC_OFF, BOD_OFF); // Wartezeit zwischen Wiederholungen
        Serial.print("..");
      }
      LetztesGewicht[i] = Gewicht[i];

      if ((i == 1) || (i == 3) || (i == 5)) { // vier/sechs Kanal
        scale.power_down();
      }
      if ((i == 0) && (Anzahl_Sensoren_Gewicht == 1)) { // HX711 Nr. 1 abschalten
        scale.power_down();
      }
      if ((i == 2) && (Anzahl_Sensoren_Gewicht == 3)) { // HX711 Nr. 2 abschalten
        scale.power_down();
      }
      if ((i == 4) && (Anzahl_Sensoren_Gewicht == 5)) { // HX711 Nr. 3 abschalten
        scale.power_down();
      }
      Serial.println(" ");
      Serial.flush();
    }
  }
}
//----------------------------------------------------------------
