#include "sha256.c"
#include "freertos/FreeRTOS.h"

void generate_signature(uint8_t* output, uint8_t* buffer, size_t buffer_size){
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, buffer,buffer_size);
    sha256_final(&ctx, output);

}

int verify_message_validity(uint8_t* output, uint8_t* buffer, size_t buffer_size){
    uint8_t hash[SHA256_BLOCK_SIZE];
    generate_signature(hash, buffer, buffer_size);
    for(int i = 0; i < SHA256_BLOCK_SIZE; i++){
        if(hash[i] != output[i]){
            return 0;
        }
    }
    return 1;
}
