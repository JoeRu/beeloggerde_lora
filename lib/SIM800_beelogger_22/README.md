This library provides easy access to SIM800 based GSM/GPRS module by implementing straight-forward APIs for HTTP communication and some network features. 
Original by Stanley Huang <stanleyhuangyc@gmail.com> http://arduinodev.com
Modified by Thorsten Gurzan <info@beelogger.de> https://beelogger.de for beelogger-Solar GSM
Modified by R. Schick for beelogger-solar GSM v2.0

Library API
-----------

Initialize the module

    bool init();

Setup network

    byte setup(const char* apn);

Get network operator name

    bool getOperatorName();


Check for incoming SMS

    bool checkSMS();

Get signal quality level (in dB)

    int getSignalQuality();

Get GSM location and network time

    bool getLocation(GSM_LOCATION* loc);


Initialize HTTP connection

    bool httpInit();

Terminate HTTP connection

    void httpUninit();

Connect to HTTP server

    bool httpConnect(const char* url, const char* args = 0);

Check if HTTP connection is established (returns 0 for in progress, 1 for success, 2 for error)

    byte httpIsConnected();

Read data from HTTP connection

    void httpRead();

Check if HTTP connection is established (returns 0 for in progress, -1 for error, bytes of http payload on success)

    int httpIsRead();

Toggle low-power mode

    bool sleep(bool enabled);

