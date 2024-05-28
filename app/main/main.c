#include "main.h"
#include "protocol.c"
#include "rtc.c"

const char *MAIN_TAG = "Main";
const char *REGULAR_TAG = "Regular";

static TaskHandle_t my_task = NULL;
static TaskHandle_t discover_task = NULL;

void regular_task(void){
    ESP_LOGI(REGULAR_TAG, "Starting Regular");
    long unsigned int times_to_sleep[4];
    while(1){

        // Classic node
        if (!wifi && connected){
            
            ESP_LOGI(REGULAR_TAG, "Starting Regular Routine");
            // Sleep or discover
            if (discover){
                struct discover_schedule ds = {times_to_sleep[0] - time_to_wait_standard, time_window_standard};
                xTaskCreate(&discover_task, "discover_tx", 2048, &ds, 2, discover_listening);
            }else{
                vTaskDelay(pdMS_TO_TICKS(times_to_sleep[0]));
            }

            // ascolto primi messaggi
            first_listening(time_to_wait_standard);

            // parlo
            first_talk();
            vTaskDelay(pdMS_TO_TICKS(times_to_sleep[1]));

            // ascolto secondi messaggi
            second_listening(time_to_wait_standard);

            // misuro

            // parlo
            second_talk();

            // decido nuove fasce di ascolto
            int* slp_s = end_of_hour_procedure();
            // sincronizzo RTC
            set_time_to_sleep(times_to_sleep, slp_s);

        }
    }
}

void app_main(void){
    ESP_LOGI(MAIN_TAG, "Starting Main");
    if(lora_init()==1)printf("Lora setup should be ok \n");
    lora_set_frequency(868e6);
    lora_enable_crc();

    ESP_LOGI(MAIN_TAG, "Starting task");
    xTaskCreate(&my_task, "regular_tx", 2048, NULL, 2, regular_task);

    //xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, my_task);
    //xTaskCreate(&receive_tx, "receive_tx", 2048, NULL, 5, NULL);
}