#include <avr/pgmspace.h>
#include <Arduino.h>
#include <EEPROM.h>

#include <wifiESP.h>

#define MAX_PACKET_SIZE 768

SoftwareSerial WifiSerial(CNC_SHIELD_RX_PIN, CNC_SHIELD_TX_PIN);

// Some util functions...
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

// Urldecode function from https://gist.github.com/jmsaavedra
// Updated to decode '+' as spaces (typical on form parameters)
void urldecode2(char *dst, const char *src)
{
  char a, b;
  while (*src) {
    if ((*src == '%') &&
        ((a = src[1]) && (b = src[2])) &&
        (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a')
        a -= 'a' - 'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a' - 'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16 * a + b;
      src += 3;
    }
    else {
      if (*src == '+') {
        *dst++ = ' ';    // whitespaces encoded '+'
        src++;
      }
      else
        *dst++ = *src++;
    }
  }
  *dst++ = '\0';
}


wifiESP::wifiESP(bool readConfig)
{
    WifiSerial.begin(19200);
    WifiConfig.status = 0;
    strcpy(WifiConfig.proxy, "");
    strcpy(WifiConfig.pass, "");
    strcpy(WifiConfig.ssid, "");
    WifiConfig.port = 0;
    if (readConfig)
        readConfigEEPROM();

}

void wifiESP::_flushSerial(void)
{
    //char ch_aux;
    // Serial flush
    WifiSerial.flush();
    /* while (Serial.available() > 0)
        ch_aux = Serial.read(); */
}

void wifiESP::readConfigEEPROM(void)
{
    EEPROM.get(0, WifiConfig);
}

void wifiESP::writeWifiConfigEEPROM(uint8_t status, char ssid[30], char pass[30], char proxy[30], unsigned int port)
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
    if (WifiSerial.available()) {
        c[4] = c[3];
        c[3] = c[2];
        c[2] = c[1];
        c[1] = c[0];
        c[0] = WifiSerial.read();
        return 1;
    }
    else
        return 0; 
}

// ESP functions...
void wifiESP::_wait(unsigned int timeout_secs)
{
  char c;
  long timer_init;
  uint8_t timeout = 0;

  timer_init = millis();
  while (!timeout) {
    if (((millis() - timer_init) / 1000) > timeout_secs) { // Timeout?
      timeout = 1;
    }
    if (WifiSerial.available()) {
      c = WifiSerial.read();
      Serial.print(c);
    }
  }
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
            _flushSerial();
            //WifiSerial.println();
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
        _flushSerial();
        WifiSerial.println();
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
        if (WifiSerial.available()) {
            c2 = c1;
            c1 = WifiSerial.read();
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
                        WifiSerial.println();
                        WifiSerial.flush();
                        MAC[12] = '\0';
                        return 1;  // Ok
                    }
                    break;
            } // end switch
        } // Serial_available
    } // while (!timeout)
    
    WifiSerial.flush();
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
    while (WifiSerial.available()) {
        ch = WifiSerial.read();
        mc7 = mc6; mc6 = mc5; mc5 = mc4; mc4 = mc3; mc3 = mc2; mc2 = mc1; mc1 = mc0; mc0 = ch; // Hardcoding this rotate buffer is more efficient
        Serial.print(" ");
        Serial.println(ch);
        if ((mc5 == 'C') && (mc4 == 'L') && (mc3 == 'O') && (mc2 == 'S') && (mc1 == 'E') && (mc0 == 'D')) {
            Serial.println(F("CCLOSED!"));
            WifiSerial.flush();
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
                    Serial.print("SIZE:");
                    Serial.println(message_size);
                    if (message_size > 1460) {
                        Serial.println(F("!PACKET_SIZE_ERROR"));
                        return -1;
                    }
                    message_status = 2;
                } else {
                    message_size = message_size * 10 + int(ch - '0');
                }
                    break;
            case 2:
                message_index++;
                if (message_index >= message_size) {
                    Serial.println("END");
                    message_status = 0;
                    break;
                }
                // Detecting packet start 4009 4001 (FA9,FA1)(mc2=0xFA,mc1=0x9F,mc0=0xA1)
                if ((uint8_t(mc2) == 0b11111010) && (uint8_t(mc1) == 0b10011111) && (uint8_t(mc0) == 0b10100001)) {
                    Serial.println(F("Packet start!"));
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
                    Serial.println("END");
                    message_status = 0;
                    break;
                }
                // Reading packet
                if (message_index_buffer < MAX_PACKET_SIZE) {
                    Serial.print(ch);
                    buffer[message_index_buffer] = ch;
                    message_index_buffer++;
                } else {
                    Serial.println(F("Error: message too large!"));
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

    Serial.print("Free RAM:");
    Serial.println(freeRam());
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
    if (_waitFor("OK", 5))
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
        _waitFor(">", 3);
        Serial.println(cmd_get);
        //DebugSerial.print("SEND:");
        //DebugSerial.println(cmd_get);
        _waitFor("OK", 5);
        return 1;
    }
    else {
        digitalWrite(13,LOW);
        Serial.println(F("Connection error"));
        WifiSerial.println("AT+CIFSR");
        _waitFor("OK", 5);
        WifiSerial.println("AT+CIPCLOSE");
        _waitFor("OK", 5);
        delay(4000);  // delay on error...
        digitalWrite(13,HIGH);
        return 0;
    }
}

