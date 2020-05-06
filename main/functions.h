#ifndef __USER_FUNCTIONS__
#define __USER_FUNCTIONS__

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// mqtt
#include "MQTTClient.h"

// water sensor, adc
#include "driver/adc.h"

// servo
#include "esp8266/gpio_register.h" 
#include "esp8266/pin_mux_register.h" 
#include "driver/pwm.h" 

// necesary for diode
#include "driver/gpio.h" 

// temperature and humidity meter
#include <dht/dht.h>


#define MQTT_CLIENT_THREAD_NAME         "mqtt_client_thread"
#define MQTT_CLIENT_THREAD_STACK_WORDS  4096
#define MQTT_CLIENT_THREAD_PRIO         8

// servo defines 
#define PWM_0_SERVO    2 
#define PWM_PINS_NUM   1
#define PWM_PERIOD    (20000) // period lenght 20ms = 20000us

// led defines
#define LED_RED_PIN            14 
#define LED_GRN_PIN            15 

// humidity and temperature defines
#define DHT_GPIO 5 

// fotosensor defines
#define GPIO_INPUT_IO_0     4 
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0) 

void initialize_led_gpio(void);
void initialize_watersensor_adc(void);
void initialize_fotosensor_gpio(void);

#endif