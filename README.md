# Software programming final project

## 1. Introduction

This project features fully functional ESP8266 MQTT Client together with 
* DHT11 (humidity and temperature sensor), 
* Remote control of LED diode HLMP-4000-002, 
* Servo SG90 (using PWM), 
* Water sensor (using adc)
* Fotoresistor PGM5526
* 2x resistor 270R 

Pins used:
* DHT11 - 5, 5V, GND 
* LED - 14, 15, GND
* Servo - 5V, GND, 2
* Water sensor - ADC, 5V, GND
* Fotoresistor - GND, 4

### Example
Testing setup
![Sample photo](animation/sample_photo.jpg)

View sample video [sample_usage.mp4](animation/sample_usage.mp4)

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