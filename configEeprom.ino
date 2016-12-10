/**
 * File: configEeprom.ino
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


/*
 * This file contains helper functions to load and save configuration.
 * Config structure needs to specified before these functions can be accessed.
 * You also, a CONFIG_VERSION needs to be specified to ensure eeprom content is the thing that we are looking for.
 */

#include <EEPROM.h>

#define CONFIG_START 0 // -- We might want to place the config in the eeprom in an offset.

void initConfig(Config* configToFill)
{
  EEPROM.begin(CONFIG_START + sizeof(*configToFill));
}

/**
 * Load the configuration from the eeprom.
 */
void loadConfig(Config* configToFill)
{
  if (testVersion())
  {
    for (unsigned int t=0; t<sizeof(*configToFill); t++)
    {
      *((char*)configToFill + t) = EEPROM.read(CONFIG_START + t);
    }
  }
  else
  {
Serial.println("Wrong config version, using defaults.");
  }
}

void saveConfig(Config* configToSave) {
  for (unsigned int t=0; t<sizeof(*configToSave); t++)
  {
    EEPROM.write(CONFIG_START + t, *((char*)configToSave + t));
  }
  EEPROM.commit();
}

boolean testVersion()
{
  for (unsigned int t=0; t<4; t++)
  {
    if(EEPROM.read(CONFIG_START + t) != CONFIG_VERSION[t])
    {
      return false;
    }
  }
  return true;
}

