#include "main.h"

#include "lora.c"

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

void print_structure(){
    printf("Alert is: %d\n Alone is: %d\n Gateway is: %d\n Max Level is: %d\n Last Round Succeded is: %d\n ROund failed is: %d\n", structure.alert, structure.alone, structure.gateway_id, structure.max_known_level, structure.last_round_succeded, structure.rounds_failed);
}

node_structure future_structure = {
    false,
    true,
    -1,
    -1,
    -1,
    false,
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
    struct node_structure a_structure;
    int *alerts;
    int number_of_alerts;
    node_alerts *alerts;
} protocol_message;

bool new_connected = false;

uint8_t fake_hash = {1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8};

int fixed_partial_size_message = NULL;
int hash_size_message = NULL;

bool future_discover = false;
bool info = false;

struct node_status *cluster;
int cluster_occupation = -1;
int cluster_real_occupation = -1;
int cluster_lenght = 1;

int *not_seen_nodes;
int not_seen_nodes_lenght;

node_alerts *alerts;
int alerts_occupation;
int alerts_lenght;

protocol_message* data_to_read = NULL;

void marshal_and_send_message(protocol_message * pm){
    ESP_LOGI(PROTOCOL_TAG, "Starting Marshaling");

    int size_of_array = sizeof(node_alerts)*(pm->number_of_alerts);
    data_send_lenght = size_of_array + fixed_partial_size_message + hash_size_message;

    data_to_send = malloc(data_send_lenght);
    uint8_t *pointer_to_send = data_to_send;
    
    //ESP_LOGI(PROTOCOL_TAG, "Malloc data to send");
    memcpy(data_to_send,&(pm->hash), hash_size_message);
    //ESP_LOGI(PROTOCOL_TAG, "Copied hash part");
    pointer_to_send += hash_size_message;
    memcpy(pointer_to_send, pm, fixed_partial_size_message);
    //ESP_LOGI(PROTOCOL_TAG, "Copied fixed part");
    pointer_to_send += fixed_partial_size_message;    
    memcpy(pointer_to_send, &(pm->alerts), size_of_array);
    //ESP_LOGI(PROTOCOL_TAG, "Copied array");
    ESP_LOGI(PROTOCOL_TAG, "Marshaling complete");
    send_data(data_to_send, data_send_lenght, device);
}

void unmarshal_and_copy_message(uint8_t* data_from_source, int size_of_read){
    ESP_LOGI(PROTOCOL_TAG, "Starting Unmarshaling");
    uint8_t *pointer_to_array = data_from_source;

    data_to_read = malloc(sizeof(protocol_message));
    //ESP_LOGI(PROTOCOL_TAG, "Malloc data to read");
    memcpy(data_to_read, data_from_source, hash_size_message);
    //ESP_LOGI(PROTOCOL_TAG, "Copied hash data");

    // how to read from received array: we data->array return value of array, not its pointer. to use as array, we need &
    /*for (int z = 0; z < 32; z++){
        printf("%d\n", (int)(&(data_to_read->hash)[z]));
    }*/
    
    pointer_to_array += hash_size_message;
    memcpy(data_to_read, pointer_to_array, fixed_partial_size_message);
    //ESP_LOGI(PROTOCOL_TAG, "Copied fixed data");
    uint8_t size_of_array = (u_int8_t)(size_of_read-(fixed_partial_size_message+hash_size_message));
    pointer_to_array += fixed_partial_size_message;
    data_to_read->alerts = malloc(size_of_array);
    //ESP_LOGI(PROTOCOL_TAG, "Malloc array");
    memcpy(data_to_read->alerts, pointer_to_array, size_of_array);
    //ESP_LOGI(PROTOCOL_TAG, "Copied array data");
    ESP_LOGI(PROTOCOL_TAG, "Completed Unmarshaling");
}


void add_to_array(int *array, int *max_lenght, int *actual_lenght, int lenght_augment, int element){
    *(actual_lenght) += 1;

        // There is still space?
        if (*(actual_lenght) > *(max_lenght)){
            *(max_lenght) += lenght_augment;
            int *array2 = malloc(*(max_lenght)*sizeof(int));

            memcpy(array2, array, (*actual_lenght -1)*sizeof(int));
            free(array);

            array = array2;
        }

        array[*(actual_lenght)] = element;
}

void add_alert_to_array(node_alerts *array, int *max_lenght, int *actual_lenght, int lenght_augment, node_alerts element){
    *(actual_lenght) += 1;

        // There is still space?
        if (*(actual_lenght) > *(max_lenght)){
            *(max_lenght) += lenght_augment;
            node_alerts *array2 = malloc(*(max_lenght)*sizeof(node_alerts));

            memcpy(array2, array, (*actual_lenght -1)*sizeof(node_alerts));
            free(array);

            array = array2;
        }

        array[*(actual_lenght)] = element;
}

