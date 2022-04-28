/**
 * IotWebConf.cpp -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2020 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <EEPROM.h>

#include "IotWebConf.h"

#ifdef IOTWEBCONF_CONFIG_USE_MDNS
# ifdef ESP8266
#  include <ESP8266mDNS.h>
# elif defined(ESP32)
#  include <ESPmDNS.h>
# endif
#endif

#define IOTWEBCONF_STATUS_ENABLED ((this->_statusPin >= 0) && this->_blinkEnabled)

////////////////////////////////////////////////////////////////

namespace iotwebconf
{

IotWebConf::IotWebConf(
    const char* defaultThingName, DNSServer* dnsServer, WebServerWrapper* webServerWrapper,
    const char* initialApPassword, const char* configVersion)
{
  this->_thingNameParameter.defaultValue = defaultThingName;
  this->_dnsServer = dnsServer;
  this->_webServerWrapper = webServerWrapper;
  this->_initialApPassword = initialApPassword;
  this->_configVersion = configVersion;

  this->_apTimeoutParameter.visible = false;
  this->_systemParameters.addItem(&this->_thingNameParameter);
  this->_systemParameters.addItem(&this->_apPasswordParameter);
  this->_systemParameters.addItem(&this->_wifiParameters);
  this->_systemParameters.addItem(&this->_apTimeoutParameter);

  this->_allParameters.addItem(&this->_systemParameters);
  this->_allParameters.addItem(&this->_customParameterGroups);
  this->_allParameters.addItem(&this->_hiddenParameters);

  this->_wifiAuthInfo = {this->_wifiParameters._wifiSsid, this->_wifiParameters._wifiPassword};
}

char* IotWebConf::getThingName()
{
  return this->_thingName;
}

void IotWebConf::setConfigPin(int configPin)
{
  this->_configPin = configPin;
}

void IotWebConf::setStatusPin(int statusPin, int statusOnLevel)
{
  this->_statusPin = statusPin;
  this->_statusOnLevel = statusOnLevel;
}

bool IotWebConf::init()
{
  // -- Setup pins.
  if (this->_configPin >= 0)
  {
    pinMode(this->_configPin, INPUT_PULLUP);
    this->_forceDefaultPassword = (digitalRead(this->_configPin) == LOW);
  }
  if (IOTWEBCONF_STATUS_ENABLED)
  {
    pinMode(this->_statusPin, OUTPUT);
    digitalWrite(this->_statusPin, !this->_statusOnLevel);
  }

  // -- Load configuration from EEPROM.
  bool validConfig = this->loadConfig();
  if (!validConfig)
  {
    // -- No config
    this->_apPassword[0] = '\0';
    this->_wifiParameters._wifiSsid[0] = '\0';
    this->_wifiParameters._wifiPassword[0] = '\0';
  }
  this->_apTimeoutMs = atoi(this->_apTimeoutStr) * 1000;

  return validConfig;
}

//////////////////////////////////////////////////////////////////

void IotWebConf::addParameterGroup(ParameterGroup* group)
{
  this->_customParameterGroups.addItem(group);
}

void IotWebConf::addHiddenParameter(ConfigItem* parameter)
{
  this->_hiddenParameters.addItem(parameter);
}

void IotWebConf::addSystemParameter(ConfigItem* parameter)
{
  this->_systemParameters.addItem(parameter);
}

int IotWebConf::initConfig()
{
  int size = this->_allParameters.getStorageSize();
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print("Config version: ");
  Serial.println(this->_configVersion);
  Serial.print("Config size: ");
  Serial.println(size);
#endif

  return size;
}

/**
 * Load the configuration from the eeprom.
 */
bool IotWebConf::loadConfig()
{
  int size = this->initConfig();
  EEPROM.begin(
    IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VERSION_LENGTH + size);

  bool result;
  if (this->testConfigVersion())
  {
    int start = IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VERSION_LENGTH;
    IOTWEBCONF_DEBUG_LINE(F("Loading configurations"));
    this->_allParameters.loadValue([&](SerializationData* serializationData)
    {
        this->readEepromValue(start, serializationData->data, serializationData->length);
        start += serializationData->length;
    });
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    this->_allParameters.debugTo(&Serial);
#endif
    result = true;
  }
  else
  {
    IOTWEBCONF_DEBUG_LINE(F("Wrong config version. Applying defaults."));
    this->_allParameters.applyDefaultValue();
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    this->_allParameters.debugTo(&Serial);
#endif

    result = false;
  }

  EEPROM.end();
  return result;
}

