/*************************************************************************
* SIM800 GPRS/HTTP Library
* Distributed under GPL v2.0
* Written by Stanley Huang <stanleyhuangyc@gmail.com>
* For more information, please visit http://arduinodev.com
*
* Modified by Thorsten Gurzan <info@beelogger.de> https://beelogger.de for beelogger-Solar GSM
* added:
*   function shutdown()
*   function GET_Action()
*
* modified:
*    move available from inline in "h" to "cpp"
*    init: add timeout parameter
*    setup: added apn_benuzter, apn_passwort
*    httpInit: add timeout parameter
*
* Version 24.03.2018
*	 added define for SIM-Module Serial TX/RX ports
*    function setup parameter changed to const char
*
* updated 01092018
*    move definition of SIM-Module Serial TX/RX ports to main sketch
*
* Updated 11032019 R.Schick
*   added CIP-functionality
*   init() modified
*
* Updated 18042019 R.Schick
*  new Lib-Name
*
* Version 2.3 19032021 R.Schick
*    RX buffer size, local buffers
*
*************************************************************************/

#include <Arduino.h>

#ifndef __SIM800__
#define __SIM800__


// define DEBUG to one serial UART to enable debug information output
//#define DEBUG Serial

typedef enum {
    HTTP_DISABLED = 0,
    HTTP_READY,
    HTTP_CONNECTING,
    HTTP_READING,
    HTTP_ERROR,
} HTTP_STATES;

typedef struct {
  float lat;
  float lon;
  uint8_t year; /* year past 2000, e.g. 15 for 2015 */
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} GSM_LOCATION;

const char GSM_BAUD[]  PROGMEM  = "AT+IPR=9600";		// Baudrate SIM800
const char GSM_TASK[]  PROGMEM  = "AT+CSTT=\"";		    // Start Task to APN
const char CIP_MUX[]   PROGMEM  = "AT+CIPMUX=0";		// 0 = only one Connection
const char CIP_START[] PROGMEM  = "AT+CIPSTART=\"TCP\",\"";	// Start Connection
const char CIP_CLOSE[] PROGMEM  = "AT+CIPCLOSE";		// Close Connection
const char CIP_SEND[]  PROGMEM  = "AT+CIPSEND";		    // Send data

class CGPRS_SIM800 {
public:

    CGPRS_SIM800():httpState(HTTP_DISABLED) {}
    // initialize the module
    uint8_t init(unsigned int timeout);
    // setup network
    uint8_t setup(const char* apn, const char* apn_benutzer, const char* apn_passwort);
    // get network operator name
    uint8_t getOperatorName();
    // check for incoming SMS
    uint8_t checkSMS();
    // get signal quality level (in dB)
    int getSignalQuality();
    // get Voltage at SIM800 (in mV)
    int getSIM800_Voltage();
    // get GSM base location and network time
    uint8_t getLocation(GSM_LOCATION* loc);
    // initialize HTTP connection
    uint8_t httpInit(unsigned int timeout);
    // terminate HTTP connection
    void httpUninit();
    // connect to HTTP server
    uint8_t httpConnect(const char* url, const char* args = 0);
    // check if HTTP connection is established
    // return 0 for in progress, 1 for success, 2 for error
    int8_t httpIsConnected();
    // read data from HTTP connection
    void httpRead();
    // check if HTTP connection is established
    // return 0 for in progress, -1 for error, bytes of http payload on success
    int httpIsRead();
    // send AT command and check for expected response
    uint8_t sendCommand(const char* cmd, unsigned int timeout = 2000, const char* expected = 0);
    // send AT command and check for two possible responses
    uint8_t sendCommand(const char* cmd, const char* expected1, const char* expected2, unsigned int timeout = 2000);

	// Start Stop Task to APN
	uint8_t start(const char* apn);
	void stop();
	
	// prepare and send data
	uint8_t prep_send();
    void send(const char* data);
	
	// Connect/disConnect TCP via CIP
	uint8_t Connect(const char* host_name);
	void disConnect();
	
    // check if there is available serial data
    uint8_t available();
    // close software Serial
    void shutdown();
    // send AT commands
    void sendAT(const char* cmd);
    // Get Action
    uint8_t GET_Action();
#define BUFFER_Size 64
    char buffer[BUFFER_Size];
    uint8_t httpState;
	
private:
    uint8_t checkbuffer(const char* expected1, const char* expected2 = 0, unsigned int timeout = 2000);
    void purgeSerial();
    uint8_t m_bytesRecv;
    uint32_t m_checkTimer;
};
#endif

