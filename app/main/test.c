#include "main.h"

void task_tx(void *p)
{
   for(;;) {
        printf("sending packet...\n");
        vTaskDelay(pdMS_TO_TICKS(100));
        lora_send_packet((uint8_t*)"Hello my name is lora device01!!", 32);
        printf("packet sent...\n");
   }
}

uint8_t buf[32];

void receive_tx(void *p)
{
   printf("Starting...");
   int x;
   for(;;) {
    
      printf("Trying to receive...");
      lora_receive();    // put into receive mode
      while(lora_received()) {
         x = lora_receive_packet(buf, sizeof(buf));
         buf[x] = 0;
         printf("Received: %s\n", buf);
         lora_receive();
      }
      vTaskDelay(pdMS_TO_TICKS(1));
   }
}