writeConfigTree App
==================

### Description

It is easy to use read and write operations on an existing app's config tree when you have access to the console.
However, this is not always the case; Suppose I want to write to a config tree remotely without having physical
access to the target. That is where [AirVantage API](https://airvantage.net/) comes in : A tool to allow me to 
manage my device and change configurations over-the-air!

This app was developed using:

* legato version 17.07
* WP85 release 14
* [MangOH Green](http://mangoh.io/mangoh-green) + [IoT expansion card (for GPIO)](http://mangoh.io/documentation/iot_expansion_cards.html)
* [AirVantage (comes with mangOH Green)](https://airvantage.net/)

There are some useful documentations on how the data can be exchanged in the Legato docs:
http://legato.io/legato-docs/latest/avExchangeData.html
