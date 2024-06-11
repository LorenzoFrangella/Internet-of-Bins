#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <ultrasonic.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include <esp_log.h>
#include <esp_err.h>

#define MAX_DISTANCE_CM 500 
#define GARB_AVG_SIZE_CM 8				

#define SENSOR_1 1
#define TRIGGER_GPIO_SENSOR_1 48
#define ECHO_GPIO_SENSOR_1 47

#define SENSOR_2 2
#define TRIGGER_GPIO_SENSOR_2 33
#define ECHO_GPIO_SENSOR_2  34

#define MQ_ADC_SENSOR_1 ADC_CHANNEL_1
#define ADC_ATTEN ADC_ATTEN_DB_12
#define RatioMQ135CleanAir 3.6
