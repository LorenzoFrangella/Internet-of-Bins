#include "main.h"

#define MISSING_HOUR_LIMIT 24

const char *PROTOCOL_TAG = "Protocol";
const char *GATHERING_TAG = "Gathering";
const char *DISCOVER_TAG = "Discovery";

struct node_structure {
    bool alert;
    bool alone;
    int level;
    int gateway_id;
    int max_known_level;
    bool last_round_succeded;
    int rounds_failed;
};

struct node_structure structure = {
    false,
    true,
    -1,
    -1,
    -1,
    false,
    -1
};

struct node_structure future_structure = {
    false,
    true,
    -1,
    -1,
    -1,
    false,
    -1
};

struct protocol_message {
    int id;
    struct node_structure a_structure;
    int *alerts;
    int number_of_alerts;
    bool discover;
    int time;
};

bool future_discover = false;
bool info = false;

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
        structure.alone = future_structure.alone;
        connected = !alone;
        structure.level = future_structure.level;
        structure.gateway_id = future_structure.gateway_id;
        structure.max_known_level = future_structure.max_known_level;
        structure.rounds_failed = future_structure.rounds_failed;

        info = false;
        discover = false;
    }

    // need to be careful next round
    if (future_discover){
        discover = true;
        future_discover = false;
    }

    if(structure.last_round_succeded){
        structure.rounds_failed = 0;
    }else{
        structure.rounds_failed++;
    }

    // Check if connected but not anymore
    if(structure.rounds_failed == MISSING_HOUR_LIMIT){
        ESP_LOGI(PROTOCOL_TAG, "Node is alone");
        structure.alone = true;
        connected = !alone;
        structure.level = -1;
        structure.gateway_id = -1;
        structure.max_known_level = -1;
        structure.rounds_failed = -1;
        int *ret_array = {-2,-2};
        return ret_array;
    }else if (wifi){
        int *ret_array = {-1,structure.max_known_level*2};
        return ret_array;
    }else{
        int *ret_array = {structure.level-1,(structure.max_known_level - structure.level)*2};
        return ret_array;
    }
}


void protocol_hour_check(bool new_alert){
    structure.alert = new_alert;
}

//***************************** MESSAGE HANDLING *******************************

int messages_starting_lenght = 10;
struct protocol_message *messages;
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

        messages = messages2;
    }

    messages[messages_occupation] = message;

}

uint8_t gather_buf[sizeof(struct protocol_message)+sizeof(int)*1024];

int gather_messages(int time_to_listen){

    messages = malloc(messages_starting_lenght*sizeof(struct protocol_message));

    long unsigned int start_count = xx_time_get_time();
    long unsigned int end_count;
    
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
// Save it for later and communicate
bool discover_listen_and_answer(struct protocol_message message){
    if (message.a_structure.level < structure.level - 1 && message.a_structure.last_round_succeded){
        future_structure.last_round_succeded = true;
        if(message.discover){
            future_discover = true;
        }
        ESP_LOGI(DISCOVER_TAG, "Node is connected");
        future_structure.alone = false;
        connected = !alone;
        future_structure.level = message.a_structure.level + 1;
        future_structure.gateway_id = message.a_structure.gateway_id;
        if (future_structure.level > future_structure.max_known_level){
            future_structure.max_known_level = message.a_structure.level;
        }else{
            future_structure.max_known_level = message.a_structure.max_known_level;
        }
        future_structure.rounds_failed = 0;

        return true;
    }
    return false;
}

void discover_listening(struct discover_schedule ds){
    long unsigned int time_to_listen = ds.time_to_wait;
    long unsigned int wait_window = ds.wait_window;
    messages = malloc(messages_starting_lenght*sizeof(struct protocol_message));

    long unsigned int start_count = xx_time_get_time();
    long unsigned int end_count;
    
    ESP_LOGI(DISCOVER_TAG,"Trying to receive for discovery...");
    struct protocol_message last_message;

    while (true) {
        
        lora_receive();    // put into receive mode
        while(lora_received()) {
            lora_receive_packet(gather_buf, sizeof(gather_buf));

            last_message = *(struct protocol_message*)gather_buf;
            printf("Received a packet from: %d\n", last_message.id);
            bool new_connected = discover_listen_and_answer(last_message);

            if (new_connected){
                ESP_LOGI(DISCOVER_TAG, "Response talk");
                vTaskDelay(pdMS_TO_TICKS(wait_window));

                struct protocol_message message = {
                    id, structure, NULL, 0, true, 0
                };
                lora_send_packet((uint8_t*)&message, sizeof(message));
                ESP_LOGI(DISCOVER_TAG, "Response sent");
                ESP_LOGI(DISCOVER_TAG, "Closing discovery window");
                vTaskDelete(NULL);
            }
            lora_receive();
        }

        end_count = xx_time_get_time();

        if (end_count - start_count >= time_to_listen){
            ESP_LOGI(DISCOVER_TAG, "Closing discovery window");
            vTaskDelete(NULL);
        }

        //Needed for watchdog?
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Check if connected to wifi during this round
void first_listen(struct protocol_message message){
    if (message.a_structure.gateway_id == structure.gateway_id && message.a_structure.level == structure.level - 1 && message.a_structure.last_round_succeded){
        structure.last_round_succeded = true;
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
    bool need_to_discover = false;
    if (discover || future_discover){
        need_to_discover = true;
    }
    struct protocol_message message = {
        id, structure, NULL, 0, need_to_discover, 0
    };
    lora_send_packet((uint8_t*)&message, sizeof(message));
    ESP_LOGI(PROTOCOL_TAG, "Message sent");
}

int* new_alerts;
int new_alert_occupation = 0;
int new_alerts_lenght = 0;

void second_listen(struct protocol_message message){
    if (message.a_structure.gateway_id == structure.gateway_id && message.a_structure.level == structure.level + 1){
        // check if there are new nodes
        if (message.a_structure.max_known_level > structure.max_known_level){
            structure.max_known_level = message.a_structure.max_known_level;
        }
        for (int i = 0; i < message.number_of_alerts; i++){
            bool new = true;
            for (int j = 0; j < alerts_lenght; j ++){
                if (message.alerts[i] == alerts[j]){
                    new = false;
                }
            }
            if (new){
                add_to_array(new_alerts, new_alerts_lenght, new_alert_occupation, 2, message.alerts[i]);
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
        id, structure, NULL, 0, false, 0
    };
    lora_send_packet((uint8_t*)&message, sizeof(message));
    ESP_LOGI(PROTOCOL_TAG, "Message sent");
}