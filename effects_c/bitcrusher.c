#include "bitcrusher.h"



static uint32_t xorshift32(uint32_t *state) 
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}


void process_bitcrusher(bitcrusher_config_t* cfg, float* input, float* output)
{
    const uint32_t dsf = cfg->downsample_factor;
    uint32_t cnt = cfg->counter;
    float tmp = cfg->tmp_sample;
    const float steps = (float)((1 << (cfg->bit_depth - 1)) - 1);
    const float inv_steps = 1.0f / (steps + 0.000001f);
    const float mix = cfg->mix_parameter;
    const float dither = cfg->dither_level;

    for (uint32_t i = 0; i < cfg->sample_size; i++)
    {
        if(cnt == 0)
        {
            float n1 = (float)((int32_t)xorshift32(&cfg->rng_state)) * (1.0f / 2147483648.0f);
            float n2 = (float)((int32_t)xorshift32(&cfg->rng_state)) * (1.0f / 2147483648.0f);
            float x = input[i] * steps + (n1 + n2) * 0.5f  * dither;
            if (x >= 0.0f) x = (float)((int32_t)(x + 0.5f));
            else x = (float)((int32_t)(x - 0.5f));
            x = fmaxf(-steps, fminf(steps, x));
            tmp = x * inv_steps;
        }
        output[i] = input[i] + mix * (tmp - input[i]);
        if (++cnt >= dsf) 
        {
            cnt = 0;
        }
    }
    cfg->counter = cnt;
    cfg->tmp_sample = tmp;
}