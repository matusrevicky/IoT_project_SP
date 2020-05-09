#include "functions.h"

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

