//----------------------------------------------------------------
// Funktion LORA - Daten Senden
//----------------------------------------------------------------

#include "my_Multi_LORA_config.h"

//05.05.2020 User Service
//29.12.2020 Lora sequence number, LoRa Prefix
//12.02.2021 Lora sequence number in EE-Prom

#include <EEPROM.h>

//----------------------------------------------------------------
// Konfiguration RFM95 / SX1276 LORA
//----------------------------------------------------------------
#define SERVER_TM_OUT 8000 // 10000 = 10 sec TX-Timeout

//Folgende Anpassungen an „arduino-lmic\src\lmic\config.h“ notwendig:
//#define LMIC_DEBUG_LEVEL 0
//#define DISABLE_JOIN
//#define DISABLE_PING
//#define DISABLE_BEACONS
//#define DISABLE_MCMD_PING_SET // set ping freq, automatically disabled by DISABLE_PING
//#define DISABLE_MCMD_BCNI_ANS // next beacon start, automatical disabled by DISABLE_BEACON

// LORA PIN-Belegung
#define LMIC_RXTX LMIC_UNUSED_PIN
#define LMIC_RST LMIC_UNUSED_PIN
#define LMIC_DIO0 8
#define LMIC_DIO1 9
#define LMIC_DIO2 LMIC_UNUSED_PIN
#define LMIC_NSS 10
#define LMIC_MOSI 11
#define LMIC_MISO 12
#define LMIC_SCK 13

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = LMIC_NSS,
    .rxtx = LMIC_RXTX,
    .rst = LMIC_RST,
    .dio = {LMIC_DIO0, LMIC_DIO1, LMIC_DIO2},
};

// LORA Datenhandling
boolean flag_TXCOMPLETE = false;
uint8_t payload[64];

// Payload definitonen
#define TYPE_TEMP 0x01     //2 bytes | Temp -327.68°C -->327.67°C
#define TYPE_RH 0x02       //1 byte  | Humidity 0-100%
#define TYPE_VCC 0x03      //2 bytes | VCC 0-65535mV
#define TYPE_SOLAR 0x04    //2 bytes | VCC 0-65535mV
#define TYPE_LIGHT 0x05    //2 bytes | Light 0-->65535 Lux
#define TYPE_WEIGHT_1 0x06 //3 bytes | 0->0xFFFFFF (0-->16777215g)
#define TYPE_EXT_TEMP 0x11 //2 bytes | Temp -327.68°C -->327.67°C
#define TYPE_EXT_RH 0x12   //1 byte  | Humidity   0-100%
#define TYPE_SERVICE 0x13  //1 byte  | Service   An-Zeit
#define TYPE_AUX1 0x14     //2 bytes | Luftdruck in hPA * 10.0 ( 9900 - 11000)
#define TYPE_AUX2 0x15     //2 bytes |
#define TYPE_AUX3 0x16     //4 bytes | Sketch ID  4 Byte

#define TYPE_TEMP_2 0x21   //2 bytes | Temp -327.68°C -->327.67°C
#define TYPE_RH_2 0x22     //1 byte  | Humidity 0-100%
#define TYPE_WEIGHT_2 0x23 //3 bytes | 0->0xFFFFFF (0-->16777215g)
#define TYPE_WEIGHT_3 0x24 //3 bytes | 0->0xFFFFFF (0-->16777215g)
#define TYPE_WEIGHT_4 0x25 //3 bytes | 0->0xFFFFFF (0-->16777215g)
#define TYPE_TEMP_3 0x26   //2 bytes | Temp -327.68°C -->327.67°C
#define TYPE_TEMP_4 0x27   //2 bytes | Temp -327.68°C -->327.67°C

//----------------------------------------------------------------

void peripherial_off()
{
  digitalWrite(LMIC_NSS, LOW);
  pinMode(LMIC_MOSI, INPUT); //  SD MOSI
};

void peripherial_on()
{
  digitalWrite(LMIC_NSS, HIGH);
  //pinMode(LMIC_MOSI, OUTPUT);      //  SD MOSI
};

#define eeAddress 0x20       // EE-Prom Adress wear leveling
#define eeAd_data 0x30       // EE-Prom Adress LoRa sequence number
uint8_t data_base;           // EE-Prom data adress
uint8_t data_cnt;            // EE-Prom wear Counter
uint16_t LoRaSeqCounter = 0; // TTN Sequence numbber