// Extract parameter from GET string  ?param1=xxx&param2=yyy
// return value: 0 while extracting, 1: Extracted OK, 2: Error
// First time: param should be initialized ('\0') externally
uint8_t wifiESP::_extractGETParam(char *param, char ch)
{
  static uint8_t index;

  if ((ch == '\n') || (ch == ' ') || (ch == '&')) { // End of param?
    index = 0;
    return 1;
  }

  if ((index == 0) && (ch == '=')) { // end of param name?
    index = 1;  // increase index to know that we are reading the parameter
  }
  else if (index > 0) {    // extracting param...
    param[index - 1] = ch;
    param[index] = '\0';
    index++;
    if (index >= WIFICONFIG_MAXLEN) {
      Serial.println("Error!:Param too large");
      index = 0;
      return 2;   // Error: param too large!
    }
  }
  return 0;
}

// Mini WEB SERVER to CONFIG the WIFI parameters: SSID, passeword and optionally proxy and port
// If the server receive an url with parameters: decode it and store on EEPROM
// If the server receive any other thing: show the wifi config form page
void wifiESP::webServerConfig(void)
{
    char ch;
    uint8_t tcp_ch = 0;
    uint8_t result;
    bool configured = false;
    uint8_t webserver_status = 0;
    char user_param[WIFICONFIG_MAXLEN];

    while (!configured) {
        // Led blink on wifi config...
        if ((millis() / 100) % 2 == 0)
            digitalWrite(13, HIGH);
        else
            digitalWrite(13, LOW);

        while (WifiSerial.available()) {
            ch = WifiSerial.read();
            Serial.print(ch);
            // State machine
            switch (webserver_status) {
                case 0:
                    // Waiting for +IPD,...
                    mc4 = mc3; mc3 = mc2; mc2 = mc1; mc1 = mc0; mc0 = ch; // Hardcoding this rotate buffer is more efficient
                    if ((mc4 == '+') && (mc3 == 'I') && (mc2 == 'P') && (mc1 == 'D') && (mc0 == ',')) {
                        webserver_status = 1;
                    }
                    break;
                case 1:
                    // Read channel
                    tcp_ch = ch - '0';
                    Serial.print("->ch:");
                    Serial.println(tcp_ch);
                    webserver_status = 2;
                case 2:
                    // Check the page requested (/ or /config)
                    // If we found an "?" on the first line then this is the configuration page
                    if (ch == '\n')
                        webserver_status = 3;
                    if (ch == 'v')
                        webserver_status = 9;
                    if (ch == '?') {
                        user_param[0] = '\0';
                        webserver_status = 4;
                    }
                    break;
                case 3:
                    // Webserver root => Show configuration page
                    Serial.println();
                    Serial.println("->Config page");
                    delay(20);
                    _flushSerial();
                    configWeb(tcp_ch);
                    delay(20);
                    _flushSerial();
                    delay(50);
                    _flushSerial();
                    webserver_status = 0;
                    break;
                case 9:
                    // Webserver root => Show configuration page
                    Serial.println();
                    Serial.println("->Advanced page");
                    delay(20);
                    _flushSerial();
                    configWeb_advanced(tcp_ch);
                    delay(20);
                    _flushSerial();
                    delay(50);
                    _flushSerial();
                    webserver_status = 0;
                    break;
                case 4:
                    Serial.println("->P1");
                    result = _extractGETParam(user_param, ch);
                    if (result == 1) { // Ok => extraemos siguiente parametro
                        urldecode2(WifiConfig.ssid, user_param);
                        user_param[0] = '\0';
                        webserver_status = 5;
                    }
                    if (result == 2) // Error => Show config page again...
                        webserver_status = 3;
                    break;
                case 5:
                    Serial.println("->P2");
                    result = _extractGETParam(user_param, ch);
                    if (result == 1) { // Ok => extraemos siguiente parametro
                        urldecode2(WifiConfig.pass, user_param);
                        user_param[0] = '\0';
                        webserver_status = 6;
                    }
                    if (result == 2) // Error => Show config page again...
                        webserver_status = 3;
                    break;
                case 6:
                    Serial.println("->P3");
                    result = _extractGETParam(user_param, ch);
                    if (result == 1) { // Ok => extraemos siguiente parametro
                        urldecode2(WifiConfig.proxy, user_param);
                        Serial.print("->proxy=");
                        Serial.println(user_param);
                        user_param[0] = '\0';
                        webserver_status = 7;
                    }
                    if (result == 2) // Error => Show config page again...
                        webserver_status = 3;
                    break;
                case 7:
                    Serial.println("->P4:");
                    result = _extractGETParam(user_param, ch);
                    if (result == 1) { // Ok => extraemos siguiente parametro
                        if (strlen(user_param) > 0)
                        WifiConfig.port = atoi(user_param);
                        else
                        WifiConfig.port = 0;
                        //urldecode2(WifiConfig.pass, user_param);
                        Serial.print("->port=");
                        Serial.println(user_param);
                        user_param[0] = '\0';
                        webserver_status = 8;
                    }
                    if (result == 2) // Error => Show config page again...
                        webserver_status = 3;
                    break;
                case 8:
                    _flushSerial();
                    delay(50);
                    _flushSerial();
                    configWebOK(tcp_ch);   // OK webpage to user
                    delay(100);
                    _flushSerial();
                    configured = true;
                    break;
                default:
                    webserver_status = 3;
            }  // end switch(webserverstatus)
        }   // while WifiSerial.available
    } // while !configured
}

