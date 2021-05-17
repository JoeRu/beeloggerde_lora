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

#include "SIM76_beelogger.h"

extern byte GSM_RX; 
extern byte GSM_TX; 

#include <AltSoftSerial.h>
AltSoftSerial SIM_SERIAL; // RX, TX vom AT-Mega

// Debug Information Datenempfang aktivieren, Serial.begin() muss in Hauptsketch
#define DEB_data 0
// erweiterte Debuginformationen
#define DEB_more 0
#define DEB_cmd 0

//#####################################
bool CGPRS_SIM76::init(unsigned int timeout)
{  
	#define try_count 4 
   char buf[16];
   SIM_SERIAL.begin(9600);
#if DEB_data
	Serial.println("10-08-2020");
	Serial.flush();
#endif

	strcpy_P(buf, SIM7600_UP);
	int i =0;
	do { 
      if (sendCommand(0,6000,buf)) {
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
void CGPRS_SIM76::shutdown()
{
  sendCommand("AT+CFUN=0", 10000);
  SIM_SERIAL.end();  // stop listening
}


//#####################################
// start TCP/IP functions
//#####################################
uint8_t CGPRS_SIM76::start(const char* apn,const char* usr,const char* pw)
{
  bool success = false;
  byte n = 0;
  do {
    if (sendCommand("AT+CREG?", 2000)) {  // are we registerd to a gsm-network
        char *p = strstr(buffer, "0,");   // expected 0,1 or 0,5
        if (p) {
          char mode = *(p + 2);
#if DEB_more
        Serial.print(F("Mode:"));
        Serial.println(mode);
		Serial.flush();
#endif
          if ((mode == '1' || mode == '5')) {  // net or roaming
            success = true;
          }
        }
	}
    if(success == false){ delay(2000);}
	n++;
  } while ((n < 30) && (success == false));
  		
  if (!success) return 1;
  
	n = 0;
	do {
		if (sendCommand("AT+CGREG?", 2000)) {  // are we registerd to a gsm-network
			char *p = strstr(buffer, "0,");    // expected 0,1 or 0,5
			if (p) {
				char mode = *(p + 2);
				if ((mode == '1' || mode == '5')) {// net or roaming
					success = true;
				}
			}
		}
		if(success == false){ delay(2000);}
		n++;
	} while ((n < 5) && (success == false));

  if (!success) return 2;
  
  //Configure Context with APN
  char buf[64];
  strcpy(buf, "AT+CGDCONT=1,\"IP\",\"");
  strcat(buf, apn);
  strcat(buf, "\"");
  if (sendCommand(buf, 10000)) {
	delay(1000);  // wait for SIM7600 to get ready
	if (sendCommand("AT+NETOPEN", 2000)) {  // activate Context
		delay(1000);  // wait for SIM7600 to get ready
		if(strlen(usr) > 0){
			strcpy(buf, "AT+CGAUTH=1,3,\"");
			strcat(buf,usr);
			strcat(buf,"\",\"");
			strcat(buf,pw);
			strcat(buf,"\"");
			sendCommand(buf, 5000);
			delay (500);
		}
		n = 0;
		do {
			if (sendCommand("AT+IPADDR", 5000,"0.")) {  // are we done internally
				return(0);
			}
			delay(1000);
			n++;
		} while (n < 5);
    	return(5);
    }
	return(4);
  }
  return(3);
}


//#####################################
//Deactivate Context
//makes context come back to original state
void CGPRS_SIM76::stop()
{
  sendCommand("AT+NETCLOSE", 5000); 
}

//#####################################
// CIP start connection to host
bool CGPRS_SIM76::Connect(const char* host_name)
{
  strcpy(buffer, "AT+CIPOPEN=1,\"TCP\",\"");
  strcat(buffer, host_name);
  strcat(buffer, "\",80");     	// port 80
  // set up or check for a connection
  if (sendCommand(buffer, 20000, "+CIPOPEN: 1,0")) {
    return true;
  }
  disConnect();
  return false;
}

//#####################################
//close TCP Connection
void CGPRS_SIM76::disConnect()
{  
  sendCommand("AT+CIPCLOSE=1", 3000,"+CIPCLOSE");    
}


//#####################################
// prepare Sending data using CIPSEND
// and wait for being ready by returning ">"
bool CGPRS_SIM76::prep_send()
{
  if (sendCommand("AT+CIPSEND=1,", 2000, ">")) { // receive ready?
    return (true);
  }
  return (false);
}
//#####################################
// start sending data by calling function: send(0x00)
void CGPRS_SIM76::send(const char* data)
{
  if (data == 0x00) {
    SIM_SERIAL.write(0x1A);  	    // transmission of data start
  }
  else {
#if DEB_data
    Serial.print(data);
    Serial.flush();
#endif
    SIM_SERIAL.print(data);
  }
}
// end TCP/IP functions
//#####################################



//#####################################
int CGPRS_SIM76::getSignalQuality()
{
  sendCommand("AT+CSQ",2000);
  char *p = strstr(buffer, "CSQ: ");
  if (p) {
    int n = atoi(p + 5);
    if (n == 99 || n == -1) return -1;
    return n * 2 - 114;
  } else {
   return -2; 
  }
}
//#####################################
// Location command:
//AT+CLBS=4
//OK
//+CLBS: 0,31.228525,121.380295,500,2025/06/07,10:49:08
//
bool CGPRS_SIM76::getLocation(GSM_LOCATION* loc)
{
	bool x_val = false;	
	if (sendCommand("AT+CNETSTART", 15000)) {
		delay(1000);  // wait for SIM7600 to get ready
		if (sendCommand("AT+CLBS=4", 15000,"+CLBS")){
			uint32_t t = millis();
			byte n = 0;
			do { // 'OK' before +CLBS, read til EOL
				if (SIM_SERIAL.available()) {
					char c = SIM_SERIAL.read();			
					buffer[n++] = c;
					if (c == '\r') {break;}  // EOL
				}
			} while ((n < 50) && (millis() - t < 3000));
			buffer[n] = 0;
			char *p = buffer;
			do {
				if (!(p = strchr(p, ','))) break;
				loc->lon = atof(++p);
				if (!(p = strchr(p, ','))) break;
				loc->lat = atof(++p);
			/*
			if (!(p = strchr(p, ','))) break;									
			loc->year = atoi(++p) - 2000;
			if (!(p = strchr(p, '/'))) break;
			loc->month = atoi(++p);
			if (!(p = strchr(p, '/'))) break;
			loc->day = atoi(++p);
			if (!(p = strchr(p, ','))) break;
			loc->hour = atoi(++p);
			if (!(p = strchr(p, ':'))) break;
			loc->minute = atoi(++p);
			if (!(p = strchr(p, ':'))) break;
			loc->second = atoi(++p);
			*/
				x_val = true;
				break;
			} while(0);
		}
#if DEB_data
			Serial.print(buffer);
			Serial.flush();
#endif
		sendCommand("AT+CNETSTOP", 5000);
	}	
	return x_val;
}



//#####################################
//#####################################
// Utility functions
//#####################################
//#####################################

//#####################################
/*
bool CGPRS_SIM76::available()
{
   return SIM_SERIAL.available(); 
}
*/

//#####################################
byte CGPRS_SIM76::sendCommand(const char* cmd, unsigned int timeout, const char* expected)
{
  if ( cmd != 0) {
#if DEB_cmd
    Serial.print(F(">"));
    Serial.println(cmd);
    Serial.flush();
	delay(5);
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
/*
byte CGPRS_SIM76::sendCommand(const char* cmd, const char* expected1, const char* expected2, unsigned int timeout)
{
  if (cmd) {
    purgeSerial();
#if DEB_more
    Serial.print('>');
    Serial.println(cmd);
	Serial.flush();
#endif
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
      if (strstr(buffer, expected1)) {
#if DEB_more
       Serial.print(F("[2]"));
       Serial.println(buffer);
	   Serial.flush();
#endif
       return 1;
      }
      if (strstr(buffer, expected2)) {
#if DEB_more
       Serial.print(F("[3]"));
       Serial.println(buffer);
	   Serial.flush();
#endif
       return 2;
      }
    }
  } while (millis() - t < timeout);
#if DEB_more
   Serial.print(F("Timeout"));
   Serial.println(buffer);
   Serial.flush();
#endif
  return 0;
}
*/
//#####################################
/*
byte CGPRS_SIM76::checkbuffer(const char* expected1, const char* expected2, unsigned int timeout)
{
    byte m_bytesRecv;
    uint32_t m_checkTimer;
	m_checkTimer = millis();
    while (SIM_SERIAL.available()) {
        char c = SIM_SERIAL.read();
        if (m_bytesRecv >= sizeof(buffer) - 1) { // buffer full, discard first half
            m_bytesRecv = sizeof(buffer) / 2 - 1;
            memcpy(buffer, buffer + sizeof(buffer) / 2, m_bytesRecv);
        }
        buffer[m_bytesRecv++] = c;
        buffer[m_bytesRecv] = 0;
        if (strstr(buffer, expected1)) {
            return 1;
        }
        if (expected2 && strstr(buffer, expected2)) {
            return 2;
        }
    }
    return (millis() - m_checkTimer < timeout) ? 0 : 3;
}
*/
//#####################################
void CGPRS_SIM76::purgeSerial()
{
  while (SIM_SERIAL.available()) SIM_SERIAL.read();
}

