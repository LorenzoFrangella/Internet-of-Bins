#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>


#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <ultrasonic.h>

#include "esp_random.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include <esp_log.h>
#include <esp_err.h>

#include <sensors.h>

#define MAX_DISTANCE_CM 500 
#define GARB_AVG_SIZE_CM 0			

#define TRIGGER_GPIO_SENSOR_1 32
#define ECHO_GPIO_SENSOR_1 33

#define TRIGGER_GPIO_SENSOR_2 4
#define ECHO_GPIO_SENSOR_2  2

#define MQ_ADC_SENSOR_1 ADC_CHANNEL_7
#define ADC_ATTEN ADC_ATTEN_DB_12
#define RatioMQ135CleanAir 3.6
#define GAS_CO2_THRESHOLD 100



const static char *SENSORS_TAG = "MQ";								// TAG for MQ 135 sensor
const static char *US_1_TAG = "US 1";						// TAG for Ultrasonic sensor 1
const static char *US_2_TAG = "US 2";						// TAG for Ultrasonic sensor 2

uint8_t ultrasonic_error(uint8_t n_sensor, esp_err_t  err)
{
	if (err != ESP_OK)
	{
		printf("Error (sensor %u) %d: ", n_sensor, err);
		switch (err)
		{
			case ESP_ERR_ULTRASONIC_PING:
				printf("Cannot ping (device is in invalid state)\n");
				break;
			case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
				printf("Ping timeout (no device found)\n");
				break;
			case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
				printf("Echo timeout (i.e. distance too big)\n");
				break;
			default:
				printf("%s\n", esp_err_to_name(err));
          }
          
          return 1; 
	}
	
	return 0;
}

/**********************************************************************/
/* ----------------------- MQ sensor -------------------------------- */

typedef struct {
	
	// Attributes of the MQ sensor
	char* type;
	int adc_pin ; 
	int8_t adc_unit; 
	int8_t adc_bit_resolution; 
	float volt_resolution; 						 
	
	int adc_raw;
	int sensor_volt;
	
	float rl; 							
	
	int8_t regression_method;			
	
	
    float a, b;
    float r0, rs_air, ratio, ppm, rs_calc; 
    
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config;
    adc_oneshot_chan_cfg_t config; 
    adc_cali_handle_t adc_cali_chan_handle; 
    
    bool adc_calibrated;
	

} mq_sensor_t;

typedef struct{
	i2c_dev_t dev;
	int* capacity_flag;
	int* gas_flag;
	int* temperature_flag;
	garbage_sensors_t* sensors;
} monitor_task_parameters;

