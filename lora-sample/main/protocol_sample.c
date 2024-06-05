#include "main.h"

struct node_structure {
    bool alert;
    bool alone;
    int level;
    int gateway_id;
    int max_known_level;
    bool last_round_succeded;
    int rounds_failed;
};

const struct node_structure structure = {
    false,
    true,
    -1,
    10,
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

struct protocol_message pm1 = {
    7, structure, {1,2,3}, 3, true, 4321
};