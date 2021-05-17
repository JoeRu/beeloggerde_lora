#ifndef __beelogger_config__
#define __beelogger_config__
//22.01.2020
//05.05.2020

//----------------------------------------------------------------
// Konfiguration DEBUG-Information
//               Serial Monitor via FTDI / USB
//----------------------------------------------------------------
#define myDEBUG 1 // Debug via Serial Monitor - Hier für Finale auf 0 setzen - für serial debugging auf 1.

//----------------------------------------------------------------
// Konfig der seriellen Schnittstelle
//----------------------------------------------------------------
#define Serial_Baudrate 9600
//----------------------------------------------------------------

//----------------------------------------------------------------
// EE-Prom-Sketche:Änderung Gewicht in Kilogramm bei der Daten versenden erfolgt
//----------------------------------------------------------------
const float Alarm_Gewicht = 1.0;
//----------------------------------------------------------------

//################################################################
// I2C Adressen
//################################################################

//----------------------------------------------------------------
// Konfiguration SHT31 - Temperatur und Luftfeuchte
//----------------------------------------------------------------
uint8_t SHT31_adresse[2] = {0x44, 0x45};
//----------------------------------------------------------------

//----------------------------------------------------------------
// Konfiguration BME280 - Temperatur und Luftfeuchte
//----------------------------------------------------------------
#define BME280_1_adresse (0x76)
#define BME280_2_adresse (0x77)
//----------------------------------------------------------------

//----------------------------------------------------------------
// Konfiguration Si7021 - Temperatur und Luftfeuchte
//----------------------------------------------------------------
// Si7021 I2C-Adresse : 0x40 (64d), Wert ist in Adafruit_Si7021.h
//----------------------------------------------------------------

//################################################################
// AT-Mega Pins
//################################################################

//----------------------------------------------------------------
// Konfiguration One-Wire-Bus für DS18B20
//----------------------------------------------------------------
#define ONE_WIRE_BUS 3
//----------------------------------------------------------------

//----------------------------------------------------------------
// Power on/off
//----------------------------------------------------------------
#define Power_Pin 4
//----------------------------------------------------------------

//----------------------------------------------------------------
// Konfiguration DHT21 / DHT22 - Temperatur und Luftfeuchte
// DHT Nr. 2 an D 6 nur bei Single/Duo-Systemen
//----------------------------------------------------------------
byte DHT_Sensor_Pin[2] = {5, 5}; // nur ein DHT
//----------------------------------------------------------------

//----------------------------------------------------------------
// Konfiguration HX711 SCK /Data
//----------------------------------------------------------------
/*byte HX711_SCK[4] = {A0, A0, 6, 6}; // HX711 Nr.1: A,B;  HX711 Nr.2: A,B   S-Clock
byte HX711_DT[4] =  {A1, A1, 7, 7}; // HX711 Nr.1: A,B;  HX711 Nr.2: A,B   Data
*/
byte HX711_SCK[4] = {A0, A0, 6, 6}; // HX711 Nr.1: A,B;  HX711 Nr.2: A,B  HX711 Nr.3: A,B S-Clock
byte HX711_DT[4] = {A1, A1, 7, 7};  // HX711 Nr.1: A,B;  HX711 Nr.2: A,B  HX711 Nr.3: A,B Data
//----------------------------------------------------------------
//----------------------------------------------------------------

//----------------------------------------------------------------
// Konfiguration ESP / SIM800
//----------------------------------------------------------------
// Pin Belegung ESP8266
#define ESP_RESET A2
byte ESP_RX = 8;
byte ESP_TX = 9;

// Pin  Belegung SIM 800
#define GSM_Power_Pin A2
byte GSM_TX = 9;
byte GSM_RX = 8; // beelogger_Universal, solar Multishield 2.5, neue Konfiguration
//byte GSM_RX = 10; // beelogger Solar, alte Konfiguration

//----------------------------------------------------------------

//----------------------------------------------------------------
// Seriellen Schnittstelle
//----------------------------------------------------------------
#define USB_RX 0 // Pin 0 RX-USB
#define USB_TX 1 // Pin 1 TX-USB
//----------------------------------------------------------------

//----------------------------------------------------------------
// Interrupt
//----------------------------------------------------------------
#define DS3231_Interrupt_Pin 2
//----------------------------------------------------------------

//----------------------------------------------------------------
// I2C
//----------------------------------------------------------------
#define SDA A4 // I2C Daten
#define SCL A5 // I2C Clock
//----------------------------------------------------------------

//----------------------------------------------------------------
// Pins Spannungen messen
//----------------------------------------------------------------
#define Batterie_messen A6
#define Solarzelle_messen A7
//----------------------------------------------------------------

//################################################################
// Global Libraries
//################################################################
#include <Sodaq_DS3231.h>
#include <LowPower.h>
//----------------------------------------------------------------

//################################################################
// Debug Print Functions
//################################################################

#if myDEBUG == 1
#define debugbegin(x) Serial.begin(x)
#define debugprintF(x) Serial.print(F(x))
#define debugprintlnF(x) Serial.println(F(x))
#define debugprint(x) Serial.print(x)
#define debugprintH(x) Serial.print(x, HEX)
#define debugprintln(x) Serial.println(x)
#define debugflush() Serial.flush()
#define debugend() Serial.end()
#else
#define debugbegin(x)
#define debugprintF(x)
#define debugprintlnF(x)
#define debugprint(x)
#define debugprintH(x)
#define debugprintln(x)
#define debugflush()
#define debugend()
#endif

void Serial_rxtx_off()
{
  delay(20);
  digitalWrite(USB_RX, LOW); // Port aus
  pinMode(USB_TX, INPUT);
  digitalWrite(USB_TX, LOW); // Port aus
}
//----------------------------------------------------------------
#endif // beelogger_config.h