#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define NTP_SERVER "pool.ntp.org"


#include "ds3231.h"

#define CHECK_ARG(ARG) do { if (!ARG) return ESP_ERR_INVALID_ARG; } while (0)

uint8_t bcd2dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & 0x0f);
}

uint8_t dec2bcd(uint8_t val)
{
    return ((val / 10) << 4) + (val % 10);
}

esp_err_t ds3231_init_desc(i2c_dev_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);

    dev->port = port;
    dev->addr = DS3231_ADDR;
    dev->sda_io_num = sda_gpio;
    dev->scl_io_num = scl_gpio;
    dev->clk_speed = I2C_FREQ_HZ;
    return i2c_master_init(port, sda_gpio, scl_gpio);
}

esp_err_t ds3231_set_time(i2c_dev_t *dev, struct tm *time)
{
    CHECK_ARG(dev);
    CHECK_ARG(time);

    uint8_t data[7];

    /* time/date data */
    data[0] = dec2bcd(time->tm_sec);
    data[1] = dec2bcd(time->tm_min);
    data[2] = dec2bcd(time->tm_hour);
    /* The week data must be in the range 1 to 7, and to keep the start on the
     * same day as for tm_wday have it start at 1 on Sunday. */
    data[3] = dec2bcd(time->tm_wday + 1);
    data[4] = dec2bcd(time->tm_mday);
    data[5] = dec2bcd(time->tm_mon + 1);
    data[6] = dec2bcd(time->tm_year - 2000);

    return i2c_dev_write_reg(dev, DS3231_ADDR_TIME, data, 7);
}

esp_err_t ds3231_get_raw_temp(i2c_dev_t *dev, int16_t *temp)
{
    CHECK_ARG(dev);
    CHECK_ARG(temp);

    uint8_t data[2];

    esp_err_t res = i2c_dev_read_reg(dev, DS3231_ADDR_TEMP, data, sizeof(data));
    if (res == ESP_OK)
        *temp = (int16_t)(int8_t)data[0] << 2 | data[1] >> 6;

    return res;
}

esp_err_t ds3231_get_temp_integer(i2c_dev_t *dev, int8_t *temp)
{
    CHECK_ARG(temp);

    int16_t t_int;

    esp_err_t res = ds3231_get_raw_temp(dev, &t_int);
    if (res == ESP_OK)
        *temp = t_int >> 2;

    return res;
}

esp_err_t ds3231_get_temp_float(i2c_dev_t *dev, float *temp)
{
    CHECK_ARG(temp);

    int16_t t_int;

    esp_err_t res = ds3231_get_raw_temp(dev, &t_int);
    if (res == ESP_OK)
        *temp = t_int * 0.25;

    return res;
}

esp_err_t ds3231_get_time(i2c_dev_t *dev, struct tm *time)
{
    CHECK_ARG(dev);
    CHECK_ARG(time);

    uint8_t data[7];

    /* read time */
    esp_err_t res = i2c_dev_read_reg(dev, DS3231_ADDR_TIME, data, 7);
        if (res != ESP_OK) return res;

    /* convert to unix time structure */
    time->tm_sec = bcd2dec(data[0]);
    time->tm_min = bcd2dec(data[1]);
    if (data[2] & DS3231_12HOUR_FLAG)
    {
        /* 12H */
        time->tm_hour = bcd2dec(data[2] & DS3231_12HOUR_MASK) - 1;
        /* AM/PM? */
        if (data[2] & DS3231_PM_FLAG) time->tm_hour += 12;
    }
    else time->tm_hour = bcd2dec(data[2]); /* 24H */
    time->tm_wday = bcd2dec(data[3]) - 1;
    time->tm_mday = bcd2dec(data[4]);
    time->tm_mon  = bcd2dec(data[5] & DS3231_MONTH_MASK) - 1;
    time->tm_year = bcd2dec(data[6]) + 2000;
    time->tm_isdst = 0;

    // apply a time zone (if you are not using localtime on the rtc or you want to check/apply DST)
    //applyTZ(time);

    return ESP_OK;
}


void time_sync_notification_cb(struct timeval *tv){
	ESP_LOGI(TAG, "Notification of a time synchronization event");
}

void initialize_sntp(void){
	ESP_LOGI(TAG, "Initializing SNTP");
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	//sntp_setservername(0, "pool.ntp.org");
	ESP_LOGI(TAG, "Your NTP Server is %s", NTP_SERVER);
	sntp_setservername(0, NTP_SERVER);
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
	sntp_init();
}

