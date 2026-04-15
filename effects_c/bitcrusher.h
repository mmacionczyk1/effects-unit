#ifndef BITCRUSHER_H_
#define BITCRUSHER_H_
#include <stdint.h>


typedef struct
{
    uint32_t bit_depth;
    uint32_t downsample_factor;
    float mix_parameter;
    uint32_t sample_size;
    // "private"
    float tmp_sample;
    uint32_t counter;
} bitcrusher_config_t;


void process_bitcrusher(bitcrusher_config_t* cfg, float* input, float* output);




#endif