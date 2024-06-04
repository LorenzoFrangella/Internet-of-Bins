#include <stdio.h>
#include "main.h"
#include "lora.c"


//#include "receive.c"
//#include "send.c"

void app_main(void)
{
    //app_main_receive();
    //app_main_send();

    lora_setup();

    bootloader_random_enable();
    int b = ((int)esp_random())%2;
    if (b == 0){ // Sending
        ESP_LOGI(LORA_TAG, "Sending procedure");
        sx127x_tx_set_callback(sending_callback, device);
        data_to_send = malloc(sizeof(uint8_t)*3);
        data_to_send[0] = (uint8_t)'1'; 
        data_to_send[1] = (uint8_t)'2';
        data_to_send[2] = (uint8_t)'3';
        data_send_lenght = 3;
        sending_callback(device);
    } else if (b == 1){ // Receiveing
        ESP_LOGI(LORA_TAG, "Receving procedure");
        sx127x_rx_set_callback(receive_callback, device);
        sx127x_lora_cad_set_callback(cad_callback, device);
        ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, SX127x_MODULATION_LORA, device));
    }else{
        vTaskDelete(NULL);
    }
    
    while (1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

