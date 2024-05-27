struct protocol_message {
    int id;
    int level;
    int max_know_level;
    int gateway;
    bool last_round_succeded;
    int *alerts;
};