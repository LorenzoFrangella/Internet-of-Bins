#ifndef main
#define main

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <string.h>
#include "esp_log.h"
#include <esp_random.h>

typedef struct {
	long unsigned int time_to_wait;
	long unsigned int wait_window;
} discover_schedule;

const int time_window_standard = 200;
//assuming each windows stay up for maximum 60 sec
const int standard_max_number_of_levels = 3;
int delay_module;

long unsigned int start_count_total;
int max_time;
int times_to_sleep[4];

int id = 17;
int wifi = 0;
int alone;
int connected = 0;
int discover = 0;

const char *UTILITY_TAG = "Utility";

long unsigned int xx_time_get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

long unsigned int get_random_delay(){
    long unsigned int d = esp_random()%delay_module;
    //ESP_LOGW(UTILITY_TAG, "Delay is: %lu", d);
    return d;
}

void print_time(){
    long int real_difference = (xx_time_get_time() - start_count_total)/10;
    long int time_to_end_round = (long int)(max_time) - real_difference;
    ESP_LOGW(UTILITY_TAG, "Real difference: %lu, Time to end round: %ld", real_difference, time_to_end_round);
}

#endif