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
    fixed_partial_size_message = sizeof(protocol_message) - 8; // int al posto di un int
    hash_size_message = sizeof(uint8_t) * 32;

    delay_offset = (int)(time_window_standard*0.1);
    delay_module = (int)(time_window_standard*0.5);
    delay_window = delay_module + delay_offset*2;

    fake_hash = malloc(hash_size_message);
    for (int h = 0; h < 32; h++){
        fake_hash[h] = h;
    }

    messages = malloc(messages_lenght * sizeof(protocol_message));
    alerts = malloc(alerts_lenght * sizeof(node_alerts));

    while(1){
        int time_to_wait;
        long unsigned int s_t;
        long unsigned int end_count_total;
        long unsigned int passed_time;
        long unsigned int start_send_time;
        long unsigned int random_delay;
        long int remaining_time;

        // Classic node
        if (wifi || connected){
            
            ESP_LOGI(REGULAR_TAG, "Starting Classic Routine");
            max_time = ((structure.max_known_level + 2)*2 *time_window_standard);
            start_count_total = xx_time_get_time();

            vTaskDelay(pdMS_TO_TICKS(times_to_sleep[0]*10));

            ESP_LOGW(REGULAR_TAG, "Before First Listen");
            print_time();

            // ascolto primi messaggi
            first_listening();

            remaining_time = (start_count_total + (time_window_standard + times_to_sleep[0])*10) - xx_time_get_time();
            vTaskDelay(pdMS_TO_TICKS(remaining_time));

            
            ESP_LOGW(REGULAR_TAG, "Before first talk");
            print_time();
            
            random_delay = get_random_delay();
            vTaskDelay(pdMS_TO_TICKS(random_delay*10));
            first_talk(random_delay);

            remaining_time = time_window_standard - random_delay;
            //ESP_LOGW(REGULAR_TAG, "Remaining: %ld, Delay: %lu", remaining_time, random_delay);
            vTaskDelay(pdMS_TO_TICKS(remaining_time*10));
 
            //ESP_LOGW(REGULAR_TAG, "After First Talk");
            //print_time();

            //Aspetto Response round
            time_to_wait = times_to_sleep[2]- times_to_sleep[1];
            vTaskDelay(pdMS_TO_TICKS(time_to_wait*10));


            ESP_LOGW(REGULAR_TAG, "Before Second Listen");
            print_time();

            // response round
            second_listening();
            remaining_time = (start_count_total + (times_to_sleep[3] - time_window_standard)*10) - xx_time_get_time();
            if (remaining_time < 0){
                remaining_time = 0;
            }
            //ESP_LOGW(REGULAR_TAG, "Remaining: %ld", remaining_time);
            vTaskDelay(pdMS_TO_TICKS(remaining_time));

            //ESP_LOGW(REGULAR_TAG, "Before second talk");
            //print_time();

            // misuro
            node_alerts my_alert = { id, 1, 2, 3};
            add_alert_to_array(my_alert);

            
            if(wifi){ // mando online
                start_send_time = xx_time_get_time();
                sending_alerts();
                working_time = (xx_time_get_time() - start_send_time)/100;
                remaining_time = time_window_standard - working_time;
                ESP_LOGW(REGULAR_TAG, "Remaining time: %ld", remaining_time);
                vTaskDelay(pdMS_TO_TICKS(remaining_time*10));
                
            }else{ // parlo (waiting randomly to avoid collisions)
                random_delay = get_random_delay();
                vTaskDelay(pdMS_TO_TICKS(random_delay*10));
                second_talk(random_delay);

                //Sync co finestra
                remaining_time = time_window_standard - random_delay;
                vTaskDelay(pdMS_TO_TICKS(remaining_time*10));
            }

            //ESP_LOGW(REGULAR_TAG, "After Second Talk");
            //print_time();

            

            //Dormo fino a fine round
            remaining_time = max_time - times_to_sleep[3];
            ESP_LOGW(REGULAR_TAG, "Max time: %d, Time 3: %d", max_time, times_to_sleep[3]);
            ESP_LOGW(REGULAR_TAG, "Remaining time: %ld", remaining_time);
            vTaskDelay(pdMS_TO_TICKS(remaining_time*10));


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