void inc_LoRaSeq()
{ // increment Lora sequence number
  LoRaSeqCounter += 1;
  if (LoRaSeqCounter > 16382)
    LoRaSeqCounter = 0;
  if ((LoRaSeqCounter % 128) == 0)
  {                // check wearlevel data every 128 writecycle
    data_cnt += 1; // inc write counter
    if (data_cnt > 0xEE)
    {                 // if a cell has been written 128 * 238 (0xEE) times (11300)
      data_cnt = 0;   // reset write counter
      data_base += 1; // use next element in eeprom
    }
    if (data_base > 16)
      data_base = 0; // 16 elements only
    EEPROM.put((eeAddress), data_base);
    EEPROM.put((eeAddress + 1), data_cnt);
  }
  EEPROM.put(eeAd_data + (data_base * 2), LoRaSeqCounter); // *2 =  two bytes
  debugprintF("Cnt:");                                     //LoRa Counter: ");
  debugprint(LoRaSeqCounter);
  debugprintF(" bs:"); // Base
  debugprint(data_base);
  debugprintF(" wr:"); //writes
  debugprintln(data_cnt);
  debugflush();
}

void init_LoRaSeq()
{ // init Lora sequence number
  EEPROM.get(eeAddress, data_base);
  if (data_base > 16)
    data_base = 0; // Check needed for very first start
  EEPROM.get((eeAddress + 1), data_cnt);
  EEPROM.get((eeAd_data + (data_base * 2)), LoRaSeqCounter); // read,  *2 =  two bytes
  inc_LoRaSeq();                                             // call to check limits
};

//----------------------------------------------------------------
// Setup SX1276 - LORA
//----------------------------------------------------------------
void setup_LORA()
{
  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.

  // These values are stored in flash and only copied to RAM once.
  // Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession(PREFIX, DEVADDR, nwkskey, appskey);

  //#if defined(CFG_eu868)
  // Set up the channels used by the Things Network, which corresponds
  // to the defaults of most gateways. Without this, only three base
  // channels from the LoRaWAN specification are used, which certainly
  // works, so it is good for debugging, but can overload those
  // frequencies, so be sure to configure the full frequency range of
  // your network here (unless your network autoconfigures them).
  // Setting up channels should happen after LMIC_setSession, as that
  // configures the minimal channel set.
  // NA-US channels 0-71 are configured automatically
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // g-band

  /*
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
  */
  // TTN defines an additional channel at 869.525Mhz using SF9 for class B
  // devices' ping slots. LMIC does not have an easy way to define set this
  // frequency and support for class B is spotty and untested, so this
  // frequency is not configured here.

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7, 14); //Low-Power
  //  LMIC_setDrTxpow(DR_SF8, 14);   // Med-Power
  //  LMIC_setDrTxpow(DR_SF9, 14); // High-Power
}
//----------------------------------------------------------------

