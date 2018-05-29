/**
 * IotWebConf.cpp -- IotWebConf is an ESP8266 non blocking WiFi/AP 
 *   web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf 
 *
 * Copyright (C) 2018 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <EEPROM.h>

#include "IotWebConf.h"

#ifdef IOTWEBCONF_CONFIG_USE_MDNS
# include <ESP8266mDNS.h>
#endif

#define IOTWEBCONF_STATUS_ENABLED (this->_statusPin >= 0)

IotWebConfParameter::IotWebConfParameter()
{
}
IotWebConfParameter::IotWebConfParameter(
  const char *label,
  const char *id,
  char *valueBuffer,
  int length,
  const char *type,
  const char *placeholder,
  const char *defaultValue,
  const char *customHtml)
{
  this->label = label;
  this->id = id;
  this->valueBuffer = valueBuffer;
  this->length = length;
  this->type = type;
  this->placeholder = placeholder;
  this->customHtml = customHtml;
}
IotWebConfParameter::IotWebConfParameter(
  const char *id,
  char *valueBuffer,
  int length,
  const char *customHtml,
  const char *type)
{
  this->label = NULL;
  this->id = id;
  this->valueBuffer = valueBuffer;
  this->length = length;
  this->type = type;
  this->customHtml = customHtml;
}

IotWebConfSeparator::IotWebConfSeparator()  : IotWebConfParameter(NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL)
{
}

////////////////////////////////////////////////////////////////

IotWebConf::IotWebConf(const char* defaultThingName, DNSServer* dnsServer, ESP8266WebServer* server,
  const char *initialApPassword,
  const char* configVersion)
{
  strncpy(this->_thingName, defaultThingName, IOTWEBCONF_WORD_LEN);
  this->_dnsServer = dnsServer;
  this->_server = server;
  this->_initialApPassword = initialApPassword;
  this->_configVersion = configVersion;
  itoa(this->_apTimeoutMs / 1000, this->_apTimeoutStr, 10);

  this->_thingNameParameter = IotWebConfParameter("Thing name", "iwcThingName", this->_thingName, IOTWEBCONF_WORD_LEN);
  this->_apPasswordParameter = IotWebConfParameter("AP password", "iwcApPassword", this->_apPassword, IOTWEBCONF_WORD_LEN, "password");
  this->_wifiSsidParameter = IotWebConfParameter("WiFi SSID", "iwcWifiSsid", this->_wifiSsid, IOTWEBCONF_WORD_LEN);
  this->_wifiPasswordParameter = IotWebConfParameter("WiFi password", "iwcWifiPassword", this->_wifiPassword, IOTWEBCONF_WORD_LEN, "password");
  this->_apTimeoutParameter = IotWebConfParameter("Startup delay (seconds)", "iwcApTimeout", this->_apTimeoutStr, IOTWEBCONF_WORD_LEN, "number", NULL, "min='1' max='600'");
  this->addParameter(&this->_thingNameParameter);
  this->addParameter(&this->_apPasswordParameter);
  this->addParameter(&this->_wifiSsidParameter);
  this->addParameter(&this->_wifiPasswordParameter);
}

char* IotWebConf::getThingName()
{
  return this->_thingName;
}

void IotWebConf::setConfigPin(int configPin)
{
  this->_configPin = configPin;
}

void IotWebConf::setStatusPin(int statusPin)
{
  this->_statusPin = statusPin;
}

void IotWebConf::setupUpdateServer(ESP8266HTTPUpdateServer* updateServer, const char* updatePath, const char* updateUsername, const char* updatePassword)
{
  updateServer->setup(this->_server, updatePath, updateUsername, updatePassword);
  this->_updatePath = updatePath;
}

boolean IotWebConf::init()
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
    digitalWrite(this->_statusPin, IOTWEBCONF_STATUS_ON);
  }

  // -- Load configuration from EEPROM.
  this->configInit();
  boolean validConfig = this->configLoad();
  if (!validConfig)
  {
    // No config
    this->_apPassword[0] != '\0';
    this->_wifiSsid[0] = '\0';
    this->_wifiPassword[0] = '\0';
    this->_apTimeoutMs = IOTWEBCONF_DEFAULT_AP_MODE_TIMEOUT_MS;
  }
  else
  {
    this->_apTimeoutMs = atoi(this->_apTimeoutStr) * 1000;
  }

  // Setup mdns
#ifdef IOTWEBCONF_CONFIG_USE_MDNS
  MDNS.begin(this->_thingName);
  MDNS.addService("http", "tcp", 80);
#endif

  return validConfig;
}

//////////////////////////////////////////////////////////////////

bool IotWebConf::addParameter(IotWebConfParameter *parameter)
{
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print("Adding parameter '");
  Serial.print(parameter->id);
  Serial.println("'");
#endif
  if (this->_firstParameter == NULL)
  {
    this->_firstParameter = parameter;
    IOTWEBCONF_DEBUG_LINE(F("Adding as first"));
    return true;
  }
  IotWebConfParameter *current = this->_firstParameter;
  while(current->_nextParameter != NULL)
  {
    current = current->_nextParameter;
  }

  current->_nextParameter = parameter;
  return true;
}

void IotWebConf::configInit()
{
  int size = 0;
  IotWebConfParameter *current = this->_firstParameter;
  while(current != NULL)
  {
    size += current->length;
    current = current->_nextParameter;
  }
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print("Config size: ");
  Serial.println(size);
#endif

  EEPROM.begin(IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VESION_LENGTH + size);
}

/**
 * Load the configuration from the eeprom.
 */
