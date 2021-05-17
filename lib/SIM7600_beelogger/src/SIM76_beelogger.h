/*************************************************************************
* SIM7600E TCPIP Library
* based on SIM800 GPRS/HTTP Library
* Distributed under GPL v2.0
* Written by Stanley Huang <stanleyhuangyc@gmail.com>
* For more information, please visit http://arduinodev.com
*
* Modified for use with SIM7600E
*
*************************************************************************/

#include <Arduino.h>

#ifndef __SIM7600__
#define __SIM7600__


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

//const char GSM_BAUD[] PROGMEM   = "AT+IPREX=9600";	// Baudrate SIM7600
const char SIM7600_UP[] PROGMEM = "PB DONE";		// SIM7600 up and ready

class CGPRS_SIM76 {
public:

    // initialize the module
    bool init(unsigned int timeout);
    // get signal quality level (in dB)
    int getSignalQuality();
    // get GSM location and network time
    bool getLocation(GSM_LOCATION* loc);
 
    // send AT command and check for expected response
    byte sendCommand(const char* cmd, unsigned int timeout = 2000, const char* expected = 0);
    // send AT command and check for two possible responses
    //byte sendCommand(const char* cmd, const char* expected1, const char* expected2, unsigned int timeout = 2000);

	// Start Stop Task to APN
	byte start(const char* apn,const char* usr,const char* pw);
	void stop();
	
	// prepare and send data
	bool prep_send();
    void send(const char* data);
	
	// Connect/disConnect TCP via CIP
	bool Connect(const char* host_name);
	void disConnect();

    // close software Serial
    void shutdown();

    char buffer[256];
private:
    // check buffer for given data
    //byte checkbuffer(const char* expected1, const char* expected2 = 0, unsigned int timeout = 2000);
    // empty serial in buffer
    void purgeSerial();
    // check if there is available serial data
    //bool available();
};
#endif