// HTML Page to Config Wifi parameters: SSID, password and optional: proxy and port
// We use html static pages with precalculated sizes (memory optimization)
void wifiESP::configWeb(uint8_t tcp_ch)
{
    WifiSerial.print("AT+CIPSEND=");
    WifiSerial.print(tcp_ch);
    WifiSerial.println(",573");   // Header length: 84 Content length: 489 = 573
    _waitFor(">", 3);
    WifiSerial.println(F("HTTP/1.1 200 OK")); //15+2
    WifiSerial.println(F("Content-Type: text/html")); //23+2
    WifiSerial.println(F("Connection: close")); //17+2
    WifiSerial.println(F("Content-Length: 489")); //19+2
    WifiSerial.println();  //2  => total: 17+25+19+21+2 = 84
    delay(10);
    WifiSerial.print(F("<!DOCTYPE HTML><html><head><meta name='viewport' content='width=device-width'>")); //78
    WifiSerial.print(F("<body><h3>Wifi Configuration Page</h3><form method='get' action='config'>")); //73
    WifiSerial.print(F("<label>SSID:</label><br><input type='text' name='ssid' size='30'><br>")); //69
    WifiSerial.print(F("<label>Password:</label><br><input type='password' name='password' size='30'><br>")); //81
    WifiSerial.print(F("<br><a href='av'>advanced</a><input hidden type='text' name='proxy' value=''><input hidden type='text' name='port' value=''>")); //124
    WifiSerial.print(F("<br><br><input type='submit' value='SEND!'></form></body></html>")); //60
    // Total=78+73+69+81+124+64= 489
    _waitFor("OK", 5);
    delay(100);
    // Close the connection from server side (safety)
    Serial.println("->CIPCLOSE");
    WifiSerial.print("AT+CIPCLOSE=");
    WifiSerial.println(tcp_ch);
    Serial.print("Free RAM:");
    Serial.println(freeRam());
}

