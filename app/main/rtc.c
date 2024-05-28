#include "main.h"

void set_time_to_sleep(long unsigned int* array, int* slp_s){
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
            array[i] = (long unsigned int)slp_s * time_to_multiply;
            break;
        }
    }
    array[2] = time_to_wait_standard;
}