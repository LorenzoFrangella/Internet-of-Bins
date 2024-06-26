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
    long unsigned int time;
    int number_of_alerts;
    node_structure sender_structure;
    node_alerts *alerts;
} protocol_message;

int new_connected = 0;

uint8_t* fake_hash;

int fixed_partial_size_message = 0;
int hash_size_message = 0;

int future_discover = 0;
int info = 0;

int *not_seen_nodes;
int not_seen_nodes_lenght;

protocol_message* data_to_read = 0;

void marshal_and_send_message(protocol_message * pm){
    ESP_LOGI(PROTOCOL_TAG, "Starting Marshaling");

    int size_of_array = sizeof(node_alerts)*(pm->number_of_alerts);
    data_send_lenght = size_of_array + fixed_partial_size_message + hash_size_message;

    data_to_send = malloc(data_send_lenght);
    uint8_t *pointer_to_send = data_to_send;
    uint8_t * pointer_to_struct = pm;
    
    //ESP_LOGI(PROTOCOL_TAG, "Malloc data to send");
    memcpy(data_to_send,pm->hash, hash_size_message);
    /*
    for (int z = 0; z < 32; z++){
        printf("%d\n", (int)(((uint8_t*)(data_to_send))[z]));
    }
    */
    //ESP_LOGI(PROTOCOL_TAG, "Copied hash part");
    pointer_to_send += hash_size_message;
    pointer_to_struct += sizeof(uint8_t*);
    int size_no_structure = fixed_partial_size_message - sizeof(node_structure);    
    memcpy(pointer_to_send, pointer_to_struct, size_no_structure);
    protocol_message * pointer_to_send_as_protocol = (protocol_message *)(pointer_to_send - 4);
    //ESP_LOGI(PROTOCOL_TAG, "Gateway should be: %d", pointer_to_send_as_protocol->sender_structure.gateway_id);

    //pointer_to_send_as_protocol->id = *pointer_to_struct;
    //ESP_LOGI(PROTOCOL_TAG, "Id should be: %d", pointer_to_send_as_protocol->id);
    //pointer_to_send_as_protocol->round = *(pointer_to_struct + 4);
    //ESP_LOGI(PROTOCOL_TAG, "Round should be: %d", pointer_to_send_as_protocol->round);
    //pointer_to_send_as_protocol->delay = *(pointer_to_struct + 8);
    //ESP_LOGI(PROTOCOL_TAG, "Delay should be: %lu", pointer_to_send_as_protocol->delay);
    //pointer_to_send_as_protocol->discover = *((long unsigned int*)(pointer_to_struct) + 12);
    ESP_LOGI(PROTOCOL_TAG, "Discover should be: %d", pointer_to_send_as_protocol->discover);
    //pointer_to_send_as_protocol->time = *((long long int*)(pointer_to_struct) + 16);
    //ESP_LOGI(PROTOCOL_TAG, "Time should be: %lu", pointer_to_send_as_protocol->time);
    //pointer_to_send_as_protocol->number_of_alerts = *(pointer_to_struct + 24);
    //ESP_LOGI(PROTOCOL_TAG, "N. alerts should be: %d", pointer_to_send_as_protocol->number_of_alerts);
    //pointer_to_send_as_protocol->sender_structure = *((node_structure *)(pointer_to_struct + 28));
    pointer_to_send += size_no_structure;
    pointer_to_send_as_protocol->sender_structure.alert = pm->sender_structure.alert;
    //ESP_LOGI(PROTOCOL_TAG, "Alert should be: %d", pointer_to_send_as_protocol->sender_structure.alert);
    pointer_to_send_as_protocol->sender_structure.alone = pm->sender_structure.alone;
    //ESP_LOGI(PROTOCOL_TAG, "Alone should be: %d", pointer_to_send_as_protocol->sender_structure.alone);
    pointer_to_send_as_protocol->sender_structure.level = pm->sender_structure.level;
    //ESP_LOGI(PROTOCOL_TAG, "Level should be: %d", pointer_to_send_as_protocol->sender_structure.level);
    pointer_to_send_as_protocol->sender_structure.gateway_id = pm->sender_structure.gateway_id;
    //ESP_LOGI(PROTOCOL_TAG, "Gateway should be: %d", pointer_to_send_as_protocol->sender_structure.gateway_id);
    pointer_to_send_as_protocol->sender_structure.max_known_level = pm->sender_structure.max_known_level;
    //ESP_LOGI(PROTOCOL_TAG, "Max Level should be: %d", pointer_to_send_as_protocol->sender_structure.max_known_level);
    pointer_to_send_as_protocol->sender_structure.last_round_succeded = pm->sender_structure.last_round_succeded;
    //ESP_LOGI(PROTOCOL_TAG, "Last Rounds failed should be: %d", pointer_to_send_as_protocol->sender_structure.last_round_succeded);
    pointer_to_send_as_protocol->sender_structure.rounds_failed = pm->sender_structure.rounds_failed;
    //ESP_LOGI(PROTOCOL_TAG, "Last Rounds failed should be: %d", pointer_to_send_as_protocol->sender_structure.rounds_failed);

    //ESP_LOGI(PROTOCOL_TAG, "Copied fixed part");
    pointer_to_send += sizeof(node_structure); 
    memcpy(pointer_to_send, &(pm->alerts), size_of_array);
    size_t offset = sizeof(node_alerts);
    uint8_t *pointer_to_array2 = pointer_to_send;
    for (int a = 0; a < pm->number_of_alerts; a++){
        //ESP_LOGW(PROTOCOL_TAG, "Copying node alert id: %d", pm->alerts[0].node_id);
        memcpy(pointer_to_array2, &(pm->alerts[a]), offset);
        //ESP_LOGW(PROTOCOL_TAG, "Copyied node alert id: %d", (int)(pointer_to_array2[0]));
        pointer_to_array2 += offset;
    }
    //ESP_LOGI(PROTOCOL_TAG, "Copied array");
    ESP_LOGI(PROTOCOL_TAG, "Marshaling complete");
    send_data(data_to_send, data_send_lenght, device);
}


    // how to read from received array: we data->array return value of array, not its pointer. to use as array, we need &
    /*for (int z = 0; z < 32; z++){
        printf("%d\n", (int)(&(data_to_read->hash)[z]) );
    }*/

