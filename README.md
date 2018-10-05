writeConfigTree App
==================

### Description

It is easy to use read and write operations on an existing app's config tree when you have access to the console.
However, this is not always the case; Suppose I want to write to a config tree remotely without having physical
access to the target. That is where [AirVantage API](https://airvantage.net/) comes in : A tool to allow me to 
manage my device and change configurations over-the-air!

This app was developed using:

* [legato](https://legato.io/) version 17.07
* WP85 release 14
* [MangOH Green](http://mangoh.io/mangoh-green) + [IoT expansion card (for GPIO)](https://mangoh.io/iot-card-resources)
* [AirVantage (comes with mangOH Green)](https://airvantage.net/)
* SIM card

There are some useful documentations on how the data can be exchanged in the Legato docs: 
http://legato.io/legato-docs/latest/avExchangeData.html

### Usage

After running this app on your target, you can write to any config tree you want wirelessly(over-the-air). 
Suppose I want to write to `/url` in my app, trafficLight.
From shell,
```shell
#!/bin/sh

server="https://eu.airvantage.net
access_token=< Associated with your device >
uid=< Associated with your device >
resource=/url                           #path/to/config/tree you want to write to
value="https://www.google.ca"           #value to write to
curl -X POST -s "${server}/api/v1/operations/systems/settings?access_token=${access_token}" -H 'Content-Type: application/json' -d "{\"reboot\":false,\"system\":{\"uids\":[\"${uid}\"]}, \"settings\":[{\"key\":\"${resource}\", \"value\":${value}}]}"
```
This will write `https://www.google.ca` to trafficLight:/url from CLI
