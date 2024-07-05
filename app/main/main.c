#include "main_protocol.h"
#include "protocol.c"
#include "rtc.c"

#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "ble_key.c"

#include "signature.c"
#include "wifi.c"
#include "mqtt.c"



#define MAIN_TAG "Main"

QueueHandle_t interQueue;
i2c_dev_t dev;
int count =0;
int pinNumber=0;

int flag_temperature;
int flag_gas;
int flag_capacity;



// HAVE to create the three buffers for all the alarms
QueueHandle_t alarmQueue1;

static void IRAM_ATTR gpio_interrupt_handler(void *args){
    int pinNumber = (int) args;
    xQueueSendFromISR(interQueue, &pinNumber, NULL);
}

#define ALARM_PIN 25





void app_main(){
    /*
	if(wifi){
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

	wifi_init_sta();
	
    }
    */
	


	/*
    esp_mqtt_client_handle_t client = mqtt_app_start();

	esp_mqtt_client_publish(client, "/test", "HELLO WORLD", 0, 1, 0);
    */

    
    
    ble_obtain_key(key_copy);
    printf("Key: %.*s\n",64, key_copy);
    printf("Size of key: %d\n", sizeof(key_copy));
    nimble_port_stop();

    nvs_flash_erase();
    nvs_flash_init();

    if (wifi){wifi_init_sta();}

   /*

	uint8_t message_to_hash[10];
	uint8_t hash[SHA256_BLOCK_SIZE];
	printf("The block size is: %d\n",SHA256_BLOCK_SIZE);
	generate_signature(hash, message_to_hash, sizeof(message_to_hash));
	printf("Hash: ");
	
	for(int i = 0; i < SHA256_BLOCK_SIZE; i++){
		printf("%02x", hash[i]);
	}
	printf("\n");
	*/
	
	esp_rom_gpio_pad_select_gpio(ALARM_PIN);
    gpio_set_direction(ALARM_PIN, GPIO_MODE_INPUT);
	gpio_pulldown_dis(ALARM_PIN);
    gpio_pullup_en(ALARM_PIN);
	gpio_set_intr_type(ALARM_PIN, GPIO_INTR_POSEDGE);
	interQueue = xQueueCreate(10, sizeof(int));
	gpio_install_isr_service(0);
	gpio_isr_handler_add(ALARM_PIN, gpio_interrupt_handler, (void*) ALARM_PIN);
	//xTaskCreate(alarm_task, "alarm_task", 4096, NULL, 1, NULL);
	
	
	// Initialize the RTC
    
    dev = initialize_rtc();
	configure_alarms(dev);
	reset_alarms(dev);
	ds3231_alarm_config(&dev);    
    if(wifi) sync_from_ntp(dev);

	esp_sleep_enable_ext0_wakeup(ALARM_PIN, 1);

    // Sensors init
	garbage_sensors_t sensors = {
		
		.us_1 = {
			.trigger_pin = TRIGGER_GPIO_SENSOR_1,
			.echo_pin = ECHO_GPIO_SENSOR_1,
		},
		.us_2 =  {
			.trigger_pin = TRIGGER_GPIO_SENSOR_2,
			.echo_pin = ECHO_GPIO_SENSOR_2,
		},
		
		.mq = {
			.type = "MQ-135",
			.adc_pin = MQ_ADC_SENSOR_1,
			.adc_unit = ADC_UNIT_1,
			.volt_resolution = 5.0,				// Volt Resolution: 5.0V or 3.3 V
			.rl = 10.0f,						// Resistence value in kiloOhms
			.adc_calibrated = false,
			.regression_method = 1, 			// Regression method: 1 (exponential) or 2 (linear)
			
		},
		
	};

	garbage_sensors_init(&sensors);
	
    //starting the monitor Taks
    parameters_monitor = malloc(sizeof(monitor_task_parameters));
    parameters_monitor->dev = dev;
    parameters_monitor->capacity_flag = &flag_capacity;
    parameters_monitor->temperature_flag = &flag_temperature;
    parameters_monitor->gas_flag = &flag_gas;
    parameters_monitor->sensors = &sensors;

    protocol_task_parameters* parameters_protocol = malloc(sizeof(protocol_task_parameters));
    parameters_protocol->dev = dev;
    parameters_protocol->capacity_flag = &flag_capacity;
    parameters_protocol->temperature_flag = &flag_temperature;
    parameters_protocol->gas_flag = &flag_gas;

    flag_capacity = 0;
    flag_temperature = 0;
    flag_gas = 0;

    filling_levels = malloc(sizeof(float)*24);
    filling_lenght = 24;

   	//xTaskCreate(monitor_task , "monitor_task", configMINIMAL_STACK_SIZE*3 , parameters_monitor, 5, NULL);
    
	ESP_LOGI(MAIN_TAG, "Setup Protocol");
    lora_setup();

    xTaskCreate(protocol,"protocol_task", 8192, parameters_protocol, 1, NULL);
    
    ESP_LOGI(MAIN_TAG, "Starting regular task");
    reset_alarms(dev);
   
	
}