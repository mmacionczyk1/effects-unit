#ifndef EFFECTS_WRAPPERS_H_
#define EFFECTS_WRAPPERS_H_

#include <stdint.h>
#include <stdbool.h>
#include "equaliser.h"
#include "bitcrusher.h"
#include "overdrive.h"

typedef void (*audio_effect_func_t)(void* config, float* in, float* out);
typedef void (*audio_set_size_func_t)(void* config, uint32_t n);



void equaliser_wrapper(void* config, float* in, float* out);
void overdrive_wrapper(void* config, float* in, float* out);
void bitcrusher_wrapper(void* config, float* in, float* out);

void equaliser_set_size(void* config, uint32_t n);
void overdrive_set_size(void* config, uint32_t n);
void bitcrusher_set_size(void* config, uint32_t n);


#endif /* EFFECTS_WRAPPERS_H_ */
