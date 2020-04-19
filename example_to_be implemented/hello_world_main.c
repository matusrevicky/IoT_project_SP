#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 

#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
#include "freertos/queue.h" 

#include "esp_log.h" 
#include "esp_system.h" 
#include "esp_err.h" 

#include "esp8266/gpio_register.h" 
#include "esp8266/pin_mux_register.h" 

#include "driver/pwm.h" 

#define PWM_0_RED     14 
#define PWM_1_GREEN   15 
#define PWM_PINS_NUM   2 


#define PWM_PERIOD    (20000) 


static const char *TAG = "PWM"; 

const uint32_t pin_num[PWM_PINS_NUM] = { 
    PWM_0_RED, 
    PWM_1_GREEN, 
}; 
  
uint32_t duties[PWM_PINS_NUM] = { 
    0, 0, 
}; 

int16_t phase[PWM_PINS_NUM] = { 
    0, 0, 
}; 

  

void app_main() 

{ 

    pwm_init(PWM_PERIOD, duties, PWM_PINS_NUM, pin_num); 

    //pwm_set_channel_invert(0x1 << 0); 

    pwm_set_phases(phase); 

    pwm_start(); 

     

    while (1) { 

        for (uint8_t i=2; i<5; i++) { 

            duties[0] = 500*i;

            pwm_set_duties(duties); 

            pwm_start(); 

            ESP_LOGI(TAG, "STEP: %d \n", i); 

            vTaskDelay(5000 / portTICK_RATE_MS); 

        } 

        duties[0] = 0; 

        // pwm_stop(0x0);         

        // for (int8_t i=64; i>=0; i--) { 

        //     duties[1] = i; 

        //     pwm_set_duties(duties); 

        //     pwm_start(); 

        //     ESP_LOGI(TAG, "STEP: %d \n", i); 

        //     vTaskDelay(100 / portTICK_RATE_MS); 

        // } 

    } 

  

} 

  