void protocol_init(bool wifi_init){
    wifi = wifi_init;
}

int* end_of_hour_procedure(){
    ESP_LOGI(PROTOCOL_TAG, "Ending hour...");
    int *ret_array = (int*)malloc(sizeof(int)*2);

    if (wifi){
        ESP_LOGI(PROTOCOL_TAG, "We are wifi");
        ret_array[0] = structure.max_known_level*2 -2;
        ret_array[1] = (standard_number_of_windows-structure.max_known_level)*2 - 1;
        return ret_array;
    }
    // end of discovery
    if(discover && new_connected){
        ESP_LOGI(PROTOCOL_TAG, "Setting new parameters after discover");
        structure.alone = future_structure.alone;
        connected = !structure.alone;
        structure.level = future_structure.level;
        structure.gateway_id = future_structure.gateway_id;
        structure.max_known_level = future_structure.max_known_level;
        structure.rounds_failed = future_structure.rounds_failed;
        structure.last_round_succeded = future_structure.last_round_succeded;

        info = false;
        discover = false;
        new_connected = false;
    }

    // need to be careful next round
    if (future_discover){
        
        ESP_LOGI(PROTOCOL_TAG, "Setting discover since future discover");
        discover = true;
        future_discover = false;
    }

    if(structure.last_round_succeded){
        structure.rounds_failed = 0;
    }else{
        
        ESP_LOGI(PROTOCOL_TAG, "Last round failed");
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
        
        ret_array[0] = -2;
        ret_array[1] = -2;
        return ret_array;
    }else{
        ret_array[0] = structure.level - 1;
        if (structure.max_known_level - structure.level == 1){
            ret_array[1] = 1;
        }else{
            ret_array[1] = (structure.max_known_level - structure.level)*2 - 1;
        }
        return ret_array;
    }

    return ret_array;
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

        memcpy(messages2, messages, (messages_occupation - 1)*sizeof(protocol_message));
        free(messages);

        messages = messages2;
    }

    messages[messages_occupation] = message;

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

void gather_messages(int time_to_listen){

    messages = malloc(messages_starting_lenght*sizeof(struct protocol_message));

    long unsigned int start_count = xx_time_get_time();
    long unsigned int end_count;
    
    long unsigned int start_count = xx_time_get_time();
    long unsigned int end_count;
    
    ESP_LOGI(GATHERING_TAG,"Trying to receive...");
    struct protocol_message last_message;

    while(!received){
                vTaskDelay(10000 / portTICK_PERIOD_MS);
                end_count = xx_time_get_time();
                long unsigned int passed_time = end_count - start_count;
                //ESP_LOGI(DISCOVER_TAG, "Time passed: %lu. Time to pass: %lu", passed_time, time_to_listen);

                if (passed_time >= time_to_listen){
                    ESP_LOGI(DISCOVER_TAG, "Time elapsed: closing gathering window");
                    break;
                    //vTaskDelete(NULL);
                }
            }

            last_message = *(struct protocol_message*)gather_buf;
            add_to_messages(last_message);

            printf("Received a packet from: %d\n", last_message.id);
            lora_receive();
        }
        break;

        end_count = xx_time_get_time();

        if (end_count - start_count >= time_to_listen){
            ESP_LOGI(GATHERING_TAG, "Closing gathering window");
            vTaskDelete(NULL);
        }

        //Needed for watchdog?
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    vTaskDelete(NULL);
}

TaskHandle_t gather_handler;

