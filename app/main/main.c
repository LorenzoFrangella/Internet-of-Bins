#include "main_protocol.h"
#include "protocol.c"
#include "rtc.c"

#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "ds3231.c"
#include "ble_key.c"

#include "sensors.c"
#include "signature.c"
#include "wifi.c"
#include "mqtt.c"

QueueHandle_t interQueue;

static void IRAM_ATTR gpio_interrupt_handler(void *args){
    int pinNumber = (int) args;
    xQueueSendFromISR(interQueue, &pinNumber, NULL);
}

i2c_dev_t dev;

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
	

    int pinNumber, count = 0;
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

long unsigned int time_since_2024(){
	struct tm time = get_clock_from_rtc(dev);
	time_t time_value = mktime(&time);
	return time_value - time_to_2024;
}

const char *MAIN_TAG = "Main";
const char *REGULAR_TAG = "Regular";

static TaskHandle_t my_task = NULL;
static TaskHandle_t discover_task = NULL;

void print_time_struct(struct tm *time){
	ESP_LOGI(TAG, "%04d-%02d-%02d %02d:%02d:%02d", 
		time->tm_year, time->tm_mon + 1,
		time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
}

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

		ds3231_reset_alarm(&dev);

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
            first_talk(random_delay, time_since_2024());

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
			//temp
			if (get_temperature(dev) > 40){
				al_temp = 1;
			}else{
				al_temp = 0;
			}

            node_alerts my_alert = { id, al_load, al_temp, al_gas};
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
                second_talk(random_delay, time_since_2024());

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



		struct tm current_time_stucture;
		struct tm time_alarm = get_clock_from_rtc(dev);
		print_time_struct(&time_alarm);
		time_alarm.tm_sec = time_alarm.tm_sec + (9 - time_alarm.tm_sec % 10);
		//printf("time alarm %d\n", time_alarm.tm_sec);
		set_alarm_time(dev,time_alarm);
		esp_light_sleep_start();
    }
}

void app_main(){



    // Set the GPIO pin to high (logical 1)
    

    
    
	
	/*
	esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

	wifi_init_sta();
	*/


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
	//xTaskCreate(alarm_task, "alarm_task", 4096, NULL, 1, NULL);
	//gpio_install_isr_service(0);
	//gpio_isr_handler_add(ALARM_PIN, gpio_interrupt_handler, (void*) ALARM_PIN);
	
	
	
    dev = initialize_rtc();
	
	control_registers_status(dev);
	configure_alarms(dev);
	reset_alarms(dev);
	control_registers_status(dev);
	ds3231_alarm_config(&dev);
	/*
	//sync_from_ntp(dev);
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

    ESP_LOGI(MAIN_TAG, "Starting regular task");
    xTaskCreate(regular_task, "regular_tx", 4096, NULL, 5, &my_task);
	
}