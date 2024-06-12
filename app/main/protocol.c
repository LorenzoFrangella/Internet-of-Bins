#include "main.h"

#include "lora.c"

#define MISSING_HOUR_LIMIT 2

const char *PROTOCOL_TAG = "Protocol";
const char *GATHERING_TAG = "Gathering";
const char *DISCOVER_TAG = "Discovery";

typedef struct {
    int alert;
    int alone;
    int level;
    int gateway_id;
    int max_known_level;
    int last_round_succeded;
    int rounds_failed;
} node_structure;

node_structure structure = {
    0,
    1,
    -1,
    -1,
    -1,
    0,
    -1
};

void print_structure(){
    printf("Alert is: %d\n Alone is: %d\n Gateway is: %d\n Level is: %d\n Max Level is: %d\n Last Round Succeded is: %d\n ROund failed is: %d\n", structure.alert, structure.alone, structure.gateway_id, structure.level, structure.max_known_level, structure.last_round_succeded, structure.rounds_failed);
}

node_structure future_structure = {
    0,
    1,
    -1,
    -1,
    -1,
    0,
    -1
};

typedef struct {
    int node_id;
    int load_alarm;
    int temp_alarm;
    int gas_alarm;
} node_alerts;

typedef struct {
    uint8_t *hash;
    int id;
    int round;
    long unsigned int delay;
    int discover;
    long long int time;
    int number_of_alerts;
    node_structure sender_structure;
    node_alerts *alerts;
} protocol_message;

int new_connected = 0;

uint8_t fake_hash = {1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8};

int fixed_partial_size_message = NULL;
int hash_size_message = NULL;

int future_discover = 0;
int info = 0;

int *not_seen_nodes;
int not_seen_nodes_lenght;

node_alerts alerts[50];
int alerts_occupation = 0;
const int alerts_lenght = 50;

protocol_message* data_to_read = NULL;

void marshal_and_send_message(protocol_message * pm){
    ESP_LOGI(PROTOCOL_TAG, "Starting Marshaling");

    int size_of_array = sizeof(node_alerts)*(pm->number_of_alerts);
    data_send_lenght = size_of_array + fixed_partial_size_message + hash_size_message;

    data_to_send = malloc(data_send_lenght);
    uint8_t *pointer_to_send = data_to_send;
    
    ESP_LOGI(PROTOCOL_TAG, "Malloc data to send");
    memcpy(data_to_send,&(pm->hash), hash_size_message);
    ESP_LOGI(PROTOCOL_TAG, "Copied hash part");
    pointer_to_send += hash_size_message;
    memcpy(pointer_to_send, pm, fixed_partial_size_message);
    ESP_LOGI(PROTOCOL_TAG, "Copied fixed part");
    pointer_to_send += fixed_partial_size_message; 
    memcpy(pointer_to_send, &(pm->alerts), size_of_array);
    size_t offset = sizeof(node_alerts);
    uint8_t *pointer_to_array2 = pointer_to_send;
    for (int a = 0; a < pm->number_of_alerts; a++){
        ESP_LOGW(PROTOCOL_TAG, "Copying node alert id: %d", pm->alerts[0].node_id);
        memcpy(pointer_to_array2, &(pm->alerts[0]), offset);
        pointer_to_array2 += offset;
    }
    ESP_LOGI(PROTOCOL_TAG, "Copied array");
    ESP_LOGI(PROTOCOL_TAG, "Marshaling complete");
    send_data(data_to_send, data_send_lenght, device);
}


    // how to read from received array: we data->array return value of array, not its pointer. to use as array, we need &
    /*for (int z = 0; z < 32; z++){
        printf("%d\n", (int)(&(data_to_read->hash)[z]) );
    }*/