// Listen for better network configuration
// Save it for later and communicate
void discover_listen_and_answer(protocol_message message){
    if (((message.sender_structure.level < structure.level - 1) || (structure.alone && message.sender_structure.level > -1)) && message.sender_structure.last_round_succeded){
        future_structure.last_round_succeded = true;
        if(message.discover){
            future_discover = true;
        }
        ESP_LOGI(DISCOVER_TAG, "Node is connected");
        future_structure.alone = false;
        connected = !alone;
        future_structure.level = message.sender_structure.level + 1;
        future_structure.gateway_id = message.sender_structure.gateway_id;
        if (future_structure.level >= message.sender_structure.max_known_level){
            future_structure.max_known_level = future_structure.level + 1;
        }else{
            future_structure.max_known_level = message.a_structure.max_known_level;
        }
        future_structure.rounds_failed = 0;
        future_structure.last_round_succeded = true;

        new_connected = true;
    }else{
        new_connected = false;
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

void discover_listening(discover_schedule ds){
    ESP_LOGI(DISCOVER_TAG,"Starting discovering...");
    long unsigned int time_to_listen = ds.time_to_wait;
    long unsigned int wait_window = ds.wait_window;

    ESP_LOGI(DISCOVER_TAG, "Time to wait: %lu , Wait Window: %lu", time_to_listen, wait_window);

    long unsigned int start_count = xx_time_get_time();
    long unsigned int end_count;
    
    ESP_LOGI(DISCOVER_TAG,"Trying to receive for discovery...");
    
    receive_data(discover_receive_callback, device);

    ESP_LOGI(DISCOVER_TAG, "Enter loop");

    while (true) {
        
        if (new_connected){
            lora_set_idle();
            ESP_LOGI(DISCOVER_TAG, "Lora set idle");

            ESP_LOGI(DISCOVER_TAG, "Response talk");
            vTaskDelay(pdMS_TO_TICKS(wait_window));

            protocol_message message = {
                fake_hash, id, structure, true, 1234, 0, NULL
            };

            //(waiting randomly to avoid collisions)
            vTaskDelay(pdMS_TO_TICKS(esp_random()%(int)(time_to_wait_standard-(time_to_wait_standard/10.0))));
            marshal_and_send_message(&message); 

            ESP_LOGI(DISCOVER_TAG, "Response sent");
            ESP_LOGI(DISCOVER_TAG, "Closing discovery window");
                //vTaskDelete(NULL);
            return;
        }

        end_count = xx_time_get_time();
        long unsigned int passed_time = end_count - start_count;
        //ESP_LOGI(DISCOVER_TAG, "Time passed: %lu. Time to pass: %lu", passed_time, time_to_listen);

        if (passed_time >= time_to_listen){
            ESP_LOGI(DISCOVER_TAG, "Time elapsed: closing discovery window");
            return;
            //vTaskDelete(NULL);
        }

        //Wait a little
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Check if connected to wifi during this round
void first_listen(protocol_message message){
    if (message.sender_structure.gateway_id == structure.gateway_id && message.sender_structure.level == structure.level - 1 && message.sender_structure.last_round_succeded){
        structure.last_round_succeded = true;
        if(message.discover){
            future_discover = true;
        }
    }
}

void first_listening(int time_to_wait){
    // gather all messages from LoRa
    ESP_LOGI(PROTOCOL_TAG, "Opening first gathering window");
    gather_messages(time_to_wait);

    ESP_LOGI(PROTOCOL_TAG, "Processing messages");

    for (int i = 0; i < messages_occupation; i++){
        first_listen(messages[i]);
    }

    ESP_LOGI(PROTOCOL_TAG, "Messages processed");

    free(messages);
    ESP_LOGI(PROTOCOL_TAG, "Freed messages 1");
}

void first_talk(){
    ESP_LOGI(PROTOCOL_TAG, "First talk");
    bool need_to_discover = false;
    if (discover || future_discover){
        need_to_discover = true;
    }
    protocol_message message = {
        fake_hash, id, structure, need_to_discover, 4321, 0, NULL
    };
    //lora_reset();
    //set_lora();
    marshal_and_send_message(&message);
    ESP_LOGI(PROTOCOL_TAG, "Message sent");
}

node_alerts* new_alerts;
int new_alert_occupation = 0;
int new_alerts_lenght = 0;

void second_listen(protocol_message message){
    if (message.sender_structure.gateway_id == structure.gateway_id && message.sender_structure.level == structure.level + 1){
        // check if there are new nodes
        if (message.a_structure.max_known_level > structure.max_known_level){
            structure.max_known_level = message.a_structure.max_known_level;
        }
        for (int i = 0; i < message.number_of_alerts; i++){
            bool new = true;
            for (int j = 0; j < alerts_lenght; j ++){
                if (message.alerts[i].node_id == alerts[j].node_id){
                    new = false;
                }
            }
            if (new){
                add_alert_to_array(new_alerts, new_alerts_lenght, new_alert_occupation, 2, message.alerts[i]);
            }
        }

    }
}

void second_listening(int time_to_wait){
     // gather all messages from LoRa
    ESP_LOGI(PROTOCOL_TAG, "Opening second gathering window");
    gather_messages(time_to_wait);

    ESP_LOGI(PROTOCOL_TAG, "Processing messages");

    for (int i = 0; i < messages_occupation; i++){
        second_listen(messages[i]);
    }

    for (int i = 0; i < new_alert_occupation; i++){
        add_alert_to_array(alerts, alerts_lenght, alerts_occupation, 2, new_alerts[i]);
    }

    free(new_alerts);
    new_alert_occupation = 0;
    new_alerts_lenght = 0;

    ESP_LOGI(PROTOCOL_TAG, "Messages processed");

    free(messages);
    ESP_LOGI(PROTOCOL_TAG, "Freed messages 1");

    //Send alarms forward

}

void second_talk(){
    ESP_LOGI(PROTOCOL_TAG, "Second talk");
    protocol_message message = {
        fake_hash, id, structure, false, 4321, 0, NULL
    };
    //lora_reset();
    //set_lora();
    marshal_and_send_message(&message);
    ESP_LOGI(PROTOCOL_TAG, "Message sent");
}