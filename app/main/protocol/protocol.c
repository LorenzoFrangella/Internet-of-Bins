#include "main_protocol.h"

#include "lora.c"
#include "time.h"
#include "esp_wifi.h"
#include "signature.c"



#define MISSING_HOUR_LIMIT 2
#define MAX_LEVEL 3
#define WINDOW_SIZE 5


uint8_t key_copy[64];
int level = -1;
int min_heared_level=+1;
struct tm * timeinfo;
struct tm * next_round_time;
time_t next_round_time_t;
int received_time = 0;
i2c_dev_t dev;

const char *PROTOCOL_TAG = "Protocol";
const char *GATHERING_TAG = "Gathering";
const char *DISCOVER_TAG = "Discovery";

typedef struct{
	i2c_dev_t dev;
	int* capacity_flag;
	int* gas_flag;
	int* temperature_flag;
} protocol_task_parameters;

long unsigned int xx_time_get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}


typedef struct {
    uint8_t hash[32];
    int id;
    int level;
    long long unsigned int curent_time;
    long long unsigned int next_round;
    int alarm_capacity;
    int alarm_gas;
    int alarm_temperature;
    
} protocol_message;

void print_protocol_message(protocol_message* pm){
    ESP_LOGI(PROTOCOL_TAG, "Hash:");
    for(int i = 0; i < 32; i++){
        printf("%02x", pm->hash[i]);
    }
    ESP_LOGI(PROTOCOL_TAG, "ID: %d", pm->id);
    ESP_LOGI(PROTOCOL_TAG, "Level: %d", pm->level);
    ESP_LOGI(PROTOCOL_TAG, "Current Time: %lld", pm->curent_time);
    ESP_LOGI(PROTOCOL_TAG, "Next Round: %lld", pm->next_round);
    ESP_LOGI(PROTOCOL_TAG, "Capacity: %d", pm->alarm_capacity);
    ESP_LOGI(PROTOCOL_TAG, "Temperature: %d", pm->alarm_temperature);
    ESP_LOGI(PROTOCOL_TAG, "GAS LEVELS: %d", pm->alarm_gas);
}


typedef struct {
    int id;
    int level;
    time_t curent_time;
    time_t next_round;
    int alarm_capacity;
    int alarm_gas;
    int alarm_temperature;
    uint8_t key[64];    
} verification_message;

void compute_message_hash(protocol_message* message,uint8_t* hash_destination){
    print_protocol_message(message);
    verification_message v_message;
    v_message.id = message->id;
    v_message.level = message->level;
    v_message.curent_time = message->curent_time;
    v_message.next_round = message->next_round;
    v_message.alarm_capacity = message->alarm_capacity;
    v_message.alarm_gas = message->alarm_gas;
    v_message.alarm_temperature = message->alarm_temperature;
    memcpy(v_message.key, key_copy, 64);

    uint8_t hash[32];
    generate_signature(hash, (uint8_t*)&v_message, sizeof(verification_message));
    //print the hash
    for(int i = 0; i < 32; i++){
        printf("%02x", hash[i]);
    }

    memcpy(hash_destination, hash, 32);
}

int verify_message(protocol_message* messsage){
    printf("Verifying message\n");
    print_protocol_message(messsage);
    uint8_t proof[32];
    compute_message_hash(messsage, proof);
    for(int i = 0; i < 32; i++){
        if(proof[i] != messsage->hash[i]){
            ESP_LOGE("Protocol", "Message is not valid");
            return 0;
        }
    }
    ESP_LOGE("Protocol", "Message is valid");
    return 1;
}


protocol_message buffer_of_received_messages[10];
int messages_in_buffer = 0;

void print_time_calendar(struct tm * time){
    printf("Time: %02d:%02d:%02d\n", time->tm_hour, time->tm_min, time->tm_sec);
    printf("Date: %02d/%02d/%02d\n", time->tm_mday, time->tm_mon+1, time->tm_year+1900);
}





void message_receive_callback(sx127x *device, uint8_t *data, uint16_t data_length) {
    int16_t rssi;
    ESP_ERROR_CHECK(sx127x_rx_get_packet_rssi(device, &rssi));
    float snr;
    ESP_ERROR_CHECK(sx127x_lora_rx_get_packet_snr(device, &snr));
    int32_t frequency_error;
    ESP_ERROR_CHECK(sx127x_rx_get_frequency_error(device, &frequency_error));
    ESP_LOGI(LORA_TAG, "received: %d rssi: %d snr: %f freq_error: %" PRId32, data_length, rssi, snr, frequency_error);
    data_received = malloc(data_length);
    memcpy(data_received, data, data_length);
    printf("Data received: %d\n",data_length);
    if(data_length==sizeof(protocol_message) && verify_message((protocol_message*)data_received)){
        protocol_message *message = (protocol_message*)data_received;
        print_protocol_message(message);
        if(message->level<min_heared_level){
            min_heared_level=message->level;
        }
        time_t time_rcv = message->curent_time;
        time_t time_rcv_next = message->next_round;
        timeinfo = localtime(&time_rcv);
        syncronize_from_received_time(dev, *timeinfo);
        printf ( "Time received is: %s", asctime (timeinfo) );
        time_rcv = message->next_round;
        next_round_time_t = message->next_round;
        next_round_time = localtime(&time_rcv);
        printf ( "Next Round is: %s", asctime (next_round_time) );

        ESP_LOGI(PROTOCOL_TAG, "Level discovered until now: %d", min_heared_level);
    }
    else{
        ESP_LOGI(PROTOCOL_TAG, "Received data is not a protocol message");
    }
    received = 0;
    ESP_LOGI(LORA_TAG, "Data Copied. End Callback");
    
}