boolean IotWebConf::configLoad()
{
  if (this->configTestVersion())
  {
    IotWebConfParameter *current = this->_firstParameter;
    int start = IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VESION_LENGTH;
    while(current != NULL)
    {
      if (current->id != NULL)
      {
        this->readEepromValue(start, current->valueBuffer, current->length);
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
        Serial.print("Loaded config '");
        Serial.print(current->id);
        Serial.print("'= '");
        Serial.print(current->valueBuffer);
        Serial.println("'");
#endif
  
        start += current->length;
      }
      current = current->_nextParameter;
    }
    return true;
  }
  else
  {
    IOTWEBCONF_DEBUG_LINE(F("Wrong config version."));
    return false;
  }
}

void IotWebConf::configSave()
{
  this->configSaveConfigVersion();
  IotWebConfParameter *current = this->_firstParameter;
  int start = IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VESION_LENGTH;
  while(current != NULL)
  {
    if (current->id != NULL)
    {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.print("Saving config '");
      Serial.print(current->id);
      Serial.print("'= '");
      Serial.print(current->valueBuffer);
      Serial.println("'");
#endif
  
      this->writeEepromValue(start, current->valueBuffer, current->length);
      start += current->length;
    }
    current = current->_nextParameter;
  }
  EEPROM.commit();

  if (this->_configSavedCallback != NULL)
  {
    this->_configSavedCallback();
  }
}

void IotWebConf::readEepromValue(int start, char* valueBuffer, int length)
{
    for (unsigned int t=0; t<length; t++)
    {
      *((char*)valueBuffer + t) = EEPROM.read(start + t);
    }
}
void IotWebConf::writeEepromValue(int start, char* valueBuffer, int length)
{
    for (unsigned int t=0; t<length; t++)
    {
      EEPROM.write(start+t, *((char*)valueBuffer + t));
    }
}

boolean IotWebConf::configTestVersion()
{
  for (byte t=0; t<IOTWEBCONF_CONFIG_VESION_LENGTH; t++)
  {
    if(EEPROM.read(IOTWEBCONF_CONFIG_START + t) != this->_configVersion[t])
    {
      return false;
    }
  }
  return true;
}

