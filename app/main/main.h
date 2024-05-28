#include <stdio.h>
#include <lora.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>

#include "protocol.c"
#include "rtc.c"

int id;
bool wifi = false;
bool connected = false;
bool discover = false;

int64_t xx_time_get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}