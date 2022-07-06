Example: Load config from JSON file on FS
Description:
  This example is based on the "03 Custom Parameters" example.
  The idea is that we prepare a configuration file located
  in the flash filesystem. When this file exists, it is loaded
  into the IotWebConf parameters and the changes are saved.
  The used config file is then deleted from the flash FS.
  NOTE! JSON support is not available by default, you need to
  enable it with the IOTWEBCONF_ENABLE_JSON compile time directive.
  Please read further sections for more details!
  Please compare this source code with "03 Custom Parameters"!

Using this example:
  This example is intended to be used under PlatformIO, as PIO
  provides compile time configurations and FS upload capabilities.
  With PlatformIO you must use "build_flags = -DIOTWEBCONF_ENABLE_JSON"
  as seen in the "platformio.ini".

Preparing configuration file:
  You can find a "config.json" file in the "data" subfolder
  containing the example values. For CheckboxParameter you either
  want to set "" or "selected" as value. Your project must contains
  this "data" folder. Use PlatformIOs "Build file system image"
  and "Upload file system image" to upload the config.json to the
  flash area. The file is deleted after it has been processed, so
  you might want to re-upload it several times while testing this
  example.

Hardware setup for this example:
  - An LED is attached to LED_BUILTIN pin with setup On=LOW.
  - [Optional] A push button is attached to pin D2, the other leg of the
    button should be attached to GND.
