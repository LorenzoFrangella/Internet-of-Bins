#ifndef main_protocol
#define main_protocol

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "esp_log.h"
#include <esp_random.h>
#include "sensors.c"

#include "ds3231.c"

int gateway = 0;
int id;
int wifi = 0;

monitor_task_parameters* parameters_monitor;

#define MISSING_HOUR_LIMIT 2
#define MAX_LEVEL 3
#define WINDOW_SIZE 5

#endif