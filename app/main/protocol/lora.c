#include "main_protocol.h"

#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <esp_intr_alloc.h>
#include "sx127x.h"

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 23
#define DIO0 26

static const char *LORA_TAG = "sx127x";

sx127x *device = NULL;
TaskHandle_t handle_interrupt;
int supported_power_levels[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20};
int supported_power_levels_count = sizeof(supported_power_levels) / sizeof(int);
int current_power_level = 0;

uint8_t *data_to_send;
size_t data_send_lenght;

uint8_t *data_received;
size_t data_received_lenght;

int trasmitted = 0;
int received = 0;

// ********* SEND *********

void send_data(int* data_to_send, int data_send_lenght, sx127x * device){
  ESP_ERROR_CHECK(sx127x_lora_tx_set_for_transmission(data_to_send, data_send_lenght, device));
  ESP_LOGI(LORA_TAG, "transmitting %d data", data_send_lenght);
  trasmitted = 1;
  ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_TX, SX127x_MODULATION_LORA, device));
}

void sending_callback(sx127x *device) {
    ESP_LOGI(LORA_TAG, "transmitted");
    trasmitted = 0;
}

// ********* RECEIVE *********

void receive_callback(sx127x *device, uint8_t *data, uint16_t data_length) {
    int16_t rssi;
    ESP_ERROR_CHECK(sx127x_rx_get_packet_rssi(device, &rssi));
    float snr;
    ESP_ERROR_CHECK(sx127x_lora_rx_get_packet_snr(device, &snr));
    int32_t frequency_error;
    ESP_ERROR_CHECK(sx127x_rx_get_frequency_error(device, &frequency_error));
    ESP_LOGI(LORA_TAG, "received: %d rssi: %d snr: %f freq_error: %" PRId32, data_length, rssi, snr, frequency_error);
    data_received = malloc(data_length);
    memcpy(data_received, data, data_length);
    received = 1;
    ESP_LOGI(LORA_TAG, "Data Copied. End Callback");
}

void cad_callback(sx127x *device, int cad_detected) {
    if (cad_detected == 0) {
        ESP_LOGI(LORA_TAG, "cad not detected");
        ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_CAD, SX127x_MODULATION_LORA, device));
        return;
    }
    // put into RX mode first to handle interrupt as soon as possible
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, SX127x_MODULATION_LORA, device));
    ESP_LOGI(LORA_TAG, "cad detected\n");
}

void receive_data(void (*rc_call)(sx127x, uint8_t, uint16_t), sx127x *device){
  sx127x_rx_set_callback(rc_call, device);
  sx127x_lora_cad_set_callback(cad_callback, device);
  ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, SX127x_MODULATION_LORA, device));
}


// ********* GENERAL *********

void lora_handle_interrupt_task(void *arg) {
  while (1) {
    vTaskSuspend(NULL);
    sx127x_handle_interrupt((sx127x *)arg);
  }
}

void IRAM_ATTR handle_interrupt_fromisr(void *arg) {
  xTaskResumeFromISR(handle_interrupt);
}

void setup_gpio_interrupts(gpio_num_t gpio, sx127x *device) {
    gpio_set_direction(gpio, GPIO_MODE_INPUT);
    gpio_pulldown_en(gpio);
    gpio_pullup_dis(gpio);
    gpio_set_intr_type(gpio, GPIO_INTR_POSEDGE);
    gpio_isr_handler_add(gpio, handle_interrupt_fromisr, (void *)device);
}

void lora_setup(){
    ESP_LOGI(LORA_TAG, "Starting up");
    spi_bus_config_t config = {
        .mosi_io_num = MOSI,
        .miso_io_num = MISO,
        .sclk_io_num = SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &config, 1));
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 3E6,
        .spics_io_num = SS,
        .queue_size = 16,
        .command_bits = 0,
        .address_bits = 8,
        .dummy_bits = 0,
        .mode = 0};
    spi_device_handle_t spi_device;
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &dev_cfg, &spi_device));
    ESP_ERROR_CHECK(sx127x_create(spi_device, &device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_SLEEP, SX127x_MODULATION_LORA, device));
    ESP_ERROR_CHECK(sx127x_set_frequency(437200012, device));
    ESP_ERROR_CHECK(sx127x_lora_reset_fifo(device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, SX127x_MODULATION_LORA, device));
    ESP_ERROR_CHECK(sx127x_lora_set_bandwidth(SX127x_BW_125000, device));
    ESP_ERROR_CHECK(sx127x_lora_set_implicit_header(NULL, device));
    ESP_ERROR_CHECK(sx127x_lora_set_modem_config_2(SX127x_SF_9, device));
    ESP_ERROR_CHECK(sx127x_lora_set_syncword(18, device));
    ESP_ERROR_CHECK(sx127x_set_preamble_length(8, device));

    
    ESP_LOGI(LORA_TAG, "First Setup ok");

    BaseType_t task_code = xTaskCreatePinnedToCore(lora_handle_interrupt_task, "handle interrupt", 8196, device, 2, &handle_interrupt, xPortGetCoreID());
    if (task_code != pdPASS) {
        ESP_LOGE(LORA_TAG, "can't create task %d", task_code);
        sx127x_destroy(device);
        return;
    }
    
    ESP_LOGI(LORA_TAG, "Interrupt task created");
    
    gpio_install_isr_service(0);
    setup_gpio_interrupts((gpio_num_t)DIO0, device);

    ESP_ERROR_CHECK(sx127x_tx_set_pa_config(SX127x_PA_PIN_BOOST, supported_power_levels[current_power_level], device));
    sx127x_tx_header_t header = {
        .enable_crc = 1,
        .coding_rate = SX127x_CR_4_5};
    ESP_ERROR_CHECK(sx127x_lora_tx_set_explicit_header(&header, device));

  
    ESP_LOGI(LORA_TAG, "Setup Finalized");
  
}

void lora_set_idle(){
  ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, SX127x_MODULATION_LORA, device));
}