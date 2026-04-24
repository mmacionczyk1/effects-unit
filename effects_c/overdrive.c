#include "overdrive.h"
#include <math.h>

typedef struct 
{
    float b0, b1, b2;
    float a1, a2;
    float s1;
    float s2;
} biquad_t;

static biquad_t _bq =  // butter lp 8k @ 48kHz
{
    0.1551451462f,
    0.3102902923f,
    0.1551451462f,
    -0.6197185377f,
    0.2402991224f,
    0.0f,
    0.0f
};

static float process_biquad(biquad_t* bq, float x)
{
    float output = (x * bq->b0) + bq->s1;
    bq->s1 = (x * bq->b1) - (output * bq->a1) + bq->s2;
    bq->s2 = (x * bq->b2) - (output * bq->a2);
    return output;
}

void process_overdrive(overdrive_config_t* cfg, float* input, float* output)
{
    const float gain = cfg->gain;
    const driving_functions_e f = cfg->driving_function;

    for(uint32_t i = 0; i < cfg->sample_size; i++)
    {
        float x = input[i] * gain;
        float y;
        
        switch(f)
        {
            case DRIVE_HARD:
                x = fmaxf(-1.0f, fminf(1.0f, x));
                y = x;
                break;
            case DRIVE_SOFT:
                x = fmaxf(-1.0f, fminf(1.0f, x));
                y = x - x*x*x * 0.333333f;
                break;
            default:
                y = x;
        }
        output[i] = (y * _bq.b0) + _bq.s1;
        _bq.s1 = (y * _bq.b1) - (output[i] * _bq.a1) + _bq.s2;
        _bq.s2 = (y * _bq.b2) - (output[i] * _bq.a2);
    }
}
