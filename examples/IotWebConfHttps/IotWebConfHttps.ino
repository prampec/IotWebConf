#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266HTTPUpdateServer.h>

#include <IotWebConf.h>

const char SSL_key[] PROGMEM = R"=====(
YOUR PRIVATE KEY HERE
)=====";

const char SSL_cert[] PROGMEM = R"=====(
YOUR SERT FILE HERE
)=====";

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "testThing";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "smrtTHNG8266";

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "dem1"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN D2

DNSServer dnsServer;
WebServer serverHTTP(80);
WebServerSecure server(443);
HTTPUpdateServerSecure httpUpdater;

IotWebConfSecure iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);


void secureRedirect() {
  serverHTTP.sendHeader("Location", String("https://") + WiFi.localIP().toString(), true);
  serverHTTP.send(301, "text/plain", "");
}

void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 04 Update Server</title></head><body>Hello world!";
  s += "Go to <a href='config'>configure page</a> to change values.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting up...");

  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.setupUpdateServer(&httpUpdater);

  // -- Initializing the configuration.
  iotWebConf.init();

  serverHTTP.on("/", secureRedirect);
  serverHTTP.begin();

  server.getServer().setRSACert(new BearSSL::X509List(SSL_cert), new BearSSL::PrivateKey(SSL_key));

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });
  server.begin();

  Serial.println("Ready.");

}

void loop() {
  serverHTTP.handleClient();
  iotWebConf.doLoop();
}