void IotWebConf::saveConfig()
{
  int size = this->initConfig();
  if (this->_configSavingCallback != nullptr)
  {
    this->_configSavingCallback(size);
  }
  EEPROM.begin(
    IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VERSION_LENGTH + size);

  this->saveConfigVersion();
  int start = IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VERSION_LENGTH;
  IOTWEBCONF_DEBUG_LINE(F("Saving configuration"));
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  this->_allParameters.debugTo(&Serial);
  Serial.println();
#endif
  this->_allParameters.storeValue([&](SerializationData* serializationData)
  {
    this->writeEepromValue(start, serializationData->data, serializationData->length);
    start += serializationData->length;
  });

  EEPROM.end();

  this->_apTimeoutMs = atoi(this->_apTimeoutStr) * 1000;

  if (this->_configSavedCallback != nullptr)
  {
    this->_configSavedCallback();
  }
}

void IotWebConf::readEepromValue(int start, byte* valueBuffer, int length)
{
  for (int t = 0; t < length; t++)
  {
    *((char*)valueBuffer + t) = EEPROM.read(start + t);
  }
}
void IotWebConf::writeEepromValue(int start, byte* valueBuffer, int length)
{
  for (int t = 0; t < length; t++)
  {
    EEPROM.write(start + t, *((char*)valueBuffer + t));
  }
}

bool IotWebConf::testConfigVersion()
{
  for (byte t = 0; t < IOTWEBCONF_CONFIG_VERSION_LENGTH; t++)
  {
    if (EEPROM.read(IOTWEBCONF_CONFIG_START + t) != this->_configVersion[t])
    {
      return false;
    }
  }
  return true;
}

void IotWebConf::saveConfigVersion()
{
  for (byte t = 0; t < IOTWEBCONF_CONFIG_VERSION_LENGTH; t++)
  {
    EEPROM.write(IOTWEBCONF_CONFIG_START + t, this->_configVersion[t]);
  }
}

void IotWebConf::setWifiConnectionCallback(std::function<void()> func)
{
  this->_wifiConnectionCallback = func;
}

void IotWebConf::setConfigSavingCallback(std::function<void(int size)> func)
{
  this->_configSavingCallback = func;
}

void IotWebConf::setConfigSavedCallback(std::function<void()> func)
{
  this->_configSavedCallback = func;
}

void IotWebConf::setFormValidator(
  std::function<bool(WebRequestWrapper* webRequestWrapper)> func)
{
  this->_formValidator = func;
}

void IotWebConf::setWifiConnectionTimeoutMs(unsigned long millis)
{
  this->_wifiConnectionTimeoutMs = millis;
}

////////////////////////////////////////////////////////////////////////////////

