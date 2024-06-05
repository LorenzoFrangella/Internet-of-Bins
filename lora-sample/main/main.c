#include <stdio.h>
#include "main.h"
#include "lora.c"
#include "protocol_sample.c"

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
        receive_data(receive_callback, device);
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
            receive_data(receive_callback, device);
            while(!received){
                vTaskDelay(10000 / portTICK_PERIOD_MS);
            }
            received = false;
        }

    } else if (b == 1){ // Receiveing
        ESP_LOGI(LORA_TAG, "Receving first procedure");

        while(1){
            receive_data(receive_callback, device);
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

void protocol_receive_callback(sx127x *device, uint8_t *data, uint16_t data_length){
    struct protocol_message* my_pm = (struct protocol_message *)(data);
    
    char *frase = malloc(sizeof(char)*(my_pm->number_of_alerts+1));
    int i = 0;
    for (i = 0; i < my_pm->number_of_alerts; i++){
      frase[i] = (char)my_pm->alerts[i];
      //printf("char %d is %c \n", i, frase[i]);
    }
    frase[i] = '\0';

    ESP_LOGI(LORA_TAG, "Received from: %d, Alerts: %s, Gateway: %d Discover: %d", my_pm->id, frase, my_pm->a_structure.gateway_id, my_pm->discover);
    received = true;
}

void  demo_protocol_task(int b){

    sx127x_tx_set_callback(sending_callback, device);

    ESP_LOGI(LORA_TAG, "My id is: %d", pm1.id);
    data_to_send = &pm1;
    data_send_lenght = sizeof(data_to_send);

    struct protocol_message* my_pm = (struct protocol_message *)(data_to_send);
    
    char *frase = malloc(sizeof(char)*(my_pm->number_of_alerts+1));
    int i = 0;
    int* rc_alerts = sizeof(int)*my_pm->number_of_alerts;
    memcpy(rc_alerts,my_pm->alerts,sizeof(int)*my_pm->number_of_alerts);
    for (i = 0; i < my_pm->number_of_alerts; i++){
        ESP_LOGI(LORA_TAG, "EGHIMI");
      frase[i] = (char)(rc_alerts[i]);
      //printf("char %d is %c \n", i, frase[i]);
    }
    frase[i] = '\0';
    

    ESP_LOGI(LORA_TAG, "EGHIMI2");

    ESP_LOGI(LORA_TAG, "Message to send. ID: %d, Alerts: %s, Gateway: %d Discover: %d", my_pm->id, frase, my_pm->a_structure.gateway_id, my_pm->discover);
    

    if (b == 0){ // Sending

        ESP_LOGI(LORA_TAG, "Sending first procedure");
    
        while(1){
            send_data(data_to_send, data_send_lenght, device);
            receive_data(protocol_receive_callback,device);
            while(!received){
                vTaskDelay(10000 / portTICK_PERIOD_MS);
            }
            received = false;
        }

    } else if (b == 1){ // Receiveing
        ESP_LOGI(LORA_TAG, "Receving first procedure");

        while(1){
            receive_data(protocol_receive_callback, device);
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
    
    xTaskCreate(demo_protocol_task, 'switching_task', 4096, b, 5, &task);
}

