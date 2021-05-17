/*************************************************************************
* SIM800 GPRS/HTTP Library
* Distributed under GPL v2.0
* Written by Stanley Huang <stanleyhuangyc@gmail.com>
* For more information, please visit http://arduinodev.com
*
* Modified by Thorsten Gurzan <info@beelogger.de> https://beelogger.de for beelogger-Solar GSM
*  added use of apn_benuzter, apn_passwort
*  added functions
*    available: check for available data, moved from "h" to "cpp"
*    GET_Action: initiate HTTP Method Action
*    sendAT: Send Datat to Module
*    shutdown: send CFUN=0, turns connection off
*
*	modified
*     function init(), add timeout parameter, no reset in use, because of power off/on, 
*                      baudrate set to 9600, add retry when asking for "AT"
*     function httpinit modified, httpTerm added when failure, add timeout parameter
*
*
* Version 24.03.2018
*    use defines from "SIM800L.h" for SIM-Module Serial TX/RX ports
*    function setup parameter changed to const char
*    function setup: use strlen > 0 when testing apn_benutzer
*    function getSignalQuality fixed
*
*    change Debug output options
*    Debug: Serial.flush added after Serial.print
*
* Updated 01092018 R.Schick
*  use GSM_RX,GSM_TX from main sketch
*  flush buffer in sendAT
*
* Updated 24122018 R.Schick
*     getSignalQuality update to return error value
*
* Updated 11032019 R.Schick
*   added CIP-functionality
*   init() modified
*
* Updated 18042019 R.Schick
*  new Lib-Name
*  init()  set to old version
*  setup() set to old version
*
* Updated 20072019 R.Schick
*  modify Connect to check for a connection
*
* Updated 02082019 R.Schick
*  modify Start function
*
* Updated 15032020 R.Schick
*  modify AT-command location
* Updated 22032020 R.Schick
*  modify setup function
* Updated 31072020 R.Schick
*  modify sendCommand function
*
* Updated 06012021 R.Schick
*    function getSignalQuality timeout
*
* Version 2.3 19032021 R.Schick
*    RX buffer size, local buffers
*
*************************************************************************/

#include <SoftwareSerial.h>

#include "SIM800_beelogger_23.h"

extern uint8_t GSM_RX; 
extern uint8_t GSM_TX; 

SoftwareSerial SIM_SERIAL(GSM_RX,GSM_TX); // RX, TX vom AT-Mega

// Debug Information Datenempfang aktivieren, Serial.begin() muss in Hauptsketch
#define DEB_data 0
// erweiterte Debuginformationen
#define DEB_more 0
#define DEB_cmd 0

//#####################################
uint8_t CGPRS_SIM800::init(unsigned int timeout)
{   
    SIM_SERIAL.begin(9600);
#if DEB_data
	Serial.println("21-03-21");
	Serial.flush();
#endif

    uint32_t t = millis();
    do { 
      if (sendCommand("AT")) {
        sendCommand("AT+IPR=9600");
        sendCommand("ATE0");
        sendCommand("AT+CFUN=1", 10000);
        return true;
      }
	} 
    while (millis() - t < timeout);
    return false;
}


//#####################################
void CGPRS_SIM800::shutdown()
{
  sendCommand("AT+CFUN=0", 10000);
  SIM_SERIAL.end();  // stop listening
}


//#####################################
uint8_t CGPRS_SIM800::setup(const char* apn, const char* apn_benutzer, const char* apn_passwort)
{
  uint8_t success = false;
  uint8_t n =0;
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
    delay(1000);
	n++;
  } while ((n < 30) && (success == false));
  		
  if (!success)
    return 1;
  //Check if the MS is connected to the GPRS network
  success = false;
  n = 0;
  do{
	if (sendCommand("AT+CGATT?",5000,"1")){
		success = true;
	}
	n++;
	delay(2000); // wait a moment
  } while ((n < 5) && (success == false)); 
  if (!success)
    return 2;

  //  Activate bearer profile  -> GPRS
  if (!sendCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\""))
    return 3;
  // AT+SAPBR=3,1,"APN", Provider-apn
  char buf[64];
  strcpy(buf,"AT+SAPBR=3,1,\"APN\",\"");
  strcat(buf,apn);
  strcat(buf,"\"");
  if(!sendCommand(buf, 5000)) { return 4;} 