void IotWebConf::handleConfig(WebRequestWrapper* webRequestWrapper)
{
  if (this->_state == OnLine)
  {
    // -- Authenticate
    if (!webRequestWrapper->authenticate(
            IOTWEBCONF_ADMIN_USER_NAME, this->_apPassword))
    {
      IOTWEBCONF_DEBUG_LINE(F("Requesting authentication."));
      webRequestWrapper->requestAuthentication();
      return;
    }
  }

  bool dataArrived = webRequestWrapper->hasArg("iotSave");
  if (!dataArrived || !this->validateForm(webRequestWrapper))
  {
    // -- Display config portal
    IOTWEBCONF_DEBUG_LINE(F("Configuration page requested."));

    // Send chunked output instead of one String, to avoid
    // filling memory if using many parameters.
    webRequestWrapper->sendHeader(
        "Cache-Control", "no-cache, no-store, must-revalidate");
    webRequestWrapper->sendHeader("Pragma", "no-cache");
    webRequestWrapper->sendHeader("Expires", "-1");
    webRequestWrapper->setContentLength(CONTENT_LENGTH_UNKNOWN);
    webRequestWrapper->send(200, "text/html; charset=UTF-8", "");

    String content = htmlFormatProvider->getHead();
    content.replace("{v}", "Config ESP");
    content += htmlFormatProvider->getScript();
    content += htmlFormatProvider->getStyle();
    content += htmlFormatProvider->getHeadExtension();
    content += htmlFormatProvider->getHeadEnd();

    content += htmlFormatProvider->getFormStart();

    webRequestWrapper->sendContent(content);

#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.println("Rendering parameters:");
    this->_systemParameters.debugTo(&Serial);
    this->_customParameterGroups.debugTo(&Serial);
#endif
    // -- Add parameters to the form
    this->_systemParameters.renderHtml(dataArrived, webRequestWrapper);
    this->_customParameterGroups.renderHtml(dataArrived, webRequestWrapper);

    content = htmlFormatProvider->getFormEnd();

    if (this->_updatePath != nullptr)
    {
      String pitem = htmlFormatProvider->getUpdate();
      pitem.replace("{u}", this->_updatePath);
      content += pitem;
    }

    // -- Fill config version string;
    {
      String pitem = htmlFormatProvider->getConfigVer();
      pitem.replace("{v}", this->_configVersion);
      content += pitem;
    }

    content += htmlFormatProvider->getEnd();

    webRequestWrapper->sendContent(content);
    webRequestWrapper->sendContent(F(""));
    webRequestWrapper->stop();
  }
  else
  {
    // -- Save config
    IOTWEBCONF_DEBUG_LINE(F("Updating configuration"));
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    this->_systemParameters.debugTo(&Serial);
    this->_customParameterGroups.debugTo(&Serial);
    Serial.println();
#endif
    this->_systemParameters.update(webRequestWrapper);
    this->_customParameterGroups.update(webRequestWrapper);

    this->saveConfig();

    String page = htmlFormatProvider->getHead();
    page.replace("{v}", "Config ESP");
    page += htmlFormatProvider->getScript();
    page += htmlFormatProvider->getStyle();
//    page += _customHeadElement;
    page += htmlFormatProvider->getHeadExtension();
    page += htmlFormatProvider->getHeadEnd();
    page += "Configuration saved. ";
    if (this->_apPassword[0] == '\0')
    {
      page += F("You must change the default AP password to continue. Return "
                "to <a href=''>configuration page</a>.");
    }
    else if (this->_wifiParameters._wifiSsid[0] == '\0')
    {
      page += F("You must provide the local wifi settings to continue. Return "
                "to <a href=''>configuration page</a>.");
    }
    else if (this->_state == NotConfigured)
    {
      page += F("Please disconnect from WiFi AP to continue!");
    }
    else
    {
      page += F("Return to <a href='/'>home page</a>.");
    }
    page += htmlFormatProvider->getEnd();

    webRequestWrapper->sendHeader("Content-Length", String(page.length()));
    webRequestWrapper->send(200, "text/html; charset=UTF-8", page);
  }
}

bool IotWebConf::validateForm(WebRequestWrapper* webRequestWrapper)
{
  // -- Clean previous error messages.
  this->_systemParameters.clearErrorMessage();
  this->_customParameterGroups.clearErrorMessage();

  // -- Call external validator.
  bool valid = true;
  if (this->_formValidator != nullptr)
  {
    valid = this->_formValidator(webRequestWrapper);
  }

  // -- Internal validation.
  int l = webRequestWrapper->arg(this->_thingNameParameter.getId()).length();
  if (3 > l)
  {
    this->_thingNameParameter.errorMessage =
        "Give a name with at least 3 characters.";
    valid = false;
  }
  l = webRequestWrapper->arg(this->_apPasswordParameter.getId()).length();
  if ((0 < l) && (l < 8))
  {
    this->_apPasswordParameter.errorMessage =
        "Password length must be at least 8 characters.";
    valid = false;
  }
  l = webRequestWrapper->arg(this->_wifiParameters.wifiPasswordParameter.getId()).length();
  if ((0 < l) && (l < 8))
  {
    this->_wifiParameters.wifiPasswordParameter.errorMessage =
        "Password length must be at least 8 characters.";
    valid = false;
  }

#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print(F("Form validation result is: "));
  Serial.println(valid ? "positive" : "negative");
#endif

  return valid;
}

