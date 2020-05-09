/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

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

#include "MQTTClient.h"
#include "esp_ota_ops.h"

////
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

#include "nvs.h"
#include "nvs_flash.h"

////

#include "functions.h"

#define CONF_OTA_SERVER_IP      CONFIG_MODULE_OTA_SERVER_IP
#define CONF_OTA_SERVER_PORT    CONFIG_MODULE_OTA_SERVER_PORT

#define ACTUAL_FW_VERSION        6
#define OTA_LOOP_COUNT          90

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

const uint32_t pin_num[PWM_PINS_NUM] = {
    PWM_0_SERVO,
};

uint32_t duties[PWM_PINS_NUM] = {
    0,
};

int16_t phase[PWM_PINS_NUM] = {
    0,
};

volatile uint8_t status = 0;

static const char *TAG = "mqtt_client_test";

//**********************************************************************
extern uint16_t system_adc_read(void);

static SemaphoreHandle_t water_sem, temp_sem, dark_sem;
static uint16_t water;
static int16_t humidity, temperature, dark;

//**********************************************************************

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;

    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
        if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT)
        {
            /*Switch to 802.11 bgn mode */
            esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
        }
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;

    default:
        break;
    }

    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void messageArrived(MessageData *data)
{
    ESP_LOGI(TAG, "Message arrived[len:%u]: %.*s",
             data->message->payloadlen, data->message->payloadlen, (char *)data->message->payload);

    if (strncmp((char *)data->message->payload, "1", 1) == 0)
    {
        ESP_LOGI(TAG, "LED GREEN ON"); 
        gpio_set_level(LED_GRN_PIN, 1);
    }
    if (strncmp((char *)data->message->payload, "2", 1) == 0)
    {
        ESP_LOGI(TAG, "LED RED ON"); 
        gpio_set_level(LED_RED_PIN, 1);
    }
    if (strncmp((char *)data->message->payload, "3", 1) == 0)
    {
        ESP_LOGI(TAG, "LED GREEN OFF"); 
        gpio_set_level(LED_GRN_PIN, 0);
    }
    if (strncmp((char *)data->message->payload, "4", 1) == 0)
    {
        ESP_LOGI(TAG, "LED RED OFF"); 
        gpio_set_level(LED_RED_PIN, 0);
    }
}

//********************************************************