void unmarshal_and_copy_message(uint8_t* data_from_source, int size_of_read){
    ESP_LOGI(PROTOCOL_TAG, "Starting Unmarshaling");
    uint8_t *pointer_to_array = data_from_source;

    data_to_read = malloc(sizeof(protocol_message));
    //ESP_LOGI(PROTOCOL_TAG, "Malloc data to read");
    memcpy(data_to_read, data_from_source, hash_size_message);
    //ESP_LOGI(PROTOCOL_TAG, "Copied hash data");
    
    pointer_to_array += hash_size_message;
    memcpy(data_to_read, pointer_to_array, fixed_partial_size_message);
    //ESP_LOGI(PROTOCOL_TAG, "Copied fixed data");
    uint8_t size_of_array = (u_int8_t)(size_of_read-(fixed_partial_size_message+hash_size_message));
    pointer_to_array += fixed_partial_size_message;
    data_to_read->alerts = malloc(size_of_array);
    //ESP_LOGI(PROTOCOL_TAG, "Malloc array");
    //memcpy(data_to_read->alerts, pointer_to_array, size_of_array);

    size_t offset = sizeof(node_alerts);
    uint8_t *pointer_to_array2 = pointer_to_array;
    uint8_t *pointer_to_array_copied = data_to_read->alerts;
    for (int a = 0; a < data_to_read->number_of_alerts; a++){
        memcpy(pointer_to_array_copied, pointer_to_array2, offset);
        pointer_to_array2 += offset;
        pointer_to_array_copied += offset;
    }
    //ESP_LOGI(PROTOCOL_TAG, "Copied array data");
    ESP_LOGI(PROTOCOL_TAG, "Completed Unmarshaling");
}

void add_alert_to_array(node_alerts element){
    alerts_occupation +=1;
    alerts[alerts_occupation-1] = element;
    ESP_LOGW(UTILITY_TAG, "Alert added to array id: %d", alerts[alerts_occupation-1].node_id);
}

void protocol_init(int wifi_init){
    wifi = wifi_init;
}

void end_of_hour_procedure(){
    ESP_LOGI(PROTOCOL_TAG, "Ending hour...");

    if (wifi){
        ESP_LOGI(PROTOCOL_TAG, "We are wifi");
        structure.last_round_succeded = 1;
        times_to_sleep[0] = (structure.level)*time_window_standard;
        times_to_sleep[1] = (structure.level + 2)*time_window_standard;
        times_to_sleep[2] = ((structure.max_known_level - structure.level))*time_window_standard + time_window_standard*structure.max_known_level;
        times_to_sleep[3] = ((structure.max_known_level - structure.level) +2)*time_window_standard + time_window_standard*structure.max_known_level;
        return;
    }
    // end of discovery
    if(discover){
        ESP_LOGI(PROTOCOL_TAG, "Setting parameters after discover");
        structure.alone = future_structure.alone;
        connected = 1;
        structure.level = future_structure.level;
        structure.gateway_id = future_structure.gateway_id;
        structure.rounds_failed = future_structure.rounds_failed;
        structure.last_round_succeded = future_structure.last_round_succeded;

        if (info){
            if (new_connected == 2){
                structure.max_known_level = future_structure.max_known_level -1;
                new_connected = 1;
            }else{
                structure.max_known_level = future_structure.max_known_level;
                new_connected = 0;
                discover = 0;
                info = 0;
            }
        }else{
            structure.max_known_level = future_structure.max_known_level;
            discover = 0;
            new_connected = 0;
        }
    }

    if(structure.last_round_succeded){
        structure.rounds_failed = 0;
    }else{
        
        ESP_LOGI(PROTOCOL_TAG, "Last round failed");
        structure.rounds_failed++;
    }
    structure.last_round_succeded = 0;

    // Check if connected but not anymore
    if(structure.rounds_failed == MISSING_HOUR_LIMIT){
        ESP_LOGI(PROTOCOL_TAG, "Node is alone");
        structure.alone = 1;
        connected = 0;
        structure.level = -1;
        structure.gateway_id = -1;
        structure.max_known_level = -1;
        structure.rounds_failed = -1;
        
        times_to_sleep[0] = -2;
        times_to_sleep[1] = -2;
        times_to_sleep[2] = -2;
        times_to_sleep[3] = -2;
        return;
    }else{
        times_to_sleep[0] = (structure.level)*time_window_standard;
        times_to_sleep[1] = (structure.level + 2)*time_window_standard;
        times_to_sleep[2] = (structure.max_known_level + 1 - structure.level)*time_window_standard + time_window_standard*(structure.max_known_level+1);
        times_to_sleep[3] = ((structure.max_known_level + 1 - structure.level) +2)*time_window_standard + time_window_standard*(structure.max_known_level+1);
        return;
    }
}

