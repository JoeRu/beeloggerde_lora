/*************************************************************************
* SIM7600E TCPIP Library
* based on SIM800 GPRS/HTTP Library
* Distributed under GPL v2.0
* Written by Stanley Huang <stanleyhuangyc@gmail.com>
* For more information, please visit http://arduinodev.com
*
* Modified for use with SIM7600E
*
*
*************************************************************************/

#include <Arduino.h>

#ifndef __SIM7600_HTTP__
#define __SIM7600_HTTP__


// define DEBUG to one serial UART to enable debug information output
//#define DEBUG Serial

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

const char SIM7600_UP[] PROGMEM  	 = "PB DONE";					// SIM7600 up and ready
const char SIM7600_HTTP_P[] PROGMEM  = "AT+HTTPPARA=\"URL\",\"";	// HTTPPARA call

class CGPRS_SIM76_HTTP {
public:

    // initialize the module
    bool init(unsigned int timeout);
    // close software Serial
    void shutdown();

    // APN connection
	byte start(const char* apn);  //prepare PDP context

    // http commandos
	bool http_init();  //start HTTP service, activate PDP context
	bool http_getcall(const char* para);	// set the URL which will be accessed
	void http_para(const char* para);		// send parameter to module , used for AT+HTTPPARA
	bool http_end_para();	             	// end Data to http_call AT+HTTPPARA
	bool http_get(unsigned long timeout = 20000);    				 	// make Get, transfer data
	bool http_read(const char* data);    // read the response information of HTTP server
	bool http_end();					 // stop HTTP Service

    // get signal quality level (in dB)
    int getSignalQuality();
    bool getLocation(GSM_LOCATION* loc);
	
    // send AT command and check for expected response
    byte sendCommand(const char* cmd, unsigned int timeout = 2000, const char* expected = 0);
    
	char buffer[128];
private:
    // empty serial in buffer
    void purgeSerial();
};
#endif