static void mqtt_client_thread(void *pvParameters)
{
    char *payload = NULL;
    MQTTClient client;
    Network network;
    int rc = 0;
    char clientID[32] = {0};
    uint16_t loop_counter = OTA_LOOP_COUNT;

    ESP_LOGI(TAG, "ssid:%s passwd:%s sub:%s qos:%u pub:%s qos:%u pubinterval:%u payloadsize:%u",
             CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD, CONFIG_MQTT_SUB_TOPIC,
             CONFIG_DEFAULT_MQTT_SUB_QOS, CONFIG_MQTT_PUB_TOPIC, CONFIG_DEFAULT_MQTT_PUB_QOS,
             CONFIG_MQTT_PUBLISH_INTERVAL, CONFIG_MQTT_PAYLOAD_BUFFER);

    ESP_LOGI(TAG, "ver:%u clientID:%s keepalive:%d username:%s passwd:%s session:%d level:%u",
             CONFIG_DEFAULT_MQTT_VERSION, CONFIG_MQTT_CLIENT_ID,
             CONFIG_MQTT_KEEP_ALIVE, CONFIG_MQTT_USERNAME, CONFIG_MQTT_PASSWORD,
             CONFIG_DEFAULT_MQTT_SESSION, CONFIG_DEFAULT_MQTT_SECURITY);

    ESP_LOGI(TAG, "broker:%s port:%u", CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT);

    ESP_LOGI(TAG, "sendbuf:%u recvbuf:%u sendcycle:%u recvcycle:%u",
             CONFIG_MQTT_SEND_BUFFER, CONFIG_MQTT_RECV_BUFFER,
             CONFIG_MQTT_SEND_CYCLE, CONFIG_MQTT_RECV_CYCLE);

    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

    NetworkInit(&network);

    if (MQTTClientInit(&client, &network, 0, NULL, 0, NULL, 0) == false)
    {
        ESP_LOGE(TAG, "mqtt init err");
        vTaskDelete(NULL);
    }

    payload = malloc(CONFIG_MQTT_PAYLOAD_BUFFER);

    if (!payload)
    {
        ESP_LOGE(TAG, "mqtt malloc err");
    }
    else
    {
        memset(payload, 0x0, CONFIG_MQTT_PAYLOAD_BUFFER);
    }

    for (;;)
    {
        ESP_LOGI(TAG, "wait wifi connect...");
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

        if ((rc = NetworkConnect(&network, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT)) != 0)
        {
            ESP_LOGE(TAG, "Return code from network connect is %d", rc);
            continue;
        }

        connectData.MQTTVersion = CONFIG_DEFAULT_MQTT_VERSION;

        sprintf(clientID, "%s_%u", CONFIG_MQTT_CLIENT_ID, esp_random());

        connectData.clientID.cstring = clientID;
        connectData.keepAliveInterval = CONFIG_MQTT_KEEP_ALIVE;

        connectData.username.cstring = CONFIG_MQTT_USERNAME;
        connectData.password.cstring = CONFIG_MQTT_PASSWORD;

        connectData.cleansession = CONFIG_DEFAULT_MQTT_SESSION;

        ESP_LOGI(TAG, "MQTT Connecting");

        if ((rc = MQTTConnect(&client, &connectData)) != 0)
        {
            ESP_LOGE(TAG, "Return code from MQTT connect is %d", rc);
            network.disconnect(&network);
            continue;
        }

        ESP_LOGI(TAG, "MQTT Connected");

#if defined(MQTT_TASK)

        if ((rc = MQTTStartTask(&client)) != pdPASS)
        {
            ESP_LOGE(TAG, "Return code from start tasks is %d", rc);
        }
        else
        {
            ESP_LOGI(TAG, "Use MQTTStartTask");
        }

#endif

        if ((rc = MQTTSubscribe(&client, CONFIG_MQTT_SUB_TOPIC, CONFIG_DEFAULT_MQTT_SUB_QOS, messageArrived)) != 0)
        {
            ESP_LOGE(TAG, "Return code from MQTT subscribe is %d", rc);
            network.disconnect(&network);
            continue;
        }

        ESP_LOGI(TAG, "MQTT subscribe to topic %s OK", CONFIG_MQTT_SUB_TOPIC);

        for (;;)
        {
            MQTTMessage message;

            message.qos = CONFIG_DEFAULT_MQTT_PUB_QOS;
            message.retained = 0;
            message.payload = payload;

            // check light and rotate servo, if gets dark close something
            xSemaphoreTake(dark_sem, portMAX_DELAY);
            uint16_t a_dark = dark;
            xSemaphoreGive(dark_sem);

            if (a_dark == 0)
            {
                duties[0] = 1000; // duty 1ms
                pwm_set_duties(duties);
                pwm_start();
            }
            else
            {
                duties[0] = 2000; // duty 2ms
                pwm_set_duties(duties);
                pwm_start();
            }

            // send temperature and humidity and read water sensor
            xSemaphoreTake(water_sem, portMAX_DELAY);
            uint16_t a_water = water;
            xSemaphoreGive(water_sem);

            xSemaphoreTake(temp_sem, portMAX_DELAY);
            int16_t a_temperature = temperature / 10;
            int16_t a_humidity = humidity / 10;
            xSemaphoreGive(temp_sem);

            ESP_LOGI(TAG, "Sending {\"Humidity\": %d, \"Temperature\": %d, \"Dark\": %d, \"Water_Level\": %d}", a_humidity, a_temperature, a_dark, a_water);
            sprintf(payload, "{\"Humidity\": %d, \"Temperature\": %d, \"Dark\": %d, \"Water_Level\": %d}", a_humidity, a_temperature, a_dark, a_water);

            message.payloadlen = strlen(payload);
            //******

            if ((rc = MQTTPublish(&client, CONFIG_MQTT_PUB_TOPIC, &message)) != 0)
            {
                ESP_LOGE(TAG, "Return code from MQTT publish is %d", rc);
            }
            else
            {
                ESP_LOGI(TAG, "MQTT published topic %s, len:%u heap:%u", CONFIG_MQTT_PUB_TOPIC, message.payloadlen, esp_get_free_heap_size());
            }

            if (rc != 0)
            {
                break;
            }

            // ota start
            if (loop_counter == OTA_LOOP_COUNT)
            {
                loop_counter = 0;
                do_ota(CONF_OTA_SERVER_IP, CONF_OTA_SERVER_PORT, ACTUAL_FW_VERSION);
            }
            else
            {
                ESP_LOGI(TAG, "OTA update: %u/%u \n", loop_counter, (uint16_t)OTA_LOOP_COUNT);
            }
            loop_counter++;
            //ota end

            vTaskDelay(CONFIG_MQTT_PUBLISH_INTERVAL / portTICK_RATE_MS);
        }

        network.disconnect(&network);
    }

    ESP_LOGW(TAG, "mqtt_client_thread going to be deleted");
    vTaskDelete(NULL);
    return;
}

