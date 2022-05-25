#include <avr/pgmspace.h>

#define WIFICONFIG_MAXLEN 30

class wifiESP
{
    private:
        //EEPROM object to store Wifi configuration
        struct WifiConfigS {
            uint8_t status;
            char ssid[WIFICONFIG_MAXLEN];
            char pass[WIFICONFIG_MAXLEN];
            char proxy[WIFICONFIG_MAXLEN];
            unsigned int port;
        };

        WifiConfigS WifiConfig;

        char MAC[13];  // MAC address of Wifi module

        // Global variables for message reading
        long message_timer_init;
        uint8_t message_readed;
        int message_index;
        int message_index_buffer;
        int message_size;
        bool message_chunked;
        uint8_t message_status;
        char mc0,mc1,mc2,mc3,mc4,mc5,mc6,mc7;  // Last 5 chars from message for parsing...
        unsigned char buffer[780];   // buffer to store incomming message from server

        uint8_t _readConfigEEPROM(void);
        uint8_t _writeWifiConfigEEPROM(uint8_t status, char ssid[30], char pass[30], char proxy[30], unsigned int port);
        uint8_t _readChar(char c[5]);
        uint8_t _waitFor(const char *stopstr, unsigned int timeout_secs);
        uint8_t _waitFor2(const char *stopstr, const char *stopstr2, unsigned int timeout_secs);
        uint8_t _waitforMessage(uint8_t timeout_secs);

    public:
        // public methods
        wifiESP(bool readConfig);
        ~wifiESP() {};
        void flushSerial(void);
        uint8_t getMac(char *MAC);
        uint8_t sendHTTP(char *url);
};
