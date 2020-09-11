/**
 * IotWebConfCompatibility.h -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2018 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 *
 * Notes on IotWebConfAsyncUpdater:
 * This file contains workarounds, and borrowed codes from other projects,
 * to keep IotWebConf code more simple and more general. See details
 * in comments for code segments bellow this file.
 *
 */

#ifndef __ASYNC_HTTP_UPDATE_SERVER_H
#define __ASYNC_HTTP_UPDATE_SERVER_H

#include <IotWebConf.h>
#ifdef IOTWEBCONF_CONFIG_USE_ASYNC

#include <ESPAsyncWebServer.h>
#ifdef ESP8266
#include <Updater.h>
#include <ESP8266mDNS.h>
#else
#include <Update.h>
#include <ESPmDNS.h>
#endif
#include <StreamString.h>

#define emptyString F("")

class AsyncWebServer;

class AsyncHTTPUpdateServer
{
  public:
    AsyncHTTPUpdateServer(bool serial_debug=false);

    void setup(AsyncWebServer *server)
    {
      setup(server, emptyString, emptyString);
    }

    void setup(AsyncWebServer *server, const String& path)
    {
      setup(server, path, emptyString, emptyString);
    }

    void setup(AsyncWebServer *server, const String& username, const String& password)
    {
      setup(server, "/update", username, password);
    }

    void setup(AsyncWebServer *server, const String& path, const String& username, const String& password);

    void updateCredentials(const String& username, const String& password)
    {
      _username = username;
      _password = password;
    }

    bool restartRequired()
    {
        return _restartRequired;
    }

  protected:
    void _setUpdaterError();

  private:
    bool _serial_output;
    AsyncWebServer *_server;
    String _username;
    String _password;
    bool _authenticated;
    String _updaterError;
    bool _restartRequired;
};

#endif

#endif