void protocol_hour_check(int new_alert){
    structure.alert = new_alert;
}

//***************************** MESSAGE HANDLING *******************************

int messages_starting_lenght = 10;
protocol_message messages[10];
int messages_lenght = 0;
int messages_occupation = 0;

void add_to_messages(protocol_message message){

    messages_occupation += 1;

    messages[messages_occupation-1] = message;
    
    ESP_LOGW(UTILITY_TAG, "Added message from: %d", messages[messages_occupation-1].id);

}

uint8_t gather_buf[sizeof(protocol_message)+sizeof(node_alerts)*256];

void gather_receive_callback(sx127x *device, uint8_t *data, uint16_t data_length){
    int16_t rssi;
    ESP_ERROR_CHECK(sx127x_rx_get_packet_rssi(device, &rssi));
    float snr;
    ESP_ERROR_CHECK(sx127x_lora_rx_get_packet_snr(device, &snr));
    int32_t frequency_error;
    ESP_ERROR_CHECK(sx127x_rx_get_frequency_error(device, &frequency_error));
    ESP_LOGI(LORA_TAG, "Received: %d rssi: %d snr: %f freq_error: %" PRId32, data_length, rssi, snr, frequency_error);
    data_received = malloc(data_length);
    memcpy(data_received, data, data_length);
    ESP_LOGI(LORA_TAG, "Data Copied");
    unmarshal_and_copy_message(data_received, data_length);
    add_to_messages(*data_to_read);
    ESP_LOGI(LORA_TAG,"Received a packet from: %d", data_to_read->id);
    ESP_LOGI(LORA_TAG, "End Callback");    
}

void gather_messages(){
    
    
    long unsigned int time_to_start_listen = start_count_total + times_to_sleep[0];
    long unsigned int time_to_end_listen = times_to_sleep[1]-time_window_standard;
    long unsigned int end_count;
    long unsigned int passed_time;
    
    ESP_LOGI(GATHERING_TAG,"Trying to receive...");
    
    receive_data(gather_receive_callback, device);

    while(!received){
                end_count = xx_time_get_time();
                passed_time = (end_count - time_to_start_listen)/10;
                //ESP_LOGI(DISCOVER_TAG, "Time passed: %lu. Time to pass: %lu", passed_time, time_to_listen);

                if (passed_time >= delay_module){
                    ESP_LOGI(DISCOVER_TAG, "Time elapsed: closing gathering window");
                    break;
                    //vTaskDelete(NULL);
                }
                
                vTaskDelay(pdMS_TO_TICKS(10));
            }

    lora_set_idle();
    ESP_LOGI(GATHERING_TAG, "Closing gathering window");
}

TaskHandle_t gather_handler;
int round_connected = -2;

