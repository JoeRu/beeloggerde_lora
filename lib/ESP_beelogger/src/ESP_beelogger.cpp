/*************************************************************************
  ESP-01 HTTP Library
  use with ESP8266_NONOS_SDK_V2.0.0_16_07_19 based on ESP Binaries
   - esp_init_data_v08
   - boot_v1.6.bin or boot_v1.7.bin
   - user1.1024.new2.bin
   - user2.1024.new2.bin
   
  see
  https://github.com/espressif/ESP8266_NONOS_SDK/tree/master/bin

  Important:
  ESP-01 must be configured fixed to 9600 or 19200Baud using:
  AT+UART_DEF=9600,8,1,0,0  or  AT+UART_DEF=19200,8,1,0,0
  AT+RESTORE must not be used after setting UART.
  Default Mode must be set to station, use function C_WLAN_ESP01::mode();
  AT+CWMODE_DEF=1

  Distributed under GPL v2.0
  For more information, please visit http://arduinodev.com

		   
  Version 1.0 08.08.2018			
  Version 1.1 28.09.2019			
            modified join   
  Version 1.2 10.01.2021			
            modified signal   

*************************************************************************/				   

#include "ESP_beelogger.h"
#include <SoftwareSerial.h>

// debug information for data and/or commands, Serial.begin() needed in main sketch
#define DEB_cmd  0
#define DEB_data 0
#define DEB_send 0
#define DEB_buff 0
extern byte ESP_RX; 
extern byte ESP_TX; 

// to be set up in main sketch
//byte ESP_RX = 8;
//byte ESP_TX = 9;

// the software serial interface														
SoftwareSerial ESP_SERIAL = SoftwareSerial(ESP_RX, ESP_TX); //RX, TX
  
  
// Initialize Class and espState
C_WLAN_ESP01::C_WLAN_ESP01() {
   espState = ESP_STOP;
}


// setup ESP in Station Mode only and single Connection
bool C_WLAN_ESP01::init(int baudrate)
{
  char c_cmd[24];
  ESP_SERIAL.begin(baudrate);
  delay(50);
  strcpy(c_cmd,"AT");  				// "AT"
  sendCommand(c_cmd); 				// dummy send for restart needed
  if (sendCommand(c_cmd)) { 		// AT ... "OK"
    strcpy_P(c_cmd, ESP_MUX);
    if (sendCommand(c_cmd)) {		// Connection mode
	  strcpy_P(c_cmd,ESP_ATE0);
      sendCommand(c_cmd);			// Echo Off
      espState = ESP_INIT;
      return true;
    }
  }
  espState = ESP_ERROR;
  return false;
}

// end / stop serial, interupts may prevent sleep
void C_WLAN_ESP01::end(){
    espState = ESP_STOP;
	ESP_SERIAL.end();		// calls Softwareserial StopListening
}

// Join WLAN Access Point
bool C_WLAN_ESP01::join(const char* access_point, const char* ap_passwort, unsigned long timeout)
{
char buf_[100];
timeout = 30000;
  if(espState == ESP_INIT){
	//example AT+CWJAP_CUR=“my_access_point”,“0123456789”
    strcpy_P(buf_, ESP_WJAP);		// Connect to Access Point
    strcat(buf_, access_point);
    strcat(buf_, "\",\"");
    strcat(buf_, ap_passwort);
    strcat(buf_, "\"");
    if (sendCommand(buf_, timeout)) {
      espState = ESP_WLAN_READY;
      return true;
    }
  }
  espState = ESP_ERROR;
  return false;
}


// Quit from Access Point and Restart the ESP
bool C_WLAN_ESP01::quit()
{
  char c_cmd[24];
  strcpy_P(c_cmd,ESP_WQAP);			// disconnect from Access Point
  sendCommand(c_cmd);     
  strcpy_P(c_cmd,ESP_RST); 			// Restart ESP
  if(sendCommand(c_cmd,5000)){
    espState = ESP_STOP;
    return(true);
  }
  espState = ESP_ERROR;
  return(false);
}

// Connect to Host at Port 80
bool C_WLAN_ESP01::Connect(const char* host_name)
{
  if (espState == ESP_WLAN_READY){
    strcpy_P(buffer, ESP_START); 	// connect to hostname
    strcat(buffer, host_name);
    strcat(buffer, "\",80");     	// port 80
    if (sendCommand(buffer, 10000, "CONNECT")) {
      espState = ESP_CONNECTED;
      return true;
    }
  }
  espState = ESP_ERROR;
  return false;
}

// Terminate Server Connection
void C_WLAN_ESP01::disConnect()
{
  char c_cmd[24];
  strcpy_P(c_cmd, ESP_CLOSE);		// end TCP Connection
  sendCommand(c_cmd); 
  espState = ESP_WLAN_READY;
}

// prepare Sending data using CIPSENDEX
// and wait for ESP being ready by returning ">"
bool C_WLAN_ESP01::prep_send(int length)
{
  if (espState == ESP_CONNECTED){
	char c_len[5];
    itoa(length, c_len, 10);   			// convert len to char Base 10
    strcpy_P(buffer, ESP_SENDEX);		// CIPSENDEX
    strcat(buffer, c_len);
    if (sendCommand(buffer, 2000, ">")) { // receive ready?
      return (true);
    }
  }
  espState = ESP_ERROR;
  return (false);
}

// send data to ESP-buffer, used with CIPSENDEX
// if 0x00 is received, ESP is asked to send data by sending "\\0"
void C_WLAN_ESP01::send(const char* data)
{
  if (espState == ESP_CONNECTED){
    if (data == 0x00) {
      ESP_SERIAL.write("\\0");  	// transmission of data start
  	  espState = ESP_WLAN_READY;
    }
    else {
      ESP_SERIAL.print(data);
      ESP_SERIAL.flush(); 		 	// Software Serial has only a 64byte buffer,  
 #if DEB_send // Debug 
        Serial.print(data);
        Serial.flush();
#endif
	}
  }
}

