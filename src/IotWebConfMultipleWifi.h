/**
 * IotWebConfMultipleWifi.h -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2021 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef IotWebConfMultipleWifi_h
#define IotWebConfMultipleWifi_h

#include "IotWebConfOptionalGroup.h"
#include "IotWebConf.h" // for WebRequestWrapper

namespace iotwebconf
{

class ChainedWifiParameterGroup : public ChainedParameterGroup
{
public:
  ChainedWifiParameterGroup(const char* id) : ChainedParameterGroup(id, "WiFi connection")
  {
    // -- Update parameter Ids to have unique ID for all parameters within the application.
    snprintf(this->_wifiSsidParameterId, IOTWEBCONF_WORD_LEN, "%s-ssid", this->getId());
    snprintf(this->_wifiPasswordParameterId, IOTWEBCONF_WORD_LEN, "%s-pwd", this->getId());

    this->addItem(&this->wifiSsidParameter);
    this->addItem(&this->wifiPasswordParameter);
  }
  TextParameter wifiSsidParameter =
    TextParameter("WiFi SSID", this->_wifiSsidParameterId, this->wifiSsid, IOTWEBCONF_WORD_LEN);
  PasswordParameter wifiPasswordParameter =
    PasswordParameter("WiFi password", this->_wifiPasswordParameterId, this->wifiPassword, IOTWEBCONF_PASSWORD_LEN);
  char wifiSsid[IOTWEBCONF_WORD_LEN];
  char wifiPassword[IOTWEBCONF_PASSWORD_LEN];
  WifiAuthInfo wifiAuthInfo = { wifiSsid,  wifiPassword};
protected:

private:
  char _wifiSsidParameterId[IOTWEBCONF_WORD_LEN];
  char _wifiPasswordParameterId[IOTWEBCONF_WORD_LEN];
};

class MultipleWifiAddition
{
public:
  MultipleWifiAddition(
    IotWebConf* iotWebConf,
    ChainedWifiParameterGroup sets[],
    size_t setsSize);
  /**
   * Note, that init() calls setFormValidator, that overwrites existing
   * formValidator setup. Thus your setFormValidator should be called
   * _after_ multipleWifiAddition.init() .
   */
  virtual void init();

  virtual bool formValidator(
    WebRequestWrapper* webRequestWrapper);

protected:
  IotWebConf* _iotWebConf;
  ChainedWifiParameterGroup* _firstSet;
  ChainedWifiParameterGroup* _currentSet;

  iotwebconf::OptionalGroupHtmlFormatProvider _optionalGroupHtmlFormatProvider;
};

}

#endif