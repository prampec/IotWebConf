# Summary
IotWebConf is a template solution for Arduino on ESP8266 to provide a standalone WiFi/AP web configuration.

# How it works
The idea is, that the Thing will provide a web interface to let modifying it's configuration. E.g. to connect to a local WiFi network, it needs the SSID and the password.

When no WiFi is configured, or the configured network is not available it creates it's own AP (access point), and let clients connect to it directly to make the configuration.

Further more there is a button (or let's say a Pin), where this button is pressed on startup it will uses a default password instead of the configured (forgot) one.

You can find the default password in the sources. :)

# Implementation
This code is meant to be a template, but it implements an NTP clock on a TM1637 display as a sample. The clock has the following configurations:
  * NTP server name
  * GMT offset
  * Day-light-saving (DST) On/Off
