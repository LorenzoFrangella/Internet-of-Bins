#ifndef main_protocol
#define main_protocol

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <string.h>
#include "esp_log.h"
#include <esp_random.h>

#include "ds3231.c"

int gateway = 0;
int id;
int wifi = 1;

typedef struct {
    int alarm_capacity;
    int alarm_gas;
    int alarm_temperature;
} alarms_structure;

typedef struct {
    uint8_t hash[32];
    int id;
    int level;
    long long unsigned int curent_time;
    long long unsigned int next_round;
    int alarm_capacity;
    int alarm_gas;
    int alarm_temperature;
    
} protocol_message;



#endif