// HTML Page to Config Wifi parameters: SSID, password and optional: proxy and port
// We use html static pages with precalculated sizes (memory optimization)
void wifiESP::configWeb_advanced(uint8_t tcp_ch)
{
    WifiSerial.print("AT+CIPSEND=");
    WifiSerial.print(tcp_ch);
    WifiSerial.println(",573");   // Header length: 84 Content length: 489 = 573
    _waitFor(">", 3);
    WifiSerial.println(F("HTTP/1.1 200 OK")); //15+2
    WifiSerial.println(F("Content-Type: text/html")); //23+2
    WifiSerial.println(F("Connection: close")); //17+2
    WifiSerial.println(F("Content-Length: 489")); //19+2
    WifiSerial.println();  //2  => total: 17+25+19+21+2 = 84
    delay(10);
    WifiSerial.print(F("<!DOCTYPE HTML><html><head><meta name='viewport' content='width=device-width'>")); //78
    WifiSerial.print(F("<body><h3>Wifi Configuration Page</h3><form method='get' action='config'>")); //73
    WifiSerial.print(F("<label>SSID:</label><br><input type='text' name='ssid' size='30'><br>")); //69
    WifiSerial.print(F("<label>Password:</label><br><input type='password' name='password' size='30'><br>")); //81
    WifiSerial.print(F("<i><h5>OPTIONAL:</h5>Proxy:<input type='text' name='proxy' size='20'>&nbsp;port:<input type='text' name='port' size='6'></i>")); //124
    WifiSerial.print(F("<br><br><input type='submit' value='SEND!'></form></body></html>")); //60
    // Total=78+73+69+81+124+64= 489
    _waitFor("OK", 5);
    delay(100);
    // Close the connection from server side (safety)
    Serial.println("->CIPCLOSE");
    WifiSerial.print("AT+CIPCLOSE=");
    WifiSerial.println(tcp_ch);
    Serial.print("Free RAM:");
    Serial.println(freeRam());
}

// HTML page showing that Wifi is configured OK
// This page has a button to continue the wizard
void wifiESP::configWebOK(uint8_t tcp_ch)
{
    WifiSerial.print("AT+CIPSEND=");
    WifiSerial.print(tcp_ch);
    WifiSerial.println(",388");   // Header length: 84 Content length: 304 = 388
    _waitFor(">", 3);
    WifiSerial.println(F("HTTP/1.1 200 OK")); //15+2
    WifiSerial.println(F("Content-Type: text/html")); //23+2
    WifiSerial.println(F("Connection: close")); //17+2
    WifiSerial.println(F("Content-Length: 304")); //19+2
    WifiSerial.println();  //2  => total: 17+25+19+21+2 = 84
    delay(10);

    WifiSerial.print(F("<!DOCTYPE HTML><html><head><meta name='viewport' content='width=device-width, user-scalable=no'>")); //96
    WifiSerial.print(F("<body><h3>ID:&nbsp")); //18
    WifiSerial.print(MAC); //12
    WifiSerial.print(F("</h3><h2>Wifi Configured!</h2><button onclick=\"location.href='http://ibbapp.jjrobots.com/wizard/wizard2.php?ID_IWBB=")); //112
    WifiSerial.print(MAC); //12
    WifiSerial.print(F("';\">Go to Test and Registration</button></body></html>")); //54
    // total = 96+18+12+112+12+54 = 304

    Serial.println("->Wait OK");
    _waitFor("OK", 5);
    Serial.println("->OK");
    delay(100);
    // Close the connection from server side (safety)
    Serial.println("->CIPCLOSE");
    WifiSerial.print("AT+CIPCLOSE=");
    WifiSerial.println(tcp_ch);
    Serial.print("Free RAM:");
    Serial.println(freeRam());
}

bool wifiESP::connect(void)
{
    WifiSerial.println(F("AT+CWQAP"));
    _waitFor("OK", 5);
    delay(1000);
    WifiSerial.println(F("AT+CWMODE=1"));   // Station mode
    _waitFor("OK", 3);
    delay(1500);
    Serial.println(F("Connecting to Wifi network..."));
    WifiSerial.print(F("AT+CWJAP=\""));
    WifiSerial.print(WifiConfig.ssid);
    WifiSerial.print("\",\"");
    WifiSerial.print(WifiConfig.pass);
    WifiSerial.println("\"");
    if (_waitFor2("OK", "DISCO", 14) == 1)
        return true;
    else
        return false;
}

