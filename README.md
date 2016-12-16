# Summary
IotWebConf is a template solution for Internet of Things with Arduino on ESP8266 to provide a standalone WiFi/AP web configuration.

# How it works
The idea is, that the Thing will provide a web interface to let modifying it's configuration. E.g. to connect to a local WiFi network, it needs the SSID and the password.

When no WiFi is configured, or the configured network is not available it creates it's own AP (access point), and let clients connect to it directly to make the configuration.

Further more there is a button (or let's say a Pin), where this button is pressed on startup it will uses a default password instead of the configured (forgot) one.

You can find the default password in the sources. :)

# Use cases
  1. **You turn on your IoT first time** - It turns into AP (access point) mode, and waits you on the 192.168.4.1 address with a web interface to set up your local network (and other configurations). For the first time a default password "smrtTHNG8266" is used when you connect to the AP. When confuguration is done, you must leave the AP. The device detects, that no one is connected to the AP, and continue with normal operation.
  1. **Wifi configuration is changed, e.g. the Thing is moved to another location** - When the Thing cannot connect to the configured Wifi, it falls back to AP mode, and waits you to change the network configuration. When no configuration were made, then it keeps trying to connect with the already configured settings. The Thing will not switch off the AP, while anyone is connected to it, so you must leave the AP when finnished with the configuration.
  1. **You want to connect to the AP, but have forgot the configured AP Wifi password** - Connect pin D0 to ground while powering up the device: the Thing will start the AP mode with the default password. (See Case 1.)
  1. **You want to chage the configuration before anything is done by the Thing** - Fine! The Thing always start up in AP mode and provide you a time frame to connect to it and make any modification in the configuration. Any time one is connected to the AP (provided by the device) the AP will stay on until the connection were closed. So thake the time for the changes, the Thing will wait for you while you are connected to it.

# Implementation
This code is meant to be a template, but it implements an NTP clock on a TM1637 display as a sample. The clock has the following configurations:
  * NTP server name
  * GMT offset
  * Day-light-saving (DST) On/Off
