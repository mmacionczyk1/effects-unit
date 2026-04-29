#ifndef AUDIO_PROCESS_H_
#define AUDIO_PROCESS_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "effects_wrappers.h"


#define EFFECT_CHAIN_MAX_SLOTS  8


void audio_process_init(I2S_HandleTypeDef* hi2s, ADC_HandleTypeDef* hadc);
void audio_pipeline(uint8_t half);

bool effect_chain_add(audio_effect_func_t func, audio_set_size_func_t set_size, void* config);
void effect_chain_clear(void);
void effect_chain_set_enabled(uint8_t index, bool enabled);

#endif /* AUDIO_PROCESS_H_ */