protocol_message generate_message(time_t alarm_time){
    time_t raw_time;
    time(&raw_time);
    protocol_message message;
    message.id=id;
    message.level=level;
    message.curent_time=raw_time;
    message.next_round=alarm_time;
    message.alarm_capacity=0;
    message.alarm_gas=0;
    message.alarm_temperature=0;
    compute_message_hash(&message, message.hash);
    
    return message;
}

void message_receive_store_callback(sx127x *device, uint8_t *data, uint16_t data_length) {
    int16_t rssi;
    ESP_ERROR_CHECK(sx127x_rx_get_packet_rssi(device, &rssi));
    float snr;
    ESP_ERROR_CHECK(sx127x_lora_rx_get_packet_snr(device, &snr));
    int32_t frequency_error;
    ESP_ERROR_CHECK(sx127x_rx_get_frequency_error(device, &frequency_error));
    ESP_LOGI(LORA_TAG, "received: %d rssi: %d snr: %f freq_error: %" PRId32, data_length, rssi, snr, frequency_error);
    data_received = malloc(data_length);
    memcpy(data_received, data, data_length);

    if(data_length==sizeof(protocol_message) && verify_message((protocol_message*)data_received)){

        protocol_message *message = (protocol_message*)data_received;
        print_protocol_message(message);
        buffer_of_received_messages[messages_in_buffer] = *message;
        //print_protocol_message(buffer_of_received_messages[messages_in_buffer]);
        messages_in_buffer++;

    }
    else{
        ESP_LOGI(PROTOCOL_TAG, "Received data is not a protocol message");
    }
    received = 0;
    ESP_LOGI(LORA_TAG, "Data Copied. End Callback");
    
}

void recevice_message_time_synch(sx127x *device, uint8_t *data, uint16_t data_length) {
    int16_t rssi;
    ESP_ERROR_CHECK(sx127x_rx_get_packet_rssi(device, &rssi));
    float snr;
    ESP_ERROR_CHECK(sx127x_lora_rx_get_packet_snr(device, &snr));
    int32_t frequency_error;
    ESP_ERROR_CHECK(sx127x_rx_get_frequency_error(device, &frequency_error));
    ESP_LOGI(LORA_TAG, "received: %d rssi: %d snr: %f freq_error: %" PRId32, data_length, rssi, snr, frequency_error);
    data_received = malloc(data_length);
    memcpy(data_received, data, data_length);

    if(data_length==sizeof(protocol_message)&& verify_message((protocol_message*)data_received)){

        protocol_message *message = (protocol_message*)data_received;
        //accept only messages from levels under me
        if(message->level==level-1){
            ESP_LOGW("Protocol","Received message for time synch");
            time_t time_rcv = message->curent_time;
            timeinfo = localtime(&time_rcv);
            syncronize_from_received_time(dev, *timeinfo);
            time_t received_next_round = message->next_round;
            next_round_time = localtime(&received_next_round);
            ESP_LOGW("Protocol","Next round time will be: ");
            print_time_calendar(next_round_time);
            set_alarm_time(dev, *next_round_time);
            protocol_message message = generate_message(received_next_round);
            //sendding the message to the next level
            int * message_to_send = (int *)&message;
            send_data(message_to_send, sizeof(protocol_message), device);

        }
        
        buffer_of_received_messages[messages_in_buffer] = *message;
        //print_protocol_message(buffer_of_received_messages[messages_in_buffer]);
        messages_in_buffer++;

    }
    else{
        ESP_LOGI(PROTOCOL_TAG, "Received data is not a protocol message");
    }
    received = 0;
    ESP_LOGI(LORA_TAG, "Data Copied. End Callback");
    
}