// Listen for better network configuration
// Save it for later and communicate
void discover_listen_and_answer(protocol_message message){
    if (((message.sender_structure.level < structure.level - 1) || (structure.alone && message.sender_structure.level > -1)) && message.sender_structure.last_round_succeded){
        future_structure.last_round_succeded = 1;
        if(message.discover){
            future_discover = 1;
        }
        ESP_LOGI(DISCOVER_TAG, "Node is connected");
        future_structure.alone = 0;
        connected = 1;
        future_structure.level = message.sender_structure.level + 1;
        future_structure.gateway_id = message.sender_structure.gateway_id;
        if (future_structure.level >= message.sender_structure.max_known_level){
            future_structure.max_known_level = future_structure.level + 1;
            info = 1;
        }else{
            future_structure.max_known_level = message.sender_structure.max_known_level;
        }
        future_structure.rounds_failed = 0;
        future_structure.last_round_succeded = 1;

        new_connected = 2;
        round_connected = message.round;
    }else{
        new_connected = 0;
    }
}

void discover_receive_callback(sx127x *device, uint8_t *data, uint16_t data_length){
    int16_t rssi;
    ESP_ERROR_CHECK(sx127x_rx_get_packet_rssi(device, &rssi));
    float snr;
    ESP_ERROR_CHECK(sx127x_lora_rx_get_packet_snr(device, &snr));
    int32_t frequency_error;
    ESP_ERROR_CHECK(sx127x_rx_get_frequency_error(device, &frequency_error));
    ESP_LOGI(LORA_TAG, "Received: %d rssi: %d snr: %f freq_error: %" PRId32, data_length, rssi, snr, frequency_error);
    data_received = malloc(data_length);
    memcpy(data_received, data, data_length);
    ESP_LOGI(LORA_TAG, "Data Copied");
    unmarshal_and_copy_message(data_received, data_length);
    ESP_LOGI(LORA_TAG,"Received a packet from: %d\n", data_to_read->id);
    discover_listen_and_answer(*data_to_read);
    ESP_LOGI(LORA_TAG, "End Callback");    
}