//----------------------------------------------------------------
// Prepare Payload
//----------------------------------------------------------------
byte prep_payload()
{

  byte cnt_pld = 0;
  int16_t Value_send;

  debugprintlnF("Data");
  debugflush();

#if 1
  if (SensorTemp[Beute1] != No_Val)
  {
    Value_send = (int16_t)(SensorTemp[Beute1] * 100.0); // in 100stel grad
    payload[cnt_pld] = TYPE_TEMP;
    cnt_pld++;
    payload[cnt_pld] = highByte(Value_send);
    cnt_pld++;
    payload[cnt_pld] = lowByte(Value_send);
    cnt_pld++;
    debugprintF("T-1:");
    debugprintln(Value_send);
  }
  if (SensorFeuchte[Beute1] != No_Value)
  {
    Value_send = (int16_t)SensorFeuchte[Beute1]; // 0 - 100
    payload[cnt_pld] = TYPE_RH;
    cnt_pld++;
    payload[cnt_pld] = lowByte(Value_send);
    cnt_pld++;
    debugprintF("F-1:");
    debugprintln(Value_send);
  }
#endif
  float AussenTemp = SensorTemp[Aussenwerte];
  if (AussenTemp == No_Val)
    AussenTemp = DS_Temp;
  if (AussenTemp != No_Val)
  {
    Value_send = (int16_t)(AussenTemp * 100.0); // in 100stel grad
    payload[cnt_pld] = TYPE_EXT_TEMP;
    cnt_pld++;
    payload[cnt_pld] = highByte(Value_send);
    cnt_pld++;
    payload[cnt_pld] = lowByte(Value_send);
    cnt_pld++;
    debugprintF("T-O:");
    debugprintln(Value_send);
  }

  if (SensorFeuchte[Aussenwerte] != No_Value)
  {
    Value_send = (int16_t)SensorFeuchte[Aussenwerte]; // 0 - 100
    payload[cnt_pld] = TYPE_EXT_RH;
    cnt_pld++;
    payload[cnt_pld] = lowByte(Value_send);
    cnt_pld++;
    debugprintF("F-O:");
    debugprintln(Value_send);
  }

  Value_send = (int16_t)(Batteriespannung * 1000.0); // in mV
  payload[cnt_pld] = TYPE_VCC;
  cnt_pld++;
  payload[cnt_pld] = highByte(Value_send);
  cnt_pld++;
  payload[cnt_pld] = lowByte(Value_send);
  cnt_pld++;
  debugprintF("V-B:");
  debugprintln(Value_send);

  Value_send = (int16_t)(Solarspannung * 1000.0); // in mV
  payload[cnt_pld] = TYPE_SOLAR;
  cnt_pld++;
  payload[cnt_pld] = highByte(Value_send);
  cnt_pld++;
  payload[cnt_pld] = lowByte(Value_send);
  cnt_pld++;
  debugprintF("V-S:");
  debugprintln(Value_send);

  if (Licht != No_Value)
  {
    if (Licht > 65500.0)
    {
      Licht = 65500.0; // unsigned integer max 65535
    }
    Value_send = (uint16_t)(Licht); // 0 ... 65500
    payload[cnt_pld] = TYPE_LIGHT;
    cnt_pld++;
    payload[cnt_pld] = highByte(Value_send);
    cnt_pld++;
    payload[cnt_pld] = lowByte(Value_send);
    cnt_pld++;
    debugprintF("L:");
    debugprintln(Value_send);
  }

  Value_send = (int16_t)(Gewicht[0] * 100.0); // in 100stel kilogramm
  payload[cnt_pld] = TYPE_WEIGHT_1;
  cnt_pld++;
  payload[cnt_pld] = highByte(Value_send);
  cnt_pld++;
  payload[cnt_pld] = lowByte(Value_send);
  cnt_pld++;
  debugprintF("G-1:");
  debugprintln(Value_send);

#if (Anzahl_Sensoren_Gewicht > 1)             // zweite Waage
  Value_send = (int16_t)(Gewicht[1] * 100.0); // in 100stel kilogramm
  payload[cnt_pld] = TYPE_WEIGHT_2;
  cnt_pld++;
  payload[cnt_pld] = highByte(Value_send);
  cnt_pld++;
  payload[cnt_pld] = lowByte(Value_send);
  cnt_pld++;
  debugprintF("G-2:");
  debugprintln(Value_send);

#if 1
  if (SensorTemp[Beute2] != No_Val)
  {
    Value_send = (int16_t)(SensorTemp[Beute2] * 100.0); // in 100stel grad
    payload[cnt_pld] = TYPE_TEMP_2;
    cnt_pld++;
    payload[cnt_pld] = highByte(Value_send);
    cnt_pld++;
    payload[cnt_pld] = lowByte(Value_send);
    cnt_pld++;
    debugprintF("T-2:");
    debugprintln(Value_send);
  }
  if (SensorFeuchte[Beute2] != No_Value)
  {
    Value_send = (int16_t)SensorFeuchte[Beute2]; // 0 - 100
    payload[cnt_pld] = TYPE_RH_2;
    cnt_pld++;
    payload[cnt_pld] = lowByte(Value_send);
    cnt_pld++;
    debugprintF("F-2:");
    debugprintln(Value_send);
  }
#endif
#endif

#if (Anzahl_Sensoren_Gewicht > 2)             // dritte Waage
  Value_send = (int16_t)(Gewicht[2] * 100.0); // in 100stel kilogramm
  payload[cnt_pld] = TYPE_WEIGHT_3;
  cnt_pld++;
  payload[cnt_pld] = highByte(Value_send);
  cnt_pld++;
  payload[cnt_pld] = lowByte(Value_send);
  cnt_pld++;
  debugprintF("G-3:");
  debugprintln(Value_send);
  if (SensorTemp[Beute3] != No_Val)
  {
    Value_send = (int16_t)(SensorTemp[Beute3] * 100.0); // in 100stel grad
    payload[cnt_pld] = TYPE_TEMP_3;
    cnt_pld++;
    payload[cnt_pld] = highByte(Value_send);
    cnt_pld++;
    payload[cnt_pld] = lowByte(Value_send);
    cnt_pld++;
    debugprintF("T-3:");
    debugprintln(Value_send);
  }
#endif

#if (Anzahl_Sensoren_Gewicht == 4)            // vierte Waage
  Value_send = (int16_t)(Gewicht[3] * 100.0); // in 100stel kilogramm
  payload[cnt_pld] = TYPE_WEIGHT_4;
  cnt_pld++;
  payload[cnt_pld] = highByte(Value_send);
  cnt_pld++;
  payload[cnt_pld] = lowByte(Value_send);
  cnt_pld++;
  debugprintF("G-4:");
  debugprintln(Value_send);
  if (SensorTemp[Beute4] != No_Val)
  {
    Value_send = (int16_t)(SensorTemp[Beute4] * 100.0); // in 100stel grad
    payload[cnt_pld] = TYPE_TEMP_4;
    cnt_pld++;
    payload[cnt_pld] = highByte(Value_send);
    cnt_pld++;
    payload[cnt_pld] = lowByte(Value_send);
    cnt_pld++;
    debugprintF("T-4:");
    debugprintln(Value_send);
  }
#endif

  if (Anzahl_Sensor_Luftdruck == 1)
  {
    Value_send = (uint16_t)(Aux[1] * 10.0); // hPa*10, 9000 - 11000
    payload[cnt_pld] = TYPE_AUX1;
    cnt_pld++;
    payload[cnt_pld] = highByte(Value_send);
    cnt_pld++;
    payload[cnt_pld] = lowByte(Value_send);
    cnt_pld++;
    debugprintF("P:");
    debugprintln(Value_send);
  }

  int8_t svc = (int8_t)Service;
  if (Service > 255.0)
    svc = 0xFF; // max value in payload
  payload[cnt_pld] = TYPE_SERVICE;
  cnt_pld++;
  payload[cnt_pld] = svc & 0xFF;
  cnt_pld++;
  debugprintF("S:");
  debugprintln(svc);

  if (LoRaSeqCounter % 10 == 0)
  { // ID not in every transmission
    payload[cnt_pld] = TYPE_AUX3;
    cnt_pld++;
    payload[cnt_pld] = (l_ID >> 24) & 0xFF;
    cnt_pld++;
    payload[cnt_pld] = (l_ID >> 16) & 0xFF;
    cnt_pld++;
    payload[cnt_pld] = (l_ID >> 8) & 0xFF;
    cnt_pld++;
    payload[cnt_pld] = lowByte(l_ID);
    cnt_pld++;
  }
  payload[cnt_pld] = 0;
  debugprintln(" ");
  debugflush();

  return (cnt_pld);
}
//----------------------------------------------------------------

