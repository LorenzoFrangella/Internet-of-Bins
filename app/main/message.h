struct protocol_content{
    int status;
    int *alerts;
};

struct protocol_message {
    int id;
    int level;
    int maxKnowLevel;
    int gateway;
    struct protocol_content content;
};