#include "functions.h"

static const char *TAG = "SPR";

static void __attribute__((noreturn)) task_fatal_error()
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);

    while (1);
}

void initialize_led_gpio(void)
{
    gpio_config_t io_conf; 
    io_conf.intr_type = GPIO_INTR_DISABLE; 
    io_conf.mode = GPIO_MODE_OUTPUT; 
    io_conf.pin_bit_mask = ((1ULL << LED_GRN_PIN) | (1ULL << LED_RED_PIN)); 
    io_conf.pull_down_en = 0; 
    io_conf.pull_up_en = 0; 
    gpio_config(&io_conf);

    gpio_set_level(LED_RED_PIN, 0);
    gpio_set_level(LED_GRN_PIN, 0);
}

void initialize_watersensor_adc(void)
{
    // 1. init adc
    adc_config_t adc_config;

    // Depend on menuconfig->Component config->PHY->vdd33_const value
    // When measuring system voltage(ADC_READ_VDD_MODE), vdd33_const must be set to 255.
    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 8; // ADC sample collection clock = 80MHz/clk_div = 10MHz
    ESP_ERROR_CHECK(adc_init(&adc_config));
}

void initialize_fotosensor_gpio(void)
{
    gpio_config_t io_conf; 
    io_conf.intr_type = GPIO_INTR_ANYEDGE; 
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL; 
    io_conf.mode = GPIO_MODE_INPUT; 
    io_conf.pull_up_en = 1; 
    gpio_config(&io_conf); 
}

// for ota
char *get_device_id()
{
    uint8_t mac_addr[6];
    char *id = (char*)malloc((sizeof(char)*(6*2))+1);
    if (id == NULL) return NULL;

    esp_read_mac(mac_addr, ESP_MAC_WIFI_STA);
    for (int i=0; i<6; i++) {
        snprintf(id+(i*2), 3, "%02X", mac_addr[i]);
    }
    return id;
}

uint16_t check_ota_version(const char *server, const char *port)
{
	char url[MAX_URL];
	memset(url, 0, MAX_URL);
	
	char *dev_id = get_device_id();
	if (dev_id != NULL) {
		snprintf(url, MAX_URL, "%s%s/%s", OTA_VERSION_PATH, dev_id, OTA_VERSION_FILENAME);
		ESP_LOGI("VERSION", "OTA VERSION URL: %s", url);
		free(dev_id);
	} else return 0;
	
    int socket_id = connect_to_server(server, port, url);
    if (socket_id >= 0) {
        ESP_LOGI("VERSION", "Connected to http server");
    } else {
        ESP_LOGE("VERSION", "Connect to http server failed!");
        blink_red_multiply(4);
        return 0;
    }

    char buffer[BUFFSIZE] = {0};
    char version[OTA_VERSION_LEN+1];
    memset(version, 0, OTA_VERSION_LEN+1);
    uint8_t version_len_remain = OTA_VERSION_LEN;
    bool body_flag = false;

    while (1) {
        memset(buffer, 0, BUFFSIZE);
        int read_len = read_next_data(socket_id, buffer, body_flag);
        if (read_len == -2) {
			ESP_LOGE("VERSION", "FAILED!");
			blink_red_multiply(4);
			break;
		}
		if (body_flag && read_len == 0) {
			ESP_LOGI("VERSION", "GOT: %d", atoi(version));
		    break;
		}
		if (read_len >= 0) {
			if (read_len > version_len_remain) {
				memset(version, 0, OTA_VERSION_LEN+1);
				ESP_LOGE("VERSION", "FAILED!");
				blink_red_multiply(4);
				break;
			}
			memcpy(version+(OTA_VERSION_LEN-version_len_remain), buffer, read_len);
			version_len_remain -= read_len;
			body_flag = true;
		}
	}
	close(socket_id);
	return atoi(version);
}