void discover_listening(){
    ESP_LOGI(DISCOVER_TAG,"Starting discovering...");
    
    ESP_LOGI(DISCOVER_TAG,"Trying to receive for discovery...");
    
    receive_data(discover_receive_callback, device);

    ESP_LOGI(DISCOVER_TAG, "Enter loop");

    while (1) {

        long unsigned int relative_time_passed;
        long unsigned int time_to_sync_window;
        long unsigned int time_to_end_round;
        
        if (new_connected){
            lora_set_idle();
            ESP_LOGI(DISCOVER_TAG, "Lora set idle");

            if((*data_to_read).round == 0){ // Forward round
                ESP_LOGW(DISCOVER_TAG, "Connected in forward round");
                relative_time_passed = (*data_to_read).sender_structure.level *time_window_standard + (*data_to_read).delay;
                time_to_sync_window = time_window_standard*2 - (*data_to_read).delay;
                time_to_end_round = ((*data_to_read).sender_structure.max_known_level*time_window_standard)*2 - (relative_time_passed + time_to_sync_window);
                ESP_LOGW(DISCOVER_TAG, "TIme to end round: %lu", time_to_end_round);
                vTaskDelay(pdMS_TO_TICKS(time_to_end_round));
                ESP_LOGI(DISCOVER_TAG, "Closing discovery window");
                return;
            }else if ((*data_to_read).round == 1){ // Response round
                ESP_LOGW(DISCOVER_TAG, "Connected in response round");
                relative_time_passed = ((*data_to_read).sender_structure.max_known_level - (*data_to_read).sender_structure.level)*time_window_standard + time_window_standard*(*data_to_read).sender_structure.max_known_level + (*data_to_read).delay;
                time_to_sync_window = time_window_standard*2 - (*data_to_read).delay;
                time_to_end_round = ((*data_to_read).sender_structure.max_known_level- (*data_to_read).sender_structure.level)*time_window_standard + time_to_sync_window + relative_time_passed;
                ESP_LOGW(DISCOVER_TAG, "TIme to end round: %lu", time_to_end_round);
                vTaskDelay(pdMS_TO_TICKS(time_to_end_round));
                ESP_LOGI(DISCOVER_TAG, "Closing discovery window");
                return;
            }else{
                new_connected = 0;
                ESP_LOGI(DISCOVER_TAG, "Something gone wrong with union to network. Not real connection.");
            }
        }

        //Wait a little
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Check if connected to wifi during this round
void first_listen(protocol_message message){
    ESP_LOGW(PROTOCOL_TAG, "Message listened");
    if (message.sender_structure.gateway_id == structure.gateway_id && message.sender_structure.level == structure.level - 1 && message.sender_structure.last_round_succeded){
        ESP_LOGW(PROTOCOL_TAG, "Message from prior node");
        structure.last_round_succeded = 1;
        if(message.discover){
            future_discover = 1;
        }
    }else{
        ESP_LOGW(PROTOCOL_TAG, "Sender gat: %d, Node gat: %d, Sender lv: %d, Node lv: %d, Sender succ: %d", message.sender_structure.gateway_id, structure.gateway_id, message.sender_structure.level, structure.level, message.sender_structure.last_round_succeded);
    }
}

void first_listening(){
    // gather all messages from LoRa
    ESP_LOGI(PROTOCOL_TAG, "Opening first gathering window");
    messages_occupation = 0;
    gather_messages();

    ESP_LOGI(PROTOCOL_TAG, "Processing messages");

    for (int i = 0; i < messages_occupation; i++){
    
        ESP_LOGW(PROTOCOL_TAG, "Processing message from: %d", messages[i].id);
        first_listen(messages[i]);
    }

    ESP_LOGI(PROTOCOL_TAG, "Messages processed");

    //free(messages);
    //ESP_LOGI(PROTOCOL_TAG, "Freed messages");
}

void first_talk(long unsigned int delay){
    ESP_LOGI(PROTOCOL_TAG, "First talk");
    protocol_message message = {
        fake_hash, id, 0, delay, 0, 4321, 0, structure, NULL
    };
    //lora_reset();
    //set_lora();
    marshal_and_send_message(&message);
    ESP_LOGI(PROTOCOL_TAG, "Message sent");
}

void second_listen(protocol_message message){
    if (message.sender_structure.gateway_id == structure.gateway_id && message.sender_structure.level == structure.level + 1){
        ESP_LOGW(PROTOCOL_TAG, "Message from successor node");
        // check if there are new nodes
        if (message.discover > 0){
            structure.max_known_level = message.discover;
            ESP_LOGW(PROTOCOL_TAG, "New node added");
        }
        for (int i = 0; i < message.number_of_alerts; i++){
            int new = 1;
            for (int j = 0; j < alerts_lenght; j ++){
                if (message.alerts[i].node_id == alerts[j].node_id){
                    new = 0;
                }
            }
            if (new){
                ESP_LOGW(PROTOCOL_TAG, "Adding alert: %d", message.alerts[i].node_id);
                add_alert_to_array(message.alerts[i]);
            }
        }

    }
}

void second_listening(){
     // gather all messages from LoRa
    ESP_LOGI(PROTOCOL_TAG, "Opening second gathering window");
    messages_occupation = 0;
    gather_messages();

    ESP_LOGI(PROTOCOL_TAG, "Processing messages");
    alerts_occupation = 0;

    for (int i = 0; i < messages_occupation; i++){
        second_listen(messages[i]);
    }

    ESP_LOGI(PROTOCOL_TAG, "Messages processed");

}

void second_talk(long unsigned int delay){
    ESP_LOGI(PROTOCOL_TAG, "Second talk");
    int need_to_discover = -1;
    if (new_connected == 1){
        need_to_discover = structure.max_known_level + 1;
    }
    protocol_message message = {
        fake_hash, id, 1, delay, need_to_discover, (long long int) 4321, alerts_occupation, structure, alerts
    };
    
    marshal_and_send_message(&message);
    ESP_LOGI(PROTOCOL_TAG, "Message sent");
}