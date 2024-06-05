#include <stdio.h>
#include "main.h"
#include "lora.c"

TaskHandle_t task = NULL;

//#include "receive.c"
//#include "send.c"

void demo_casual_task(int b){
    if (b == 0){ // Sending
        ESP_LOGI(LORA_TAG, "Sending procedure");
        sx127x_tx_set_callback(sending_callback, device);

        data_to_send = malloc(sizeof(uint8_t)*3);
        data_to_send[0] = (uint8_t)'1'; 
        data_to_send[1] = (uint8_t)'2';
        data_to_send[2] = (uint8_t)'3';
        data_send_lenght = 3;
    
        send_data(data_to_send, data_send_lenght, device);
        
        vTaskDelay(1000/portTICK_PERIOD_MS);

        data_to_send[0] = (uint8_t)'4'; 
        data_to_send[1] = (uint8_t)'5';
        data_to_send[2] = (uint8_t)'6';
        
        send_data(data_to_send, data_send_lenght, device);


    } else if (b == 1){ // Receiveing
        ESP_LOGI(LORA_TAG, "Receving procedure");
        receive_data(device);
    }else{
        vTaskDelete(NULL);
    }
    
    while (1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void demo_switching_task(int b){

    sx127x_tx_set_callback(sending_callback, device);

    data_to_send = malloc(sizeof(uint8_t)*3);
    data_to_send[0] = (uint8_t)'1'; 
    data_to_send[1] = (uint8_t)'2';
    data_to_send[2] = (uint8_t)'3';
    data_send_lenght = 3;

    if (b == 0){ // Sending

        ESP_LOGI(LORA_TAG, "Sending first procedure");
    
        while(1){
            send_data(data_to_send, data_send_lenght, device);
            receive_data(device);
            while(!received){
                vTaskDelay(10000 / portTICK_PERIOD_MS);
            }
            received = false;
        }

    } else if (b == 1){ // Receiveing
        ESP_LOGI(LORA_TAG, "Receving first procedure");

        while(1){
            receive_data(device);
            while(!received){
                vTaskDelay(10000 / portTICK_PERIOD_MS);
            }
            received = false;
            send_data(data_to_send, data_send_lenght, device);
        }
    }else{
        vTaskDelete(NULL);
    }
    
    while (1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    //app_main_receive();
    //app_main_send();

    lora_setup();

    bootloader_random_enable();
    int b = ((int)esp_random())%2;
    
    xTaskCreate(demo_switching_task, 'switching_task', 2048, b, 5, &task);
}

