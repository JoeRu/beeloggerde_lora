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

#ifndef __SIM7600_SSL__
#define __SIM7600_SSL__


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


class CGPRS_SIM76_SSL {
public:

    // initialize the module
    bool init(unsigned int timeout);
    // close software Serial
    void shutdown();

    // APN connection
	uint8_t start(const char* apn,const char* usr,const char* pw);  //prepare PDP context

    // http commandos
	uint8_t ssl_init();					// prepare SSL transparent mode

	uint8_t ssl_start();                   // start SSL, activate PDP context	
	uint8_t ssl_stop();					// stop SSL

	uint8_t ssl_open(const char* url);      // Open connection
	uint8_t ssl_close();                    // close connection
	void ssl_data(const char* data);     // write data to module

    // get signal quality level (in dB)
    int getSignalQuality();
    bool getLocation(GSM_LOCATION* loc);
	
    // send AT command and check for expected response
    uint8_t sendCommand(const char* cmd, unsigned int timeout = 2000, const char* expected = 0);

	//void recv();  // read module tx buffer, use for debug 
    
	char buffer[128];
private:
    // empty serial in buffer
    void purgeSerial();
};
#endif

