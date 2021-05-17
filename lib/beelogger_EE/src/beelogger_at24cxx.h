/*
* beelogger_at24cxx.h
*  based on
*
* File:   at24cxx.h
* Author: RandallR
*
* Created on March 26, 2012, 06:52 PM
*
* // https://forum.arduino.cc/index.php?topic=90594.0
*
* 09.08.2019 R.Schick
*  create beelogger_at24cxx.h
*  remove static declaration of functions
*  modify constructor
*
*/

#ifndef beelogger_AT24Cxx_h
#define beelogger_AT24Cxx_h


#define AT24Cxx_CTRL_ID_def 0x57 // default I2C address

class AT24Cxx
{
  // user-accessible "public" interface
  public:
    AT24Cxx(int addr_i2c);
    
	bool isPresent(void);      // check if the device is present
    int ReadMem(int iAddr, char Buf[], int iCnt);
    uint8_t WriteMem(int iAddr, uint8_t iVal);
    uint8_t WriteMem(int iAddr, const char *pBuf, int iCnt);

    int     ReadStr(int iAddr, char Buf[], int iBufLen);
    uint8_t WriteStr(int iAddr, const char *pBuf);

  private:
    int _i2c_addr;
};
#endif
