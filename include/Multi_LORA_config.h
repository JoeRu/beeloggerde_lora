#ifndef __multi_lora_config__
#define __multi_lora_config__
//----------------------------------------------------------------
// Sensorkonfiguration
// 1. Sensor für die Abfrage durch den Sketch aktivieren
// 2. aktive Sensoren für Temperatur/Feuchte in der Sensormatrix zuordnen
// 3. Sensor für Temperatur Wägezelle festlegen
//----------------------------------------------------------------
#define Anzahl_Sensoren_DHT     0 // Mögliche Anzahl: '0','1','2'  --- Nr 1,2 ---  (Temperatur + Luftfeuchte)

#define Anzahl_Sensoren_Si7021  0 // Mögliche Anzahl: '0','1'      --- Nr 3 -----  (Temperatur + Luftfeuchte)

#define Anzahl_Sensoren_SHT31   0 // Mögliche Anzahl: '0','1','2'  --- Nr 4,5 ---  (Temperatur + Luftfeuchte)

#define Anzahl_Sensoren_BME280  0 // Mögliche Anzahl: '0','1','2'  --- Nr 6,7 ---  (Temperatur + Luftfeuchte)

#define Anzahl_Sensoren_DS18B20 0 // Mögliche Anzahl: '0','1','2'  --- Nr 8,9 ---  (Nur Temperatur)
//                                   Mögliche Anzahl:     '3','4'  --- Nr 10,11 --- ( Messwert bei Single/Double als "Luftfeuchte" bei 8,9)


// 2. Sensormatrix, hier kann die Zuordnung der Sensoren geändert werden
// Nr 1 - 9 aus Liste oben auswählen, wenn kein Sensor gewünscht ist einfach "0" angeben
// wenn kein Sensor für die Aussentemperatur gesetzt ist, wird automatisch der Temperatursensor der RTC verwendet
#define             Aussenwerte                                          0    // 0 oder Nr. 1 - 11
// Sensor Beute 1
#define             Beute1                                               0    // 0 oder Nr. 1 - 11
// Sensor Beute 2
#define             Beute2                                               0    // 0 oder Nr. 1 - 11
// Sensor Beute 3 nur Temperatur
#define             Beute3                                               0    // 0 oder Nr. 1 - 11
// Sensor Beute 4 nur Temperatur
#define             Beute4                                               0    // 0 oder Nr. 1 - 11


//----------------------------------------------------------------
// Temperatur Wägezelle (Duo, Tripple, Quad usw.
// Sensor, der die Temperatur der Wägezelle erfasst;  vorbelegt der erste DS18B20 
// für Systeme mit einer Waage identisch zum Sensor Aussenwerte eintragen
#define             Temp_Zelle                                           0    // Nr. 0 - 11


//----------------------------------------------------------------
// weitere Sensoren
//----------------------------------------------------------------
#define Anzahl_Sensoren_Licht   0    // Mögliche Werte: '0','1' 

#define Anzahl_Sensor_Luftdruck   0  // Mögliche Werte: '0','1'  (Luftdruck von BME280)
#define Hoehe_Standort          0.0 // Höhe des Standorts über NN in Meter für Korrektur des Messwertes Luftdruck


//----------------------------------------------------------------
// Konfiguration Waage(n)
//----------------------------------------------------------------
#define Anzahl_Sensoren_Gewicht 1 // Mögliche Werte: '0','1','2','3','4'

// Anschluss / Konfiguration Wägezellen-------------------------------------------------------
// mit Anzahl_Sensoren_Gewicht 1
//   HX711(1) Kanal A = Wägeelement(e) Waage1;   Serverskript: beeloggerY
// mit Anzahl_Sensoren_Gewicht 2 zusätzlich:
//   HX711(1) Kanal B = Wägeelement(e) Waage2;   Serverskript: DuoY

