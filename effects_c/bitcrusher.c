#include "bitcrusher.h"


void process_bitcrusher(bitcrusher_config_t* cfg, float* input, float* output)
{
    const uint32_t dsf = cfg->downsample_factor;
    uint32_t cnt = cfg->counter;
    float tmp = cfg->tmp_sample;
    const float steps = (float)((1 << (cfg->bit_depth - 1)) - 1);
    const float inv_steps = 1.0f / (steps + 0.000001f);
    const float mix = cfg->mix_parameter;

    for (uint32_t i = 0; i < cfg->sample_size; i++)
    {
        if(cnt == 0)
        {
            float x = input[i] * steps;
            if (x >= 0.0f) x = (float)((int)(x + 0.5f));
            else x = (float)((int)(x - 0.5f));
            x = fmaxf(-steps, fminf(steps, x));
            tmp = x * inv_steps;
        }
        output[i] = (1.0f - mix) * input[i] + mix * tmp;
        if (++cnt >= dsf) 
        {
            cnt = 0;
        }
    }
    cfg->counter = cnt;
    cfg->tmp_sample = tmp;
}