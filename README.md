## Summary
IotWebConf is an Arduino library for ESP8266 to provide a non-blocking standalone WiFi/AP web configuration portal.

## Highlights
  - Manages WiFi connection,
  - Provides a config portal user interface,
  - You can extend the configuration with your own properties,
  - Persists configuration in the EEPROM,
  - Config portal remains available even after WiFi was connected,
  - Captive Portal.

## How it works
The idea is, that the Thing will provide a web interface to let modifying it's configuration. E.g. to connect to a local WiFi network, it needs the SSID and the password.

When no WiFi is configured, or the configured network is not available it creates it's own AP (access point), and let clients connect to it directly to make the configuration.

Further more there is a button (or let's say a Pin), where this button is pressed on startup it will uses a default password instead of the configured (forgot) one.

You can find the default password in the sources. :)

IotWebConf saves configuration in the EEPROM. You can extend the config portal with your custom config items. Those items will be also maintained by IotWebConf.

## Use cases
  1. **You turn on your IoT first time** - It turns into AP (access point) mode, and waits you on the 192.168.4.1 address with a web interface to set up your local network (and other configurations). For the first time a default password "smrtTHNG8266" is used when you connect to the AP. When you connect to the AP, your device will likely to pop up automatically the portal page. (We call this Captive Portal.) When configuration is done, you must leave the AP. The device detects, that no one is connected to the AP, and continue with normal operation.
  1. **WiFi configuration is changed, e.g. the Thing is moved to another location** - When the Thing cannot connect to the configured WiFi, it falls back to AP mode, and waits you to change the network configuration. When no configuration were made, then it keeps trying to connect with the already configured settings. The Thing will not switch off the AP, while anyone is connected to it, so you must leave the AP when finished with the configuration.
  1. **You want to connect to the AP, but have forgot the configured AP WiFi password you set up previously** - Connect the appropriate pin on the Arduino to ground with a push button. Holding the button pressed while powering up the device: the Thing will start the AP mode with the default password. (See Case 1. The pin is configured in the code.)
  1. **You want to change the configuration before anything is done by the Thing** - Fine! The Thing always start up in AP mode and provide you a time frame to connect to it and make any modification in the configuration. Any time one is connected to the AP (provided by the device) the AP will stay on until the connection were closed. So take the time for the changes, the Thing will wait for you while you are connected to it.
  1. **You want to change the configuration runtime** - No problem. IotWebConf keeps the config portal up and running even after WiFi connection is finished. Note, that in this case you might want to force reboot the Thing to let specific changes apply.

## IotWebConf vs. WifiManager
The WifiManager of tzapu is a great library. The features IotWebConf may appear very similar to WifiManager. However IotWebConf tries to be different.
  - WifiManager does not manages your custom properties. IotWebConf persists your configuration in EEPROM.
  - WifiManager does not do validation. IotWebConf allow you to validate your property changes made in the config portal.
  - With WifiManager you cannot use both startup and on-demand configuration. With IotWebConf the config portal remains available via the connected local WiFi.
  - IotWebConf is fitted for more advanced users. You can keep control the web server setup, configuration item input field behavior, validation.

## Credits
However IotWebConf started without having influenced by any other solutions, in the final code you can find some segments borrowed from WifiManager.
  - https://github.com/tzapu/WiFiManager