//  if (apn_benutzer!="") {
  if (strlen(apn_benutzer) > 0) {
  SIM_SERIAL.print("AT+SAPBR=3,1,\"USER\",\"");
    SIM_SERIAL.print(apn_benutzer);
    SIM_SERIAL.println('\"');

    SIM_SERIAL.print("AT+SAPBR=3,1,\"PWD\",\"");
    SIM_SERIAL.print(apn_passwort);
    SIM_SERIAL.println('\"');
#if 0
        Serial.print(apn_benutzer);
        Serial.print("  ");
        Serial.println(apn_passwort);
		Serial.flush();
#endif
  }

//  if (!sendCommand(0))  return 4;
  
  // AT+SAPBR=1,1  open connection
  sendCommand("AT+SAPBR=1,1", 5000);
  sendCommand("AT+SAPBR=2,1", 5000);
  if (!success)
    return 5;

  return 0;
}
//#####################################
uint8_t CGPRS_SIM800::getOperatorName()
{
  // display operator name
  if (sendCommand("AT+COPS?", "OK\r", "ERROR\r") == 1) {
      char *p = strstr(buffer, ",\"");
      if (p) {
          p += 2;
          char *s = strchr(p, '\"');
          if (s) *s = 0;
          strcpy(buffer, p);
          return true;
      }
  }
  return false;
}
//#####################################
uint8_t CGPRS_SIM800::checkSMS()
{
  if (sendCommand("AT+CMGR=1", "+CMGR:", "ERROR") == 1) {
    // reads the data of the SMS
    sendCommand(0, 100, "\r\n");
    if (sendCommand(0)) {
      // remove the SMS
      sendCommand("AT+CMGD=1");
      return true;
    }
  }
  return false;   
}
//#####################################
int CGPRS_SIM800::getSignalQuality()
{
  if(sendCommand("AT+CSQ",20000)){
    char *p = strstr(buffer, "CSQ: ");
	if (p) {
      int n = atoi(p + 5);
      if (n == 99 || n == -1) return -1;
      return n * 2 - 114;
    }
  }
  return -2; 
}
//#####################################
int CGPRS_SIM800::getSIM800_Voltage()
{
  int simVoltage = 0;
  if (sendCommand("AT + CBC", 2000)) {
    //+CBC: 0,99,4190
#if DEB_more
        Serial.print("CBC OK");
#endif
    char *p = strstr(buffer, "+CBC");
    if (p) {
      int x = 0;
      do {
        p++; x++;
      } while ( !(*p == '\n') && (x < 18)); // check EOL und Buffer
      simVoltage = atoi(p - 5);
#if DEB_more
    Serial.println(simVoltage);
   	Serial.flush();
#endif
    }
  }
  return(simVoltage);
}
//#####################################
// SIM800 Firmware Version: "1418B05SIM800L2"
// reports GSM base logged in to
uint8_t CGPRS_SIM800::getLocation(GSM_LOCATION* loc)
{
  if (sendCommand("AT+CLBS=4,1", 15000)) do {
    char *p;
    if (!(p = strchr(buffer, ':'))) break;
    if (!(p = strchr(p, ','))) break;
    loc->lon = atof(++p);
    if (!(p = strchr(p, ','))) break;
    loc->lat = atof(++p);
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
    return true;
  } while(0);
  return false;
}
//#####################################
void CGPRS_SIM800::httpUninit()
{
  sendCommand("AT+HTTPTERM");
}
//#####################################
uint8_t CGPRS_SIM800::httpInit(unsigned int timeout)
{
  uint32_t t = millis();
  do {	
    if  (sendCommand("AT+HTTPINIT", 3000) || sendCommand("AT+HTTPPARA=\"CID\",1", 3000)) {
      httpState = HTTP_READY;      
      return true;
    } else {
    	  sendCommand("AT+HTTPTERM");
    	  delay(1000);
    	}
  }
  while (millis() - t < timeout);
  httpState = HTTP_DISABLED;
  return false;
}
//#####################################
uint8_t CGPRS_SIM800::httpConnect(const char* url, const char* args)
{
    SIM_SERIAL.print("AT+HTTPPARA=\"URL\",\""); // Sets url
    SIM_SERIAL.print(url);
    if (args) {
        SIM_SERIAL.print('?');
        SIM_SERIAL.print(args);
    }

    SIM_SERIAL.println('\"');
    if (sendCommand(0))
    {
        SIM_SERIAL.println("AT+HTTPACTION=0"); // Starts GET action
        httpState = HTTP_CONNECTING;
        m_bytesRecv = 0;
        m_checkTimer = millis();
    } else {
        httpState = HTTP_ERROR;
    }
    return false;
}
//#####################################
uint8_t CGPRS_SIM800::GET_Action()
{
    if (sendCommand(0))
    {
        SIM_SERIAL.println("AT+HTTPACTION=0");         // Starts GET action
        httpState = HTTP_CONNECTING;
        m_bytesRecv = 0;
        m_checkTimer = millis();
    } else {
        httpState = HTTP_ERROR;
    }
    return false;
}