void IotWebConf::configSaveConfigVersion()
{
  for (byte t=0; t<IOTWEBCONF_CONFIG_VESION_LENGTH; t++)
  {
    EEPROM.write(IOTWEBCONF_CONFIG_START + t, this->_configVersion[t]);
  }
}

void IotWebConf::setWifiConnectionCallback( void (*func)(void) )
{
  this->_wifiConnectionCallback = func;
}

void IotWebConf::setConfigSavedCallback( void (*func)(void) )
{
  this->_configSavedCallback = func;
}

void IotWebConf::setFormValidator( boolean (*func)(void) )
{
  this->_formValidator = func;
}

void IotWebConf::setApTimeoutMs(unsigned long millis)
{
  this->_apTimeoutMs = millis;
}

void IotWebConf::setWifiConnectionTimeoutMs(unsigned long millis)
{
  this->_wifiConnectionTimeoutMs = millis;
}

////////////////////////////////////////////////////////////////////////////////

void IotWebConf::handleConfig()
{
  if (!this->_server->hasArg("iotSave") || !this->validateForm())
  {
    // -- Display config portal
    IOTWEBCONF_DEBUG_LINE(F("Configuration page requested."));
    String page = FPSTR(IOTWEBCONF_HTTP_HEAD);
    page.replace("{v}", "Config ESP");
    page += FPSTR(IOTWEBCONF_HTTP_SCRIPT);
    page += FPSTR(IOTWEBCONF_HTTP_STYLE);
    page += FPSTR(IOTWEBCONF_HTTP_HEAD_END);

    page += FPSTR(IOTWEBCONF_HTTP_FORM_START);
    char parLength[5];
    // -- Add parameters to the form
    IotWebConfParameter *current = this->_firstParameter;
    while(current != NULL)
    {
      if (current->id == NULL)
      {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
        Serial.println("Rendering separator");
#endif
        page += "</fieldset><fieldset>";
      }
      else
      {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
        Serial.print("Rendering '");
        Serial.print(current->id);
        Serial.print("' with value: ");
        Serial.println(current->valueBuffer);
#endif
  
        String pitem = FPSTR(IOTWEBCONF_HTTP_FORM_PARAM);
        if (current->label != NULL) {
          pitem.replace("{b}", current->label);
          pitem.replace("{t}", current->type);
          pitem.replace("{i}", current->id);
          pitem.replace("{p}", current->placeholder);
          snprintf(parLength, 5, "%d", current->length);
          pitem.replace("{l}", parLength);
          if (strcmp("password", current->type) == 0)
          {
            // Value of password is not rendered
            pitem.replace("{v}", "");
          }
          else if (this->_server->hasArg(current->id))
          {
            // Value from previous submit
            pitem.replace("{v}", this->_server->arg(current->id));
          }
          else
          {
            // Value from config
            pitem.replace("{v}", current->valueBuffer);
          }
          pitem.replace("{c}", current->customHtml);
          pitem.replace("{e}", current->errorMessage);
          pitem.replace("{s}", current->errorMessage == NULL ? "" : "de"); // Div style class.
        }
        else
        {
          pitem = current->customHtml;
        }
    
        page += pitem;
      }
      current = current->_nextParameter;
    }

    page += FPSTR(IOTWEBCONF_HTTP_FORM_END);
    if (this->_updatePath != NULL)
    {
      String pitem = FPSTR(IOTWEBCONF_HTTP_UPDATE);
      pitem.replace("{u}", this->_updatePath);
      page += pitem;
    }
  
    page += FPSTR(IOTWEBCONF_HTTP_END);
  
    this->_server->sendHeader("Content-Length", String(page.length()));
    this->_server->send(200, "text/html", page);
  }
  else
  {
    // -- Save config
    IOTWEBCONF_DEBUG_LINE(F("Updating configuration"));
    char temp[IOTWEBCONF_WORD_LEN];
  
    IotWebConfParameter *current = this->_firstParameter;
    while(current != NULL)
    {
      if (current->id != NULL)
      {
        if ((strcmp("password", current->type) == 0) && (current->length <= IOTWEBCONF_WORD_LEN))
        {
          // TODO: Passwords longer than IOTWEBCONF_WORD_LEN not supported.
          this->readParamValue(current->id, temp, current->length);
          if (temp[0] != '\0')
          {
            // -- Value was set.
            strncpy(current->valueBuffer, temp, current->length);
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
            Serial.print(current->id);
            Serial.println(" was set");
#endif
          }
          else
          {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
            Serial.print(current->id);
            Serial.println(" was not changed");
#endif
          }
        }
        else
        {
          this->readParamValue(current->id, current->valueBuffer, current->length);
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
          Serial.print(current->id);
          Serial.print("='");
          Serial.print(current->valueBuffer);
          Serial.println("'");
#endif
        }
      }
      current = current->_nextParameter;
    }

    this->configSave();
  
    String page = FPSTR(IOTWEBCONF_HTTP_HEAD);
    page.replace("{v}", "Config ESP");
    page += FPSTR(IOTWEBCONF_HTTP_SCRIPT);
    page += FPSTR(IOTWEBCONF_HTTP_STYLE);
//    page += _customHeadElement;
    page += FPSTR(IOTWEBCONF_HTTP_HEAD_END);
    page += "Configuration saved. ";
    if (this->_apPassword[0] == '\0')
    {
      page += F("You must change the default AP password to continue. Return to <a href=''>configuration page</a>.");
    }
    else if (this->_wifiSsid[0] == '\0')
    {
      page += F("You must provide the local wifi settings to continue. Return to <a href=''>configuration page</a>.");
    }
    else if (this->_state == IOTWEBCONF_STATE_NOT_CONFIGURED)
    {
      page += F("Please disconnect from Wifi AP to continue!");
    }
    else
    {
      page += F("Return to <a href='/'>home page</a>.");
    }
    page += FPSTR(IOTWEBCONF_HTTP_END);

    this->_server->sendHeader("Content-Length", String(page.length()));
    this->_server->send(200, "text/html", page);
  }
}

