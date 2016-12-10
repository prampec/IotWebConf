/**
 * File: network.ino
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
 * This is just helper file to separate Wifi and AP specific functions from the main logic.
 */

void checkApTimeout()
{
  if ((config.wifiSSID[0] != '\0') && (config.apPassword[0] != '\0'))
  {
    // -- Only move on, when we have a valid Wifi and AP configured.
    if ((apConnectionStatus == AP_CONNECTION_STATUS_DC) || 
    ((apStartTimeMs + AP_MODE_TIMEOUT_MS) < millis()) && (apConnectionStatus != AP_CONNECTION_STATUS_C))
    // TODO: millis() will overflow some times
    {
      changeState(STATE_WIFI_CLIENT_MODE);
    }
  }
}

/**
 * Checks whether we have anyone joined to our AP. 
 * If so, we must not change state. But when our gest leaved, we can immediatelly move on.
 */
void checkConnection()
{
  if ((apConnectionStatus == AP_CONNECTION_STATUS_NC) && (WiFi.softAPgetStationNum() > 0))
  {
    apConnectionStatus = AP_CONNECTION_STATUS_C;
    Serial.println("Connection to AP.");
  }
  else if ((apConnectionStatus == AP_CONNECTION_STATUS_C) && (WiFi.softAPgetStationNum() == 0))
  {
    apConnectionStatus = AP_CONNECTION_STATUS_DC;
    Serial.println("Disconnected from AP.");
  }
}

boolean setupWifi()
{
  Serial.print("Connecting to [");
  Serial.print(config.wifiSSID);
  Serial.print("]");
  WiFi.begin(config.wifiSSID, config.wifiPassword);

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (retryCount >= WIFI_RETRY_COUNT)
    {
      // -- Wifi not available, fall back to AP mode.
      Serial.println("Giving up.");
      WiFi.disconnect();
      changeState(STATE_AP_MODE);
      return false;
    }
    retryCount += 1;
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  return true;
}

void setupAp()
{
  WiFi.mode(WIFI_AP);
  
  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "Thing-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = "ESP8266 Thing " + macID;
  
  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, AP_NameString.length() + 1, 0);
  
  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);
    
  AP_NameChar[AP_NameString.length()]='\0';

  Serial.print("Setting up AP: ");
  Serial.println(AP_NameChar);
  if (state == STATE_NOT_CONFIGURED)
  {
    Serial.print("With default password: ");
    Serial.println(WiFiAPPSK);
    WiFi.softAP(AP_NameChar, WiFiAPPSK);
  }
  else
  {
    Serial.print("Use password: ");
    Serial.println(config.apPassword);
    WiFi.softAP(AP_NameChar, config.apPassword);
  }
}