void unmarshal_and_copy_message(uint8_t* data_from_source, int size_of_read){
    ESP_LOGI(PROTOCOL_TAG, "Starting Unmarshaling");
    uint8_t *pointer_to_source = data_from_source;

    data_to_read = malloc(sizeof(protocol_message));
    data_to_read->hash = malloc(hash_size_message);
    
    //ESP_LOGI(PROTOCOL_TAG, "Malloc data to read");
    memcpy(data_to_read->hash, data_from_source, hash_size_message);
    /*
    for (int z = 0; z < 32; z++){
        printf("%d\n", (int)(data_to_read->hash[z]));
    }
    */
    //ESP_LOGI(PROTOCOL_TAG, "Copied hash data");
    pointer_to_source += hash_size_message;
    /*
    int size_no_structure = fixed_partial_size_message - sizeof(node_structure);    
    memcpy((data_to_read + 32), pointer_to_source, size_no_structure);
    */
    
    data_to_read->id = *((int*)(pointer_to_source));
    //ESP_LOGI(PROTOCOL_TAG, "Id should be: %d", data_to_read->id);
    data_to_read->round = *((int*)(pointer_to_source + 4));
    //ESP_LOGI(PROTOCOL_TAG, "Round should be: %d", data_to_read->round);
    data_to_read->delay = *((long unsigned int*)(pointer_to_source + 8));
    //ESP_LOGI(PROTOCOL_TAG, "Delay should be: %lu", data_to_read->delay);
    data_to_read->discover = *((int*)(pointer_to_source + 12));
    //ESP_LOGI(PROTOCOL_TAG, "Discover should be: %d", data_to_read->discover);
    data_to_read->time = *((long unsigned int*)(pointer_to_source+16));
    //ESP_LOGI(PROTOCOL_TAG, "Time should be: %lu", data_to_read->time);
    data_to_read->number_of_alerts = *((int*)(pointer_to_source + 20));
    //ESP_LOGI(PROTOCOL_TAG, "N. alerts should be: %d", data_to_read->number_of_alerts);

    node_structure *pm = pointer_to_source + 24;
    data_to_read->sender_structure.alert = pm->alert;
    //ESP_LOGI(PROTOCOL_TAG, "Alert should be: %d", data_to_read->sender_structure.alert);
    data_to_read->sender_structure.alone = pm->alone;
    //ESP_LOGI(PROTOCOL_TAG, "Alone should be: %d", data_to_read->sender_structure.alone);
    data_to_read->sender_structure.level = pm->level;
    //ESP_LOGI(PROTOCOL_TAG, "Level should be: %d", data_to_read->sender_structure.level);
    data_to_read->sender_structure.gateway_id = pm->gateway_id;
    //ESP_LOGI(PROTOCOL_TAG, "Gateway should be: %d", data_to_read->sender_structure.gateway_id);
    data_to_read->sender_structure.max_known_level = pm->max_known_level;
    //ESP_LOGI(PROTOCOL_TAG, "Max Level should be: %d", data_to_read->sender_structure.max_known_level);
    data_to_read->sender_structure.last_round_succeded = pm->last_round_succeded;
    ESP_LOGI(PROTOCOL_TAG, "Last Rounds failed should be: %d", data_to_read->sender_structure.last_round_succeded);
    data_to_read->sender_structure.rounds_failed = pm->rounds_failed;
    //ESP_LOGI(PROTOCOL_TAG, "Last Rounds failed should be: %d", data_to_read->sender_structure.rounds_failed);
    //ESP_LOGI(PROTOCOL_TAG, "Copied fixed data");
    uint8_t size_of_array = (u_int8_t)(size_of_read-(fixed_partial_size_message+hash_size_message));
    pointer_to_source += fixed_partial_size_message;
    data_to_read->alerts = malloc(size_of_array);
    //ESP_LOGI(PROTOCOL_TAG, "Malloc array");
    //memcpy(data_to_read->alerts, pointer_to_source, size_of_array);
    size_t offset = sizeof(node_alerts);
    uint8_t *pointer_to_array2 = pointer_to_source;
    uint8_t *pointer_to_array_copied = data_to_read->alerts;
    for (int a = 0; a < data_to_read->number_of_alerts; a++){
        memcpy(pointer_to_array_copied, pointer_to_array2, offset);
        ESP_LOGW(PROTOCOL_TAG, "Copyied node alert id: %d", (int)(pointer_to_array_copied[a]));
        pointer_to_array2 += offset;
        pointer_to_array_copied += offset;
    }
    ESP_LOGI(PROTOCOL_TAG, "Copied array data");
    ESP_LOGI(PROTOCOL_TAG, "Completed Unmarshaling");
}