void do_ota(const char *server, const char *port, uint16_t actual_version)
{
    esp_err_t err;
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI("OTA", "Starting OTA ...\n");
    ESP_LOGI("OTA", "Checking OTA version ...\n");
    uint16_t version = check_ota_version(server, port);
    if (version <= actual_version) {
		ESP_LOGI("OTA", "No OTA required ...\n");
		return;
	}
    
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW("OTA", "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW("OTA", "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI("OTA", "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

	char url[MAX_URL];
	memset(url, 0, MAX_URL);
	
	char *dev_id = get_device_id();
	if (dev_id != NULL) {
		snprintf(url, MAX_URL, "%s%s/%s", OTA_UPDATE_PATH, dev_id, OTA_UPDATE_FILENAME);
		ESP_LOGI("OTA", "OTA UPDATE URL: %s", url);
		free(dev_id);
	} else return;

    int socket_id = connect_to_server(server, port, url);
    if (socket_id >= 0) {
        ESP_LOGI("OTA", "Connected to http server");
    } else {
        ESP_LOGE("OTA", "Connect to http server failed!");
        blink_red_multiply(4);
		return;
    }

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI("OTA", "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE("OTA", "esp_ota_begin failed, error=%d", err);
        task_fatal_error();
    }
    ESP_LOGI("OTA", "esp_ota_begin succeeded");
    
    char ota_write_data[BUFFSIZE] = {0};
    int binary_file_length = 0;
    bool body_flag = false;

    while (1) {
        memset(ota_write_data, 0, BUFFSIZE);
        int write_len = read_next_data(socket_id, ota_write_data, body_flag);
        if (write_len == -2) {
			ESP_LOGE("OTA", "OTA FAILED! Prepare to restart system!");
			blink_red_multiply(4);
			esp_restart();
			return;
		}
		if (body_flag && write_len == 0) {
			ESP_LOGI("OTA", "Total Write binary data length : %d", binary_file_length);

			if (esp_ota_end(update_handle) != ESP_OK) {
				ESP_LOGE("OTA", "esp_ota_end failed!");
				task_fatal_error();
			}
			err = esp_ota_set_boot_partition(update_partition);
			if (err != ESP_OK) {
				ESP_LOGE("OTA", "esp_ota_set_boot_partition failed! err=0x%x", err);
				task_fatal_error();
			}
			
			
			
			ESP_LOGI("OTA", "Prepare to restart system!");
			blink_green_multiply(4);
			esp_restart();
			return ;
		}
		if (write_len >= 0) {
			err = esp_ota_write(update_handle, (const void *)ota_write_data, write_len);
			if (err != ESP_OK) {
				ESP_LOGE("OTA", "Error: esp_ota_write failed! err=0x%x", err);
				blink_red_multiply(4);
				esp_restart();
				return;
			} else {
				ESP_LOGI("OTA", "esp_ota_write header OK");
				binary_file_length += write_len;
			}
			body_flag = true;
		}
	}
}

int connect_to_server(const char *server, const char *port, const char *url)
{
    ESP_LOGI("SERVER", "Server IP: %s Server Port:%s", server, port);

    int  http_connect_flag = -1;
    struct sockaddr_in sock_info;

    int socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id == -1) {
        ESP_LOGE("SERVER", "Create socket failed!");
        return false;
    }

    memset(&sock_info, 0, sizeof(struct sockaddr_in));
    sock_info.sin_family = AF_INET;
    sock_info.sin_addr.s_addr = inet_addr(server);
    sock_info.sin_port = htons(atoi(port));

    http_connect_flag = connect(socket_id, (struct sockaddr *)&sock_info, sizeof(sock_info));
    if (http_connect_flag == -1) {
        ESP_LOGE("CONNECT", "Connect to server failed! errno=%d", errno);
        close(socket_id);
        return -1;
    } else {
        ESP_LOGI("CONNECT", "Connected to server");

        const char *GET_FORMAT =
            "GET %s HTTP/1.1\r\n"
            "Host: %s:%s\r\n"
            "User-Agent: SPR FW updater\r\n\r\n";

        char *http_request = NULL;
        int get_len = asprintf(&http_request, GET_FORMAT, url, server, port);
        if (get_len < 0) {
            ESP_LOGE("GET", "Failed to allocate memory for GET request buffer");
            task_fatal_error();
        }
        int res = send(socket_id, http_request, get_len, 0);
        free(http_request);

        if (res < 0) {
            ESP_LOGE("GET", "Send GET request to server failed");
            task_fatal_error();
        } else {
            ESP_LOGI("GET", "Send GET request to server succeeded");
        }

        return socket_id;
    }

    return -1;
}

int read_next_data(int socket_id, char *body_data, bool body_flag)
{
    char text[BUFFSIZE];
	memset(text, 0, BUFFSIZE);

	int buff_len = recv(socket_id, text, (BUFFSIZE-1), 0);
	if (buff_len < 0) {
		ESP_LOGE("DATA", "Error: receive data error! errno=%d", errno);
	} else if (buff_len > 0 && !body_flag) {
		ESP_LOGI("DATA", "Received H: %d", buff_len);
		return read_past_http_header(text, buff_len, body_data);
	} else if (buff_len > 0 && body_flag) {
		ESP_LOGI("DATA", "Received B: %d", buff_len);
		memset(body_data, 0, BUFFSIZE);
		memcpy(body_data, text, buff_len);
		return buff_len;
	} else if (buff_len == 0) {
		ESP_LOGI("DATA", "Connection closed, all packets received");
		return 0;
	} else {
		ESP_LOGE("DATA", "Unexpected recv result");
	}
	return -2;
}

int read_until(char *buffer, char delim, int len)
{
    int i = 0;
    while (buffer[i] != delim && i < len) {
        ++i;
    }
    return i + 1;
}

int read_past_http_header(char text[], int total_len, char *http_data)
{
    int i = 0, i_read_len = 0;
    while (text[i] != 0 && i < total_len) {
        i_read_len = read_until(&text[i], '\n', total_len);
        if (i_read_len == 2) {
            int i_write_len = total_len - (i + 2);
            memset(http_data, 0, BUFFSIZE);
            memcpy(http_data, &(text[i + 2]), i_write_len);
            return i_write_len;
        }
        i += i_read_len;
    }
    return -1;
}

void blink_led(uint8_t led_pin, uint16_t delay, uint8_t multiply)
{
	for (uint8_t i=0; i<multiply; i++) {
		gpio_set_level(led_pin, 1);
		vTaskDelay(delay/portTICK_RATE_MS);
		gpio_set_level(led_pin, 0);
		vTaskDelay(delay/portTICK_RATE_MS);
	}
}

void blink_green()
{
	blink_led(LED_GRN_PIN, 100, 1);
}

void blink_red()
{
	blink_led(LED_RED_PIN, 100, 1);
}

void blink_green_multiply(uint8_t multiply)
{
	blink_led(LED_GRN_PIN, 50, multiply);
}

void blink_red_multiply(uint8_t multiply)
{
	blink_led(LED_RED_PIN, 50, multiply);
}
