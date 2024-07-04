#include "main_protocol.h"
#include "protocol.c"
#include "rtc.c"

#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "ble_key.c"

#include "sensors.c"
#include "signature.c"
#include "wifi.c"
#include "mqtt.c"


#define MAIN_TAG "Main"

QueueHandle_t interQueue;
i2c_dev_t dev;
int count =0;
int pinNumber=0;

static void IRAM_ATTR gpio_interrupt_handler(void *args){
    int pinNumber = (int) args;
    xQueueSendFromISR(interQueue, &pinNumber, NULL);
}

#define ALARM_PIN 25

void alarm_task(void* args){
	i2c_dev_t dev = initialize_rtc();
	ESP_LOGE("Alarm Task", "Alarm task started\n");
	configure_alarms(dev);
	reset_alarms(dev);
	time_t current_time;
	time(&current_time);
	struct tm current_time_stucture;
	struct tm time_alarm = get_clock_from_rtc(dev);
	time_alarm.tm_sec = time_alarm.tm_sec + 10;
	ds3231_reset_alarm(&dev);
	set_alarm_time(dev,time_alarm);
	

    while (1)
    {
		ESP_LOGE("Alarm Task", " Waiting for an interrupt\n");
        if (xQueueReceive(interQueue, &pinNumber, portMAX_DELAY))
        {
			
            printf("Alarm detected, now resetting the alarm \n");
			reset_alarms(dev);
			struct tm time_alarm = get_clock_from_rtc(dev);
			time_alarm.tm_sec = time_alarm.tm_sec + 5;
			set_alarm_time(dev,time_alarm);
			count++;
        }
    }
}

uint8_t key_copy[64];

