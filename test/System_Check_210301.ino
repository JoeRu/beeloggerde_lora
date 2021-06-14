/*
   (C) 2020 R.Schick - beelogger.de

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

// beelogger.de - Arduino Datenlogger für Imker
// Erläuterungen dieses Programmcodes unter http://beelogger.de
// Version 2021


#define System_with_DS3231  1    // 0 = ohne,  1 = mit DS3231  

#define Power_off_at_End    0    // 0 = aus,   1= Power an  

#define Test_SD_Card        0    // 0 = ohne,  1= test


#define One_Wire_Bus 3  // OneWire / DS18B20 an Pin 3

#define DHT_1     5     // DHT 22 Nummer 1 an Pin 5
#define DHT_2     6     // DHT 22 Nummer 2 an Pin 6

//--------------------------------------------------------------
// Ende Konfiguration
//--------------------------------------------------------------

#define Power_Pin 4



#include "Wire.h"
extern "C" {
#include "utility/twi.h"  // from Wire library, so we can do bus scanning
}
#include <OneWire.h>
#include <SPI.h>
#if Test_SD_Card
#include <SdFat.h>
#endif
#include <Sodaq_DS3231.h>
#include <EEPROM.h>        // Dateiname ablegen
#include <LowPower.h>
#include <dht.h>

#define HX711_SCK          A0    // HX711 S-Clock
#define HX711_DT           A1    // HX711 Data
#define GSM_Power_Pin      A2
#define SDA                A4      // I2C Daten
#define SCL                A5      // I2C Clock
#define Batterie_messen    A6 //
#define Solarzelle_messen  A7 //


#define x_RX      8
#define x_TX      9

#define SD_LED    7   // im Original

#define SD_CS    10   // SD Card Chip Select
#define SD_MOSI  11   // SD Card MOSI
#define SD_MISO  12   // SD Card MISO
#define SD_CLK   13   // SD Card Clock

#define DS3231_ADDRESS 0x68
#define DS3231_Stat_REG 0x0F

const float No_Licht = -1.0;
const float No_Val = 99.9;

byte LED_ON = 1;
float TempSys = No_Val;

char DatenString[70];
char File_Name[15] = "beelogger.csv";
DateTime aktuell;


OneWire  ds(One_Wire_Bus);  // Connect your 1-wire device to pin #


//--------------------------------------------------------------
void setup(void) {
  char File_Name[16] = {"        .   "};
  EEPROM.put(0, File_Name);

  Serial.begin(9600);
  while (!Serial) {};
  Serial.println(F("System Check   Version 01.03.2021\n\r"));
  Serial.println(" ");
#if defined(ARDUINO_AVR_NANO)
  Serial.println(" beelogger-Universal mit Arduino Nano  ");
#else
  Serial.println(" beelogger mit Arduino Pro-Mini  ");
#endif
  Serial.println(" ");
  Serial.println(F(" Suche One-Wire-Bus, I2C-Bus\n\r"));
  Serial.flush();

  digitalWrite(GSM_Power_Pin, LOW);  // GSM off
  pinMode(GSM_Power_Pin, OUTPUT);
  delay(5);

  digitalWrite(Power_Pin, HIGH);
  pinMode(Power_Pin, OUTPUT);
  delay(5);

  Wire.begin();
#if System_with_DS3231
  {
    Serial.println(F("System with DS3231:"));
    Serial.println(F("  setze DS3231 Osc.-Flag"));
    // Lese Status Register
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x0F);
    Wire.endTransmission();
    Wire.requestFrom(DS3231_ADDRESS, 1);
    uint8_t reg = Wire.read();

    Serial.print(F("RTC Status: "));
    Serial.println(reg, BIN);

    uint8_t value = reg & 0x7F;  // high bit = 0

    //Schreibe Osc.- Flag = 0
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write((byte)DS3231_Stat_REG);
    Wire.write((byte)value);
    Wire.endTransmission();

    // Lese Status Register
    Wire.beginTransmission (0x68);
    Wire.write(0x0F); //pointing at Control/Status Register that contains OSF flag bit at bit-7 position
    Wire.endTransmission();

    Wire.requestFrom(0x68, 1); //request to get the content of Control/Status Register
    reg = Wire.read();
    Wire.end();
    Serial.print(F("RTC Status: "));
    Serial.println(reg, BIN);
    Serial.println(" ");
    Serial.flush();

    rtc.begin();
    aktuell = rtc.now();
    TempSys = rtc.getTemperature();

    DateTime pc_tim = DateTime(__DATE__, __TIME__);
    long l_pczeit = pc_tim.get();
    aktuell = rtc.now();
    long l_zeit = aktuell.get();
    if (l_pczeit > l_zeit)  rtc.setDateTime(l_pczeit);

    rtc.clearINTStatus();

  }
#endif

  discoverOneWireDevices();

  discoverI2CDevices();

  Serial.println(F("\n\nInformation zu typischen I2C-Adressen:"));
  Serial.flush();
  Serial.println(F("BH1750  ->  0x23, 0x5C"));
  Serial.println(F("Si7021  ->  0x40"));
  Serial.println(F("SHT31   ->  0x44, 0x45"));
  Serial.println(F("AT24C32 ->  0x53, 0x57"));
  Serial.println(F("DS3231  ->  0x68"));
  Serial.println(F("BME280  ->  0x76, 0x77"));
  Serial.print(F("MCP23017  ->  0x20, 0x21, 0x22, 0x23"));
  Serial.println(" ");
  Serial.println(" ");
  Serial.flush();

  Sensor_DHT();


#if Test_SD_Card
  Daten_auf_SD_speichern();
#endif
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(F(" EE-Prom Test mit anderen Sketch ausführen."));
  Serial.println(" ");
  Serial.flush();
  TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));
  digitalWrite (A4, LOW);
  digitalWrite (A5, LOW);

  Serial.println(F("Sleep forever! "));
  Serial.flush();
  Serial.end();
}
//--------------------------------------------------------------


//#################################################
void loop(void) {
  // nothing to see here
  digitalWrite(Power_Pin, Power_off_at_End);
  delay(300);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
//#################################################



//----------------------------------------------------------------
// DS18B20 OneWire
//----------------------------------------------------------------
void discoverOneWireDevices(void) {
  byte i;

  byte addr[8];

  Serial.println(F("Looking for 1-Wire devices... (i.e. DS18B20)"));
  Serial.print(F("at Pin: D"));
  Serial.println(One_Wire_Bus);
  Serial.flush();
  while (ds.search(addr)) {
    Serial.print(F("\n\rFound \'1-Wire\' device with address:\n\r"));
    Serial.flush();
    for ( i = 0; i < 8; i++) {
      Serial.print("0x");
      if (addr[i] < 16) {
        Serial.print('0');
      }
      Serial.print(addr[i], HEX);
      if (i < 7) {
        Serial.print(", ");
      }
    }
    if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print(F("CRC is not valid!\n"));
      return;
    }
  }
  Serial.print(F("\n\r\n\rOne-Wire scan done.\r\n"));
  Serial.flush();
  ds.reset_search();
  return;
}
//--------------------------------------------------------------


//----------------------------------------------------------------
// TwoWire
//----------------------------------------------------------------
byte start_address = 8;       // lower addresses are reserved to prevent conflicts with other protocols
byte end_address = 119;       // higher addresses unlock other modes, like 10-bit addressing

void discoverI2CDevices()
{
  Serial.println(F("\nI2CScanner ready!"));

  Serial.print(F("starting scanning of I2C bus from "));
  Serial.print(start_address, DEC);
  Serial.print(" to ");
  Serial.print(end_address, DEC);
  Serial.println("...");
  Serial.flush();

  // start the scan, will call "scanFunc()" on result from each address
  scanI2CBus( start_address, end_address, scanFunc );

  Serial.println(F("\nI2C scan done"));
}


// Scan the I2C bus between addresses from_addr and to_addr.
// On each address, call the callback function with the address and result.
// If result==0, address was found, otherwise, address wasn't found
// (can use result to potentially get other status on the I2C bus, see twi.c)
// Assumes Wire.begin() has already been called
void scanI2CBus(byte from_addr, byte to_addr,
                void(*callback)(byte address, byte result) )
{
  byte rc;
  byte data = 0; // not used, just an address to feed to twi_writeTo()
  for ( byte addr = from_addr; addr <= to_addr; addr++ ) {
    rc = twi_writeTo(addr, &data, 0, 1, 0);
    callback( addr, rc );
  }
}

// Called when address is found in scanI2CBus()
// Feel free to change this as needed
// (like adding I2C comm code to figure out what kind of I2C device is there)
void scanFunc( byte addr, byte result ) {
  Serial.print("addr: 0x");
  Serial.print(addr, HEX);
  Serial.print( (result == 0) ? " found!" : "       ");
  Serial.print( (addr % 4) ? "\t" : "\n");
  Serial.flush();
}
//--------------------------------------------------------------


//----------------------------------------------------------------
// DHT OneWire
//----------------------------------------------------------------
void Sensor_DHT() {
  float Temperatur_DHT = No_Val;
  float Luftfeuchte_DHT = No_Val;
  int check;
  dht beeDHT;

  Serial.println(" ");
  Serial.println(F(" DHT 22  Nr.1: "));
  Serial.flush();
  check = beeDHT.read(DHT_1);
  LowPower.powerStandby(SLEEP_2S, ADC_OFF, BOD_OFF); //Wartezeit
  check = beeDHT.read(DHT_1);

  if (check == DHTLIB_OK) {
    Temperatur_DHT = beeDHT.temperature;
    Serial.print(F(" Temperatur [C]: "));
    Serial.println(Temperatur_DHT);
    Luftfeuchte_DHT = beeDHT.humidity;
    Serial.print(F(" Luftfeuchte [%RH]: "));
    Serial.println(Luftfeuchte_DHT);
  }
  else {
    Serial.println(F("  DHT nicht installiert oder fehlerhaft "));
  }
  Serial.flush();

  Serial.println(" ");
  Serial.println(F(" DHT 22  Nr.2: "));
  Serial.flush();
  check = beeDHT.read(DHT_2);
  LowPower.powerStandby(SLEEP_2S, ADC_OFF, BOD_OFF); //Wartezeit
  check = beeDHT.read(DHT_2);

  if (check == DHTLIB_OK) {
    Temperatur_DHT = beeDHT.temperature;
    Serial.print(F(" Temperatur [C]: "));
    Serial.println(Temperatur_DHT);
    Luftfeuchte_DHT = beeDHT.humidity;
    Serial.print(F(" Luftfeuchte [%RH]: "));
    Serial.println(Luftfeuchte_DHT);
  }
  else {
    Serial.println(F("  DHT nicht installiert oder fehlerhaft "));
  }
  Serial.flush();
}
//--------------------------------------------------------------


//----------------------------------------------------------------
#if Test_SD_Card
void Daten_auf_SD_speichern() {

  SdFat SD;
  File dataFile;
  DatenString_erstellen();
  Serial.println(DatenString);

  Serial.println(F("SD - Card need to be formated  FAT or FAT16 (not FAT32) "));
  Serial.println(" ");
  Serial.println(F("Connect to SD - Card: "));
  Serial.flush();

  if (SD.begin(SD_CS, SPI_HALF_SPEED)) {
    Serial.println("O.K.");
    Serial.flush();

    Serial.println(F("Open File: "));
    Serial.flush();
    dataFile = SD.open(File_Name, FILE_WRITE);
    if (dataFile) {
      Serial.println(F("Write File: "));
      Serial.flush();
      dataFile.println(DatenString);
      Serial.flush();
      dataFile.close();
      if (LED_ON == 1) {
        digitalWrite(SD_LED, HIGH);
        delay(150);
        digitalWrite(SD_LED, LOW);
        LED_ON = 0;
      }
      else {
        Serial.println(F("Fehler Write File!"));
      }
    }
    else {
      Serial.println(F("Fehler Open File!"));
    }
  }
  else {
    Serial.println(F("Fehler Connect to SD!"));
  }
  Serial.flush();
  if (LED_ON == 1) {
    digitalWrite(SD_LED, HIGH);
    delay(2000);
    digitalWrite(SD_LED, LOW);
  }
  delay(100);
  SPI.end();
  LED_ON = 0;
}
#endif
//--------------------------------------------------------------

//----------------------------------------------------------------
// Funktion SD-Kartenmodul - Datenstring erstellen
//----------------------------------------------------------------
void DatenString_erstellen() {

  aktuell = rtc.now();
  int count = 0;

  count = sprintf(DatenString, "%d/", aktuell.year());
  count += sprintf(DatenString + count, "%2.2d/", aktuell.month());
  count += sprintf(DatenString + count, "%2.2d ", aktuell.date());
  count += sprintf(DatenString + count, "%2.2d:", aktuell.hour());
  count += sprintf(DatenString + count, "%2.2d", aktuell.minute());

  count = Wert_hinzufuegen(count, TempSys, 2, 1);        // Testwert

  DatenString[count] = 0;
}
//----------------------------------------------------------------


//----------------------------------------------------------------
// Funktion SD-Kartenmodul - Wert hinzufügen
//----------------------------------------------------------------
int Wert_hinzufuegen(int count, double Wert, byte Nachkommastellen, float Fehler) {
  char Konvertierung[16];
  int count_neu = count;

  if (((Fehler == 1) and (Wert == No_Val)) or ((Fehler == 2) and (Wert == No_Licht)) ) {
    count_neu += sprintf(DatenString + count, ", %s", "");

  } else {
    dtostrf(Wert, 1, Nachkommastellen, Konvertierung);
    count_neu += sprintf(DatenString + count, ", %s", Konvertierung);
  }
  return count_neu;
}
//----------------------------------------------------------------