// mit Anzahl_Sensoren_Gewicht 3 zusätzlich:
//   HX711(2) Kanal A = Wägeelement(e) Waage3;   Serverskript: TripleY
// mit Anzahl_Sensoren_Gewicht 4 zusätzlich:
//   HX711(2) Kanal B = Wägeelement(e) Waage4;   Serverskript: QuadY

//--------------------------------------------------------------------------------------------
// Kalibrierwerte für die Wägezellen
const long Taragewicht[4] = {10 , 10 , 10 , 10}; // Hier ist der Wert aus der Kalibrierung einzutragen
const float Skalierung[4] = { 1.0 , 1.0 , 1.0 , 1.0}; // Hier ist der Wert aus der Kalibrierung einzutragen
 


//--------------------------------------------------------------------------------------------
// Kalibrierwerte für die Spannungsmessung Akku
const long Kalib_Spannung =   100;    // Hier muss der Wert aus der Kalibrierung eingetragen werden, sonst funktioniert der Programmcode nicht
const long Kalib_Bitwert  =  1000;    // Hier muss der Wert aus der Kalibrierung eingetragen werden, sonst funktioniert der Programmcode nicht
//----------------------------------------------------------------


//----------------------------------------------------------------
// LORA Konfiguration
//----------------------------------------------------------------
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// Verbindungsparameter:
// Beispiel für LORA-http-Intergration Setting:
// https://community.beelogger.de/UserName/beelogger1/beelogger_log.php?Passwort=deinpasswort&LORA=1

// LoRaWAN NwkSKey, network session key
// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM NWKSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const u1_t PROGMEM APPSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


// LoRaWAN end-device address (DevAddr)
//static const u4_t DEVADDR =   0x12345678;  //   <-- Change this address for every node!  
static const u4_t DEVADDR = 0x12345678;

// LoRaWAN Network Prefix
static const uint16_t PREFIX =   0x13;     //   https://www.thethingsnetwork.org/docs/lorawan/prefix-assignments.html  
// example: 0x13 = TTN
//          0x01 = your own local Lora accesspoint

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in arduino-lmic/project_config/lmic_project_config.h,
// otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

// turn LMIC-Lib Debug on/off.  "On" with this line commented out
#define NDEBUG 1


//----------------------------------------------------------------
// Allgemeine Konfiguration
//----------------------------------------------------------------
#define WeckIntervall_default 10        //  Weckintervall

#define WeckIntervall_aktiv  1          // Manuelle Intervallsteuerung 0 = inaktiv, 1 = aktiv

#define WeckIntervall_Tag 10
#define WeckIntervall_Nacht 30
#define WeckIntervall_Nacht_Anfang 23
#define WeckIntervall_Nacht_Ende 6

#define WeckIntervall_Winter_aktiv  0   // Intervall Winter 0 = inaktiv, 1 = aktiv
#define WeckIntervall_Winter       60   // Intervall in Minuten
#define WeckIntervall_Winter_Anfang 9   // Monat Winterintervall Start
#define WeckIntervall_Winter_Ende   4   // Monat Winterintervall Ende
// 
#define AlternativIntervallMinuten  120  // Weckinterval, wenn VAlternativ erreicht


// Li-Ion Akku beelogger-Solar
const float VAlternativ = 3.8;       // Minimale Spannung ab der automatisch das alternative Intervall aktiviert wird
const float VMinimum    = 3.75;      // Minimale Spannung ab der ab der keine Messungen und auch kein Versand von Daten erfolgt

// 6V PB-Akku beelogger-Universal
//const float VAlternativ = 5.9;       // Minimale Spannung ab der automatisch das alternative Intervall aktiviert wird
//const float VMinimum    = 5.7;       // Minimale Spannung ab der ab der keine Messungen und auch kein Versand von Daten erfolgt

// 12V PB-Akku beelogger-Universal
//const float VAlternativ = 11.9;     // Minimale Spannung ab der automatisch das alternative Intervall aktiviert wird
//const float VMinimum = 11.5;        // Minimale Spannung ab der keine Messungen und auch kein Versand von Daten erfolgt
//----------------------------------------------------------------
#endif
