#include "audio_process.h"
#include "audio_buffers.h"
#include <string.h>
#include "stm32f4xx.h"

typedef struct
{
    audio_effect_func_t func;
    audio_set_size_func_t set_size;
    void* config;
    bool enabled;
} effect_slot_t;



static effect_slot_t _chain[EFFECT_CHAIN_MAX_SLOTS];
static uint8_t _chain_len = 0;
static uint8_t _synced = 0;
static I2S_HandleTypeDef* _hi2s = NULL;
static ADC_HandleTypeDef* _hadc = NULL;

static void run_effect_chain(float* in, float* out, uint16_t n)
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
    bool any_active = false;

    for (uint8_t i = 0; i < _chain_len; i++)
    {
        effect_slot_t* s = &_chain[i];

        if (!s->enabled || s->func == NULL)
            continue;

        if (s->set_size != NULL)
            s->set_size(s->config, (uint32_t)n);

        s->func(s->config, any_active ? out : in, out);
        any_active = true;
    }

    if (!any_active && in != out)
        memcpy(out, in, n * sizeof(float));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
}


void audio_process_init(I2S_HandleTypeDef* hi2s, ADC_HandleTypeDef* hadc)
{
    _hi2s = hi2s;
    _hadc = hadc;
    effect_chain_clear();
}

void audio_pipeline(uint8_t half)
{
    const uint16_t n = AUDIO_HALF_BUF_LEN;
    const uint16_t f_off  = half ? n : 0u;
    const uint16_t stereo_off = half ? AUDIO_HALF_BUF_STEREO_LEN : 0u;

    float* f_in  = get_f_input_buffer() + f_off;
    float* f_out = get_f_output_buffer() + f_off;
    int16_t* i_out = get_i16_output_buffer() + stereo_off;

    convert_adc_u16f_buf(get_u16_input_buffer() + f_off, f_in, n);
    run_effect_chain(f_in, f_out, n);
    convert_fi16_stereo(f_out, f_out, i_out, n);
}


bool effect_chain_add(audio_effect_func_t func, audio_set_size_func_t set_size, void* config)
{
    if (_chain_len >= EFFECT_CHAIN_MAX_SLOTS)
        return false;

    _chain[_chain_len].func = func;
    _chain[_chain_len].set_size = set_size;
    _chain[_chain_len].config = config;
    _chain[_chain_len].enabled = true;
    _chain_len++;
    return true;
}

void effect_chain_clear(void)
{
    memset(_chain, 0, sizeof(_chain));
    _chain_len = 0;
}

void effect_chain_set_enabled(uint8_t index, bool enabled)
{
    if (index >= _chain_len)
        return;
    _chain[index].enabled = enabled;
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef* hi2s)
{
    if (hi2s != _hi2s) return;

    if (!_synced) 
    {
        HAL_ADC_Start_DMA(_hadc, (uint32_t*)get_u16_input_buffer(), AUDIO_BUF_LEN);
        _synced = 1;
        return;
    }

    audio_pipeline(0);
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* hi2s)
{
    if (hi2s != _hi2s) return;
    if (!_synced) return;
    audio_pipeline(1);
}