node_alerts *alerts;
int alerts_occupation = 0;
int alerts_lenght = 10;

void add_alert_to_array(node_alerts element){
    if (alerts_occupation == alerts_lenght){
        alerts_lenght += 10;
        alerts = realloc(alerts, alerts_lenght*sizeof(node_alerts));
    }
    alerts[alerts_occupation] = element;
    alerts_occupation +=1;
    ESP_LOGW(UTILITY_TAG, "Alert added to array id: %d", alerts[alerts_occupation-1].node_id);
}

void sending_alerts(){
    for (int i = 0; i < alerts_occupation; i++){
        printf("Alert id: %d \n", alerts[i].node_id);
    }
}

void protocol_init(int wifi_init){
    wifi = wifi_init;
}

void set_time_to_sleep(){
    if(new_connected){
        times_to_sleep[0] = (structure.level)*time_window_standard;
        times_to_sleep[1] = (structure.level + 2)*time_window_standard;
        times_to_sleep[2] = (structure.max_known_level + 2)*time_window_standard;
        times_to_sleep[3] = (structure.max_known_level + 4)*time_window_standard; //+ time_window_standard;
    }else{
        times_to_sleep[0] = (structure.level)*time_window_standard;
        times_to_sleep[1] = (structure.level + 2)*time_window_standard;
        times_to_sleep[2] = ((structure.max_known_level  - structure.level))*time_window_standard + structure.max_known_level*time_window_standard + 2*time_window_standard;
        times_to_sleep[3] = ((structure.max_known_level - structure.level) +2)*time_window_standard + structure.max_known_level*time_window_standard + 2*time_window_standard;
    }
    
}

void end_of_hour_procedure(){
    ESP_LOGI(PROTOCOL_TAG, "Ending hour...");

    if (wifi){
        ESP_LOGI(PROTOCOL_TAG, "We are wifi");
        structure.last_round_succeded = 1;
        set_time_to_sleep();
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
        set_time_to_sleep();
        return;
    }
}

void protocol_hour_check(int new_alert){
    structure.alert = new_alert;
}

