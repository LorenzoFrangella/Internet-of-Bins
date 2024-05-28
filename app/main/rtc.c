#include "main.h"

u_int64 time_to_multiply = 100;
int time_to_wait_standard = 100;

void set_time_to_sleep(u_int64_t* array, int* slp_s){
    for (int i = 0; i < 2; i++){
        switch (slp_s[i])
        {
        case -2:
            array[i] = 0;
            break;
        case -1:
            array[i] = 0;
            break;
        
        default:
            array[i] = (u_int64_t)slp_s * time_to_multuply;
            break;
        }
    }
    array[2] = time_to_wait_standard;
}