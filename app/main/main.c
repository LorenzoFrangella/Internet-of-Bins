#include "main.h"
#include "protocol.c"
#include "rtc.c"

const char *MAIN_TAG = "Main";
const char *REGULAR_TAG = "Regular";

static TaskHandle_t my_task = NULL;
static TaskHandle_t discover_task = NULL;

void regular_task(void){
    ESP_LOGI(REGULAR_TAG, "Starting Regular");
    long unsigned int time_total_for_round = standard_number_of_windows * time_window_standard;
    long unsigned int times_to_sleep[2];
    int *slp_s;
    if (wifi){
            structure.alone = false;
            structure.level = 0;
            structure.gateway_id = id;
            structure.last_round_succeded = true;
            structure.max_known_level = 1;
            structure.rounds_failed = 0;

            slp_s = end_of_hour_procedure();
            set_time_to_sleep(times_to_sleep, slp_s);
    }
    else{
        id = (int)((esp_random() % 100) + 20);
    }
    printf("Size of int : %d\n", sizeof(int));
    printf("Size of long long int : %d\n", sizeof(long long int));
    printf("Size of long unsigned int : %d\n", sizeof(long unsigned int));
    printf("Size of node structure : %d\n", sizeof(node_structure));
    printf("Size of bool : %d\n", sizeof(bool));
    fixed_partial_size_message = sizeof(int) * 3 + sizeof(long long int) + sizeof(long unsigned int) + sizeof(node_structure) + sizeof(int); // int al posto di un bool
    hash_size_message = sizeof(uint8_t) * 32;
    
    messages_lenght = messages_starting_lenght;

    while(1){
        long unsigned int start_count_total = xx_time_get_time();
        long unsigned int end_count_total;
        long unsigned int passed_time;
        long unsigned int random_delay;
        long unsigned int remaining_time;

        // Classic node
        if (!wifi && connected){
            
            ESP_LOGI(REGULAR_TAG, "Starting Classic Routine");

            // Sleep or discover
            if (discover){
                discover_schedule ds = {times_to_sleep[0] - time_to_wait_standard, time_window_standard};
                discover_listening(ds);
            }else{
                vTaskDelay(pdMS_TO_TICKS(times_to_sleep[0]*10));
            }

            // ascolto primi messaggi
            first_listening(time_to_wait_standard);

            // parlo (waiting randomly to avoid collisions)
            vTaskDelay(pdMS_TO_TICKS(esp_random()%(int)(time_to_wait_standard-(time_to_wait_standard/10.0))));
            
            random_delay = get_random_delay();
            vTaskDelay(pdMS_TO_TICKS(random_delay));
            first_talk(random_delay);
            remaining_time = time_to_wait_standard - random_delay;
            vTaskDelay(pdMS_TO_TICKS((remaining_time + times_to_sleep[1])*10));

            // ascolto secondi messaggi
            second_listening(time_to_wait_standard);

            // misuro
            node_alerts my_alert = { id, 1, 2, 3};
            add_alert_to_array(my_alert);

            // parlo (waiting randomly to avoid collisions)
            random_delay = get_random_delay();
            vTaskDelay(pdMS_TO_TICKS(random_delay));
            second_talk(random_delay);

            //Aspetto fine round
            end_count_total = xx_time_get_time();
            passed_time = (end_count_total - start_count_total)/10;
            vTaskDelay(pdMS_TO_TICKS((time_total_for_round - passed_time)*10));

            // decido nuove fasce di ascolto
            slp_s = end_of_hour_procedure();
            // sincronizzo RTC
            set_time_to_sleep(times_to_sleep, slp_s);
            
            long unsigned int time_real = (times_to_sleep[0]+times_to_sleep[1]+4*time_window_standard);
            ESP_LOGI(MAIN_TAG, "Time total: %lu, Time real: %lu", time_total_for_round, time_real);
        }
        else if (!wifi && !connected){
            ESP_LOGI(REGULAR_TAG, "Starting Alone Routine");
            discover = true;
            discover_schedule ds = {(time_to_multiply*standard_number_of_windows), time_window_standard};
            discover_listening(ds);

            // decido nuove fasce di ascolto
            slp_s = end_of_hour_procedure();
            // sincronizzo RTC
            set_time_to_sleep(times_to_sleep, slp_s);

            long unsigned int time_real = (times_to_sleep[0]+times_to_sleep[1]+4*time_window_standard);
            ESP_LOGI(MAIN_TAG, "Time total: %lu, Time real: %lu", time_total_for_round, time_real);
        }
        else if (wifi){
            ESP_LOGI(REGULAR_TAG, "Starting Wifi Routine");
            //waiting randomly to avoid collisions
            random_delay = get_random_delay();
            vTaskDelay(pdMS_TO_TICKS(random_delay));
            first_talk(random_delay);
            remaining_time = time_to_wait_standard - random_delay;
            vTaskDelay(pdMS_TO_TICKS((remaining_time + times_to_sleep[0])*10));

            if(structure.max_known_level == 2){
                printf("wait\n");
                vTaskDelay(pdMS_TO_TICKS(time_to_wait_standard+100));
            }

            // ascolto secondi messaggi
            second_listening(time_to_wait_standard);

            // misuro

            // mando online
            for (int a = 0; a < alerts_occupation; a++){
                ESP_LOGW(REGULAR_TAG, "Alarm for id: %d, load: %d, temp: %d, gas: %d", alerts[a].node_id, alerts[a].load_alarm, alerts[a].temp_alarm, alerts[a].gas_alarm);
            }

            // aspetto per nuovo ciclo
            
            end_count_total = xx_time_get_time();
            passed_time = (end_count_total - start_count_total)/10;
            vTaskDelay(pdMS_TO_TICKS((time_total_for_round - passed_time)*10));

            // decido nuove fasce di ascolto
            slp_s = end_of_hour_procedure();
            // sincronizzo RTC
            set_time_to_sleep(times_to_sleep, slp_s);
            long unsigned int time_real = (times_to_sleep[0]+times_to_sleep[1]+1*time_window_standard);
            ESP_LOGI(MAIN_TAG, "Time total: %lu, Time real: %lu", time_total_for_round, time_real);
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