void IotWebConf::readParamValue(const char* paramName, char* target, unsigned int len)
{
  String value = this->_server->arg(paramName);
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print("Value of arg '");
  Serial.print(paramName);
  Serial.print("' is:");
  Serial.println(value);
#endif
  value.toCharArray(target, len);
}

boolean IotWebConf::validateForm()
{
  // Clean previous error messages.
  IotWebConfParameter *current = this->_firstParameter;
  while(current != NULL)
  {
    current->errorMessage = NULL;
    current = current->_nextParameter;
  }
  
  // TODO call external validator.
  boolean valid = true;
  if (this->_formValidator != NULL)
  {
    valid = this->_formValidator();
  }

  // Internal validation.
  int l = this->_server->arg(this->_thingNameParameter.id).length();
  if (3 > l)
  {
    this->_thingNameParameter.errorMessage = "Give a name with at least 3 characters.";
    valid = false;
  }
  l = this->_server->arg(this->_apPasswordParameter.id).length();
  if ((0 < l) && (l < 8))
  {
    this->_apPasswordParameter.errorMessage = "Password length must be at least 8 characters.";
    valid = false;
  }
  l = this->_server->arg(this->_wifiPasswordParameter.id).length();
  if ((0 < l) && (l < 8))
  {
    this->_wifiPasswordParameter.errorMessage = "Password length must be at least 8 characters.";
    valid = false;
  }
  
  return valid;
}


