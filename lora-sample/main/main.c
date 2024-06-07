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
    

    char * pippo = (char)(&data_to_send);
    
    ESP_LOGI(LORA_TAG, "Frase è: %s", pippo);

    received = true;
}

void  demo_protocol_task(int b){

    sx127x_tx_set_callback(sending_callback, device);

    
    ESP_LOGI(LORA_TAG, "My id is: %d", pm1.id);

    size_t partial_size = sizeof(int) * (1 + 1 + 1) + sizeof(bool) + sizeof(struct node_structure);
    ESP_LOGI(LORA_TAG, "Partial size of protocol message is: %u", partial_size);
    size_t size_of_array = sizeof(int) * pm1.number_of_alerts;
    ESP_LOGI(LORA_TAG, "Size of array : %u", size_of_array);
    size_t total_size = partial_size + size_of_array;
    ESP_LOGI(LORA_TAG, "Size of protocol message is: %u", total_size);
    size_t actual_size = total_size + (sizeof(uint8_t) - total_size%sizeof(uint8_t));
    ESP_LOGI(LORA_TAG, "Actual size of protocol message is: %u", actual_size);

    data_to_send = malloc(actual_size);
    ESP_LOGI(LORA_TAG, "Malloc data to send");
    memcpy(data_to_send,&pm1, partial_size);
    ESP_LOGI(LORA_TAG, "first memcpy");
    uint8_t *pointer_to_array = data_to_send + partial_size;
    memcpy(pointer_to_array, &(pm1.alerts), size_of_array);
    ESP_LOGI(LORA_TAG, "second memcpy");
    data_send_lenght = actual_size;

    int* other_pointer = (int*)pointer_to_array;
    printf("First number is: %d\n", other_pointer[0]);

    struct protocol_message* my_pm = malloc(total_size);
    ESP_LOGI(LORA_TAG, "malloc protocol message");
    memcpy(my_pm, data_to_send, total_size);
    ESP_LOGI(LORA_TAG, "protocol message memcpy");
    
    
    char *frase = malloc(sizeof(char)*(my_pm->number_of_alerts+1));
    ESP_LOGI(LORA_TAG, "Number of alerts: %d", my_pm->number_of_alerts);

    size_t size_of_received_array = sizeof(int) * my_pm->number_of_alerts;
    int* rc_alerts = malloc(size_of_received_array);
    printf("First number is: %d\n", my_pm->alerts[0]);
    memcpy(rc_alerts,&(my_pm->alerts),size_of_received_array);
    ESP_LOGI(LORA_TAG, "Malloc rc");

    int i = 0;
    for (i = 0; i < my_pm->number_of_alerts; i++){
        ESP_LOGI(LORA_TAG, "EGHIMI");
        frase[i] = (char)(rc_alerts[i]);
        printf("char %d is %c as %d \n", i, frase[i], rc_alerts[i]);
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

