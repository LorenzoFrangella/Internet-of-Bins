#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "driver/gpio.h"

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


#define ALARM_PIN 22

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
			time_alarm.tm_sec = time_alarm.tm_sec + 10;
			set_alarm_time(dev,time_alarm);
			count++;
			




        }
    }
}



void app_main(){



	
	 /*esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

	wifi_init_sta();
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
	gpio_pulldown_en(ALARM_PIN);
    gpio_pullup_dis(ALARM_PIN);
	gpio_set_intr_type(ALARM_PIN, GPIO_INTR_POSEDGE);
	interQueue = xQueueCreate(10, sizeof(int));
	xTaskCreate(alarm_task, "alarm_task", 4096, NULL, 1, NULL);
	gpio_install_isr_service(0);
	gpio_isr_handler_add(ALARM_PIN, gpio_interrupt_handler, (void*) ALARM_PIN);
	


    /*i2c_dev_t dev = initialize_rtc();
	
	control_registers_status(dev);
	configure_alarms(dev);
	reset_alarms(dev);
	control_registers_status(dev);
	ds3231_alarm_config(&dev);
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
	float temp = get_temperature(dev);
	ESP_LOGE(pcTaskGetName(0), "Temperature: %.2f", temp);
	//sleep for 12 seconds
	vTaskDelay(12000 / portTICK_PERIOD_MS);
	control_registers_status(dev);
	vTaskDelay(12000 / portTICK_PERIOD_MS);
	reset_alarms(dev);
	*/
    
   	//xTaskCreate(monitor_task , "monitor_task", configMINIMAL_STACK_SIZE*3 , NULL, 5, NULL);
	while(1){
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}