bool obtain_time(void){
	ESP_ERROR_CHECK( nvs_flash_init() );
	ESP_ERROR_CHECK( esp_netif_init() );
	ESP_ERROR_CHECK( esp_event_loop_create_default() );

	/* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
	 * Read "Establishing Wi-Fi or Ethernet Connection" section in
	 * examples/protocols/README.md for more information about this function.
	 */
	ESP_ERROR_CHECK(example_connect());

	initialize_sntp();

	// wait for time to be set
	int retry = 0;
	const int retry_count = 10;
	while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
		ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}

	ESP_ERROR_CHECK( example_disconnect() );
	if (retry == retry_count) return false;
	return true;
}




struct tm get_clock_from_rtc(i2c_dev_t dev){
	// Get RTC date and time
	struct tm rtcinfo;
	if (ds3231_get_time(&dev, &rtcinfo) != ESP_OK) {
		ESP_LOGE(pcTaskGetName(0), "Could not get time.");
		while (1) { vTaskDelay(1); }
	}
	rtcinfo.tm_year = rtcinfo.tm_year;
	rtcinfo.tm_isdst = -1;
	ESP_LOGI(pcTaskGetName(0), "%04d-%02d-%02d %02d:%02d:%02d", 
		rtcinfo.tm_year, rtcinfo.tm_mon + 1,
		rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec);
	
	return rtcinfo;
}



void syncronize_from_received_time(i2c_dev_t dev,struct tm received_time){

	if (ds3231_set_time(&dev, &received_time) != ESP_OK) {
		ESP_LOGE(pcTaskGetName(0), "Could not set time.");
		while (1) { vTaskDelay(1); }
	}
	ESP_LOGI(pcTaskGetName(0), "Set initial date time done");

}

i2c_dev_t initialize_rtc(){
	i2c_dev_t dev;
	if (ds3231_init_desc(&dev, I2C_NUM_0, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO) != ESP_OK) {
		ESP_LOGE(pcTaskGetName(0), "Could not init device descriptor.");
		while (1) { vTaskDelay(1); }
	}
	return dev;
}

float get_temperature(i2c_dev_t dev){
	float temp;
	if (ds3231_get_temp_float(&dev, &temp) != ESP_OK) {
		ESP_LOGE(pcTaskGetName(0), "Could not get temperature.");
		while (1) { vTaskDelay(1); }
	}
	return temp;
}

void set_alarm_time(i2c_dev_t dev, struct tm time){
	if (ds3231_set_alarm(&dev, &time) != ESP_OK) {
		ESP_LOGE(pcTaskGetName(0), "Could not set alarm.");
		while (1) { vTaskDelay(1); }
	}
}

void sync_from_ntp(i2c_dev_t dev){

	ESP_LOGI(pcTaskGetName(0), "Connecting to WiFi and getting time over NTP.");
	if(!obtain_time()) {
		ESP_LOGE(pcTaskGetName(0), "Fail to getting time over NTP.");
		while (1) { vTaskDelay(1); }
	}

	time_t now;
	struct tm timeinfo;
	time(&now);
	now = now + (TIMEZONE*60*60);
	localtime_r(&now, &timeinfo);

	if (ds3231_set_time(&dev, &timeinfo) != ESP_OK) {
		ESP_LOGE(pcTaskGetName(0), "Could not set time.");
		while (1) { vTaskDelay(1); }
	}
	ESP_LOGI(pcTaskGetName(0), "Set initial date time done");

}

void reset_alarms(i2c_dev_t dev){
	if (ds3231_reset_alarm(&dev) != ESP_OK) {
		ESP_LOGE(pcTaskGetName(0), "Could not reset alarm.");
		while (1) { vTaskDelay(1); }
	}
}

void configure_alarms(i2c_dev_t dev){
	if (ds3231_alarm_config(&dev) != ESP_OK) {
		ESP_LOGE(pcTaskGetName(0), "Could not configure alarm.");
		while (1) { vTaskDelay(1); }
	}
}

void control_registers_status(i2c_dev_t dev){
	uint8_t data[2];
	if (ds3231_read_control_registers(&dev,data) != ESP_OK) {
		ESP_LOGE(pcTaskGetName(0), "Could not get control registers.");
		while (1) { vTaskDelay(1); }
	}
	ESP_LOGI(pcTaskGetName(0), "Control registers in binary: %d%d%d%d%d%d%d%d",
		(data[0] >> 7) & 1,
		(data[0] >> 6) & 1,
		(data[0] >> 5) & 1,
		(data[0] >> 4) & 1,
		(data[0] >> 3) & 1,
		(data[0] >> 2) & 1,
		(data[0] >> 1) & 1,
		data[0] & 1);
	ESP_LOGI(pcTaskGetName(0), "Status registers in binary: %d%d%d%d%d%d%d%d",
		(data[1] >> 7) & 1,
		(data[1] >> 6) & 1,
		(data[1] >> 5) & 1,
		(data[1] >> 4) & 1,
		(data[1] >> 3) & 1,
		(data[1] >> 2) & 1,
		(data[1] >> 1) & 1,
		data[1] & 1);
}