//----------------------------------------------------------------
// Eventhandler  LORA
//----------------------------------------------------------------
void onEvent(ev_t ev)
{

  debugprint(os_getTime());
  debugprint(": ");

  switch (ev)
  {
  case EV_TXCOMPLETE:

    debugprintlnF("TX!");

    /*
        if (LMIC.txrxFlags & TXRX_ACK)

        debugprintlnF("Received ack");

           if (LMIC.dataLen) {

              debugprintlnF("Received ");
              debugprintln(LMIC.dataLen);
              debugprintlnF(" bytes of payload");

            }
      */
    inc_LoRaSeq();
    flag_TXCOMPLETE = true;
    break;
  case EV_RXCOMPLETE:
    // data received in ping slot
    debugprintlnF("RX!");

    break;
  default:

    debugprintlnF("?");

    break;
  }
  debugflush();
}
//----------------------------------------------------------------
//----------------------------------------------------------------
// Daten_Senden SX1276 - LORA
//----------------------------------------------------------------
void Daten_Senden()
{

  debugprintlnF("\nLoRa:");
  debugflush();
  setup_LORA(); // Setup RFM Modul
  debugprintlnF("!");
  debugflush();

  byte cnt = prep_payload(); // prepare Payload

  if (cnt < sizeof(payload))
  {
    // Prepare upstream data transmission at the next possible time.

    debugprintF("len:");
    debugprintln(cnt);
    debugflush();
    LMIC.seqnoUp = LoRaSeqCounter;
    LMIC_setTxData2(1, payload, cnt, 0);

    //Run LMIC loop until he as finish or timeout
    long send_loop = millis();
    while ((flag_TXCOMPLETE == false) && ((millis() - send_loop) < SERVER_TM_OUT))
    {
      os_runloop_once();
    }
    if (flag_TXCOMPLETE == false)
    {
      debugprintlnF(".tout");
    }
  }
  else
  {
    debugprintlnF("err");
  }

  flag_TXCOMPLETE = false;
  LMIC_reset();
  LMIC_shutdown();
  SPI.end();
}
//----------------------------------------------------------------
