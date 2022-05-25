#include <avr/pgmspace.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <wifiESP.h>

#define MAX_PACKET_SIZE 768

wifiESP::wifiESP(bool readConfig)
{
    if (readConfig)
        _readConfigEEPROM();
}

void wifiESP::flushSerial(void)
{
    //char ch_aux;
    // Serial flush
    Serial.flush();
    /* while (Serial.available() > 0)
        ch_aux = Serial.read(); */
}

uint8_t wifiESP::_readConfigEEPROM(void)
{
    EEPROM.get(0, WifiConfig);
}

uint8_t wifiESP::_writeWifiConfigEEPROM(uint8_t status, char ssid[30], char pass[30], char proxy[30], unsigned int port)
{
    WifiConfig.status = status;  // Status=1 -> configured
    strcpy(WifiConfig.ssid, ssid);
    strcpy(WifiConfig.pass, pass);
    strcpy(WifiConfig.proxy, proxy);
    WifiConfig.port = port;
    EEPROM.put(0, WifiConfig);
}
// Read a new char and rotate buffer (5 char buffer)
uint8_t wifiESP::_readChar(char c[5])
{
    if (Serial.available()) {
        c[4] = c[3];
        c[3] = c[2];
        c[2] = c[1];
        c[1] = c[0];
        c[0] = Serial.read();
        return 1;
    }
    else
        return 0; 
}


// Wait for response (max 5 characters)
uint8_t wifiESP::_waitFor(const char *stopstr, unsigned int timeout_secs)
{
    char c[5];
    bool found = false;
    long timer_init;

    timer_init = millis();
    while (!found) {
        if (((millis() - timer_init) / 1000) > timeout_secs) { // Timeout?
            return 0;  // timeout
        }
        _readChar(c);
        uint8_t stopstrSize = strlen(stopstr);
        if (stopstrSize > 5)
            stopstrSize = 5;
        found = true;
        for (uint8_t i = 0; i < stopstrSize; i++) {
            if (c[i] != stopstr[stopstrSize - 1 - i]) {
                found = false;
                break;
            }
        }
        if (found) {
            flushSerial();
            Serial.println();
        }
    } // end while (!found)
    //delay(250);
    return 1;
}

// Wait for response (max 5 characters)
uint8_t wifiESP::_waitFor2(const char *stopstr, const char *stopstr2, unsigned int timeout_secs)
{
    char c[5];
    uint8_t found = 0;
    long timer_init;
    uint8_t stopstrSize;

    timer_init = millis();
    while (found == 0) {
        if (((millis() - timer_init) / 1000) > timeout_secs) { // Timeout?
        return 0;  // timeout
        }
        _readChar(c);
        stopstrSize = strlen(stopstr);
        if (stopstrSize > 5)
            stopstrSize = 5;
        found = 1;
        for (uint8_t i = 0; i < stopstrSize; i++) {
            if (c[i] != stopstr[stopstrSize - 1 - i]) {
                found = 0;
                break;
            }
        }
        if (found == 0) {
            stopstrSize = strlen(stopstr2);
            if (stopstrSize > 5)
                stopstrSize = 5;
            found = 2;
            for (uint8_t i = 0; i < stopstrSize; i++) {
                if (c[i] != stopstr2[stopstrSize - 1 - i]) {
                    found = 0;
                    break;
                }
            }
        }
        if (found > 0) {
        delay(20);
        flushSerial();
        Serial.println();
        }
    } // end while (!found)
    //delay(250);
    return found;
}

// getMacAddress from ESP wifi module
uint8_t wifiESP::getMac(char *MAC)
{
    char c1, c2;
    bool timeout = false;
    long timer_init;
    uint8_t state = 0;
    uint8_t index = 0;

    timer_init = millis();
    while (!timeout) {
        if (((millis() - timer_init) / 1000) > 4) // Timeout 4 seconds
            timeout = true;
        if (Serial.available()) {
            c2 = c1;
            c1 = Serial.read();
            switch (state) {
                case 0:
                    if (c1 == ':')
                        state = 1;
                    break;
                case 1:
                    if (c1 == '\r')
                        state = 2;
                    else {
                        if ((c1 != '"') && (c1 != ':')) {
                        if (index < 12)
                            MAC[index++] = toupper(c1);  // Uppercase
                        }
                    }
                    break;
                case 2:
                    if ((c2 == 'O') && (c1 == 'K')) {
                        Serial.println();
                        Serial.flush();
                        MAC[12] = '\0';
                        return 1;  // Ok
                    }
                    break;
            } // end switch
        } // Serial_available
    } // while (!timeout)
    
    Serial.flush();
    return -1;  // timeout
}

