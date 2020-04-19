# ESP8266 MQTT Client Demo

## 1. Introduction

This project features fully functional ESP8266 MQTT Client together with DHT11 (humidity and temperature sensor), remote control of LED diode, Water sensor (using adc)

## 2. Configuration
* export `IDF_PATH`
* `make menuconfig` -> `Example Configuration` to config your example
* `make menuconfig` -> `Component config` -> `MQTT(Paho)` -> to config your MQTT parameters

## 3. Compiling & Execution

Once all the aforementioned works are done, the MQTT client demo will:

* Connect to the MQTT Broker
* Subscribe to the topic "/espressif/sub"
* Publish messages to the topic "/espressif/pub"
* MQTT keep alive interval is 30s, so if no sending and receiving actions happended during this interval, ping request will be sent and ping response is expected to be received 