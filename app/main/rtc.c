#include "main.h"

const char *RTC_TAG = "RTC";

void set_time_to_sleep(long unsigned int* array, int* slp_s){
    ESP_LOGI(RTC_TAG, "Setting time sleep");
    for (int i = 0; i < 2; i++){
        ESP_LOGI(RTC_TAG, "Setting for %d value %d", i, slp_s[i]);
        switch (slp_s[i])
        {
        case -2:
            array[i] = 0;
            break;
        case -1:
            array[i] = 0;
            break;
        
        default:
            //ESP_LOGI(RTC_TAG, "Setting for classic node");
            array[i] = (long unsigned int)slp_s * time_to_multiply;
            break;
        }
    }
    
    ESP_LOGI(RTC_TAG, "Out of for");
    array[2] = time_to_wait_standard;
}