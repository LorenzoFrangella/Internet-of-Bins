#ifndef main
#define main

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <string.h>
#include "esp_log.h"
#include <stdbool.h>
#include <esp_random.h>

typedef struct {
	long unsigned int time_to_wait;
	long unsigned int wait_window;
} discover_schedule;

const int time_to_multiply = 200;
const int time_window_standard = time_to_multiply;
const int time_to_wait_standard = time_window_standard;
//assuming each windows stay up for maximum 60 sec
const int standard_max_number_of_levels = 3;
const int standard_number_of_windows = standard_max_number_of_levels*2 - 1;

int id = 17;
bool wifi = false;
bool alone;
bool connected = false;
bool discover = false;

long unsigned int xx_time_get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

#endif