/*
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
    printf("Size of struct tm : %d\n", sizeof(struct tm));
    printf("Size of node structure : %d\n", sizeof(node_structure));
    printf("Size of int : %d\n", sizeof(int));
    fixed_partial_size_message = sizeof(protocol_message) - 8; // int al posto di un int
    hash_size_message = sizeof(uint8_t) * 32;

    delay_offset = (int)(time_window_standard*1000*0.1);
    delay_module = (int)(time_window_standard*1000*0.5);
    delay_window = delay_module + delay_offset*2;

    fake_hash = malloc(hash_size_message);
    for (int h = 0; h < 32; h++){
        fake_hash[h] = h;
    }

    messages = malloc(messages_lenght * sizeof(protocol_message));
    alerts = malloc(alerts_lenght * sizeof(node_alerts));

	int al_load = 0;
	int al_temp = 0;
	int al_gas = 0;

    while(1){
        int time_to_wait;
        long unsigned int s_t;
        long unsigned int end_count_total;
        long unsigned int passed_time;
        long unsigned int start_send_time;
        long unsigned int random_delay;
        long int remaining_time;
        int sec = 0;

		ds3231_reset_alarm(&dev);

        // Classic node
        if (wifi || connected){
            
            ESP_LOGI(REGULAR_TAG, "Starting Classic Routine");
            max_time = ((structure.max_known_level + 2)*2 *time_window_standard);
            start_count_total = xx_time_get_time();
            
            struct tm time_alarm = get_clock_from_rtc(dev);
            sec = time_alarm.tm_min*60 + time_alarm.tm_sec;


            //vTaskDelay(pdMS_TO_TICKS(times_to_sleep[0]*10));
            int pinNumber=25;
            reset_alarms(dev);
            time_alarm = get_clock_from_rtc(dev);
            time_alarm.tm_sec = time_alarm.tm_sec + times_to_sleep[0]+time_window_standard;
            if(time_alarm.tm_sec >= 60){
                time_alarm.tm_sec = time_alarm.tm_sec%60;
                time_alarm.tm_min += 1;
            }
            set_alarm_time(dev,time_alarm);
            xQueueReceive(interQueue, &pinNumber, portMAX_DELAY);
            reset_alarms(dev);


            ESP_LOGW(REGULAR_TAG, "Before First Listen");
            //print_time();

            
            time_alarm = get_clock_from_rtc(dev);
            time_alarm.tm_sec = time_alarm.tm_sec + (time_window_standard);
            if(time_alarm.tm_sec >= 60){
                time_alarm.tm_sec = time_alarm.tm_sec%60;
                time_alarm.tm_min += 1;
            }
            set_alarm_time(dev,time_alarm);


            // ascolto primi messaggi
            first_listening();

            xQueueReceive(interQueue, &pinNumber, portMAX_DELAY);
            reset_alarms(dev);

                    
            ESP_LOGW(REGULAR_TAG, "Before first talk");

        
            time_alarm = get_clock_from_rtc(dev);
            struct tm tm_2 = time_alarm;
            time_alarm.tm_sec = time_alarm.tm_sec + (time_window_standard);
            if(time_alarm.tm_sec >= 60){
                time_alarm.tm_sec = time_alarm.tm_sec%60;
                time_alarm.tm_min += 1;
            }
            print_time_struct(&time_alarm);
            set_alarm_time(dev,time_alarm);
            
            random_delay = get_random_delay();
            ESP_LOGI(REGULAR_TAG, "Delay is: %lu", random_delay);
            vTaskDelay(pdMS_TO_TICKS(random_delay));
            first_talk(random_delay, tm_2);

            xQueueReceive(interQueue, &pinNumber, portMAX_DELAY);
            reset_alarms(dev);
 
            ESP_LOGW(REGULAR_TAG, "After First Talk");
        
            //Aspetto Response round

            time_alarm = get_clock_from_rtc(dev);
            time_to_wait = times_to_sleep[2]- times_to_sleep[1];
            time_alarm.tm_sec = time_alarm.tm_sec + (time_to_wait);
            if(time_alarm.tm_sec >= 60){
                time_alarm.tm_sec = time_alarm.tm_sec%60;
                time_alarm.tm_min += 1;
            }
            print_time_struct(&time_alarm);
            set_alarm_time(dev,time_alarm);
            //vTaskDelay(pdMS_TO_TICKS(time_to_wait*10));
            xQueueReceive(interQueue, &pinNumber, portMAX_DELAY);
            reset_alarms(dev);

            ESP_LOGW(REGULAR_TAG, "Before Second Listen");
            
            time_alarm = get_clock_from_rtc(dev);
            time_alarm.tm_sec = time_alarm.tm_sec + (time_window_standard);
            if(time_alarm.tm_sec >= 60){
                time_alarm.tm_sec = time_alarm.tm_sec%60;
                time_alarm.tm_min += 1;
            }
            set_alarm_time(dev,time_alarm);
            print_time_struct(&time_alarm);

            // response round
            second_listening();
            
            xQueueReceive(interQueue, &pinNumber, portMAX_DELAY);
            reset_alarms(dev);


            ESP_LOGW(REGULAR_TAG, "Before second talk");
            //print_time();
            time_alarm = get_clock_from_rtc(dev);
            tm_2 = time_alarm;
            time_alarm.tm_sec = time_alarm.tm_sec + (time_window_standard);
            if(time_alarm.tm_sec >= 60){
                time_alarm.tm_sec = time_alarm.tm_sec%60;
                time_alarm.tm_min += 1;
            }
            set_alarm_time(dev,time_alarm);
            print_time_struct(&time_alarm);

            // misuro
			//temp
			if (get_temperature(dev) > 40){
				al_temp = 1;
			}else{
				al_temp = 0;
			}

            node_alerts my_alert = { id, al_load, al_temp, al_gas};
            add_alert_to_array(my_alert);

            
            if(wifi){ // mando online
                sending_alerts();
                
            }else{ // parlo (waiting randomly to avoid collisions)
                random_delay = get_random_delay();
                ESP_LOGI(REGULAR_TAG, "Delay is: %lu", random_delay);
                vTaskDelay(pdMS_TO_TICKS(random_delay));
                second_talk(random_delay, tm_2);
            }

            xQueueReceive(interQueue, &pinNumber, portMAX_DELAY);
            reset_alarms(dev);
            //ESP_LOGW(REGULAR_TAG, "After Second Talk");
            //print_time();

            

            //Dormo fino a fine round
            remaining_time = max_time - times_to_sleep[3];
            printf("remaining time: %ld \n",remaining_time);
            time_alarm = get_clock_from_rtc(dev);
            time_alarm.tm_sec = time_alarm.tm_sec + (remaining_time);
            if(time_alarm.tm_sec >= 60){
                time_alarm.tm_sec = time_alarm.tm_sec%60;
                time_alarm.tm_min += 1;
            }
            if(remaining_time!=0){
                set_alarm_time(dev,time_alarm);
                xQueueReceive(interQueue, &pinNumber, portMAX_DELAY);
            }
            reset_alarms(dev);
            //ESP_LOGW(REGULAR_TAG, "Max time: %d, Time 3: %d", max_time, times_to_sleep[3]);
            //ESP_LOGW(REGULAR_TAG, "Remaining time: %ld", remaining_time);
            //vTaskDelay(pdMS_TO_TICKS(remaining_time*10));


            // decido nuove fasce di ascolto
            end_of_hour_procedure();

            
            time_alarm = get_clock_from_rtc(dev);
            printf("Time passed: %d while max time: %d\n", time_alarm.tm_min*60 + time_alarm.tm_sec - sec, max_time);
            
        }
        else if (!wifi && !connected){
            ESP_LOGI(REGULAR_TAG, "Starting Alone Routine");
            discover = 1;
            discover_listening();

            // decido nuove fasce di ascolto
            end_of_hour_procedure();
        }
        print_structure();

		struct tm current_time_stucture;
		struct tm time_alarm = get_clock_from_rtc(dev);
		print_time_struct(&time_alarm);
		time_alarm.tm_sec = time_alarm.tm_sec + (9 - time_alarm.tm_sec % 10);
		//printf("time alarm %d\n", time_alarm.tm_sec);
		set_alarm_time(dev,time_alarm);
		esp_light_sleep_start();
    }
}
*/

