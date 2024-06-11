#include "sha256.h"

void compute_hash(uint8_t *input, uint8_t *output){
    TCSha256State_t s = (TCSha256State_t)malloc(sizeof(struct tc_sha256_state_struct));
    tc_sha256_init(s);
    tc_sha256_update(s, input, strlen(input));
    tc_sha256_final(output, s);
    free(s);
}