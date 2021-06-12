# IotWebConf users manual template

This documentation is mainly for makers are about to came out with their
own product, and prepare for that with a Users manual.
Or for ones who are want to understand the basic functionality of
IotWebConf.

This document can be used as a template, modify it with the actual
product specific details. We will only cover the basic functionality
(including status and reset), but will __not__ talk about options like:
- Firmware update (OTA),
- Static IP option,
- Multiply WiFi option,
- Offline mode option,
- Skip AP-mode option.

#### Hardware setup

The document assumes, that the device is equipped with
the indicator LED and the "reset" button, just as it can be seen
in example `IotWebConf02StatusAndReset`.
 
As mentioned above a "default" options-set is assumed as well.

## Starting up the device at the first time

When you are starting up the device for the first time, the device
will create its own WiFi Access Point with SSID _testThing_.
The status indicator rapidly flashes (it is mostly on).

Use your smartphone (or tablet, or computer) to detect the created
WiFi network, and connect to it by using the default password
_smrtThng8266_. After a successful WiFi connection a configuration
page is popped up in your smartphone's web browser.

Note, that at this point even if the network is not configured,
the device is already ready to use with the factory defaults in
an off-line manner.

## Configuration page (Config portal)

After you have connected to the access point created by the
device (as described above), you need to enter the configuration
page on the web-browser of your smartphone.
On the configuration page you will see some fields you can
set.

Except for the password fields, you will see the item values
previously set. (Or for first time setup, the factory default.)

For the password fields you will never see any previously set
values, and typed values are also hidden. You can __reveal the
password__ text you have typed by double-clicking (double-tapping) on
the password field. You can then double-click (double-tap) 
a second time to hide the text again. It is recommended to
hide the passwords before submitting the configuration form,
as browsers are likely to save non-password form values to
use them as recommendation.

_TODO: you can provide some more description on specific
fields you are about to use._

When you are finished providing all the values of your need,
please press the Apply button.

Some fields are protected with constraints, and a validation is
performed. In case there is an error in the validation of
any field none of them are saved. You even need to re-type values
for filled-out passwords in this case.

## Configuration options

After the first boot, there are some values needs to be set up.
These items are maked with __*__ (star) in the list below.

You can set up the following values in the configuration page:

-  __Thing name__ - Please change the name of the device to
a name you think describes it the most. It is advised to
incorporate a location here in case you are planning to
set up multiple devices in the same area. You should only use
english letters, and the "_" underscore character. Thus, must not
use Space, dots, etc. E.g. `lamp_livingroom` __*__
- __AP password__ - This password is used, when you want to
access the device later on. You must provide a password with at least 8,
at most 32 characters.
You are free to use any characters, further more you are
encouraged to pick a password at least 12 characters long containing
at least 3 character classes. __*__
- __WiFi SSID__ - The name of the WiFi network you want the device
to connect to. __*__
- __WiFi password__ - The password of the network above. Note, that
unsecured passwords are not supported in your protection. __*__

_TODO: add your own configuration here_

Note, that in "First boot" mode you are free to save any
partial data, but on-line mode will not enter until you have
provided all mandatory data.

## Connecting to a WiFi network

When you have successfully applied the mandatory configurations,
the device will try to connect to the required WiFi network. While
connecting, the indicator LED will alter between On/Off in a moderated
speed.

If the WiFi connection is successfull, the indicator LED will turn off,
and performes rapid blinks with long delays.

If the WiFi connection fails, the device will __fall back to Access Point
mode__. This means, that the device will form its own WiFi network, on
what you can connect to, and correct network connection setup.

This time you will see the `Thing Name` value as the access point name
 (SSID),
and you need to use the `AP password` you have configured previously.

This also means, that if the configured WiFi network is not available,
the device will fall back to Access Point mode, stays there for some
seconds (so that you can perform changes if needed), and after some
seconds it will __retry connecting to the WiFi network__.

The Access Point mode is indicated with rapid blinks, where the indicator
LED is mostly on.
Access Point mode will kept as long as any connection is alive to it.
Thus, you need to disconnect from the Access Point for the device to
continue its operation.

## Second startup and rescue mode

In case you have already set up a WiFi network in the config portal,
the device will automatically tries to (re)connect to it. However,
when the device boots up it will __always starts the Access Point
mode__ as described in previous section.

In case you have __lost the password__ you have configured, you need to
enter the rescue mode. To __enter the rescue mode__, you need to press
and hold the button on the device while powering it up. This time you
can enter the Access Point provided by the device with the factory-default
password, that is _smrtThng8266_. The rescue mode will not be release
until you have connected to the access point, it will be
released after you have disconnected from it.

## Configuration in connected mode

After the device successfully connected to the configured WiFi network
the temporary Access Point is terminated, but you can still connect
to the device using its IP address. To determine the IP address of the
device, you might want to consult with your WiFi router. (Devices
are intented to be access by name as well, but this option is not
reliable, thus, it cannot be recommended.)

When you want to access the Config Portal of the device __via a WiFi
network__ from your Web Browser, a login page will be displayed, where
you need to enter:
- User name: `admin`
- Password: the password you have set up previously as __AP Password__.

#### Security notes connecting from WiFi network

While WiFi networks are known to be relatively safe by hiding its trafic
from the public, it is not safe between parties connected to the network.
And our device does not support secure Web connection.

This means, when enter the configuration page via WiFi network, other
parties can monitor your traffic.

Recommendations to avoid compromisation:
- Try to make your configurations in Access Point mode,
- Set up a dedicated WiFi network to your IOT devices, where uncertain
parties are not allowed to connect.


## Blinking codes

Prevoius chapters were mentioned blinking patterns, now here is a
table summarize the menaning of the blink codes.

- __Rapid blinking__ (mostly on, interrupted by short off periods) -
Entered Access Point mode. This means the device create an own WiFi
network around it. You can connect to the device with your smartphone
(or WiFi capable computer).
- __Alternating on/off blinking__ - Trying to connect the configured
WiFi network.
- __Mostly off with occasional short flash__ - The device is online.

## Troubleshooting

>I have turned on my device, but it blinks like crazy. What should I do?

- Diagnose: Your device is not configured.
- Solution:
You need to turn on your smartphone, search for WiFi networks, and connect
to `testThing`. Follow the instruction at
[Starting up the device at the first time](#starting-up-the-device-at-the-first-time)

>After I start up the device, the device just blinks as crazy and
>later chills. But while it is blinking I cannot connect to it. Is this
>intended?

- Diagnose: This is an expected behaviour. At startup time you are able
 connect directly
to the device directly via a temporary created access point to perform some
configuration changes. (The idea here, is that you can change WiFi
setting in case it was changed before trying to connect to it.)

>My device is either blinks like crazy, or with an alternating pattern,
>but eventually it does not stop that. Why is that?

- Diagnose: Your device cannot connect to the configured WiFi
network.
Your network setup was changed, you have miss-typed the settings, or
the device is out of the network range.
- Solution: At the time the device rapidly blinks, connect to it with
your smartphone and alter the WiFi configuration (SSID and password).
If this doesn't help try to provide stronger WiFi signal for the device.

> I have forgot the password I have set. What should I do?

- Solution: Turn off your device. Press and hold the button on the
device while powering it up. The device will start up in Access Pont
mode, you can connect to this temporary WiFi network with your
smartphone using the initial password, that is _smrtThng8266_, and
set up a new password for the device.