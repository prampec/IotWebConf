/**
 * File: IotWebConf.ino
 * Description:
 * IotWebConf is an ESP8266 stand alone WiFi/AP web configuration template for Arduino.
 *
 * Author: Balazs Kelemen
 * Contact: prampec+arduino@gmail.com
 * Copyright: 2012 Balazs Kelemen
 * Copying permission statement:
    This file is part of IotWebConf.
    PciManager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * See https://github.com/prampec/IotWebConf for details.
 */
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TM1637Display.h>

// -- TM1637 clock connected to D2, D3 pins.
#define CLK D2
#define DIO D3

// -- Default password to connect to the Thing, when it creates an own Access Point.
const char WiFiAPPSK[] = "smrtTHNG8266";

// -- Configuration specific part
#define CONFIG_VERSION "Mxw1"
#define WORD_LEN 32
// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the default password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN D0

typedef struct Config {
  char version[5];
  char apPassword[WORD_LEN];
  char wifiSSID[WORD_LEN];
  char wifiPassword[WORD_LEN];
  // -- Custom configuration section below
  char ntpServerName[256];
  short gmtOffset;
  boolean dstOn; // -- Day light saving is currently on?
};

Config config {
  CONFIG_VERSION,
  "\0",
  "\0",
  "\0",
  // -- Custom configuration defaults below
  "time.nist.gov",
  0,
  false
};

Config newConfig {
  CONFIG_VERSION,
  "\0",
  "\0",
  "\0",
  // -- Custom configuration section below
  "\0",
  0,
  false
};

// -- State of the Thing
#define STATE_BOOT             0
#define STATE_NOT_CONFIGURED   1
#define STATE_AP_MODE          2
#define STATE_WIFI_CLIENT_MODE 3
#define STATE_WIFI_SERVER_MODE 4
byte state = STATE_BOOT;

// -- Access pont specific configuration
// -- Thin will stay in AP mode for AP_MODE_TIMEOUT_MS on boot, and before retrying to connect to Wifi.
#define AP_MODE_TIMEOUT_MS 30000
unsigned long apStartTimeMs = 0;

#define AP_CONNECTION_STATUS_NC 0
#define AP_CONNECTION_STATUS_C  1
#define AP_CONNECTION_STATUS_DC 2
byte apConnectionStatus = AP_CONNECTION_STATUS_NC;

// -- How often we should disturb the server mode with some local action.
#define CLIENT_REPEAT_INTERVAL_MS 100
unsigned long lastClientRun = 0;

// -- Wifi specific data
// -- Should try to connect to the local network WIFI_RETRY_COUNT times before falling back to AP mode.
#define WIFI_RETRY_COUNT 60
WiFiServer server(80);

// -- Custom variables below
#define LOCAL_UDP_PORT 2390UL
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp;
// -- Do not ask for NTP data more frequently than NTP_ASK_INTERVAL_MS
#define NTP_ASK_INTERVAL_MS 600000L
unsigned long lastNtpAsked = 0;
TM1637Display display(CLK, DIO);
unsigned long epoch = 0;
unsigned long millisAtNtpResponse;
boolean hasFreshTime = false;

void setup() 
{
  initConfig(&config);
  initHardware();
  loadConfig(&config);
}

/**
 * States are here to different job in each state.
 */
void loop() 
{
  if (state == STATE_BOOT)
  {
    // -- After boot, fall immediatelly to AP mode.
    changeState(STATE_AP_MODE);
  }
  else if (state == STATE_NOT_CONFIGURED)
  {
    // -- If wifi is not configured, we should provide web interface for the user.
    webServerLoop();
  }
  else if (state == STATE_AP_MODE)
  {
    // -- We must only leave the AP mode, when no slaves are connected.
    // -- Other than that AP mode has a timeout. E.g. after boot, or when retry connecting to WiFi
    checkConnection();
    checkApTimeout();
    webServerLoop();
  }
  else if (state == STATE_WIFI_SERVER_MODE)
  {
    // -- In server mode we provide web interface. And check whether it is time to run the client.
    webServerLoop();
    checkClientRepeatInterval();
  }
  else if (state == STATE_WIFI_CLIENT_MODE)
  {
    // -- Run our custom logic here.
    clientLoop();
    lastClientRun = millis();
  }
}

/**
 * What happens, when a state changed...
 */
