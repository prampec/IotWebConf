/**
 * IotWebConfSettings.h -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2019 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef IotWebConfSettings_h
#define IotWebConfSettings_h

// -- We might want to place the config in the EEPROM in an offset.
#define IOTWEBCONF_CONFIG_START 0

// -- Maximal length of any string used in IotWebConfig configuration (e.g.
// ssid, password).
#define IOTWEBCONF_WORD_LEN 33

// -- IotWebConf tries to connect to the local network for an amount of time
// before falling back to AP mode.
#define IOTWEBCONF_DEFAULT_WIFI_CONNECTION_TIMEOUT_MS 30000

// -- Thing will stay in AP mode for an amount of time on boot, before retrying
// to connect to a WiFi network.
#define IOTWEBCONF_DEFAULT_AP_MODE_TIMEOUT_MS 30000

// -- mDNS should allow you to connect to this device with a hostname provided
// by the device. E.g. mything.local
#define IOTWEBCONF_CONFIG_USE_MDNS

// -- Logs progress information to Serial if enabled.
#define IOTWEBCONF_DEBUG_TO_SERIAL

// -- Logs passwords to Serial if enabled.
//#define IOTWEBCONF_DEBUG_PWD_TO_SERIAL

// -- Helper define for serial debug
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
# define IOTWEBCONF_DEBUG_LINE(MSG) Serial.println(MSG)
#else
# define IOTWEBCONF_DEBUG_LINE(MSG)
#endif

// -- EEPROM config starts with a special prefix of length defined here.
#define IOTWEBCONF_CONFIG_VESION_LENGTH 4
#define IOTWEBCONF_DNS_PORT 53

#endif