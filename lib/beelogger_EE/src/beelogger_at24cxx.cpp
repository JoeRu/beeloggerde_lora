/*
* beelogger_at24cxx.cpp
*  based on
*
* File:   at24cxx.cpp
* Author: RandallR
*
* Created on March 26, 2012, 06:52 PM
*
* // https://forum.arduino.cc/index.php?topic=90594.0
*
* 19.07.2019 R.Schick
* Modified delay to 5 msec
*
* 09.08.2019 R.Schick
*  create beelogger_at24cxx.cpp
*  declaration of I2C-Address extern
*  modify constructor
*  modify delays after write operation
*
*  16.08.2019 R.Schick
*  prolonged delay in write
*
*  18.08.2019 R.Schick
*  isPresent rewritten
*
*  03.05.2021 R.Schick
*  isPresent rewritten
*/


#include <Arduino.h>
#include <Wire.h>

#include "beelogger_at24cxx.h"
#define min(a,b) ((a)<(b)?(a):(b))


AT24Cxx::AT24Cxx(int addr)
{
  _i2c_addr = addr;
  Wire.begin();
}
//
// PUBLIC FUNCTIONS

bool AT24Cxx::isPresent(void)      // check if the device is present
{
	// use the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
  Wire.beginTransmission(_i2c_addr);
  uint8_t rc = Wire.endTransmission();
  Wire.beginTransmission(_i2c_addr);
  return(rc);  // present: rc = 0
}

int AT24Cxx::ReadMem(int iAddr, char Buf[], int iCnt)
{
  int iRead=0, iBytes;
  while (iCnt>0) {
    Wire.beginTransmission(_i2c_addr);
#if ARDUINO >= 100
    Wire.write(iAddr>>8);   // Address MSB
    Wire.write(iAddr&0xff); // Address LSB
#else
    Wire.send(iAddr>>8);   // Address MSB
    Wire.send(iAddr&0xff); // Address LSB
#endif
    Wire.endTransmission();

    iBytes = min(iCnt, 128);
    Wire.requestFrom(_i2c_addr, iBytes);

    while (Wire.available() && iCnt>0) {
#if ARDUINO >= 100
      Buf[iRead] = Wire.read();
#else
      Buf[iRead] = Wire.receive();
#endif
      iRead++; iCnt--; iAddr++;
    }  /* while */
  }
  return (iRead);
}

int AT24Cxx::ReadStr(int iAddr, char Buf[], int iCnt)
{
  int iRead=0, iBytes;
  char c;
  while (iCnt>0) {
    Wire.beginTransmission(_i2c_addr);
#if ARDUINO >= 100
    Wire.write(iAddr>>8);   // Address MSB
    Wire.write(iAddr&0xff); // Address LSB
#else
    Wire.send(iAddr>>8);   // Address MSB
    Wire.send(iAddr&0xff); // Address LSB
#endif
    Wire.endTransmission();

    iBytes = min(iCnt, 128);
    Wire.requestFrom(_i2c_addr, iBytes);

    while (Wire.available() && iCnt>0) {
#if ARDUINO >= 100
      c = Wire.read();
#else
      c = Wire.receive();
#endif
      Buf[iRead] = c;
      if (c == '\0') {
        iCnt=0; break;
      }  /* if */
      iRead++; iCnt--; iAddr++;
    }  /* while */
  }
  return (iRead);
}

uint8_t AT24Cxx::WriteMem(int iAddr, uint8_t iVal)
{
  uint8_t iRC=0;
  Wire.beginTransmission(_i2c_addr);
#if ARDUINO >= 100
    Wire.write(iAddr>>8);   // Address MSB
    Wire.write(iAddr&0xff); // Address LSB
    Wire.write(iVal);
#else
    Wire.send(iAddr>>8);   // Address MSB
    Wire.send(iAddr&0xff); // Address LSB
    Wire.send(iVal);
#endif
  iRC = Wire.endTransmission();
  delay(10);
  return(iRC);
}

// BYTE WRITE:
// A write operation requires two 8-bit data word addresses following the device address word and acknowledgment.
// Upon receipt of this address, the EEPROM will again respond with a zero and then clock in the first 8-bit data
// word. Following receipt of the 8-bit data word, the EEPROM will output a zero and the addressing device, such as
// a microcontroller, must terminate the write sequence with a stop condition. At this time the EEPROM enters an
// internally-timed write cycle, tWR, to the nonvolatile memory. All inputs are disabled during this write cycle and
// the EEPROM will not respond until the write is complete (refer to Figure 2).

// PAGE WRITE:
// The 32K/64K EEPROM is capable of 32-byte page writes. A page write is initiated the same way as a byte write, but
// the microcontroller does not send a stop condition after the first data word is clocked in. Instead, after the EEPROM
// acknowledges receipt of the first data word, the microcontroller can transmit up to 31 more data words. The EEPROM
// will respond with a zero after each data word received. The microcontroller must terminate the page write sequence
// with a stop condition (refer to Figure 3).

// The data word address lower 5 bits are internally incremented following the receipt of each data word. The higher
// data word address bits are not incremented, retaining the memory page row location. When the word address, internally
// generated, reaches the page boundary, the following byte is placed at the beginning of the same page. If more than 32
// data words are transmitted to the EEPROM, the data word address will "roll over" and previous data will be overwritten.
uint8_t AT24Cxx::WriteMem(int iAddr, const char *pBuf, int iCnt)
{
  uint8_t iBytes, iRC=0;

// Writes are restricted to a single 32 byte page.  Therefore. if a write spans a page
// boundry we must split the write.

  while (iCnt > 0) {
    iBytes = min(iCnt, BUFFER_LENGTH-2);
    int iCurPage = iAddr & ~((int)0x1f);
    if (iAddr+iBytes > iCurPage+32) { // Number of bytes is too large
      iBytes = (iCurPage+32) - iAddr;
    }

    Wire.beginTransmission(_i2c_addr);
#if ARDUINO >= 100
    Wire.write( highByte(iAddr) ); // Address MSB
    Wire.write( lowByte(iAddr) );  // Address LSB
    Wire.write((uint8_t*)pBuf, iBytes);
#else
    Wire.send( highByte(iAddr) );   // Address MSB
    Wire.send( lowByte(iAddr) ); // Address LSB
    Wire.send(pBuf, iBytes);
#endif
    Wire.endTransmission();
    iRC  +=(int)iBytes;
    iCnt -=(int)iBytes;
    iAddr+=(int)iBytes;
    pBuf +=(int)iBytes;
	delay(10); // delay(50);  // Give the EEPROM time to write its data
  }  /* while */
  delay(20);
  return(iRC);
}

uint8_t AT24Cxx::WriteStr(int iAddr, const char *pBuf)
{
  uint8_t iRC=0;
  int iCnt = strlen(pBuf);

  iRC = WriteMem(iAddr, pBuf, iCnt+1); // Write the NULL terminator
  return(iRC);
}