static void dark_task(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(dark_sem, portMAX_DELAY);
        dark = gpio_get_level(GPIO_INPUT_IO_0);
        xSemaphoreGive(dark_sem);
        ESP_LOGI(TAG, "Fotosensor sensor: %d ", dark);
        vTaskDelay((CONFIG_MQTT_PUBLISH_INTERVAL) / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

static void water_task(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(water_sem, portMAX_DELAY);
        water = system_adc_read();
        xSemaphoreGive(water_sem);
        ESP_LOGI(TAG, "WATER sensor: %u", water);
        vTaskDelay((CONFIG_MQTT_PUBLISH_INTERVAL) / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

static void temp_humidity_task(void *pvParameters)
{
    while (1)
    {
        vTaskDelay((CONFIG_MQTT_PUBLISH_INTERVAL) / portTICK_RATE_MS);
        xSemaphoreTake(temp_sem, portMAX_DELAY);
        dht_read_data(DHT_TYPE_DHT11, DHT_GPIO, &humidity, &temperature);
        xSemaphoreGive(temp_sem);
        ESP_LOGI(TAG, "Temperature measured: %d st.C, Humidity: %d", temperature, humidity);
        
    }
    vTaskDelete(NULL);
}

//********************************************************
void app_main(void)
{

    initialize_led_gpio();
    initialize_watersensor_adc();
    initialize_fotosensor_gpio();
    dht_init(DHT_GPIO, true);

    // servo
    pwm_init(PWM_PERIOD, duties, PWM_PINS_NUM, pin_num);
    pwm_set_phases(phase);
    pwm_start();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    initialise_wifi();

    water_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(water_sem);

    temp_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(temp_sem);

    dark_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(dark_sem);

    xTaskCreate(temp_humidity_task, "temperature", 1024, NULL, 8, NULL);
    xTaskCreate(water_task, "water", 1024, NULL, 8, NULL);
    xTaskCreate(dark_task, "dark", 1024, NULL, 8, NULL);

    ret = xTaskCreate(&mqtt_client_thread,
                      MQTT_CLIENT_THREAD_NAME,
                      MQTT_CLIENT_THREAD_STACK_WORDS,
                      NULL,
                      MQTT_CLIENT_THREAD_PRIO,
                      NULL);

    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "mqtt create client thread %s failed", MQTT_CLIENT_THREAD_NAME);
    }
}
