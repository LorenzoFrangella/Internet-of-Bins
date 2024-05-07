#include <stdio.h>
#include <lora.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>



void task_tx(void *p)
{
   for(;;) {
        printf("sending packet...\n");
        vTaskDelay(pdMS_TO_TICKS(100));
        lora_send_packet((uint8_t*)"Hello", 5);
        printf("packet sent...\n");
   }
}

void app_main(void){
    if(lora_init()==1)printf("Lora setup should be ok");
    lora_set_frequency(868e6);
    lora_enable_crc();
    xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, NULL);
}