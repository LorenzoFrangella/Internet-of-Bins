#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ds3231.c"
#include "ble_key.c"





void app_main(){
    uint8_t key_copy[64];
    ble_obtain_key(key_copy);
    printf("Key: %.*s\n",64, key_copy);
    printf("Size of key: %d\n", sizeof(key_copy));
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