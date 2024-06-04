#include "main.h"
#include "protocol.c"
#include "rtc.c"
#include "test.c"

const char *MAIN_TAG = "Main";
const char *REGULAR_TAG = "Regular";

static TaskHandle_t my_task = NULL;
static TaskHandle_t discover_task = NULL;

void regular_task(void){
    ESP_LOGI(REGULAR_TAG, "Starting Regular");
    long unsigned int times_to_sleep[4];
    int *slp_s;
    if (wifi){
            slp_s = end_of_hour_procedure();
            set_time_to_sleep(times_to_sleep, slp_s);
    }

    while(1){

        // Classic node
        if (!wifi && connected){
            
            ESP_LOGI(REGULAR_TAG, "Starting Classic Routine");
            // Sleep or discover
            if (discover){
                struct discover_schedule ds = {times_to_sleep[0] - time_to_wait_standard, time_window_standard};
                discover_listening(ds);
            }else{
                vTaskDelay(pdMS_TO_TICKS(times_to_sleep[0]));
            }

            // ascolto primi messaggi
            first_listening(time_to_wait_standard);

            // parlo (waiting randomly to avoid collisions)
            vTaskDelay(pdMS_TO_TICKS(esp_random()%(int)(time_to_wait_standard-(time_to_wait_standard/10.0))));
            first_talk();
            vTaskDelay(pdMS_TO_TICKS(times_to_sleep[1]));

            // ascolto secondi messaggi
            second_listening(time_to_wait_standard);

            // misuro

            // parlo (waiting randomly to avoid collisions)
            vTaskDelay(pdMS_TO_TICKS(esp_random()%(int)(time_to_wait_standard-(time_to_wait_standard/10.0))));
            second_talk();

            // decido nuove fasce di ascolto
            slp_s = end_of_hour_procedure();
            // sincronizzo RTC
            set_time_to_sleep(times_to_sleep, slp_s);

        }
        else if (!wifi && !connected){
            ESP_LOGI(REGULAR_TAG, "Starting Alone Routine");
            discover = true;
            struct discover_schedule ds = {time_to_multiply*standard_number_of_windows*2, time_window_standard};
            discover_listening(ds);

            // decido nuove fasce di ascolto
            slp_s = end_of_hour_procedure();
            // sincronizzo RTC
            set_time_to_sleep(times_to_sleep, slp_s);
        }
        else if (wifi){
            ESP_LOGI(REGULAR_TAG, "Starting Wifi Routine");
            //waiting randomly to avoid collisions
            vTaskDelay(pdMS_TO_TICKS(esp_random()%(int)(time_to_wait_standard-(time_to_wait_standard/10.0))));
            first_talk();
            vTaskDelay(pdMS_TO_TICKS(times_to_sleep[0]));

            // ascolto secondi messaggi
            second_listening(time_to_wait_standard);

            // misuro

            // mando online

            // aspetto per nuovo ciclo
            
            vTaskDelay(pdMS_TO_TICKS(times_to_sleep[1]));

            // decido nuove fasce di ascolto
            slp_s = end_of_hour_procedure();
            // sincronizzo RTC
            set_time_to_sleep(times_to_sleep, slp_s);
        }
    }
}

void app_main(void){
    ESP_LOGI(MAIN_TAG, "Starting Main");
    set_lora();

    ESP_LOGI(MAIN_TAG, "Starting task");
    xTaskCreate(regular_task, "regular_tx", 4096, NULL, 5, &my_task);

    //xTaskCreate(&task_tx, "task_tx", 4096, NULL, 5, my_task);
   // xTaskCreate(receive_tx, "receive_tx", 4096, NULL, 5, &my_task);
}