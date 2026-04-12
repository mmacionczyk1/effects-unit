#ifndef OVERDRIVE_H_
#define OVERDRIVE_H_
#include <stdint.h>

typedef enum 
{
    DRIVE_SOFT,
    DRIVE_HARD
} driving_functions_e;

typedef struct
{
    float gain;
    driving_functions_e driving_function;
    uint32_t sample_size;

} overdrive_config_t;



void process_overdrive(overdrive_config_t* cfg, float* input, float* output);



#endif