void IotWebConf::handleNotFound(WebRequestWrapper* webRequestWrapper)
{
  if (this->handleCaptivePortal(webRequestWrapper))
  {
    // If captive portal redirect instead of displaying the error page.
    return;
  }
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print(F("Requested a non-existing page '"));
  Serial.print(webRequestWrapper->uri());
  Serial.println("'");
#endif
  String message = "Requested a non-existing page\n\n";
  message += "URI: ";
  message += webRequestWrapper->uri();
  message += "\n";

  webRequestWrapper->sendHeader(
      "Cache-Control", "no-cache, no-store, must-revalidate");
  webRequestWrapper->sendHeader("Pragma", "no-cache");
  webRequestWrapper->sendHeader("Expires", "-1");
  webRequestWrapper->sendHeader("Content-Length", String(message.length()));
  webRequestWrapper->send(404, "text/plain", message);
}

/**
 * Redirect to captive portal if we got a request for another domain.
 * Return true in that case so the page handler do not try to handle the request
 * again. (Code from WifiManager project.)
 */
bool IotWebConf::handleCaptivePortal(WebRequestWrapper* webRequestWrapper)
{
  String host = webRequestWrapper->hostHeader();
  String thingName = String(this->_thingName);
  thingName.toLowerCase();
  if (!isIp(host) && !host.startsWith(thingName))
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print("Request for ");
    Serial.print(host);
    Serial.print(" redirected to ");
    Serial.print(webRequestWrapper->localIP());
    Serial.print(":");
    Serial.println(webRequestWrapper->localPort());
#endif
    webRequestWrapper->sendHeader(
      "Location", String("http://") + toStringIp(webRequestWrapper->localIP()) + ":" + webRequestWrapper->localPort(), true);
    webRequestWrapper->send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    webRequestWrapper->stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Is this an IP? */
bool IotWebConf::isIp(String str)
{
  for (size_t i = 0; i < str.length(); i++)
  {
    int c = str.charAt(i);
    if (c != '.' && c != ':' && (c < '0' || c > '9'))
    {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String IotWebConf::toStringIp(IPAddress ip)
{
  String res = "";
  for (int i = 0; i < 3; i++)
  {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

/////////////////////////////////////////////////////////////////////////////////

void IotWebConf::delay(unsigned long m)
{
  unsigned long delayStart = millis();
  while (m > millis() - delayStart)
  {
    this->doLoop();
    // -- Note: 1ms might not be enough to perform a full yield. So
    // 'yield' in 'doLoop' is eventually a good idea.
    delayMicroseconds(1000);
  }
}

void IotWebConf::doLoop()
{
  doBlink();
  yield(); // -- Yield should not be necessary, but cannot hurt either.
  if (this->_state == Boot)
  {
    // -- After boot, fall immediately to AP mode.
    NetworkState startupState = ApMode;
    if (this->_startupOffLine)
    {
      startupState = OffLine;
    }
    else if (this->_skipApStartup)
    {
      if (mustStayInApMode())
      {
        IOTWEBCONF_DEBUG_LINE(
            F("SkipApStartup is requested, but either no WiFi was set up, or "
              "configButton was pressed."));
      }
      else
      {
        // -- Startup state can be WiFi, if it is requested and also possible.
        IOTWEBCONF_DEBUG_LINE(F("SkipApStartup mode was applied"));
        startupState = Connecting;
      }
    }
    this->changeState(startupState);
  }
  else if (
      (this->_state == NotConfigured) ||
      (this->_state == ApMode))
  {
    // -- We must only leave the AP mode, when no slaves are connected.
    // -- Other than that AP mode has a timeout. E.g. after boot, or when retry
    // connecting to WiFi
    checkConnection();
    checkApTimeout();
    this->_dnsServer->processNextRequest();
    this->_webServerWrapper->handleClient();
  }
  else if (this->_state == Connecting)
  {
    if (checkWifiConnection())
    {
      this->changeState(OnLine);
      return;
    }
  }
  else if (this->_state == OnLine)
  {
    // Required for mDNS to work on ESP8266
#ifdef IOTWEBCONF_CONFIG_USE_MDNS
# ifdef ESP8266
    MDNS.update();
# endif
#endif
    // -- In server mode we provide web interface. And check whether it is time
    // to run the client.
    this->_webServerWrapper->handleClient();
    if (WiFi.status() != WL_CONNECTED)
    {
      IOTWEBCONF_DEBUG_LINE(F("Not connected. Try reconnect..."));
      this->changeState(Connecting);
      return;
    }
  }
}

/**
 * What happens, when a state changed...
 */
void IotWebConf::changeState(NetworkState newState)
{
  switch (newState)
  {
    case ApMode:
    {
      // -- In AP mode we must override the default AP password. Otherwise we stay
      // in STATE_NOT_CONFIGURED.
      if (mustUseDefaultPassword())
      {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
        if (this->_forceDefaultPassword)
        {
          Serial.println("AP mode forced by reset pin");
        }
        else
        {
          Serial.println("AP password was not set in configuration");
        }
#endif
        newState = NotConfigured;
      }
      break;
    }
    default:
      break;
  }
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print("State changing from: ");
  Serial.print(this->_state);
  Serial.print(" to ");
  Serial.println(newState);
#endif
  NetworkState oldState = this->_state;
  this->_state = newState;
  this->stateChanged(oldState, newState);
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print("State changed from: ");
  Serial.print(oldState);
  Serial.print(" to ");
  Serial.println(newState);
#endif
}

/**
 * Cleanly stopping mDNS after network failure.
 */
void IotWebConf::endMDns(NetworkState oldState)
{
#ifdef IOTWEBCONF_CONFIG_USE_MDNS
  if (oldState == OnLine)
  {
    MDNS.end();
    IOTWEBCONF_DEBUG_LINE(F("Deactivated mDNS until reconnected to WiFi."));
  }
#endif
}

/**
 * What happens, when a state changed...
 */
void IotWebConf::stateChanged(NetworkState oldState, NetworkState newState)
{
//  updateOutput();
  switch (newState)
  {
    case OffLine:
      endMDns(oldState);
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      this->blinkInternal(22000, 6);
      break;
    case ApMode:
    case NotConfigured:
      if (newState == ApMode)
      {
        this->blinkInternal(300, 90);
      }
      else
      {
        this->blinkInternal(300, 50);
      }
      if ((oldState == Connecting) ||
        (oldState == OnLine))
      {
        endMDns(oldState);
        WiFi.disconnect(true);
      }
      setupAp();
      if (this->_updateServerSetupFunction != nullptr)
      {
        this->_updateServerSetupFunction(this->_updatePath);
      }
      this->_webServerWrapper->begin();
      this->_apConnectionState = NoConnections;
      this->_apStartTimeMs = millis();
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      if (mustStayInApMode())
      {
        if (this->_forceDefaultPassword)
        {
          Serial.println(F("Default password was forced."));
        }
        if (this->_apPassword[0] == '\0')
        {
          Serial.println(F("AP password was not set."));
        }
        if (this->_wifiParameters._wifiSsid[0] == '\0')
        {
          Serial.println(F("WiFi SSID was not set."));
        }
        if (this->_forceApMode)
        {
          Serial.println(F("AP was forced."));
        }
        Serial.println(F("Will stay in AP mode."));
      }
      else
      {
        Serial.print(F("AP timeout (ms): "));
        Serial.println(this->_apTimeoutMs);
      }
#endif
      break;
    case Connecting:
      if ((oldState == ApMode) ||
          (oldState == NotConfigured))
      {
        stopAp();
      }
      if ((oldState == Boot) && (this->_updateServerSetupFunction != nullptr))
      {
        // We've skipped AP mode, so update server needs to be set up now.
        this->_updateServerSetupFunction(this->_updatePath);
      }
      endMDns(oldState);
      this->blinkInternal(1000, 50);
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.print("Connecting to [");
      Serial.print(this->_wifiAuthInfo.ssid);
# ifdef IOTWEBCONF_DEBUG_PWD_TO_SERIAL
      Serial.print("] with password [");
      Serial.print(this->_wifiAuthInfo.password);
      Serial.println("]");
# else
      Serial.println(F("] (password is hidden)"));
# endif
      Serial.print(F("WiFi timeout (ms): "));
      Serial.println(this->_wifiConnectionTimeoutMs);
#endif
      this->_wifiConnectionStart = millis();
      // The order of WiFi.mode and WiFi.setHostname matters based on the platform
#ifdef ESP8266
      WiFi.mode(WIFI_STA);
      WiFi.setHostname(this->_thingName);
#elif defined(ESP32)
      WiFi.setHostname(this->_thingName);
      WiFi.mode(WIFI_STA);
#endif
      this->_wifiConnectionHandler(
          this->_wifiAuthInfo.ssid, this->_wifiAuthInfo.password);
      break;
    case OnLine:
      // -- Initialize mdns after network connection
#ifdef IOTWEBCONF_CONFIG_USE_MDNS
      MDNS.begin(this->_thingName);
      MDNS.addService("http", "tcp", IOTWEBCONF_CONFIG_USE_MDNS);
# ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.printf("Active mDNS services: %d \n", MDNS.queryService("http", "tcp"));
# endif
#endif
      this->blinkInternal(8000, 2);
      if (this->_updateServerUpdateCredentialsFunction != nullptr)
      {
        this->_updateServerUpdateCredentialsFunction(
            IOTWEBCONF_ADMIN_USER_NAME, this->_apPassword);
      }
      this->_webServerWrapper->begin();
      IOTWEBCONF_DEBUG_LINE(F("Accepting connection"));
      if (this->_wifiConnectionCallback != nullptr)
      {
        this->_wifiConnectionCallback();
      }
      break;
    default:
      break;
  }
}

void IotWebConf::checkApTimeout()
{
  if ( !mustStayInApMode() )
  {
    // -- Only move on, when we have a valid WifF and AP configured.
    if ((this->_apConnectionState == Disconnected) ||
        (((millis() - this->_apStartTimeMs) > this->_apTimeoutMs) &&
         (this->_apConnectionState != HasConnection)))
    {
      this->changeState(Connecting);
    }
  }
}

void IotWebConf::goOnLine(bool apMode)
{
  if (this->_state != OffLine)
  {
    IOTWEBCONF_DEBUG_LINE(F("Requested OnLine mode, but was not offline."));
    return;
  }
  if (apMode || mustStayInApMode())
  {
    this->changeState(ApMode);
  }
  else
  {
    this->changeState(Connecting);
  }
}

/**
 * Checks whether we have anyone joined to our AP.
 * If so, we must not change state. But when our guest leaved, we can
 * immediately move on.
 */
void IotWebConf::checkConnection()
{
  if ((this->_apConnectionState == NoConnections) &&
      (WiFi.softAPgetStationNum() > 0))
  {
    this->_apConnectionState = HasConnection;
    IOTWEBCONF_DEBUG_LINE(F("Connection to AP."));
  }
  else if (
      (this->_apConnectionState == HasConnection) &&
      (WiFi.softAPgetStationNum() == 0))
  {
    this->_apConnectionState = Disconnected;
    IOTWEBCONF_DEBUG_LINE(F("Disconnected from AP."));
    if (this->_forceDefaultPassword)
    {
      IOTWEBCONF_DEBUG_LINE(F("Releasing forced AP mode."));
      this->_forceDefaultPassword = false;
    }
  }
}

bool IotWebConf::checkWifiConnection()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    if ((millis() - this->_wifiConnectionStart) > this->_wifiConnectionTimeoutMs)
    {
      // -- WiFi not available, fall back to AP mode.
      IOTWEBCONF_DEBUG_LINE(F("Giving up."));
      WiFi.disconnect(true);
      WifiAuthInfo* newWifiAuthInfo = _wifiConnectionFailureHandler();
      if (newWifiAuthInfo != nullptr)
      {
        // -- Try connecting with another connection info.
        this->_wifiAuthInfo.ssid = newWifiAuthInfo->ssid;
        this->_wifiAuthInfo.password = newWifiAuthInfo->password;
        this->changeState(Connecting);
      }
      else
      {
        this->changeState(ApMode);
      }
    }
    return false;
  }

  // -- Connected
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif

  return true;
}

void IotWebConf::setupAp()
{
  WiFi.mode(WIFI_AP);

#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print("Setting up AP: ");
  Serial.println(this->_thingName);
#endif
  if (this->_state == NotConfigured)
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print("With default password: ");
# ifdef IOTWEBCONF_DEBUG_PWD_TO_SERIAL
    Serial.println(this->_initialApPassword);
# else
    Serial.println(F("<hidden>"));
# endif
#endif
    this->_apConnectionHandler(this->_thingName, this->_initialApPassword);
  }
  else
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print("Use password: ");
# ifdef IOTWEBCONF_DEBUG_PWD_TO_SERIAL
    Serial.println(this->_apPassword);
# else
    Serial.println(F("<hidden>"));
# endif
#endif
    this->_apConnectionHandler(this->_thingName, this->_apPassword);
  }

#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print(F("AP IP address: "));
  Serial.println(WiFi.softAPIP());
#endif
  //  delay(500); // Without delay I've seen the IP address blank
  //  Serial.print(F("AP IP address: "));
  //  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  this->_dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  this->_dnsServer->start(IOTWEBCONF_DNS_PORT, "*", WiFi.softAPIP());
}

void IotWebConf::stopAp()
{
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

////////////////////////////////////////////////////////////////////

void IotWebConf::blink(unsigned long repeatMs, byte dutyCyclePercent)
{
  if (repeatMs == 0)
  {
    this->stopCustomBlink();
  }
  else
  {
    this->_blinkOnMs = repeatMs * dutyCyclePercent / 100;
    this->_blinkOffMs = repeatMs * (100 - dutyCyclePercent) / 100;
  }
}

void IotWebConf::fineBlink(unsigned long onMs, unsigned long offMs)
{
  this->_blinkOnMs = onMs;
  this->_blinkOffMs = offMs;
}

void IotWebConf::stopCustomBlink()
{
  this->_blinkOnMs = this->_internalBlinkOnMs;
  this->_blinkOffMs = this->_internalBlinkOffMs;
}

void IotWebConf::blinkInternal(unsigned long repeatMs, byte dutyCyclePercent)
{
  this->blink(repeatMs, dutyCyclePercent);
  this->_internalBlinkOnMs = this->_blinkOnMs;
  this->_internalBlinkOffMs = this->_blinkOffMs;
}

void IotWebConf::doBlink()
{
  if (IOTWEBCONF_STATUS_ENABLED)
  {
    unsigned long now = millis();
    unsigned long delayMs =
      this->_blinkStateOn ? this->_blinkOnMs : this->_blinkOffMs;
    if (delayMs < now - this->_lastBlinkTime)
    {
      this->_blinkStateOn = !this->_blinkStateOn;
      this->_lastBlinkTime = now;
      digitalWrite(this->_statusPin, this->_blinkStateOn ? this->_statusOnLevel : !this->_statusOnLevel);
    }
  }
}

void IotWebConf::forceApMode(bool doForce)
{
  if (this->_forceApMode == doForce)
  {
     // Already in the requested mode;
    return;
  }

  this->_forceApMode = doForce;
  if (doForce)
  {
    if (this->_state != ApMode)
    {
      IOTWEBCONF_DEBUG_LINE(F("Start forcing AP mode"));
      this->changeState(ApMode);
    }
  }
  else
  {
    if (this->_state == ApMode)
    {
      if (this->mustStayInApMode())
      {
        IOTWEBCONF_DEBUG_LINE(F("Requested stopping to force AP mode, but we cannot leave the AP mode now."));
      }
      else
      {
        IOTWEBCONF_DEBUG_LINE(F("Stopping AP mode force."));
        this->changeState(Connecting);
      }
    }
  }
}

bool IotWebConf::connectAp(const char* apName, const char* password)
{
  return WiFi.softAP(apName, password);
}
void IotWebConf::connectWifi(const char* ssid, const char* password)
{
  WiFi.begin(ssid, password);
}
WifiAuthInfo* IotWebConf::handleConnectWifiFailure()
{
  return nullptr;
}

} // end namespace
