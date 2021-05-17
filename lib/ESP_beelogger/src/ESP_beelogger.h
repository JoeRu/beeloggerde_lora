/*************************************************************************
  ESP-01 HTTP Library

  Distributed under GPL v2.0
  For more information, please visit http://arduinodev.com


    Version 1.0 08.08.2018
    Version 1.2 10.01.2021			
            modified signal   

*************************************************************************/

#include <Arduino.h>

#ifndef __ESP_beelogger__
#define __ESP_beelogger__

//								   012345678901234567890
const char ESP_ATE0[] PROGMEM   = "ATE0";	 			// Echo off
const char ESP_RST[] PROGMEM    = "AT+RST"; 			// Restart ESP
const char ESP_MUX[] PROGMEM    = "AT+CIPMUX=0";		// 0 = only one Connection
const char ESP_WJAP[] PROGMEM   = "AT+CWJAP_CUR=\"";	// attach to Access Point
const char ESP_WQAP[] PROGMEM   = "AT+CWQAP";			// Quit from access point
const char ESP_WLAP[] PROGMEM   = "AT+CWLAP";		    // List available AP
const char ESP_WLAPOPT[] PROGMEM= "AT+CWLAPOPT=1,6";	// Parameter list AP: AP and signalstrength only
const char ESP_STATUS[] PROGMEM = "AT+CIPSTATUS";		// IP - Status
const char ESP_START[] PROGMEM  = "AT+CIPSTART=\"TCP\",\"";	// Start Connection
const char ESP_CLOSE[] PROGMEM  = "AT+CIPCLOSE";		// Close Connection
const char ESP_SEND[] PROGMEM   = "AT+CIPSEND=";		// Send data using send with length
const char ESP_SENDEX[] PROGMEM = "AT+CIPSENDEX=";		// Send Data using SENDEX (start with \\0)
const char ESP_GMR[] PROGMEM    = "AT+GMR";				// Request firmware Version

const char ESP_MODE[] PROGMEM   = "AT+CWMODE_DEF=1";	// 1 = Station Mode only, set permanent
const char ESP_SLP[] PROGMEM    = "AT+SLEEP="; 			// Sleep, 0 = off, 1 = light sleep, 2 = modem sleep


typedef enum {
  ESP_STOP = 0,
  ESP_INIT,
  ESP_WLAN_READY,
  ESP_CONNECTED,
  ESP_ERROR,
} ESP_STATES;


class C_WLAN_ESP01 {
  public:

    C_WLAN_ESP01();
	
    // initialize the module
    bool init(int baudrate);
    // end/stop Serial
    void end();

    // join wlan
    bool join(const char* access_point, const char* ap_passwort, unsigned long timeout = 10000);
    // terminate network and restart esp
    bool quit();

    // connect to server
    bool Connect(const char* host_name);
    // terminate connection
    void disConnect();

    // send AT command and check for expected response
    byte sendCommand(char* cmd, unsigned long timeout = 2000, const char* expected = 0);

    // prepare Sending data
    bool prep_send(int length);  // using CIPSENDEX
    void send(const char* data); // using with CIPSENDEX
	
	// aktivate sleep Mode
	void sleep(byte sleep_mode);
	
	
    //************************************************************************
    // following functions only for initialising and testing the hardware	
    //version information
    void firmware(unsigned long timeout = 2000);
	
    //signal strength information
    uint8_t signal(const char* ap = 0,unsigned long timeout = 10000);
	
	// set Station Mode
    bool mode();
	
    // show serial TX data from ESP, for debug only, need Serial.begin somewhere in a sketch
    void check(unsigned long timeout = 2000);

    char buffer[128];
    byte espState;

    // Send data string using CIPSEND
    bool write_d(const char * data, int length);
	

  private:
    void empty_rcv(); // empty RX buffer
	

};  // ; needed
#endif

