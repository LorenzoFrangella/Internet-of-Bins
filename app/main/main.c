#include "main.h"

static TaskHandle_t my_task = NULL;

void app_main(void){
    if(lora_init()==1)printf("Lora setup should be ok \n");
    lora_set_frequency(868e6);
    lora_enable_crc();

    //xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, my_task);
    //xTaskCreate(&receive_tx, "receive_tx", 2048, NULL, 5, NULL);
}

void regular_task(void){
    u_int64_t times_to_sleep[4];
    while(1){

        // Classic node
        if (!wifi && connected){
            // Sleep or discover
            if (discover){
                discover_listening();
            }else{
                vTaskDelay(pdMS_TO_TICKS(times_to_sleep[0]));
            }

            // ascolto primi messaggi
            first_listening();

            // parlo
            first_speak();

            vTaskDelay(pdMS_TO_TICKS(times_to_sleep[1]));

            // ascolto secondi messaggi
            second_listening();

            // misuro

            // parlo
            second_speak();

            // decido nuove fasce di ascolto
            int* slp_s = end_of_hour_procedure();
            // sincronizzo RTC
            set_times_to_sleep(times_to_sleep, slp_s);

        }
    }
}