#include "main.h"
#include "message.h"

struct node_status{
    int node_id;
    bool alert;
    int missing;
};

const char *PROTOCOL_TAG = "Protocol";
#define MISSING_HOUR_LIMIT = 24;

bool wifi = false;

struct node_status my_status;
struct node_status *cluster;
int occupation_cluster = -1;
int real_occupation_cluster = -1;
int lenght_cluster = 10;

int *not_seen_nodes;
int lenght_not_seen_nodes;

void add_to_cluster(struct node_status node){
    if (real_occupation_cluster < occupation_cluster){
        for (int i; i < occupation_cluster; i++){
            if (cluster[i].node_id == -1){
                cluster[i] = node;
                real_occupation_cluster += 1;
                return;
            }
        }
    }else{

        occupation_cluster += 1;

        if (occupation_cluster > lenght_cluster){
            lenght_cluster += 10;
            struct node_status *cluster2 = malloc(lenght_cluster*sizeof(struct node_status));

            memcpy(&cluster2, &cluster, sizeof(cluster));
            free(cluster);

            cluster = cluster2;
        }

        cluster[occupation_cluster] = node;
    }
}

bool delete_from_cluster(int id){
    for (int i; i < occupation_cluster; i++){
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

bool update_missing(int id){
    for (int i; i < occupation_cluster; i++){
        if (cluster[i].node_id == id){
            cluster[i] = (struct node_status){-1,false,-1};
            real_occupation_cluster -= 1;
            return true;
        }else{
            return false;
        }
    }
    return false;
}

void protocol_init(bool wifi_init){
    wifi = wifi_init;
    cluster = malloc(lenght_cluster*sizeof(struct node_status));
    my_status = (struct node_status){id, false, 0};
    add_to_cluster(my_status);
}


void protocol_hour_check(bool alert){
    my_status.alert = alert;
    // Checking if nodes did not communicate last round
    for (int i = 0; i < lenght_not_seen_nodes; i++){
        bool to_eliminate = update_missing(not_seen_nodes[i]);
        if(to_eliminate){
            delete_from_cluster(not_seen_nodes[i]);
        }
    }
    free(not_seen_nodes);
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