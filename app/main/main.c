#include "main.h"

static TaskHandle_t my_task = NULL,

void app_main(void){
    if(lora_init()==1)printf("Lora setup should be ok \n");
    lora_set_frequency(868e6);
    lora_enable_crc();
    xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, my_task);
    //xTaskCreate(&receive_tx, "receive_tx", 2048, NULL, 5, NULL);
}