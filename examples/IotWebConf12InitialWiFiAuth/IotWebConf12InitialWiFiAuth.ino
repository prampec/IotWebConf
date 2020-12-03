/**
 * IotWebConf12InitialWiFiAuth.ino -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf 
 *
 * Copyright (C) 2018 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/**
 * Example: Initial WiFi Auth info
 * Description:
 *   This example shows how to provide default WiFi authentication info so that your
 *   "Thing" will connect automatically to your WiFi Router/Access Point when EEPROM
 *   configuration values do not exist or the configuration version has changed causing 
 *   the saved values to be invalidated.
 * 
 *   Your "Thing" will boot and read stored values from EEPROM. If saved WiFi 
 *   Authentication info is not found in EEPROM then these default values will be used. 
 * 
 *   If this default/initial authentication info works then your IOT device will be
 *   assigned a local IP address and can be configured further using the web interface.
 *   The default/initial authentication info will be retained in the web interface and 
 *   saved to EEPROM when the configuration form is submitted. 
 * 
 * 
 *   If the default/initial authentication info does not work then the IOT device will
 *   revert back to AP mode using IP "192.168.4.1". The default/initial authentication
 *   info will be erased and will require input using the configuration form. 
 * 
 *   Note that you can find detailed debug information in the serial console depending
 *   on the settings IOTWEBCONF_DEBUG_TO_SERIAL, IOTWEBCONF_DEBUG_PWD_TO_SERIAL set
 *   in the IotWebConf.h .
 */

#include <IotWebConf.h>

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "testThing";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "smrtTHNG8266";

DNSServer dnsServer;
WebServer server(80);

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);
// -- Set the default/initial WiFi authentication info used to connect to your WiFi
// -- Router or Access Point.
IotWebConfWifiAuthInfo wifiAuthInfoDefault{"yourWiFiSSID", "yourWiFiPassword"};

void setup() 
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting up...");

  // -- Initializing the configuration.
  iotWebConf.init();
  // -- Initialize default Wifi authentication info. 
  iotWebConf.setInitialWifiAuthInfo(&wifiAuthInfoDefault);
  
  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  Serial.println("Ready.");
}

void loop() 
{
  // -- doLoop should be called as frequently as possible.
  iotWebConf.doLoop();
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 12 Initial WiFi Authentication</title></head><body>Hello world!";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