void protocol(void *pvParameters){
    messages_in_buffer=0;
    protocol_task_parameters* parameters = (protocol_task_parameters*) pvParameters;
    dev = parameters->dev;
    int pinNumber=0;
    

    if(wifi) level = 0;
    long unsigned int start_time = xx_time_get_time();
    long unsigned int current_time;
    //if level is -1, the device must discover its own level
    if(level == -1){
        
        ESP_LOGI(PROTOCOL_TAG, "Discovering level");
        receive_data(message_receive_callback, device);
        

        while(!received){
            printf("Waiting for data\n");
            current_time = xx_time_get_time();

            if(current_time - start_time > 50000){
                //if ten seconds are passed and no data is received i will wait for next discovering task
                ESP_LOGI(PROTOCOL_TAG, "Timeout in discovering of nearby nodes");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));

        }
        lora_set_idle();
        if(min_heared_level==MAX_LEVEL+1){
            level=MAX_LEVEL-1;
        }
        else{
            level=min_heared_level+1;
        }

        ESP_LOGI(PROTOCOL_TAG, "Level discovered: %d", level);
        get_clock_from_rtc(dev);
        //print alarm time  

    }
    else{
        time_t raw_time;
        time(&raw_time);
        time_t next_round = raw_time + 60;
        while(1){
            current_time = xx_time_get_time();
            if(current_time - start_time > 10000){
                ESP_LOGI(PROTOCOL_TAG, "Timeout in discovering of nearby nodes");
                break;
            }

            
            ESP_LOGI(PROTOCOL_TAG, "Sending Data");
            time(&raw_time);
            
            protocol_message message = generate_message(next_round);
            

            int * message_to_send = (int *)&message;

            send_data(message_to_send, sizeof(protocol_message), device);

            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        next_round_time = localtime(&next_round);

    }
    if(level!=-1){
        configure_alarms(dev);
        reset_alarms(dev);
        ESP_LOGE("protocol","Time of the rtc");
        get_clock_from_rtc(dev);

        print_time_calendar(next_round_time);
        set_alarm_time(dev, *next_round_time);
        control_registers_status(dev);
        
        ESP_LOGE("PROTOCOL", "Sleeping until next round");
        esp_wifi_stop();
        esp_light_sleep_start();
        ESP_LOGE("PROTOCOL", "Waking up");
        reset_alarms(dev);

        next_round_time->tm_min+=1;
        set_alarm_time(dev, *next_round_time);
        ESP_LOGW("Protocol", "Next round time will be: ");
        print_time_calendar(next_round_time);
    }
    
    int time_to_wait = WINDOW_SIZE*(MAX_LEVEL-(level+1));

    if(time_to_wait<0)time_to_wait=0;
    printf("my time to sleep is: %d\n",time_to_wait);
    

    

    while(1){
        vTaskDelay(pdMS_TO_TICKS(time_to_wait*1000));
        //if im in the maximum level i do not transmit
        if(level!=MAX_LEVEL){
            start_time = xx_time_get_time();
            messages_in_buffer = 0;
            receive_data(message_receive_store_callback, device);
            ESP_LOGW("Protocol","Entering in receiving phase");
            while(1){
                current_time = xx_time_get_time();
                if(current_time - start_time > 5000){
                    ESP_LOGI(PROTOCOL_TAG, "Timeout in receiving packets");
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            lora_set_idle();
        }
        ESP_LOGW("Protocol","Entering in trasmitting phase");
        //then i transmit all the messages i have in the queue
        vTaskDelay(pdMS_TO_TICKS(1000));
        for(int i=0; i<messages_in_buffer; i++){
            printf("messages in buffer: %d\n",messages_in_buffer);
            int * message_to_send = (int *)&buffer_of_received_messages[i];
            printf("sending packet\n");
            send_data(message_to_send, sizeof(protocol_message), device);
        }
        messages_in_buffer = 0;
        protocol_message message = generate_message(0);
        int * message_to_send = (int *)&message;
        send_data(message_to_send, sizeof(protocol_message), device);


        //synchronization procedure

        if(level!=0){
            start_time = xx_time_get_time();
            receive_data(recevice_message_time_synch, device);
            ESP_LOGW("Protocol","Entering in receiving phase to synch time");
            while(1){
                current_time = xx_time_get_time();
                if(current_time - start_time > 5000){
                    ESP_LOGI(PROTOCOL_TAG, "Timeout in receiving packets");
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            lora_set_idle();
        }
        else{
            
            time_t now_time;
            time(&now_time);
            timeinfo = localtime(&now_time);
            syncronize_from_received_time(dev, *timeinfo);
            next_round_time_t = now_time + 60;
            protocol_message message = generate_message(next_round_time_t);
            int * message_to_send = (int *)&message;
            send_data(message_to_send, sizeof(protocol_message), device);
            next_round_time = localtime(&next_round_time_t);

        }
        printf("Setting the alarm at: ");
        reset_alarms(dev);
        print_time_calendar(next_round_time);
        set_alarm_time(dev, *next_round_time);
        esp_light_sleep_start();
        reset_alarms(dev);

    }

        

    

}