void changeState(byte newState)
{
  switch(newState)
  {
    case STATE_AP_MODE:
    {
      // -- In AP mode we must override the default AP password. Othervise we stay in STATE_NOT_CONFIGURED.
      boolean forceDefaultPassword = (digitalRead(CONFIG_PIN) == LOW);
      if (forceDefaultPassword || (config.apPassword[0] == '\0'))
      {
        newState = STATE_NOT_CONFIGURED;
      }
      setupAp();
      server.begin();
      apConnectionStatus = AP_CONNECTION_STATUS_NC;
      apStartTimeMs = millis();
      break;
    }
    case STATE_WIFI_SERVER_MODE:
      if (state != STATE_WIFI_CLIENT_MODE)
      {
        if (!setupWifi())
        {
          return;
        }
      }
      server.begin();
      Serial.println("Accepting connection");
      break;
    case STATE_WIFI_CLIENT_MODE:
      if (state != STATE_WIFI_SERVER_MODE)
      {
        if (!setupWifi())
        {
          return;
        }
      }
      initClient();
      break;
  }
  Serial.print("State changed from: ");
  Serial.print(state);
  Serial.print(" to ");
  Serial.println(newState);
  state = newState;
}

/**
 * Custom logic are usually do not need to run very frequently.
 */
void checkClientRepeatInterval()
{
  if((lastClientRun + CLIENT_REPEAT_INTERVAL_MS) < millis()) // -- TODO: millis() will overflow some times
  {
    if(beforeEnterClientMode())
    {
      changeState(STATE_WIFI_CLIENT_MODE);
    }
  }
}

void initClient()
{
}

/**
 * Return TRUE to enter client mode. Returning FALSE means, we do not have to enter client mode this time.
 */
boolean beforeEnterClientMode()
{
  displayActualTime();
  // -- Should enter client mode, when no fresh time available, or some time already passed since last NTP.
  return !hasFreshTime || ((lastNtpAsked + NTP_ASK_INTERVAL_MS) < millis());
}

void displayActualTime()
{
  if (epoch > 0)
  {
    unsigned long epoch2 = epoch + (millis() - millisAtNtpResponse) / 1000;
    unsigned long time = (epoch2  % 86400L) / 3600;
    time *= 100;
    time += (epoch2  % 3600) / 60;
    // -- Display double dot
//    display.showNumberDecEx(time, (epoch2 %= 2) ? 0 : (0x80 >> 1));
    displayTime(time, epoch2 %= 2);
  }
}

/**
 * We display time while waiting.
 */
void delay2(unsigned long ms)
{
  unsigned long start = millis();

  while( start + ms > millis() ) // -- TODO: millis will overflow some times
  {
    displayActualTime();
    delay(10);
  }
}

