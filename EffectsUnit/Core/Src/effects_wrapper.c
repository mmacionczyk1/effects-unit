#include "effects_wrappers.h"



void equaliser_wrapper(void* config, float* in, float* out)
{
	eq_process((equaliser_t* )config, in, out);
}

void overdrive_wrapper(void* config, float* in, float* out)
{
    overdrive_process((overdrive_config_t* )config, in, out);
}

void bitcrusher_wrapper(void* config, float* in, float* out)
{
	process_bitcrusher((bitcrusher_config_t* )config, in, out);
}

void equaliser_set_size(void* config, uint32_t n)
{
    ((equaliser_t*)config)->sample_size = n;
}

void overdrive_set_size(void* config, uint32_t n)
{
    ((overdrive_config_t*)config)->sample_size = n;
}

void bitcrusher_set_size(void* config, uint32_t n)
{
    ((bitcrusher_config_t*)config)->sample_size = n;
}