void mq_sensor_init(mq_sensor_t* sensor)
{
	
	// MQ Sensor: set initial ADC configuration
	sensor->init_config.unit_id = sensor->adc_unit; 
    
    sensor->config.bitwidth = sensor->adc_bit_resolution;
    sensor->config.atten = ADC_ATTEN_DB_12;
	
	// MQ sensor: ADC configuration
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&sensor->init_config, &sensor->adc_handle));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(sensor->adc_handle, sensor->adc_pin, &sensor->config));
	
	
	// MQ sensor: ADC calibration
	adc_cali_handle_t handle = NULL;
	esp_err_t ret = ESP_FAIL;
	
	ESP_LOGI(SENSORS_TAG, "calibration scheme version is %s", "Line Fitting");
	adc_cali_line_fitting_config_t cali_config = {
		.unit_id = sensor->adc_unit,
		.atten = ADC_ATTEN_DB_12,
		.bitwidth = sensor->adc_bit_resolution,
	};
	
	ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
	
	if (ret == ESP_OK) {
		sensor->adc_calibrated = true;
	}
	
	sensor->adc_cali_chan_handle = handle;
	
	if (ret == ESP_OK) {
        ESP_LOGI(SENSORS_TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !sensor->adc_calibrated ) {
        ESP_LOGW(SENSORS_TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(SENSORS_TAG, "Invalid arg or no memory");
    }
    
}

float mq_sensor_get_voltage(mq_sensor_t* sensor)
{
	
	int avg = 0;
	for(int i = 0; i < 10; i++){
		ESP_ERROR_CHECK(adc_oneshot_read(sensor->adc_handle, sensor->adc_pin, &sensor->adc_raw));
		avg += sensor->adc_raw;
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	sensor->adc_raw = avg / 10;
    if (sensor->adc_calibrated) {
		ESP_ERROR_CHECK(adc_cali_raw_to_voltage(sensor->adc_cali_chan_handle, sensor->adc_raw, &sensor->sensor_volt));
    }
    
    return sensor->sensor_volt;
	
}

void mq_sensor_update(mq_sensor_t* sensor)
{
	sensor->sensor_volt = mq_sensor_get_voltage(sensor);
}



float mq_sensor_calibrate(mq_sensor_t* sensor, float ratio_in_clean_air)
{
	
	 /*
	  * More explained in: https://jayconsystems.com/blog/understanding-a-gas-sensor
	  * V = I x R
	  * VRL = [VC / (RS + RL)] x RL 
	  * VRL = (VC x RL) / (RS + RL) 
	  * 
	  * Así que ahora resolvemos para RS:
	  * VRL x (RS + RL) = VC x RL
	  * (VRL x RS) + (VRL x RL) = VC x RL 
	  * (VRL x RS) = (VC x RL) - (VRL x RL)
	  * RS = [(VC x RL) - (VRL x RL)] / VRL
	  * RS = [(VC x RL) / VRL] - RL
	*/
	
	float rs_air;   // Define variable for sensor resistance
	float r0;		// Define variable for r0
	float voltage = (float)sensor->sensor_volt / 1000.f;
	
	//Calculate RS in fresh air
	rs_air = ((sensor->volt_resolution * sensor->rl) / voltage ) - sensor->rl;
	if(rs_air < 0 ) rs_air = 0; 			// No negative values accepted
	
	// Calculate r0
	r0 = rs_air/ratio_in_clean_air;	
	if (r0 < 0) r0 = 0; 					// No negative values accepted
	
	return r0;
}	


float mq_sensor_read_sensor(mq_sensor_t* sensor, float correction_factor, bool injected)
{
	//Convert mV in V
	float voltage = (float) sensor->sensor_volt / 1000.0f;
	
	//Get value of RS in a gas
	sensor->rs_calc = ((sensor->volt_resolution * sensor->rl) / voltage) - sensor->rl;
	if(sensor->rs_calc < 0) sensor->rs_calc = 0; 	// No negative value accepted
	
	if(!injected) sensor->ratio = sensor->rs_calc / sensor->r0;			// Get ratio RS_gas/RS_air
	sensor->ratio += correction_factor;
	
	if(sensor->ratio <= 0 ) sensor->ratio = 0; //No negative values accepted or upper datasheet recomendation.
	if(sensor->regression_method == 1) {
		sensor->ppm= sensor->a*powf(sensor->ratio, sensor->b);
	} 
	else 
	{
		//Get ppm value in linear scale according to the the ratio value
		double ppm_log = (log10(sensor->ratio)-sensor->b)/sensor->a;   
		sensor->ppm = powf(10, ppm_log); //Convert ppm value to log scale  
	}
	
	if(sensor->ppm < 0)  sensor->ppm = 0; //No negative values accepted or upper datasheet recomendation.
	
	
	return sensor->ppm;
}
	



/*-------------------------------------------------------------------- */
/**********************************************************************/

// ---------------- Garbage Libary ----------------------------
// ------------------------------------------------------------

typedef struct {
	
	// Sensors
	ultrasonic_sensor_t us_1;
	ultrasonic_sensor_t us_2;
	mq_sensor_t mq;
	
	// Bin parameters
	float garbage_avg_size;
	float bin_size;
	float bin_current_size; 
	
	// Bin status parameters
	float gas_ppm;
	float capacity;
	
} garbage_sensors_t;

void garbage_sensors_init(garbage_sensors_t* sensors){
			
			if(sensors == NULL) return ; 

			// Init
			ultrasonic_init(&sensors->us_1);
			ultrasonic_init(&sensors->us_2);
			mq_sensor_init(&sensors->mq);
			
			// - Configuration
			float sum_1 = 0;
			float distance_1;
			for(int i = 0; i < 10; i++){
				esp_err_t res_1 = ultrasonic_measure(&sensors->us_1, MAX_DISTANCE_CM, &distance_1);
				ultrasonic_error(1, res_1);
				sum_1 += distance_1;
				vTaskDelay(pdMS_TO_TICKS(20));
			}
			distance_1 = sum_1 / 10;
			
			float sum_2 = 0;
			float distance_2;
			for(int i=0; i < 10; i++){
				esp_err_t res_2 = ultrasonic_measure(&sensors->us_2, MAX_DISTANCE_CM, &distance_2);
				ultrasonic_error(2, res_2);
				sum_2 += distance_2;
				vTaskDelay(pdMS_TO_TICKS(20));
			}
			distance_2 = sum_2 / 10;

			// - Set bin parameters
			sensors->garbage_avg_size = GARB_AVG_SIZE_CM;
			sensors->bin_size = (((distance_1 + distance_2) * 100) / 2) - sensors->garbage_avg_size;
			
			
			// MQ Sensors init and configuration
	
			
			// - Configuration
			float calc_r0 = 0;
			for(int i = 0; i < 10; i++)
			{
				mq_sensor_update(&sensors->mq);
				calc_r0 += mq_sensor_calibrate(&sensors->mq , RatioMQ135CleanAir);
				vTaskDelay(pdMS_TO_TICKS(20));
			}	
	
			sensors->mq.r0 = calc_r0/10.f;
			
			
			return;
}

float garbage_sensors_get_capacity(garbage_sensors_t* sensors){

	if(sensors == NULL) return (esp_random() % 100 ) / 100.f;
	
	// Read the capacity of the bin
	float sum_1 = 0;
	float distance_1;
	for(int i = 0; i < 10; i++){
		esp_err_t res_1 = ultrasonic_measure(&sensors->us_1, MAX_DISTANCE_CM, &distance_1);
		ultrasonic_error(1, res_1);			

		sum_1 += distance_1;
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	distance_1 = sum_1 / 10;
			
	float sum_2 = 0;
	float distance_2;
	for(int i=0; i < 10; i++){
		esp_err_t res_2 = ultrasonic_measure(&sensors->us_2, MAX_DISTANCE_CM, &distance_2);
		ultrasonic_error(2, res_2);
		sum_2 += distance_2;
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	distance_2 = sum_2 / 10;
	
	
			
	sensors->bin_current_size = (((distance_1 + distance_2) * 100) / 2) - sensors->garbage_avg_size;
	sensors->capacity = 1.f - (sensors->bin_current_size / sensors->bin_size) ;

	if(sensors->capacity < 0) sensors->capacity = 0;
	
	return sensors->capacity;  
	
}


	/*
	 * Exponential regression:
	 * GAS      | a      | b
	 * CO       | 605.18 | -3.937  
	 * Alcohol  | 77.255 | -3.18 
	 * CO2      | 110.47 | -2.862
	 * Toluen   | 44.947 | -3.445
	 * NH4      | 102.2  | -2.473
	 * Aceton   | 34.668 | -3.369
	*/
	
float garbage_sensors_get_gas_CO(garbage_sensors_t* sensors){
	
	if(sensors == NULL) {
		int p = esp_random() % 100;
		if (p < 60) {
			return (float) (esp_random() % GAS_CO2_THRESHOLD);
		}
		else {
			return (float) GAS_CO2_THRESHOLD + (esp_random() % 100);
		}
	}

	sensors->mq.a = 605.18; 
	sensors->mq.b = -3.937;
	
	float sum = 0;
	for(int i = 0; i < 10; i++){
		sum += mq_sensor_read_sensor(&sensors->mq, 0.0, false);
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	sensors->gas_ppm = sum / 10;
	return sensors->gas_ppm;
}

float garbage_sensors_get_gas_Alcohol(garbage_sensors_t* sensors){
	
	if(sensors == NULL) {
		int p = esp_random() % 100;
		if (p < 60) {
			return (float) (esp_random() % GAS_CO2_THRESHOLD);
		}
		else {
			return (float) GAS_CO2_THRESHOLD + (esp_random() % 100);
		}
	}

	sensors->mq.a = 77.255; 
	sensors->mq.b = -3.18;
	float sum = 0;
	for(int i = 0; i < 10; i++){
		sum += mq_sensor_read_sensor(&sensors->mq, 0.0, false);
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	sensors->gas_ppm = sum / 10;
	
	return sensors->gas_ppm;
}

float garbage_sensors_get_gas_CO2(garbage_sensors_t* sensors){
	
	if(sensors == NULL) {
		int p = esp_random() % 100;
		if (p < 60) {
			return (float) (esp_random() % GAS_CO2_THRESHOLD);
		}
		else {
			return (float) GAS_CO2_THRESHOLD + (esp_random() % 100);
		}
	}

	sensors->mq.a = 110.47; 
	sensors->mq.b = -2.862;
	float sum = 0;
	for(int i = 0; i < 10; i++){
		sum += mq_sensor_read_sensor(&sensors->mq, 0.0, false);
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	sensors->gas_ppm = sum / 10;
	
	return sensors->gas_ppm;
}

float garbage_sensors_get_gas_Toluen(garbage_sensors_t* sensors){
	
	if(sensors == NULL) {
		int p = esp_random() % 100;
		if (p < 60) {
			return (float) (esp_random() % GAS_CO2_THRESHOLD);
		}
		else {
			return (float) GAS_CO2_THRESHOLD + (esp_random() % 100);
		}
	}

	sensors->mq.a = 44.947; 
	sensors->mq.b = -3.445;
	float sum = 0;
	for(int i = 0; i < 10; i++){
		sum += mq_sensor_read_sensor(&sensors->mq, 0.0, false);
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	sensors->gas_ppm = sum / 10;
	
	return sensors->gas_ppm;
}

float garbage_sensors_get_gas_NH4(garbage_sensors_t* sensors){
	
	if(sensors == NULL) {
		int p = esp_random() % 100;
		if (p < 60) {
			return (float) (esp_random() % GAS_CO2_THRESHOLD);
		}
		else {
			return (float) GAS_CO2_THRESHOLD + (esp_random() % 100);
		}
	}

	sensors->mq.a = 102.2;  
	sensors->mq.b = -2.473;
	float sum = 0;
	for(int i = 0; i < 10; i++){
		sum += mq_sensor_read_sensor(&sensors->mq, 0.0, false);
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	sensors->gas_ppm = sum / 10;
	
	return sensors->gas_ppm;
}

float garbage_sensors_get_gas_Aceton(garbage_sensors_t* sensors){

	if(sensors == NULL) {
		int p = esp_random() % 100;
		if (p < 60) {
			return (float) (esp_random() % GAS_CO2_THRESHOLD);
		}
		else {
			return (float) GAS_CO2_THRESHOLD + (esp_random() % 100);
		}
	}

	sensors->mq.a = 34.668;  
	sensors->mq.b = -3.369;
	float sum = 0;
	for(int i = 0; i < 10; i++){
		sum += mq_sensor_read_sensor(&sensors->mq, 0.0, false);
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	sensors->gas_ppm = sum / 10;
	
	return sensors->gas_ppm;
}

// ------------------------------------------------------------
// ------------------------------------------------------------


int time_to_next_gather(i2c_dev_t dev){	
	struct tm time_g = get_clock_from_rtc(dev);
	time_t t_time = mktime(&time_g);
	
	return (t_time - last_gather)/60;
}

void emptied(int wind_since_gather){
	filling_occupation = -1;
	
	if(gathering_collected < 6){
		gathering_collected += 1;
		gathering_times[gathering_collected] = wind_since_gather;
	}else{
		for (int i = 0; i < gathering_collected; i++){
			gathering_times[i] = gathering_times[i+1];
		}
		gathering_times[6] = wind_since_gather;
	}

	ESP_LOGW(SENSORS_TAG, "Emptied!");
}

void add_fill(int capacity){
	filling_occupation += 1;

	if (filling_occupation > filling_lenght){
		filling_lenght += 24;
		filling_levels = realloc(filling_levels, sizeof(int)*filling_lenght);
	}

	filling_levels[filling_occupation] = capacity;

}

void monitor_sensors(void *pvParameters)
{
	monitor_task_parameters* parameters = (monitor_task_parameters*) pvParameters;
	float temperature = get_temperature(parameters->dev);
	ESP_LOGE("Monitor Task", "Temperature: %f", temperature);
	ESP_LOGE("Monitor Task","The capacity flag is %d", *(parameters->capacity_flag));
	ESP_LOGE("Monitor Task","The temperature flag is %d", *(parameters->temperature_flag));
	ESP_LOGE("Monitor Task","The gas flag is %d", *(parameters->gas_flag));

	garbage_sensors_t sensors = parameters->sensors;
    
	float distance_1;
	float distance_2;
	
	// MQ ADC sensor read
	mq_sensor_update(&sensors.mq);
	
		/*
		* Exponential regression:
		* GAS      | a      | b
		* CO       | 605.18 | -3.937  
		* Alcohol  | 77.255 | -3.18 
		* CO2      | 110.47 | -2.862
		* Toluen   | 44.947 | -3.445
		* NH4      | 102.2  | -2.473
		* Aceton   | 34.668 | -3.369
	*/
	
	// Analys: CO
	float ppm_co = garbage_sensors_get_gas_CO(&sensors);
	ESP_LOGI(SENSORS_TAG, " CO PPM: %0.04f", ppm_co);
	
	// Analys: Alcohol
	float ppm_alcohol = garbage_sensors_get_gas_Alcohol(&sensors);
	ESP_LOGI(SENSORS_TAG, " Alcohol PPM: %0.04f", ppm_alcohol);
	
	// Analys: CO2
	float ppm_co2 = garbage_sensors_get_gas_CO2(&sensors);
	ESP_LOGI(SENSORS_TAG, " CO2 PPM: %0.04f", ppm_co2);
	
	
	// Analys: Touluen
	float ppm_touluen = garbage_sensors_get_gas_Toluen(&sensors);
	ESP_LOGI(SENSORS_TAG, " Touluen PPM: %0.04f", ppm_touluen);
	
	// Analys: NH4
	float ppm_nh4 = garbage_sensors_get_gas_NH4(&sensors);
	ESP_LOGI(SENSORS_TAG, " NH4 PPM: %0.04f", ppm_nh4);
	
	// Analys: Aceton
	float ppm_aceton = garbage_sensors_get_gas_Aceton(&sensors);
	ESP_LOGI(SENSORS_TAG, " Aceton PPM: %0.04f", ppm_aceton);
	
	// Analys: Capcity
	float capacity = garbage_sensors_get_capacity(&sensors);
	ESP_LOGI(SENSORS_TAG, " Capacity: %0.04f ", capacity);

	// Check if emptied

	int wind_since_gather = time_since_last_gather(parameters->dev);
	int wind_to_gather = avg_windows_to_gather - wind_since_gather;
	if (wind_to_gather < 1){
		wind_to_gather = 1;
	}

	if (capacity < filling_levels[filling_occupation] && capacity < 5.0){
		emptied(wind_since_gather);
	}

	// Config

	add_fill(capacity);

	float sum = 0.0;
	float weight_sum = 0.0;
	for (int i = 0; i < filling_occupation; i++){
		sum += filling_levels[i];
		weight_sum += i;
	}
	filling_rate = sum/weight_sum;

	// Set alarms

	if(ppm_co2 > co2_tresh_1){ // Check level of CO2
		parameters->gas_flag = 2;
	}else if(wind_since_gather > 48){ // Check if too much time has passed
		parameters->gas_flag = 1;
	}else{
		parameters->gas_flag = 0;
	}

	float temperature = get_temperature(parameters->dev);

	if (temperature> 50.0){
		parameters->temperature_flag = 2;
	}else if(temperature > 40.0){
		parameters->temperature_flag = 1;
	}else{
		parameters->temperature_flag = 0;
	}

	if(capacity > 95.0){
		parameters->capacity_flag = 2;
	}else if (wind_to_gather*filling_rate + capacity){
		parameters->capacity_flag = 1;
	}else{
		parameters->capacity_flag = 0;
	}
	
	printf(" ---------------------------------------------------------------------\n");
	
	return;
    
}




