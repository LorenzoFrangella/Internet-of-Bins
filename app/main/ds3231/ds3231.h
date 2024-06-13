#ifndef MAIN_DS3231_H_
#define MAIN_DS3231_H_

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"
#include "driver/gpio.h"
#include <stdbool.h>
#include "driver/i2c.h"

#include "i2cdev.h"
#include "i2cdev.c"

#define CONFIG_TIMEZIONE 2
#define CONFIG_SDA_GPIO 17
#define CONFIG_SCL_GPIO 16
#define CONFIG_NTP_SERVER "pool.ntp.org"
#define NTP_server "pool.ntp.org"
#define TIMEZONE 2


#define DS3231_ADDR 0x68 //!< I2C address

#define DS3231_STAT_OSCILLATOR 0x80
#define DS3231_STAT_32KHZ      0x08
#define DS3231_STAT_BUSY       0x04
#define DS3231_STAT_ALARM_2    0x02
#define DS3231_STAT_ALARM_1    0x01


#define DS3231_CTRL_OSCILLATOR    0x80
#define DS3231_CTRL_SQUAREWAVE_BB 0x40
#define DS3231_CTRL_TEMPCONV      0x20
#define DS3231_CTRL_ALARM_INTS    0x04
#define DS3231_CTRL_ALARM2_INT    0x02
#define DS3231_CTRL_ALARM1_INT    0x01

#define DS3231_ALARM_WDAY   0x40
#define DS3231_ALARM_NOTSET 0x80

#define DS3231_ADDR_TIME    0x00
#define DS3231_ADDR_ALARM1  0x07
#define DS3231_ADDR_ALARM2  0x0b
#define DS3231_ADDR_CONTROL 0x0e
#define DS3231_ADDR_STATUS  0x0f
#define DS3231_ADDR_AGING   0x10
#define DS3231_ADDR_TEMP    0x11

#define DS3231_12HOUR_FLAG  0x40
#define DS3231_12HOUR_MASK  0x1f
#define DS3231_PM_FLAG      0x20
#define DS3231_MONTH_MASK   0x1f

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))
#define sntp_setoperatingmode esp_sntp_setoperatingmode
#define sntp_setservername esp_sntp_setservername
#define sntp_init esp_sntp_init
#endif

#if CONFIG_SET_CLOCK
	#define NTP_SERVER CONFIG_NTP_SERVER
#endif
#if CONFIG_GET_CLOCK
	#define NTP_SERVER " "
#endif
#if CONFIG_DIFF_CLOCK
	#define NTP_SERVER CONFIG_NTP_SERVER
#endif



static const char *TAG = "DS3213";

uint8_t bcd2dec(uint8_t val);
uint8_t dec2bcd(uint8_t val);
esp_err_t ds3231_init_desc(i2c_dev_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
esp_err_t ds3231_set_time(i2c_dev_t *dev, struct tm *time);
esp_err_t ds3231_get_raw_temp(i2c_dev_t *dev, int16_t *temp);
esp_err_t ds3231_get_temp_integer(i2c_dev_t *dev, int8_t *temp);
esp_err_t ds3231_get_temp_float(i2c_dev_t *dev, float *temp);
esp_err_t ds3231_get_time(i2c_dev_t *dev, struct tm *time);
esp_err_t ds3231_set_alarm(i2c_dev_t *dev, struct tm *time);
esp_err_t ds3231_alarm_config(i2c_dev_t *dev);
esp_err_t ds3231_reset_alarm(i2c_dev_t *dev);
esp_err_t ds3231_read_control_registers(i2c_dev_t *dev,uint8_t *data);

void time_sync_notification_cb(struct timeval *tv);
void initialize_sntp(void);
bool obtain_time(void);
struct tm get_clock_from_rtc(i2c_dev_t dev);
void syncronize_from_received_time(i2c_dev_t dev,struct tm received_time);
i2c_dev_t initialize_rtc();
float get_temperature(i2c_dev_t dev);
void set_alarm_time(i2c_dev_t dev, struct tm time);
void sync_from_ntp(i2c_dev_t dev);
void reset_alarms(i2c_dev_t dev);
void configure_alarms(i2c_dev_t dev);


void control_registers_status(i2c_dev_t dev);




#endif /* MAIN_DS3231_H_ */

