#include "esp_log.h"
#include "mqtt_client.h"
#include "certificate.h"
#include "main_protocol.h"

#define MQTT_SERVER_ADDRESS "mqtts://097d1c9a75344b39bdd151c2ed02f4af.s1.eu.hivemq.cloud:8883"
#define MQTT_USERNAME "LorenzoIndividual"
#define MQTT_PASSWORD "Lorenzo00"

typedef struct{
    QueueHandle_t protocol_messages_buffer;
    esp_mqtt_client_handle_t client;
} sender_task_parameters;


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE("mqtt_example", "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD("mqtt_example", "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI("mqtt_example", "MQTT_EVENT_CONNECTED");
        //msg_id = esp_mqtt_client_publish(client, "/lollo/test", "test messaggio rabbit mq", 0, 1, 0);
        //ESP_LOGI("mqtt_example", "sent publish successful, msg_id=%d", msg_id);

        //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        //ESP_LOGI("mqtt_example", "sent subscribe successful, msg_id=%d", msg_id);

        //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        //ESP_LOGI("mqtt_example", "sent subscribe successful, msg_id=%d", msg_id);

        //msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        //ESP_LOGI("mqtt_example", "sent unsubscribe successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI("mqtt_example", "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI("mqtt_example", "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI("mqtt_example", "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI("mqtt_example", "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI("mqtt_example", "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI("mqtt_example", "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI("mqtt_example", "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI("mqtt_example", "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI("mqtt_example", "Other event id:%d", event->event_id);
        break;
    }
}


static esp_mqtt_client_handle_t mqtt_app_start(void)
{
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_SERVER_ADDRESS,
        .broker.verification.certificate = (const char*) mqtt_broker_cert_pem,

        .credentials.username =MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_mqtt_client_start(client);
    return client;
    
}

void sender_task(void *pvParameters){

    sender_task_parameters *parameters = (sender_task_parameters *) pvParameters;
    
    protocol_message protocol_message_to_send;
    QueueHandle_t protocol_messages = parameters->protocol_messages_buffer;
    esp_mqtt_client_handle_t client = parameters->client;
    char msg[256];

    while(1){
        
        xQueueReceive(protocol_messages, &protocol_message_to_send, portMAX_DELAY);
        sprintf(msg, "ID: %d, Level: %d, Current Time: %llu, Next Round: %llu, Alarm Capacity: %d, Alarm Gas: %d, Alarm Temperature: %d", protocol_message_to_send.id, protocol_message_to_send.level, protocol_message_to_send.curent_time, protocol_message_to_send.next_round, protocol_message_to_send.alarm_capacity, protocol_message_to_send.alarm_gas, protocol_message_to_send.alarm_temperature);
        esp_mqtt_client_publish(client, "/lollo/test", msg, 0, 1, 0);
        
    }





}