void wifiESP::wifiConfigurationMode(void)
{
    delay(2000);
    Serial.println(F("Wifi Configuration mode..."));
    WifiSerial.println();
    _flushSerial();
    WifiSerial.println(F("AT+RST"));
    _waitFor("ready", 10);
    WifiSerial.println(F("ATE0"));
    _waitFor("OK", 4);

    //DebugSerial.println(F("AT+CWMODE=1"));   // Station mode
    //ESPwaitFor("OK", 3);
    //DebugSerial.println(F("AT+CWLAP"));
    //ESPwaitFor("OK",3);
    //DebugSerial.println(F("AT+CWLAP"));
    //ESPgetAPlist();

    delay(50);
    Serial.println(">AT+CWMODE=2");
    WifiSerial.println(F("AT+CWMODE=2"));   // Soft AP mode
    Serial.println(_waitFor("OK", 3));
    Serial.println(">AT+CIPSTAMAC?");
    WifiSerial.println(F("AT+CIPSTAMAC?"));
    getMac(MAC);
    Serial.println(">AT+CWSAP=...");
    WifiSerial.println(F("AT+CWSAP=\"JJROBOTS_IBB\",\"87654321\",5,3"));
    Serial.println(_waitFor("OK", 6));
    Serial.println(">AT+CIPMUX=1");
    WifiSerial.println(F("AT+CIPMUX=1"));
    Serial.println(_waitFor("OK", 3));
    Serial.println(">AT+CIPSERVER=1,80");
    WifiSerial.println(F("AT+CIPSERVER=1,80"));
    Serial.println(_waitFor("OK", 4));

    Serial.println();
    Serial.println(F("Instructions:"));
    Serial.println(F("->Connect to JJROBOTS_IBB wifi network, password: 87654321"));
    Serial.println(F("->Open page: http://192.168.4.1"));
    Serial.println(F("->Fill SSID and PASSWORD of your Wifi network and press SEND!"));
    // Show web server configuration page until user introduce the configuration
    webServerConfig();
    delay(500);
    _flushSerial();
    WifiConfig.status = 1;
    // Store on EEPROM!
    // Default Host and URL
    EEPROM.put(0, WifiConfig);
    Serial.println(F("  Configured!!"));
    Serial.print(F("SSID:"));
    Serial.println(WifiConfig.ssid);
    Serial.print(F("PASS :"));
    Serial.println(WifiConfig.pass);
    Serial.println(F("HOST : "));
    Serial.println(SERVER_HOST);
    Serial.println(F("URL : "));
    Serial.println(SERVER_URL);
    Serial.print(F("PROXY : "));
    Serial.println(WifiConfig.proxy);
    Serial.print(F("PORT : "));
    Serial.println(WifiConfig.port);


    WifiSerial.println(F("AT+CWQAP"));
    _waitFor("OK", 5);
    WifiSerial.println(F("AT+CWMODE=1"));   // Station mode
    _waitFor("OK", 3);
    WifiSerial.println(F("AT+RST"));
    _waitFor("ready", 10);
    delay(1000);
}


char* wifiESP::getSSID(void)
{
    return WifiConfig.ssid;
}

char* wifiESP::getPASS(void)
{
    return WifiConfig.pass;
}

uint8_t wifiESP::getStatus(void)
{
    return WifiConfig.status;
}

char* wifiESP::getProxy(void)
{
    return WifiConfig.proxy;
}

void wifiESP::setCredentials(char* ssid, char* pass)
{
    strcpy(WifiConfig.ssid, ssid);
    strcpy(WifiConfig.pass, pass);
}

void wifiESP::setStatus(uint8_t status)
{
    WifiConfig.status = status;
}

unsigned int wifiESP::getPort(void)
{
    return WifiConfig.port;
}
void wifiESP::setPort(unsigned int port)
{
    WifiConfig.port = port;
}

void wifiESP::setProxy(char* proxy)
{
    strcpy(WifiConfig.proxy, proxy);
}

uint8_t wifiESP::reset(void)
{
    // Wifi initialization
    WifiSerial.println();
    _flushSerial();
    WifiSerial.println(F("AT+RST"));
    _waitFor("ready", 10);
    WifiSerial.println(F("AT+GMR"));
    return _waitFor("OK", 5);
}

void wifiESP::sendRaw(char msg)
{
    WifiSerial.write(msg);
}

int wifiESP::available(void)
{
    return WifiSerial.available();
}

int wifiESP::read(void)
{
    return WifiSerial.read();
}