//***************************** MESSAGE HANDLING *******************************

protocol_message* messages;
int messages_lenght = 10;
int messages_occupation = 0;
int messages_previous_max = -1;

void add_to_messages(protocol_message message){

    if (messages_occupation == messages_lenght){
        ESP_LOGW(PROTOCOL_TAG, "Increasing size of messages");
        messages_lenght += 10;
        messages = realloc(messages, messages_lenght*sizeof(protocol_message));
    }

    ESP_LOGW(PROTOCOL_TAG, "Copying message");
    messages[messages_occupation] = message;
    
    if (messages_occupation > messages_previous_max){
        ESP_LOGW(PROTOCOL_TAG, "Increasing previous max, new allocation for alerts");
        messages_previous_max = messages_occupation;
        messages[messages_occupation].alerts = malloc(messages[messages_occupation].number_of_alerts * sizeof(node_alerts) + 4); // +4 to avoid null pointer for realloc
    }else{
        ESP_LOGW(PROTOCOL_TAG, "Reallocation allocation for alerts");
        messages[messages_occupation].alerts = realloc(messages[messages_occupation].alerts, messages[messages_occupation].number_of_alerts * sizeof(node_alerts) + 4);
    }
    
    ESP_LOGW(PROTOCOL_TAG, "Copying alerts");
    for(int na = 0; na < message.number_of_alerts; na++){
        messages[messages_occupation].alerts[na] = message.alerts[na];
        ESP_LOGD(PROTOCOL_TAG, "Added alert id from: %d", messages[messages_occupation].alerts[na].node_id);
    }

    messages_occupation += 1;
    ESP_LOGD(PROTOCOL_TAG, "Added message from: %d", messages[messages_occupation-1].id);

    /*
    ESP_LOGI(PROTOCOL_TAG, "Id should be: %d", messages[messages_occupation].id);
    ESP_LOGI(PROTOCOL_TAG, "Round should be: %d", messages[messages_occupation].round);
    ESP_LOGI(PROTOCOL_TAG, "Delay should be: %lu", messages[messages_occupation].delay);
    ESP_LOGI(PROTOCOL_TAG, "Discover should be: %d", messages[messages_occupation].discover);
    ESP_LOGI(PROTOCOL_TAG, "Time should be: %lu", messages[messages_occupation].time);
    ESP_LOGI(PROTOCOL_TAG, "N. alerts should be: %d", messages[messages_occupation].number_of_alerts);

    ESP_LOGI(PROTOCOL_TAG, "Alert should be: %d", messages[messages_occupation].sender_structure.alert);    
    ESP_LOGI(PROTOCOL_TAG, "Alone should be: %d", messages[messages_occupation].sender_structure.alone);
    ESP_LOGI(PROTOCOL_TAG, "Level should be: %d", messages[messages_occupation].sender_structure.level);
    ESP_LOGI(PROTOCOL_TAG, "Gateway should be: %d", messages[messages_occupation].sender_structure.gateway_id);
    ESP_LOGI(PROTOCOL_TAG, "Max Level should be: %d", messages[messages_occupation].sender_structure.max_known_level);
    ESP_LOGI(PROTOCOL_TAG, "Last Rounds failed should be: %d", messages[messages_occupation].sender_structure.last_round_succeded);
    */

}

long unsigned int start_receive_time;