// Wait for a response message example: +IPD,2:OK  or +IPD,768:af4aedqead...
// It detects the message using the code1:4009 and detects the end because the connection close
// Return the size of the message (-1: timeout, 2: OK, <=768: data packet)
// Function return: 0 reading message 1: message readed 2: timeout
uint8_t wifiESP::_waitforMessage(uint8_t timeout_secs)
{
    char ch;

    if (((millis() - message_timer_init) / 1000) > timeout_secs) {
        message_readed = 2; // timeout
        return 2;
    }
    while (Serial.available()) {
        ch = Serial.read();
        mc7 = mc6; mc6 = mc5; mc5 = mc4; mc4 = mc3; mc3 = mc2; mc2 = mc1; mc1 = mc0; mc0 = ch; // Hardcoding this rotate buffer is more efficient
        //DebugSerial.print(" ");
        //DebugSerial.println(ch);
        if ((mc5 == 'C') && (mc4 == 'L') && (mc3 == 'O') && (mc2 == 'S') && (mc1 == 'E') && (mc0 == 'D')) {
            //DebugSerial.println(F("CCLOSED!"));
            Serial.flush();
            message_readed = 1;
            if (message_index_buffer > 0)
                message_size = message_index_buffer;
            else {
                // In case of no packet, we return the last two characters of the message (OK,ER...)
                message_size = 2;
                buffer[0] = mc7;
                buffer[1] = mc6;
            }
            return 1;
        }
        // State machine
        switch (message_status) {
        case 0:
            // Waiting for +IPD,...
            if ((mc4 == '+') && (mc3 == 'I') && (mc2 == 'P') && (mc1 == 'D') && (mc0 == ',')) {
                message_chunked = false;
                message_size = 0;
                message_index = 0;
                message_index_buffer = 0;
                message_status = 1;
            }
            break;
        case 1:
            // Reading message size
            if (ch == ':') {
                //DebugSerial.print("SIZE:");
                //DebugSerial.println(message_size);
                if (message_size > 1460) {
                    //DebugSerial.println(F("!PACKET_SIZE_ERROR"));
                    return -1;
                }
                message_status = 2;
            } else
                message_size = message_size * 10 + int(ch - '0');
                break;
        case 2:
            message_index++;
            if (message_index >= message_size) {
                //DebugSerial.println("END");
                message_status = 0;
                break;
            }
            // Detecting packet start 4009 4001 (FA9,FA1)(mc2=0xFA,mc1=0x9F,mc0=0xA1)
            if ((uint8_t(mc2) == 0b11111010) && (uint8_t(mc1) == 0b10011111) && (uint8_t(mc0) == 0b10100001)) {
                //DebugSerial.println(F("Packet start!"));
                buffer[0] = mc2;
                buffer[1] = mc1;
                buffer[2] = mc0;
                message_index_buffer = 3;
                message_status = 3;
            }
            break;
        case 3:
            message_index++;
            if (message_index > message_size) {
                //DebugSerial.println("END");
                message_status = 0;
                break;
            }
            // Reading packet
            if (message_index_buffer < MAX_PACKET_SIZE) {
                //DebugSerial.print(ch);
                buffer[message_index_buffer] = ch;
                message_index_buffer++;
            } else {
                //DebugSerial.println(F("Error: message too large!"));
                return 2;  // Error
            }
            break;
        }
    }
    return 0;  // Reading...
}

// This function sends an http GET request to an url
// it uses the configuration stored on WifiConfig global variable
uint8_t wifiESP::sendHTTP(char *url)
{
  char cmd_get[160];
  char cmd_send[15];
  char strSize[6];
  char strPort[6];

  //DebugSerial.print("Free RAM:");
  //DebugSerial.println(freeRam());
  // Open TCP connection on port 80
  strcpy(cmd_get, "AT+CIPSTART=\"TCP\",\"");
  if ((WifiConfig.port > 0) && (WifiConfig.port < 65000) && (strlen(WifiConfig.proxy) > 0)) { // Connection with proxy?
    strcat(cmd_get, WifiConfig.proxy);
    strcat(cmd_get, "\",");
    sprintf(strPort, "%d", WifiConfig.port);
    strcat(cmd_get, strPort);
  }
  else {  // Standard HTTP connection (direct to host on port 80)
    strcat(cmd_get, SERVER_HOST);
    strcat(cmd_get, "\",80");
  }
  Serial.println(cmd_get);
  if (ESPwaitFor("OK", 5))
  {
    strcpy(cmd_get, "GET ");
    strcat(cmd_get, url);
    strcat(cmd_get, " HTTP/1.1\r\nHost:");
    strcat(cmd_get, SERVER_HOST);
    strcat(cmd_get, "\r\nConnection: close\r\n\r\n");
    sprintf(strSize, "%d", strlen(cmd_get));
    strcpy(cmd_send, "AT+CIPSEND=");
    strcat(cmd_send, strSize);
    Serial.println(cmd_send);
    ESPwaitFor(">", 3);
    Serial.println(cmd_get);
    //DebugSerial.print("SEND:");
    //DebugSerial.println(cmd_get);
    ESPwaitFor("OK", 5);
    return 1;
  }
  else {
    digitalWrite(13,LOW);
    DebugSerial.println(F("Connection error"));
    Serial.println("AT+CIFSR");
    ESPwaitFor("OK", 5);
    Serial.println("AT+CIPCLOSE");
    ESPwaitFor("OK", 5);
    delay(4000);  // delay on error...
    digitalWrite(13,HIGH);
    return 0;
  }
}