void IotWebConf::handleNotFound() {
  if (this->handleCaptivePortal()) { // If captive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += this->_server->uri();
  message += "\nMethod: ";
  message += ( this->_server->method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += this->_server->args();
  message += "\n";

  for ( uint8_t i = 0; i < this->_server->args(); i++ ) {
    message += " " + this->_server->argName ( i ) + ": " + this->_server->arg ( i ) + "\n";
  }
  this->_server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  this->_server->sendHeader("Pragma", "no-cache");
  this->_server->sendHeader("Expires", "-1");
  this->_server->sendHeader("Content-Length", String(message.length()));
  this->_server->send ( 404, "text/plain", message );
}

/** Redirect to captive portal if we got a request for another domain.
  Return true in that case so the page handler do not try to handle the request again.
  */
boolean IotWebConf::handleCaptivePortal() {
  if (!isIp(this->_server->hostHeader()) ) {
    IOTWEBCONF_DEBUG_LINE(F("Request redirected to captive portal"));
    this->_server->sendHeader("Location", String("http://") + toStringIp(this->_server->client().localIP()), true);
    this->_server->send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    this->_server->client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Is this an IP? */
boolean IotWebConf::isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String IotWebConf::toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

/////////////////////////////////////////////////////////////////////////////////

void IotWebConf::delay(unsigned long m)
{
  unsigned long delayStart = millis();
  while(!this->smallerCheckOverflow(delayStart, m, millis()))
  {
    this->doLoop();
    delay(1);
  }
}

void IotWebConf::doLoop()
{
  if (this->_state == IOTWEBCONF_STATE_BOOT)
  {
    // -- After boot, fall immediatelly to AP mode.
    this->changeState(IOTWEBCONF_STATE_AP_MODE);
  }
  else if (
    (this->_state ==IOTWEBCONF_STATE_NOT_CONFIGURED)
    || (this->_state == IOTWEBCONF_STATE_AP_MODE))
  {
    // -- We must only leave the AP mode, when no slaves are connected.
    // -- Other than that AP mode has a timeout. E.g. after boot, or when retry connecting to WiFi
    checkConnection();
    checkApTimeout();
    this->_dnsServer->processNextRequest();
    this->_server->handleClient();
  }
  else if (this->_state == IOTWEBCONF_STATE_CONNECTING)
  {
      if (checkWifiConnection())
      {
        this->changeState(IOTWEBCONF_STATE_ONLINE);
        return;
      }
  }
  else if (this->_state == IOTWEBCONF_STATE_ONLINE)
  {
    // -- In server mode we provide web interface. And check whether it is time to run the client.
    this->_server->handleClient();
  }
}

/**
 * What happens, when a state changed...
 */
void IotWebConf::changeState(byte newState)
{
  switch(newState)
  {
    case IOTWEBCONF_STATE_AP_MODE:
    {
      // -- In AP mode we must override the default AP password. Othervise we stay in STATE_NOT_CONFIGURED.
      boolean forceDefaultPassword = false;
      if (this->_forceDefaultPassword || (this->_apPassword[0] == '\0'))
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
        newState = IOTWEBCONF_STATE_NOT_CONFIGURED;
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
  byte oldState = this->_state;
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
 * What happens, when a state changed...
 */
void IotWebConf::stateChanged(byte oldState, byte newState)
{
//  updateOutput();
  switch(newState)
  {
    case IOTWEBCONF_STATE_AP_MODE:
    case IOTWEBCONF_STATE_NOT_CONFIGURED:
      if (IOTWEBCONF_STATUS_ENABLED)
      {
        digitalWrite(this->_statusPin, IOTWEBCONF_STATUS_ON);
      }
      setupAp();
      this->_server->begin();
      this->_apConnectionStatus = IOTWEBCONF_AP_CONNECTION_STATE_NC;
      this->_apStartTimeMs = millis();
      break;
    case IOTWEBCONF_STATE_CONNECTING:
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.print("Connecting to [");
      Serial.print(this->_wifiSsid);
      Serial.println("]");
#endif
      this->_wifiConnectionStart = millis();
      WiFi.begin(this->_wifiSsid, this->_wifiPassword);
      break;
    case IOTWEBCONF_STATE_ONLINE:
      stopAp();
      if (IOTWEBCONF_STATUS_ENABLED)
      {
        digitalWrite(this->_statusPin, IOTWEBCONF_STATUS_OFF);
      }
      this->_server->begin();
      IOTWEBCONF_DEBUG_LINE(F("Accepting connection"));
      this->_wifiConnectionCallback();
      break;
    default:
      break;
  }
}

boolean IotWebConf::smallerCheckOverflow(unsigned long prevMillis, unsigned long diff, unsigned long currentMillis)
{
  if ((prevMillis < currentMillis) && ((prevMillis + diff) < prevMillis))
  // -- current does not overflows, but pref+diff does
  {
    return false;
  }
  return prevMillis + diff < currentMillis;
}

void IotWebConf::checkApTimeout()
{
  if ((this->_wifiSsid[0] != '\0')
    && (this->_apPassword[0] != '\0')
    && (!this->_forceDefaultPassword))
  {
    // -- Only move on, when we have a valid Wifi and AP configured.
    if (
      (this->_apConnectionStatus == IOTWEBCONF_AP_CONNECTION_STATE_DC)
       ||
      (this->smallerCheckOverflow(this->_apStartTimeMs, this->_apTimeoutMs, millis())
      && (this->_apConnectionStatus != IOTWEBCONF_AP_CONNECTION_STATE_C)))
    {
      this->changeState(IOTWEBCONF_STATE_CONNECTING);
    }
  }
}

/**
 * Checks whether we have anyone joined to our AP. 
 * If so, we must not change state. But when our gest leaved, we can immediatelly move on.
 */
void IotWebConf::checkConnection()
{
  if ((this->_apConnectionStatus == IOTWEBCONF_AP_CONNECTION_STATE_NC)
    && (WiFi.softAPgetStationNum() > 0))
  {
    this->_apConnectionStatus = IOTWEBCONF_AP_CONNECTION_STATE_C;
    IOTWEBCONF_DEBUG_LINE(F("Connection to AP."));
  }
  else if ((this->_apConnectionStatus == IOTWEBCONF_AP_CONNECTION_STATE_C)
    && (WiFi.softAPgetStationNum() == 0))
  {
    this->_apConnectionStatus = IOTWEBCONF_AP_CONNECTION_STATE_DC;
    IOTWEBCONF_DEBUG_LINE(F("Disconnected from AP."));
    if (this->_forceDefaultPassword)
    {
      IOTWEBCONF_DEBUG_LINE(F("Releasing forced AP mode."));
      this->_forceDefaultPassword = false;
    }
  }
}

boolean IotWebConf::blink()
{
  if (IOTWEBCONF_STATUS_ENABLED)
  {
    unsigned long now = millis();
    if (smallerCheckOverflow(this->_lastBlinkTime, 300, now))
    {
      this->_blinkState = 1 - this->_blinkState;
      this->_lastBlinkTime = now;
      digitalWrite(this->_statusPin, this->_blinkState);
    }
  }
}

boolean IotWebConf::checkWifiConnection()
{
  this->blink();
  if (WiFi.status() != WL_CONNECTED) {
    if (this->smallerCheckOverflow(this->_wifiConnectionStart, this->_wifiConnectionTimeoutMs, millis()))
    {
      // -- Wifi not available, fall back to AP mode.
      IOTWEBCONF_DEBUG_LINE(F("Giving up."));
      WiFi.disconnect();
      this->changeState(IOTWEBCONF_STATE_AP_MODE);
    }
    return false;
  }

  // -- Connected
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  return true;
}

void IotWebConf::setupAp()
{
  WiFi.mode(WIFI_AP);

#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print("Setting up AP: ");
  Serial.println(this->_thingName);
#endif
  if (this->_state == IOTWEBCONF_STATE_NOT_CONFIGURED)
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print("With default password: ");
    Serial.println(this->_initialApPassword);
#endif
    WiFi.softAP(this->_thingName, this->_initialApPassword);
  }
  else
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print("Use password: ");
    Serial.println(this->_apPassword);
#endif
    WiFi.softAP(this->_thingName, this->_apPassword);
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
  WiFi.softAPdisconnect();
  WiFi.mode(WIFI_STA);
}

