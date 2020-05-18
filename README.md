# Software programming final project

## 1. Smart greenhouse

The main idea was to create smart greenhouse that can be remotely monitored and upgraded.
Greenhouse is able to measure air humidity, air temperature, light and soil moisture. All of aforementioned is send via MQTT broker to vizualization tool [IoT MQTT Panel](https://play.google.com/store/apps/details?id=snr.lab.iotmqttpanel.prod&hl=sk). Based on temperature and light servo controls hatch (open during day when is hot, closed during night). It is possible to switch light on and off remotely using MQTT protocol.
It is possible to perform OTA updates using HTTP protocol. 

This project features fully functional ESP8266 MQTT Client together with 
* DHT11 (humidity and temperature sensor),
* Remote control of LED diode HLMP-4000-002, 
* Servo SG90 (using PWM), 
* Water sensor (using adc),
* Photoresistor PGM5526,
* 2x resistor 270R 

Pins used:
* DHT11 - 5, 5V, GND 
* LED - 14, 15, GND
* Servo - 5V, GND, 2
* Water sensor - ADC, 5V, GND
* Photoresistor - GND, 4

Technologies used:
* MQTT broker: broker.hivemq.com (public broker)
* ota update server ip: 158.197.31.20
* android visualization tool: [IoT MQTT Panel](https://play.google.com/store/apps/details?id=snr.lab.iotmqttpanel.prod&hl=sk)


### Getting started
Reqiured: https://github.com/espressif/ESP8266_RTOS_SDK/tree/release/v3.3, xtensa-lx106-elf

### Example
Testing setup photo
![Sample photo](animation/sample_photo.jpg)

View sample video, example usage of LED diode control using mqtt, servo moves when photoresistor measures dark [sample_usage.mp4](animation/sample_usage.mp4)

## 2. Configuration
* export `IDF_PATH`
* `make menuconfig` -> `Example Configuration` to config your example
* `make menuconfig` -> `Component config` -> `MQTT(Paho)` -> to config your MQTT parameters
* `make menuconfig` -> `Partition Table` -> `two ota partitions` -> to be able to perform ota updates
After configuring Partition table use `make erase_flash`.

To use ota updates:
* increase `ACTUAL_FW_VERSION`
* `make ota`
* copy `mqtt_demo.ota.bin` to ota update server
* rename `mqtt_demo.ota.bin` to `update.bin`
* increase Version number in file `VERSION`

## 3. Compiling & Execution

Once all the aforementioned works are done, the MQTT client demo will:

* Connect to the MQTT Broker
* Subscribe to the topic "/espressif/sub"
* Publish messages to the topic "/espressif/pub"
* MQTT keep alive interval is 30s, so if no sending and receiving actions happended during this interval, ping request will be sent and ping response is expected to be received 