// send Command to ESP
// if parameter 'cmd' is 0 (don't use "" or "0"), nothing is send,
// default 'timeout' is set in '.h' to 2000 milliseconds
// the data from ESP is checked against data in 'expected'
// if 'expected' is omitted, default string is "OK\r"
byte C_WLAN_ESP01::sendCommand(char* cmd, unsigned long timeout, const char* expected)
{
  if (cmd) {
    empty_rcv();
	strcat(cmd,"\r\n");  		// always append CR LF
#if DEB_cmd
    Serial.print(F("\r\nCmd:  "));
    Serial.println(cmd);
    Serial.flush();
#endif
    ESP_SERIAL.print(cmd);
    ESP_SERIAL.flush();		// wait for data to be transmitted
  }
  unsigned long t = millis();
  byte n = 0;
  do {
    if (ESP_SERIAL.available()) {
      char c = ESP_SERIAL.read();
#if DEB_buff
    Serial.print(c);
#endif
      if (n >= sizeof(buffer) - 1) {
        // buffer full, discard first half
        n = sizeof(buffer) / 2 - 1;
        memcpy(buffer, buffer + sizeof(buffer) / 2, n);
      }
	  // copy received character and terminate string
      buffer[n++] = c;
      buffer[n] = 0;
	  // compare buffer to expected string or "OK" if an empty one is given
      if (strstr(buffer, expected ? expected : "OK\r")) {
        return n;
      }
	  if( c == '\r') n = 0; // if newline reset counter, check a single line only
    }
  } while ((millis() - t) < timeout);
  return 0;  // not found
}

// empty RX Buffer
void C_WLAN_ESP01::empty_rcv() {
  while (ESP_SERIAL.available() > 0) {
    ESP_SERIAL.read();
  }
  delay(50);
}


// activate light sleep mode
void C_WLAN_ESP01::sleep(byte sleep_mode){
  char c_cmd[24];
  strcpy_P(c_cmd, ESP_SLP);
  if (sleep_mode == 2) {
	  strcat(c_cmd,"2");  // modem sleep
  }
#if 0  // use only with hardware wakeup
  else if (sleep_mode == 1)	{
	  strcat(c_cmd,"1");  // light sleep
  }
#endif
  else strcat(c_cmd,"0");
  sendCommand(c_cmd);
  delay(200);
}


//*************************************************
// follwing functions only for initialising and testing the hardware	


// request the firmware information of the ESP
// return values in  variable 'buffer'
void C_WLAN_ESP01::firmware(unsigned long timeout)
{
  unsigned long t = millis();
  char c;
  byte n = 0;
  empty_rcv();
  strcpy_P(buffer,ESP_GMR);
  strcat(buffer,"\r\n");
  ESP_SERIAL.print(buffer);		// firmware Information
  ESP_SERIAL.flush();		 

  do {
    if (ESP_SERIAL.available() > 0) {
      c = ESP_SERIAL.read();
      buffer[n] = c;
      buffer[n+1] = 0;
	  if (n < (sizeof(buffer)-2)) n++;
    }
  } while ((millis() - t) < timeout);
}


// request the AP information from the ESP
// return values for a given AP in var "buffer"
// w/o AP (AP = 0) data is printed to Serial.
uint8_t C_WLAN_ESP01::signal(const char* ap,unsigned long timeout)
{
  char buf[8];
 
  empty_rcv();
  strcpy_P(buffer,ESP_WLAPOPT);	// signal information parameter
  if(sendCommand(buffer, timeout)){
    strcpy_P(buffer,ESP_WLAP);	// signal information parameter
    strcat(buffer,"=\"");  		// only the given AP
    strcat(buffer, ap);
    strcat(buffer,"\"");
    strcat(buffer,"\r\n");
    strcpy(buf,")");//ap);
    if(sendCommand(buffer, timeout, buf)){
      return(1);
    }
  }
  return(0);
}


// set station Mode
bool C_WLAN_ESP01::mode(){
  char c_cmd[24];
  strcpy_P(c_cmd, ESP_MODE);
  if (sendCommand(c_cmd)) {
	return 1;
  }
  return 0;
}


// show serial TX data from ESP, for debug only, need Serial.begin somewhere in a sketch
void C_WLAN_ESP01::check(unsigned long timeout){
  unsigned long t = millis();
  char c;
  do {
    if (ESP_SERIAL.available() > 0) {
      c = ESP_SERIAL.read();
      Serial.print(c);
    }
  } while ((millis() - t) < timeout);
}	

// Send data using CIPSEND, not finally tested
// the string must be prepared in complete in advance
//  you need to put the \r\n at the end
bool C_WLAN_ESP01::write_d(const char* data, int length)
{
  if (espState == ESP_CONNECTED){
    char c_len[5];
    itoa(length, c_len, 10);   // konvert len nach char
    strcpy_P(buffer, ESP_SEND);  // sende
    strcat(buffer, c_len);
#if DEB_data // Debug 
    Serial.print(buffer);
    Serial.flush();
#endif
    if (sendCommand(buffer, 2000, ">")) { // receive ready?
      for (int i = 0; i < length; i++) {
        ESP_SERIAL.print(data[i]);
        delay(3); // Software Serial has only a 64byte buffer, so wait for byte to be send
#if DEB_data // Debug 
        Serial.print(data[i]);
        Serial.flush();
#endif
      }
      return (true);
    }
  }
  espState = ESP_ERROR;
  return (false);
}
