/**
 * IotWebConfCompatibility.cpp -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2018 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 *
 * Notes on IotWebConfCompatibility:
 * This file contains workarounds, and borrowed codes from other projects,
 * to keep IotWebConf code more simple and more general. See details
 * in comments for code segments in IotWebConfCompatibility.h .
 *
 */

/**
 * NOTE: Lines starting with /// are changed by IotWebConf
 */
#include "IotWebConfAsyncUpdater.h"
#ifdef IOTWEBCONF_CONFIG_USE_ASYNC

static const char serverIndex[] PROGMEM =
  R"(<html><body><form method='POST' action='' enctype='multipart/form-data'>
                  <input type='file' name='update'>
                  <input type='submit' value='Update'>
               </form>
         </body></html>)";
static const char successResponse[] PROGMEM =
  "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Rebooting...\n";

AsyncHTTPUpdateServer::AsyncHTTPUpdateServer(bool serial_debug)
{
  _serial_output = serial_debug;
  _server = NULL;
  _username = emptyString;
  _password = emptyString;
  _authenticated = false;
  _restartRequired = false;
}

void AsyncHTTPUpdateServer::setup(AsyncWebServer *server, const String& path, const String& username, const String& password)
{
    _server = server;
    _username = username;
    _password = password;

    // handler for the /update form page
    _server->on(path.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request){
      if(_username != emptyString && _password != emptyString && !request->authenticate(_username.c_str(), _password.c_str()))
        return request->requestAuthentication();
      request->send_P(200, PSTR("text/html"), serverIndex);
    });

    // handler for the /update form POST (once file upload finishes)
    _server->on(path.c_str(), HTTP_POST, [&](AsyncWebServerRequest *request){
      if(!_authenticated)
        return request->requestAuthentication();
      if (Update.hasError()) {
        request->send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
      } else {
        if (_serial_output)
          Serial.println("\nUpdate complete");
        request->send_P(200, PSTR("text/html"), successResponse);
        _restartRequired = true;
      }
    },[&](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
          size_t len, bool final)
    {
      // handler for the file upload, get's the sketch bytes, and writes
      // them through the Update object
      if (!index){
        _updaterError = String();
        if (_serial_output)
          Serial.setDebugOutput(true);

        _authenticated = (_username == emptyString || _password == emptyString || request->authenticate(_username.c_str(), _password.c_str()));
        if(!_authenticated){
          if (_serial_output) {
            Serial.printf("Unauthenticated Update\n");
            return;
          }
        }

        if (_serial_output)
          Serial.printf("Update: %s\n", filename.c_str());
        //content_len = request->contentLength();
        // if filename includes spiffs, update the spiffs partition
        int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
#ifdef ESP8266
        Update.runAsync(true);
        if (!Update.begin(content_len, cmd)) {
#else
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
#endif
          _setUpdaterError();
        }
      }

      if(_authenticated && !_updaterError.length()){
        if (_serial_output) Serial.printf(".");
        if (Update.write(data, len) != len) {
          _setUpdaterError();
        }
      }

      if (final && _authenticated && !_updaterError.length()) {
        if (!Update.end(true)){
          _setUpdaterError();
        }
        Serial.setDebugOutput(false);
      }
   });
}

void AsyncHTTPUpdateServer::_setUpdaterError()
{
  if (_serial_output) Update.printError(Serial);
  StreamString str;
  Update.printError(str);
  _updaterError = str.c_str();
}

#endif