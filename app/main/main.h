#ifndef main
#define main

#include <stdio.h>
#include <lora.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <string.h>
#include "esp_log.h"

struct discover_schedule {
	long unsigned int time_to_wait;
	long unsigned int wait_window;
};

long unsigned int time_to_multiply = 10;
int time_to_wait_standard = 10;
int time_window_standard = 10;
//assuming each windows stay up for maximum 60 sec
int standard_number_of_windows = 29;

int id;
bool wifi = true;
bool alone;
bool connected = false;
bool discover = false;

long unsigned int xx_time_get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

void set_lora(){
	if(lora_init()==1)printf("Lora setup should be ok \n");
    lora_set_frequency(868e6);
    lora_enable_crc();
}

#endif