#ifndef __USER_FUNCTIONS__
#define __USER_FUNCTIONS__

#define BUFFSIZE 1500

#define OTA_UPDATE_PATH            "/uimages/"
#define OTA_VERSION_PATH           "/uimages/"
#define OTA_UPDATE_FILENAME        "update.bin"
#define OTA_VERSION_FILENAME       "VERSION"
#define OTA_VERSION_LEN            20

#define MAX_URL                    256
#define HTTP_RESPONSE_LEN          20


//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/hw_timer.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_err.h"
#include "esp_ota_ops.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "onewire.h"     
#include "ds18b20.h"     
//

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
#define MQTT_CLIENT_THREAD_STACK_WORDS  8192
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
char *get_device_id();
uint16_t check_ota_version(const char *server, const char *port);
void do_ota(const char *server, const char *port, uint16_t actual_version);
int connect_to_server(const char *server, const char *port, const char *url);
int read_next_data(int socket_id, char *body_data, bool body_flag);
int read_until(char*, char, int);
int read_past_http_header(char[], int, char*);

#endif