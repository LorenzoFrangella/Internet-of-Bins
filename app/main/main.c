#include "main.h"
#include "protocol.c"
#include "rtc.c"

const char *MAIN_TAG = "Main";
const char *REGULAR_TAG = "Regular";

static TaskHandle_t my_task = NULL;
static TaskHandle_t discover_task = NULL;

void regular_task(void){
    ESP_LOGI(REGULAR_TAG, "Starting Regular");
    //long unsigned int time_total_for_round = str * time_window_standard;
    if (wifi){
            structure.alone = 0;
            structure.level = 0;
            structure.gateway_id = id;
            structure.last_round_succeded = 1;
            structure.max_known_level = 1;
            structure.rounds_failed = 0;

            end_of_hour_procedure();
    }
    else{
        id = (int)((esp_random() % 100) + 20);
    }
    printf("Size of int : %d\n", sizeof(int));
    printf("Size of long long int : %d\n", sizeof(long long int));
    printf("Size of long unsigned int : %d\n", sizeof(long unsigned int));
    printf("Size of node structure : %d\n", sizeof(node_structure));
    printf("Size of int : %d\n", sizeof(int));
    fixed_partial_size_message = sizeof(int) * 3 + sizeof(long long int) + sizeof(long unsigned int) + sizeof(node_structure) + sizeof(int); // int al posto di un int
    hash_size_message = sizeof(uint8_t) * 32;
    delay_module = (int)(time_window_standard*0.75);
    
    messages_lenght = messages_starting_lenght;

    while(1){
        int time_to_wait;
        long unsigned int s_t;
        long unsigned int end_count_total;
        long unsigned int passed_time;
        long unsigned int random_delay;
        long unsigned int remaining_time;

        // Classic node
        if (wifi || connected){
            
            ESP_LOGI(REGULAR_TAG, "Starting Classic Routine");
            start_count_total = xx_time_get_time();

            vTaskDelay(pdMS_TO_TICKS(times_to_sleep[0]));

            // ascolto primi messaggi
            first_listening();

            remaining_time = (start_count_total + (time_window_standard + times_to_sleep[0])*10) - xx_time_get_time();
            vTaskDelay(pdMS_TO_TICKS(remaining_time));
            
            random_delay = get_random_delay();
            vTaskDelay(pdMS_TO_TICKS(random_delay));
            first_talk(random_delay);

            remaining_time = time_window_standard - random_delay;
            vTaskDelay(pdMS_TO_TICKS(remaining_time));

            //Aspetto Response round
            time_to_wait = times_to_sleep[2]- times_to_sleep[1];
            vTaskDelay(pdMS_TO_TICKS(time_to_wait));

            // response round
            second_listening();
            remaining_time = (start_count_total + (time_window_standard + times_to_sleep[3])*10) - xx_time_get_time();
            vTaskDelay(pdMS_TO_TICKS(remaining_time));

            // misuro
            node_alerts my_alert = { id, 1, 2, 3};
            add_alert_to_array(my_alert);

            // parlo (waiting randomly to avoid collisions)
            random_delay = get_random_delay();
            vTaskDelay(pdMS_TO_TICKS(random_delay));
            second_talk(random_delay);

            //Aspetto fine round
            remaining_time = time_window_standard - random_delay;
            vTaskDelay(pdMS_TO_TICKS(remaining_time));

            // decido nuove fasce di ascolto
            end_of_hour_procedure();
            
        }
        else if (!wifi && !connected){
            ESP_LOGI(REGULAR_TAG, "Starting Alone Routine");
            discover = 1;
            discover_listening();

            // decido nuove fasce di ascolto
            end_of_hour_procedure();
        }

        if(wifi){
            //mando su cloud
        }

        end_count_total = xx_time_get_time();
        passed_time = (end_count_total - start_count_total)/10;
        ESP_LOGI(MAIN_TAG, "Time elapsed total: %lu", passed_time);
        print_structure();
    }
}

void app_main(void){
    ESP_LOGI(MAIN_TAG, "Starting Main");
    lora_setup();

    ESP_LOGI(MAIN_TAG, "Starting task");
    xTaskCreate(regular_task, "regular_tx", 4096, NULL, 5, &my_task);

    //xTaskCreate(&task_tx, "task_tx", 4096, NULL, 5, my_task);
   // xTaskCreate(receive_tx, "receive_tx", 4096, NULL, 5, &my_task);
}