//#####################################
// check if HTTP connection is established
// return 0 for in progress, 1 for success, 2 for error
int8_t CGPRS_SIM800::httpIsConnected()
{
    uint8_t ret = checkbuffer("0,200", "0,60", 10000);
    if (ret >= 2) {
        httpState = HTTP_ERROR;
        return -1;
    }
    return ret;
}
//#####################################
void CGPRS_SIM800::httpRead()
{
    SIM_SERIAL.println("AT+HTTPREAD");
    httpState = HTTP_READING;
    m_bytesRecv = 0;
    m_checkTimer = millis();
}
//#####################################
// check if HTTP connection is established
// return 0 for in progress, -1 for error, number of http payload bytes on success
int CGPRS_SIM800::httpIsRead()
{
    uint8_t ret = checkbuffer("+HTTPREAD: ", "Error", 10000) == 1;
    if (ret == 1) {
        m_bytesRecv = 0;
        // read the rest data
        sendCommand(0, 100, "\r\n");
        unsigned int bytes = atoi(buffer);
        sendCommand(0);
        bytes = min(bytes, sizeof(buffer) - 1);
        buffer[bytes] = 0;
        return bytes;
    } else if (ret >= 2) {
        httpState = HTTP_ERROR;
        return -1;
    }
    return 0;
}

//#####################################
// CIP functions
//#####################################
// Start Task to APN
uint8_t CGPRS_SIM800::start(const char* apn)
{
  uint8_t success = false;
  uint8_t n = 0; 
  do {
    if (sendCommand("AT+CREG?", 2000)) {  // are we registerd to a gsm-network
        char *p = strstr(buffer, "0,");
        if (p) {
          char mode = *(p + 2);
          if (mode == '1' || mode == '5') {
            success = true;
          }
        }
    }
	n++;
    delay(2000); // wait a moment
  } while ((n < 20) && (success == false)); // may take very long

  if (!success)
    return 1;

  //Check if the MS is connected to GPRS: CGATT need to return 1
  success = false;
  n =0;
  do{
	if (sendCommand("AT+CGATT?",5000,"1")){
		success = true;
	}
	n++;
	delay(2000); // wait a moment
  } while ((n < 5) && (success == false)); 
  
  if (!success)
    return 2;

  char c_cmd[64];
  strcpy_P(c_cmd, GSM_TASK);  // Start Task to APN
  strcat(c_cmd, apn);
  strcat(c_cmd, "\"");
  if (sendCommand(c_cmd, 10000)) {
    if(sendCommand("AT+CIICR", 5000)){ //Bring Up Wireless Connection with GPRS 
	  sendCommand("AT",5000); // Wait for ready
	  //Only after PDP context is activated, local IP address can be obtained
      if(sendCommand("AT+CIFSR", 5000,"0.")){ //Get Local IP Address 10.x.x.x
        return 0;
	  }
    }
  }
  stop();
  return 3;
}