void app_main(){
    


    // Set the GPIO pin to high (logical 1)
    

    
    
	
	if(wifi){
        esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

	wifi_init_sta();
	
    }
	


	/*
    esp_mqtt_client_handle_t client = mqtt_app_start();

	esp_mqtt_client_publish(client, "/test", "HELLO WORLD", 0, 1, 0);
    */

    /*
    uint8_t key_copy[64];
    ble_obtain_key(key_copy);
    printf("Key: %.*s\n",64, key_copy);
    printf("Size of key: %d\n", sizeof(key_copy));
    
    */

   /*

	uint8_t message_to_hash[10];
	uint8_t hash[SHA256_BLOCK_SIZE];
	printf("The block size is: %d\n",SHA256_BLOCK_SIZE);
	generate_signature(hash, message_to_hash, sizeof(message_to_hash));
	printf("Hash: ");
	
	for(int i = 0; i < SHA256_BLOCK_SIZE; i++){
		printf("%02x", hash[i]);
	}
	printf("\n");
	*/
	
	esp_rom_gpio_pad_select_gpio(ALARM_PIN);
    gpio_set_direction(ALARM_PIN, GPIO_MODE_INPUT);
	gpio_pulldown_dis(ALARM_PIN);
    gpio_pullup_en(ALARM_PIN);
	gpio_set_intr_type(ALARM_PIN, GPIO_INTR_POSEDGE);
	interQueue = xQueueCreate(10, sizeof(int));
	gpio_install_isr_service(0);
	gpio_isr_handler_add(ALARM_PIN, gpio_interrupt_handler, (void*) ALARM_PIN);
	//xTaskCreate(alarm_task, "alarm_task", 4096, NULL, 1, NULL);
	
	
	
    dev = initialize_rtc();
	
	control_registers_status(dev);
	configure_alarms(dev);
	reset_alarms(dev);
	control_registers_status(dev);
	ds3231_alarm_config(&dev);
	
    
    if(wifi) sync_from_ntp(dev);

    //get_clock_from_rtc(dev);

    //esp_wifi_stop();
    
	/*
	time_t current_time;
	time(&current_time);
	struct tm current_time_stucture;
	struct tm time_alarm = get_clock_from_rtc(dev);
	time_alarm.tm_sec = time_alarm.tm_sec + 10;
	ds3231_reset_alarm(&dev);
	set_alarm_time(dev,time_alarm);
	char time_string[64];
	localtime_r(&current_time, &current_time_stucture);
	strftime(time_string, sizeof(time_string), "%m-%d-%y %H:%M:%S", &current_time_stucture);
	printf("Current time on the esp: %s\n", time_string);
	get_clock_from_rtc(dev);
	syncronize_from_received_time(dev,get_clock_from_rtc(dev));
	*/
	//sync_from_ntp(dev);

	float temp = get_temperature(dev);
	ESP_LOGE(pcTaskGetName(0), "Temperature: %.2f", temp);
	//sleep for 12 seconds
	//vTaskDelay(12000 / portTICK_PERIOD_MS);
	//control_registers_status(dev);
	//vTaskDelay(12000 / portTICK_PERIOD_MS);	

	esp_sleep_enable_ext0_wakeup(ALARM_PIN, 1);
	printf("Going to sleep\n");
	//esp_light_sleep_start();
	printf("Woke up\n");
	
	reset_alarms(dev);
	
    
   	//xTaskCreate(monitor_task , "monitor_task", configMINIMAL_STACK_SIZE*3 , NULL, 5, NULL);


	ESP_LOGI(MAIN_TAG, "Setup Protocol");
    lora_setup();

    protocol(dev,interQueue);

    ESP_LOGI(MAIN_TAG, "Starting regular task");
    reset_alarms(dev);
   
	
}