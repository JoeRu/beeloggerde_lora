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

#include <SoftwareSerial.h>

#include "SIM76_http_beelogger.h"
#include <AltSoftSerial.h>
AltSoftSerial SIM_SERIAL; // RX, TX vom AT-Mega

// Debug Information Datenempfang aktivieren, Serial.begin() muss in Hauptsketch
#define DEB_data 1
// erweiterte Debuginformationen
#define DEB_more 1
#define DEB_cmd 1

//#####################################
bool CGPRS_SIM76_HTTP::init(unsigned int timeout)
{   
 #define try_count 4
  char buffer[16];
   SIM_SERIAL.begin(9600);
#if DEB_data
	Serial.println("31-07-2020");
	Serial.flush();
#endif

	strcpy_P(buffer, SIM7600_UP);
	int i =0;
	do { 
      if (sendCommand(0,6000,buffer)) {
        break;
      }
	  i++;
	} 
    while (i < try_count);
	if(i == try_count) return(false);
#if DEB_data
	Serial.println("SIM7600 is up");
	Serial.flush();
#endif
    uint32_t t = millis();
    do { 
      if (sendCommand("AT")) {
        sendCommand("ATE0");
        sendCommand("AT+CFUN=1", 10000);
        return true;
      }
	} 
    while (millis() - t < timeout);
    return false;
}

//#####################################
void CGPRS_SIM76_HTTP::shutdown()
{
  sendCommand("AT+CFUN=0", 10000);
  SIM_SERIAL.end();  // stop listening
}

//#####################################
byte CGPRS_SIM76_HTTP::start(const char* apn)
{
  bool success = false;
  byte n = 0;
  do {
    if (sendCommand("AT+CREG?", 2000)) {  // are we registerd to a gsm-network
        char *p = strstr(buffer, "0,");
        if (p) {
          char mode = *(p + 2);
#if DEB_more
        Serial.print(F("Mode:"));
        Serial.println(mode);
		Serial.flush();
#endif
          if (mode == '1' || mode == '5') {
            success = true;
          }
        }
    }
    delay(3000);
	n++;
  } while ((n < 30) && (success == false));
  		
  if (!success) return 1;
  
  n = 0;
  do {
    if (sendCommand("AT+CGREG?", 2000,"0,1")) {  // are we registerd to a gsm-network
          success = true;
    }
    delay(1000);
	n++;
  } while ((n < 5) && (success == false));

  if (!success) return 2;
  
  //Configure Context with APN
  char buf[64];
  strcpy(buf, "AT+CGSOCKCONT=1,\"IP\",\"");
  strcat(buf, apn);
  strcat(buf, "\"");
  if (sendCommand(buf, 10000)) {
	delay(1000);  // wait on SIM7600 to get ready
	if (sendCommand("AT+CGPADDR", 5000,"10.")) {  // are we done internally
		return(0);
	}	 
	return(4);
  }
  return(3);
}



//#####################################
// HTTP functions
//#####################################
//start HTTP service, activate PDP context
bool CGPRS_SIM76_HTTP::http_init()
{
  return(sendCommand("AT+HTTPINIT", 1000)); 
}
//set the URL which will be accessed, make Get
bool CGPRS_SIM76_HTTP::http_getcall(const char* para)
{
	if(sendCommand(para, 5000)){
		if(sendCommand("AT+HTTPACTION=0", 20000,"0,200")){
			return(true);
		}
	}
	return(false);
}

//read the response information of HTTP server
bool CGPRS_SIM76_HTTP::http_read(const char* data)
{
  return(sendCommand("AT+HTTPREAD=0,100", 2000, data));
}
//stop HTTP Service
bool CGPRS_SIM76_HTTP::http_end()
{
	return(sendCommand("AT+HTTPTERM", 2000));
}

//sends data to module used for "AT+HTTPPARA=\"URL\",\"  .... "
void CGPRS_SIM76_HTTP::http_para(const char* para)
{
#if DEB_data
    Serial.println(para);
    Serial.flush();
#endif
    SIM_SERIAL.print(para);
    SIM_SERIAL.flush();
}
// end HTTPPARA call with """
bool CGPRS_SIM76_HTTP::http_end_para()
{
#if DEB_data
    Serial.println("\"");
    Serial.flush();
#endif    
	SIM_SERIAL.print('\"');   // single char
	if(sendCommand(0, 3000)){  // AT+HTTP_PARA Stringende "\""
		return(true);
	}
	return(false);
}
// make Get, starts transfer of data to server
bool CGPRS_SIM76_HTTP::http_get(unsigned long timeout)
{
	if(sendCommand("AT+HTTPACTION=0", timeout,"0,200")){
		return(true);
	}
	return(false);
}





//#####################################
int CGPRS_SIM76_HTTP::getSignalQuality()
{
  sendCommand("AT+CSQ");
  char *p = strstr(buffer, "CSQ: ");
  if (p) {
    int n = atoi(p + 5);
    if (n == 99 || n == -1) return -1;
    return n * 2 - 114;
  } else {
   return -2; 
  }
}
//stop HTTP Service
bool CGPRS_SIM76_HTTP::getLocation(GSM_LOCATION* loc)
{
	return(false);
}

//#####################################
//#####################################
// Utility functions
//#####################################
//#####################################


//#####################################
byte CGPRS_SIM76_HTTP::sendCommand(const char* cmd, unsigned int timeout, const char* expected)
{
  if (cmd != 0) {
#if DEB_cmd
    Serial.print(F(">"));
    Serial.println(cmd);
    Serial.flush();
#endif
    purgeSerial();
    SIM_SERIAL.println(cmd);
  }
  uint32_t t = millis();
  byte n = 0;
  do {
    if (SIM_SERIAL.available()) {
      char c = SIM_SERIAL.read();
      if (n >= sizeof(buffer) - 1) { // buffer full, discard first half
        n = sizeof(buffer) / 2 - 1;
        memcpy(buffer, buffer + sizeof(buffer) / 2, n);
      }
      buffer[n++] = c;
      buffer[n] = 0;
      if (strstr(buffer, expected ? expected : "OK\r")) {
#if DEB_more
       Serial.print(F("[1]"));
       Serial.println(buffer);
	   Serial.flush();
#endif
       return n;
      }
  	  //if( c == '\r') n = 0; // do not use here, causes major trouble
    }
  } while (millis() - t < timeout);
#if DEB_more
   Serial.print(F("Timeout"));
   Serial.println(buffer);
   Serial.flush();
#endif
  return 0;
}

//#####################################
void CGPRS_SIM76_HTTP::purgeSerial()
{
  while (SIM_SERIAL.available()){ SIM_SERIAL.read();delay(5);}
}

