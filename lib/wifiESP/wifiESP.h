#include <avr/pgmspace.h>
#include <Arduino.h>
// Needed libraries
#include <SoftwareSerial.h>

#define CNC_SHIELD_RX_PIN A5
#define CNC_SHIELD_TX_PIN A4
#define WIFI_SERIAL_SPEED 57600
#define WIFICONFIG_MAXLEN 30

#define SERVER_HOST "ibb.jjrobots.com"
#define SERVER_URL "http://ibb.jjrobots.com/ibbsvr/ibb.php"

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

        void _flushSerial(void);
        uint8_t _readChar(char c[5]);
        void _wait(unsigned int timeout_secs);
        uint8_t _waitFor(const char *stopstr, unsigned int timeout_secs);
        uint8_t _waitFor2(const char *stopstr, const char *stopstr2, unsigned int timeout_secs);
        uint8_t _waitforMessage(uint8_t timeout_secs);
        uint8_t _extractGETParam(char *param, char ch);

    public:
        // public methods
        wifiESP(bool readConfig);
        ~wifiESP() {};
        
        void readConfigEEPROM(void);
        void writeWifiConfigEEPROM(uint8_t status, char ssid[30], char pass[30], char proxy[30], unsigned int port);
        void webServerConfig(void);
        void configWeb(uint8_t tcp_ch);
        void configWeb_advanced(uint8_t tcp_ch);
        void configWebOK(uint8_t tcp_ch);
        uint8_t getMac(char *MAC);
        uint8_t sendHTTP(char *url);
        bool connect(void);
        void wifiConfigurationMode(void);

        char* getSSID(void);
        char* getPASS(void);
        char* getProxy(void);
        unsigned int getPort(void);

        uint8_t getStatus(void);

        void setCredentials(char* ssid, char* pass);
        void setStatus(uint8_t status);
        void setPort(unsigned int port);
        void setProxy(char* proxy);

        uint8_t reset(void);
        void sendRaw(char msg);
        int available(void);
        int read(void);
};