void clientLoop()
{
  hasFreshTime = false;
  if (epoch == 0)
  {
    // -- Display IP on screen for the first time
    uint8_t data[] = { 0, 0, 0, 0 };

    IPAddress ip = WiFi.localIP();
    display.showNumberDec(ip[0]);
    delay(1400);
    display.setSegments(data);
    delay(100);
    display.showNumberDec(ip[1]);
    delay(1400);
    display.setSegments(data);
    delay(100);
    display.showNumberDec(ip[2]);
    delay(1400);
    display.setSegments(data);
    delay(100);
    display.showNumberDec(ip[3]);
    delay(1400);
    uint8_t data2[] = { SEG_G, SEG_G, SEG_G, SEG_G };
    display.setSegments(data2);
  }
  int retryCount = 0;
  while (true)
  {
    Serial.println("Starting UDP");
    int ret = udp.begin(LOCAL_UDP_PORT);
    if (ret)
    {
      break;
    }
    if (retryCount > 10)
    {
      Serial.println("Giving up.");
      changeState(STATE_WIFI_SERVER_MODE);
      return;
    }
    retryCount += 1;
  }
  
  //get server from the pool
  retryCount = 0;
  int cb;
  while(true)
  {
    if (retryCount > 15)
    {
      Serial.println("Giving up.");
      break;
    }

    IPAddress timeServerIP; // NTP server address
    Serial.print("Ip address of ");
    Serial.print(config.ntpServerName);
    Serial.print(" is ... ");
    WiFi.hostByName(config.ntpServerName, timeServerIP);
    Serial.println(timeServerIP);
    if (timeServerIP == INADDR_NONE) 
    {
      retryCount += 1;
      continue;
    }

    Serial.println("Requesting NTP");
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay2(1000);
    
    cb = udp.parsePacket();
    if (cb)
    {
      break;
    }
    Serial.println("No packet this time.");
    retryCount += 1;
    delay2(100);
  }

  if(cb) {
    millisAtNtpResponse = millis();
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    // -- Add the GMT offset and the DST to the epoch.
    epoch += (config.gmtOffset + (config.dstOn ? 1:0)) * 3600L;
    hasFreshTime = true;

    // print the hour, minute and second:
    Serial.print("The local time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }
  udp.stop();
  // wait some seconds before asking for the time again
  lastNtpAsked = millis();
  changeState(STATE_WIFI_SERVER_MODE);
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void webServerLoop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  
  // Prepare the response. Start with the common header:
  String s = "HTTP/1.1 200 OK\r\n";
  s += "Content-Type: text/html\r\n\r\n";
  s += "<!DOCTYPE HTML>\r\n<html><head></head><body>\r\n";

  if (req.indexOf("/config") != -1)
  {
    flushTillBody(&client);
    readData(&client);

    readValue("apPassword", newConfig.apPassword, WORD_LEN);
    if (newConfig.apPassword[0] == '\0')
    {
      // -- No value was set.
      strncpy(newConfig.apPassword, config.apPassword, WORD_LEN);
    }
    readValue("wifiSSID", newConfig.wifiSSID, WORD_LEN);
    readValue("wifiPassword", newConfig.wifiPassword, WORD_LEN);
    if (newConfig.wifiPassword[0] == '\0')
    {
      // -- No value was set.
      strncpy(newConfig.wifiPassword, config.wifiPassword, WORD_LEN);
    }
    // -- Custom configuration section below
    readValue("ntpServerName", newConfig.ntpServerName, WORD_LEN);
    char temp[4];
    readValue("gmtOffset", temp, WORD_LEN);
    newConfig.gmtOffset = atoi(temp);
    readValue("dstOn", temp, 4);
    newConfig.dstOn = (strncmp("On", temp, 3) == 0);

    saveConfig(&newConfig);
    loadConfig(&config);

    s += "Configuration saved.";
    if (config.apPassword[0] == '\0')
    {
      s += " You must change the default AP password to continue.";
    }
    else if (config.wifiSSID[0] == '\0')
    {
      s += " You must provide the local wifi settings to continue.";
    }
    else
    {
      " Please disconnect from WifiAP or";
    }
    s += " return to <a href='/'>configuration page</a>.";
  }
  else
  {
    s += "<form method=post action='/config'>";
    s += "AP password:<br/><input type='text' name='apPassword'><br/>";
    s += "Wifi SSID:<br/><input type='text' name='wifiSSID' value='";
    s += config.wifiSSID;
    s += "'><br/>";
    s += "Wifi password:<br/><input type='text' name='wifiPassword'><br/>";
    // -- Custom configuration section below
    s += "NTP server:<br/><input type='text' name='ntpServerName' value='";
    s += config.ntpServerName;
    s += "'><br/><hr/>";
    s += "GMT offset:<br/><input type='number' name='gmtOffset' value='";
    s += config.gmtOffset;
    s += "' min=-12 max=12><br/>";
    s += "Day light saving (DST):<br/>";
    s += "<select name='dstOn'><option value='On'";
    s += config.dstOn ? " selected>" : ">";
    s += "Summer time</option>";
    s += "<option value='Off'";
    s += config.dstOn ? ">" : " selected>";
    s += "Off</option>";
    s += "</select><br/>";
    s += "<input type='submit' value='Save'><br/>";
    s += "</form>";
  }

  s += "</body></html>\n";

  // Send the response to the client
  client.flush();
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

void initHardware()
{
  Serial.begin(115200);
  Serial.println();

  pinMode(CONFIG_PIN, INPUT_PULLUP);

  display.setBrightness(0x0f);
  uint8_t data[] = { SEG_G, SEG_G, SEG_G, SEG_G };
  display.setSegments(data);
}

void displayTime(unsigned long number, boolean dot)
{
  uint8_t digits[] = { 0, 0, 0, 0 };
  digits[3] = display.encodeDigit(number % 10);
  number /= 10;
  digits[2] = display.encodeDigit(number % 10);
  number /= 10;
  uint8_t d = display.encodeDigit(number % 10);
  if (dot)
  {
    d = addDot(d);
  }
  digits[1] = d;
  number /= 10;
  int m = number % 10;
  if (m > 0) {
    digits[0] = display.encodeDigit(m);
  }
  display.setSegments(digits, 4, 0);
}

uint8_t addDot(uint8_t digit)
{
  return digit | 0x80;
}