//#####################################
//Deactivate GPRS PDP Context
//makes PDP context come back to original state
void CGPRS_SIM800::stop()
{
  sendCommand("AT+CIPSHUT", 3000); 
}
//#####################################
// CIP start connection to host
uint8_t CGPRS_SIM800::Connect(const char* host_name)
{
  char c_cmd[96];
  strcpy_P(c_cmd, CIP_START);
  strcat(c_cmd, host_name);
  strcat(c_cmd, "\",80");     	// port 80
  // set up or check for a connection
  if (sendCommand(c_cmd, "CONNECT OK", "ALREADY", 10000)) {
    return true;
  }
  disConnect();
  return false;
}

//#####################################
// prepare Sending data using CIPSEND
// and wait for being ready by returning ">"
uint8_t CGPRS_SIM800::prep_send()
{
  char c_cmd[16];
  strcpy_P(c_cmd, CIP_SEND);
  if (sendCommand(c_cmd, 2000, ">")) { // receive ready?
    return (true);
  }
  return (false);
}

//#####################################
// start sending data by calling function: send(0x00)
void CGPRS_SIM800::send(const char* data)
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
//#####################################
//close TCP Connection
void CGPRS_SIM800::disConnect()
{  
  char c_cmd[16];
  strcpy_P(c_cmd, CIP_CLOSE);
  sendCommand(c_cmd, 3000);    
}
// end CIP functions
//#####################################


// Utility functions

//#####################################
uint8_t CGPRS_SIM800::available()
{
   return SIM_SERIAL.available(); 
}
//#####################################
uint8_t CGPRS_SIM800::sendCommand(const char* cmd, unsigned int timeout, const char* expected)
{
  if (cmd != 0) {
    purgeSerial();
#if DEB_cmd
    Serial.print(F(">"));
    Serial.println(cmd);
    Serial.flush();
#endif
    SIM_SERIAL.println(cmd);
  }
  uint32_t t = millis();
  uint8_t n = 0;
  do {
    if (SIM_SERIAL.available()) {
      char c = SIM_SERIAL.read();
      if (n >= (BUFFER_Size - 1)) { // buffer full, discard first half
        n = (BUFFER_Size / 2) - 1;
        memcpy(buffer, buffer + (BUFFER_Size / 2), n);
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
uint8_t CGPRS_SIM800::sendCommand(const char* cmd, const char* expected1, const char* expected2, unsigned int timeout)
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
  uint8_t n = 0;
  do {
    if (SIM_SERIAL.available()) {
      char c = SIM_SERIAL.read();
      if (n >= (BUFFER_Size - 1)) { // buffer full, discard first half
        n = (BUFFER_Size / 2) - 1;
        memcpy(buffer, buffer + (BUFFER_Size / 2), n);
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
//#####################################
uint8_t CGPRS_SIM800::checkbuffer(const char* expected1, const char* expected2, unsigned int timeout)
{
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
//#####################################
void CGPRS_SIM800::purgeSerial()
{
  while (SIM_SERIAL.available()) SIM_SERIAL.read();
}

//#####################################
void CGPRS_SIM800::sendAT(const char* cmd)
{
#if DEB_data
  Serial.println(cmd);
  Serial.flush();
#endif
  SIM_SERIAL.print(cmd);
}
