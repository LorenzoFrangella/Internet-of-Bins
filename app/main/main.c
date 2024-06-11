#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "sensors.c"


void app_main(){

    xTaskCreate(monitor_task , "monitor_task", configMINIMAL_STACK_SIZE*3 , NULL, 5, NULL);
    /*
    struct dummy_struct dummy;
    dummy.a = 1;
    dummy.b = 2;
    strcpy(dummy.stringa, "Hello World this is a test for computing the hash of the structure");
    
    
    printf("Size of struct: %d\n", sizeof(dummy));
    */
    /*
    uint8_t key_copy[64];
    ble_obtain_key(key_copy);
    printf("Key: %.*s\n",64, key_copy);
    printf("Size of key: %d\n", sizeof(key_copy));
    */



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
}