void gather_receive_callback(sx127x *device, uint8_t *data, uint16_t data_length){
    start_receive_time = xx_time_get_time();
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

void gather_messages(int round){
    long unsigned int time_to_start_listen;
    if(round == 0){
        time_to_start_listen = start_count_total + (times_to_sleep[0]*10);
    }else{
        time_to_start_listen = start_count_total + (times_to_sleep[2]*10);
    }
    long unsigned int time_to_end_listen = times_to_sleep[1]-time_window_standard;
    long unsigned int end_count;
    long unsigned int passed_time;
    
    ESP_LOGI(GATHERING_TAG,"Trying to receive...");
    
    receive_data(gather_receive_callback, device);

    while(!received){
                end_count = xx_time_get_time();
                passed_time = (end_count - time_to_start_listen)/10;
                //ESP_LOGI(DISCOVER_TAG, "Time passed: %lu", passed_time);

                if (passed_time >= delay_window){
                    ESP_LOGI(DISCOVER_TAG, "Time elapsed: closing gathering window");
                    break;
                    //vTaskDelete(0);
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
        long int time_to_end_round;
        int t_window;
        
        if (new_connected){

            protocol_message sm = (*data_to_read);

            if(sm.round == 0){ // Forward round
            
                lora_set_idle();
                ESP_LOGI(DISCOVER_TAG, "Lora set idle");
                ESP_LOGW(DISCOVER_TAG, "Connected in forward round");
                //ESP_LOGW(DISCOVER_TAG, "Sm id: %d", sm.id);
                //relative_time_passed = (sm.sender_structure.level + 1) * time_window_standard + sm.delay;
                //ESP_LOGW(DISCOVER_TAG, "Relative time: %lu", relative_time_passed);
                time_to_sync_window = time_window_standard - sm.delay;
                //ESP_LOGW(DISCOVER_TAG, "Time to sync window: %lu", time_to_sync_window);
                working_time = (xx_time_get_time() - start_receive_time)/100;
                //ESP_LOGW(DISCOVER_TAG, "Working time: %lu", working_time);
                // T1
                t_window = sm.sender_structure.level + 2;
                time_to_end_round = time_to_sync_window - working_time + ((sm.sender_structure.max_known_level + 2)*2 - t_window)*time_window_standard;
                //time_to_end_round = (sm.sender_structure.max_known_level + (sm.sender_structure.max_known_level - sm.sender_structure.level))*time_window_standard + time_to_sync_window - working_time;
                if (time_to_end_round < 0){
                    time_to_end_round = 0;
                }
                ESP_LOGW(DISCOVER_TAG, "Time to end round: %ld", time_to_end_round);
                vTaskDelay(pdMS_TO_TICKS(time_to_end_round*10));
                ESP_LOGI(DISCOVER_TAG, "Closing discovery window");
                return;
            }else if (sm.round == 1){ // Response round
                lora_set_idle();
                ESP_LOGI(DISCOVER_TAG, "Lora set idle");
                ESP_LOGW(DISCOVER_TAG, "Connected in response round");
                //relative_time_passed = sm.sender_structure.max_known_level - sm.sender_structure.level + sm.delay;
                time_to_sync_window = time_window_standard - sm.delay;
                working_time = (xx_time_get_time() - start_receive_time)/100;
                t_window = (sm.sender_structure.max_known_level - sm.sender_structure.level) + 2 +  sm.sender_structure.max_known_level + 2;
                time_to_end_round = time_to_sync_window - working_time + ((sm.sender_structure.max_known_level + 2)*2 - t_window)*time_window_standard;
                //time_to_end_round = (*data_to_read).sender_structure.level * time_window_standard + relative_time_passed + time_to_sync_window - working_time;
                if (time_to_end_round < 0){
                    time_to_end_round = 0;
                }
                ESP_LOGW(DISCOVER_TAG, "TIme to end round: %ld", time_to_end_round);
                vTaskDelay(pdMS_TO_TICKS(time_to_end_round*10));
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
    gather_messages(0);

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
        fake_hash, id, 0, delay, 0, (long unsigned int)(4321), 0, structure, 0
    };
    
    ESP_LOGW(PROTOCOL_TAG, "After first Talk");
    print_time();

    marshal_and_send_message(&message);
    ESP_LOGI(PROTOCOL_TAG, "Message sent");
}

void second_listen(protocol_message message){
    if (message.sender_structure.gateway_id == structure.gateway_id && message.sender_structure.level == structure.level + 1){
        ESP_LOGW(PROTOCOL_TAG, "Message from successor node");
        ESP_LOGW(PROTOCOL_TAG, "Discover: %d", message.discover);
        ESP_LOGW(PROTOCOL_TAG, "Alerts: %d", message.number_of_alerts);
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
    gather_messages(1);

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
        ESP_LOGW(PROTOCOL_TAG, "Sending level as discovery: %d", structure.max_known_level + 1);
        need_to_discover = future_structure.max_known_level;
    }
    //need_to_discover = esp_random()%100;
    protocol_message message = {
        fake_hash, id, 1, delay, need_to_discover, (long unsigned int)(4321), alerts_occupation, structure, alerts
    };

    ESP_LOGW(PROTOCOL_TAG, "After Second Talk");
    print_time();
    
    marshal_and_send_message(&message);
    ESP_LOGI(PROTOCOL_TAG, "Message sent");
}