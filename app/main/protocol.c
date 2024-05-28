#include "main.h"
#include "message.h"

const char *PROTOCOL_TAG = "Protocol";
const char *GATHERING_TAG = "Gathering";
#define MISSING_HOUR_LIMIT = 24;

bool alert = false;
bool alone = true;
int level = NULL;
int gateway_id = NULL;
int max_known_level = NULL;
bool last_round_succeded = false;
int rounds_failed = -1;

bool future_discover = false;

struct node_status my_status;
struct node_status *cluster;
int cluster_occupation = -1;
int cluster_real_occupation = -1;
int cluster_lenght = 1;

int *not_seen_nodes;
int not_seen_nodes_lenght;

int *alerts;
int alerts_occupation;
int alerts_lenght;

int sleep_schedule[4];

void add_to_array(int *array, int *max_lenght, int *actual_lenght, int lenght_augment, int element){
    *(actual_lenght) += 1;

        // There is still space?
        if (*(max_lenght) > *(actual_lenght)){
            *(max_lenght) += lenght_augment;
            int *array2 = malloc(*(max_lenght)*sizeof(int));

            memcpy(&array2, &array, sizeof(array));
            free(array);

            array = array2;
        }

        array[*(actual_lenght)] = element;
}

void protocol_init(bool wifi_init){
    wifi = wifi_init;
}

int* end_of_hour_procedure(){

    // end of discovery
    if(discover){

        discover = false;
    }

    // need to be careful next round
    if (future_discover){
        discover = true;
        future_discover = false;
    }

    if(last_round_succeded){
        rounds_failed = 0;
    }else{
        rounds_failed++;
    }

    // Check if connected but not anymore
    if(rounds_failed == MISSING_HOUR_LIMIT){
        ESP_LOGI(PROTOCOL_TAG, "Node is alone");
        alone = true;
        level = -1;
        gateway_id = -1;
        max_known_level = -1;
        rounds_failed = -1;
        return [-2,-2];
    }else if (wifi){
        return [-1,max_known_level*2-1];
    }else{
        return [level-1,(max_known_level - level)*2-1];
    }
}


void protocol_hour_check(bool new_alert){
    alert = new_alert;
}

//***************************** MESSAGE HANDLING *******************************

int messages_starting_lenght = 10;
struct protocol_messages *messages;
int messages_lenght = 0;
int messages_occupation = 0;

struct protocol_message *messages2;

void add_to_messages(struct protocol_message message){

    messages_occupation += 1;

    // There is still space?
    if (messages_occupation > messages_lenght){
        messages_lenght += 5;
        messages2 = malloc(messages_lenght*sizeof(struct protocol_message));

        memcpy(&messages2, &messages, sizeof(messages));
        free(messages);

        cluster = messages2;
    }

    messages[messages_occupation] = message;

}

uint8_t gather_buf[sizeof(struct protocol_message)+sizeof(int)*1024];

int gather_messages(int time_to_listen){

    messages = malloc(messages_starting_lenght*sizeof(struct protocol_message));

    int64_t start_count = xx_time_get_time();
    int64_t end_count;
    
    ESP_LOGI(GATHERING_TAG,"Trying to receive...");
    struct protocol_message last_message;

    while (true) {
        
        lora_receive();    // put into receive mode
        while(lora_received()) {
            lora_receive_packet(gather_buf, sizeof(gather_buf));

            last_message = *(struct protocol_message*)gather_buf;
            add_to_messages(last_message);

            printf("Received a packet from: %d\n", last_message.id);
            lora_receive();
        }

        end_count = xx_time_get_time();

        if (end_count - start_count >= time_to_listen){
            ESP_LOGI(GATHERING_TAG, "Closing gathering window");
            vTaskDelete(NULL);
        }

        //Needed for watchdog?
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

TaskHandle_t gather_handler;

// Listen for better network configuration
void discover_listen(struct protocol_message message){

}

void discover_listening(int time_to_wait){

}

// Check if connected to wifi during this round
void first_listen(struct protocol_message message){
    if (message.gateway == gateway_id && message.level == level - 1 && message.last_round_succeded){
        last_round_succeded = true;
        if(message.discover){
            future_discover = true;
        }
    }
}

void first_listening(int time_to_wait){
    // gather all messages from LoRa
    ESP_LOGI(PROTOCOL_TAG, "Opening first gathering window");
    xTaskCreate(&gather_handler, "gather_first_listening_task", 2048, time_to_wait, 2, gather_messages);
    vTaskDelay(pdMS_TO_TICKS(time_to_wait));

    ESP_LOGI(PROTOCOL_TAG, "Processing messages");

    for (int i = 0; i < messages_occupation; i++){
        first_listen(messages[i]);
    }

    ESP_LOGI(PROTOCOL_TAG, "Messages processed");

    free(messages);
}

void first_talk(){
    ESP_LOGI(PROTOCOL_TAG, "First talk");
    struct protocol_message message = {
        id, level, max_known_level, gateway_id, last_round_succeded, NULL, discover, 0
    };
    lora_send_packet((uint8_t*)&message, sizeof(message));
    ESP_LOGI(PROTOCOL_TAG, "Message sent");
}

int* new_alerts;
int new_alert_occupation = 0;
int new_alerts_lenght = 0;

void second_listen(struct protocol_message message){
    if (message.gateway == gateway_id && message.level == level + 1){
        int number_of_messages = sizeof(message.alerts)/sizeof(int);
        for (int i = 0; i < number_of_messages; i++){
            bool new = true;
            for (int j = 0; j < alerts_lenght; j ++){
                if (message.alerts[i] == alerts[j]){
                    new == false;
                }
            }
            if (new){
                add_to_array(new_alerts, new_alerts_lenght, new_alert_occupation, 2, message.alerts[i];);
            }
        }

    }
}

void second_listening(int time_to_wait){
     // gather all messages from LoRa
    ESP_LOGI(PROTOCOL_TAG, "Opening second gathering window");
    xTaskCreate(&gather_handler, "gather_second_listening_task", 2048, time_to_wait, 2, gather_messages);
    vTaskDelay(pdMS_TO_TICKS(time_to_wait));

    ESP_LOGI(PROTOCOL_TAG, "Processing messages");

    for (int i = 0; i < messages_occupation; i++){
        second_listen(messages[i]);
    }

    for (int i = 0; i < new_alert_occupation; i++){
        add_to_array(alerts, alerts_lenght, alerts_occupation, 2, new_alerts[i]);
    }

    free(new_alerts);
    new_alert_occupation = 0;
    new_alerts_lenght = 0;

    ESP_LOGI(PROTOCOL_TAG, "Messages processed");

    free(messages);

    //Send alarms forward

}

void second_talk(){
    ESP_LOGI(PROTOCOL_TAG, "Second talk");
    struct protocol_message message = {
        id, level, max_known_level, gateway_id, last_round_succeded, alerts, discover, 0
    };
    lora_send_packet((uint8_t*)&message, sizeof(message));
    ESP_LOGI(PROTOCOL_TAG, "Message sent");
}