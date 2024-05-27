#include "main.h"
#include "message.h"

struct sleep_schedule{
    int first_listening;
    int first_talk;
    int second_listening;
    int second_talk;
};

const char *PROTOCOL_TAG = "Protocol";
const char *GATHERING_TAG = "Gathering";
#define MISSING_HOUR_LIMIT = 24;

bool wifi = false;
bool alert = false;
bool alone = true;
int level = NULL;
int gateway_id = NULL;
int max_known_level = NULL;
bool last_round_succeded = false;
int rounds_failed = -1;

struct node_status my_status;
struct node_status *cluster;
int cluster_occupation = -1;
int cluster_real_occupation = -1;
int cluster_lenght = 1;

int *not_seen_nodes;
int not_seen_nodes_lenght;

int *alert;
int alert_lenght;

void add_to_cluster(struct node_status node){

    // There are holes in cluster?
    if (cluster_real_occupation < cluster_occupation){
        for (int i; i < cluster_occupation; i++){
            if (cluster[i].node_id == -1){
                cluster[i] = node;
                cluster_real_occupation += 1;
                return;
            }
        }
    }
    // No holes
    else{

        cluster_occupation += 1;

        // There is still space?
        if (cluster_occupation > cluster_lenght){
            cluster_lenght += 1;
            struct node_status *cluster2 = malloc(cluster_lenght*sizeof(struct node_status));

            memcpy(&cluster2, &cluster, sizeof(cluster));
            free(cluster);

            cluster = cluster2;
        }

        cluster[cluster_occupation] = node;
    }
}

bool update_missing(int id){
    for (int i; i < cluster_occupation; i++){
        if (cluster[i].node_id == id){
            cluster[i].missing +=1;
            if (cluster[i].missing == MISSING_HOUR_LIMIT){
                return true;
            }else{
                return false;
            }
        }
    }
}

bool delete_from_cluster(int id){
    for (int i; i < cluster_occupation; i++){
        if (cluster[i].node_id == id){
            // Creating hole
            cluster[i] = (struct node_status){-1,false,-1};
            cluster_real_occupation -= 1;
            return true;
        }else{
            return false;
        }
    }
    return false;
}

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
    cluster = malloc(cluster_lenght*sizeof(struct node_status));
    my_status = (struct node_status){id, false, 0};
    add_to_cluster(my_status);
}

void end_of_hour_procedure(){

    // Check if connected but not anymore
    if(rounds_failed == MISSING_HOUR_LIMIT){
        ESP_LOGI(PROTOCOL_TAG, "Node is alone");
        alone = true;
        level = -1;
        gateway_id = -1;
        max_known_level = -1;
        rounds_failed = -1;
        return (struct sleep_schedule){-1,-1,-1,-1};
    }else if (wifi){
        return (struct sleep_schedule){-1,0,max_known_level*2-1,-1};
    }else{
        return (struct sleep_schedule){level-1,level,(max_known_level - level)*2-1,(max_known_level - level)*2};
    }
}


void protocol_hour_check(bool new_alert){
    alert = new_alert;
    
    /*
    // Checking if nodes did not communicate last round
    for (int i = 0; i < not_seen_nodes_lenght; i++){
        bool to_eliminate = update_missing(not_seen_nodes[i]);
        // Node is lost?
        if(to_eliminate){
            delete_from_cluster(not_seen_nodes[i]);
        }
    }
    free(not_seen_nodes);
    */
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

int gather_messages(int time_to_listen){

    messages = malloc(messages_starting_lenght*sizeof(struct protocol_message));

    int64_t start_count = xx_time_get_time();
    int64_t end_count;
    
    ESP_LOGI(GATHERING_TAG,"Trying to receive...");

    uint8_t buf[sizeof(struct protocol_message)+sizeof(int)*1024];
    struct protocol_message last_message;

    while (true) {
        
        lora_receive();    // put into receive mode
        while(lora_received()) {
            lora_receive_packet(buf, sizeof(buf));

            last_message = *(struct protocol_message*)buf;
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
// Check if connected to wifi during this round
void first_listen(struct protocol_message message){
    if (message.gateway == gateway_id && message.level == level - 1 && message.last_round_succeded == 0){
        last_round_succeded = true;
    }else if(message.level < level - 1){
        //Adjust for following round
    }
}

void first_listening(int time_to_wait){
    // gather all messages from LoRa
    ESP_LOGI(PROTOCOL_TAG, "Opening gathering window");
    xTaskCreate(&gather_handler, "gather_first_listening_task", 2048, time_to_wait, 2, gather_messages);
    vTaskDelay(pdMS_TO_TICKS(time_to_wait));

    ESP_LOGI(PROTOCOL_TAG, "Processing messages");

    for (int i = 0; i < messages_occupation; i++){
        first_listen(messages[i]);
    }

    free(messages);
}

void first_talk(){

}

void second_listen(struct protocol_message message){
    
}

void second_listening(){
    //gather all messages from LoRa

    //Eventually adapt to new configuration of network
    listen(messages);

    //Send alarms forward

}
/*

if self.wifi:
            #print("Gateway : " + str(self.id), flush=True)
            #print(self.alerts, flush=True)
            pass
        if random() <= 0.01:
            self.status = False
        if not self.status:
            if random() <= 0.1:
                self.status = True
        
        self.full += self.usage
        if self.full >= 0.8:
            self.cluster['self']['alert'] = True
        else:
            self.cluster['self']['alert'] = False
        #adjust cluster
        for node in self.notSeen:
            self.cluster[node]['missing'] += 1
            if self.cluster[node]['missing'] == 12:
                self.cluster.pop(node, None)
        #checkCluster
        allNodes = self.cluster.keys()
        alerts = 0
        for node in allNodes:
            if self.cluster[node]['alert']:
                alerts += 1
        self.alerts = list(set(self.alerts))
        if alerts > len(allNodes)/2:
            self.alerts +=  allNodes
        else:
            for node in allNodes:
                try:
                    self.alerts.remove(node)
                except:
                    pass
        self.notSeen = deepcopy(list(allNodes))
        self.notSeen.remove('self')
        if len(allNodes) == 1 and self.level != None and not self.wifi:
            print('Node ' + str(self.id) + ' returning to void')
            self.void = True
            self.level = None
            self.gateway = None
            self.maxKnownLevel = None
        #define node listening
        if not self.status:
            return (None, None)
        elif self.level == None:
            return (-2, -2)
        elif self.level == 0:
            return (-1, self.maxKnownLevel*2-1)
        else:
            return (self.level-1, ((self.maxKnownLevel-self.level)*2)-1)

*/