#include "equaliser.h"
#include <math.h>

#ifndef PI
#define PI 3.1415926535f
#endif

static void eq_update_coeffs(biquad_t* bq, eq_band_config_t* bcfg, uint32_t sample_rate);
static void biquad_process(biquad_t* bq, float* x, float* y, uint32_t num);



static void biquad_process(biquad_t* bq, float* x, float* y, uint32_t num)
{
    for (uint32_t i = 0; i < num; i++)
    {
        float input = x[i];
        float output = (input * bq->b0) + bq->s1;
        bq->s1 = (input * bq->b1) - (output * bq->a1) + bq->s2;
        bq->s2 = (input * bq->b2) - (output * bq->a2);

        y[i] = output;
    }
}
void eq_process(equaliser_t* eq, float* x, float* y, uint32_t sample_size)
{
    bool first_pass = true;
    bool any_active = false;

    for (uint32_t i = 0; i < EQ_MAX_BANDS; i++)
    {
        if (eq->bands[i].enabled)
        {
            biquad_process(&eq->biquads[i], (first_pass ? x : y), y, sample_size);
            first_pass = false;
            any_active = true;
        }
    }
    
    if (!any_active && x != y)
    {
        for (uint32_t i = 0; i < sample_size; i++) y[i] = x[i];
    }
}


void eq_reset(equaliser_t* eqs)
{
    biquad_t* bqs = eqs->biquads;
	for (uint32_t i = 0; i < EQ_MAX_BANDS; i++)
	{
		bqs[i].b0 = 1.0f;
        bqs[i].b1 = 0.0f;
        bqs[i].b2 = 0.0f;
        bqs[i].a1 = 0.0f;
        bqs[i].a2 = 0.0f;
        bqs[i].s1 = 0.0f;
        bqs[i].s2 = 0.0f;
    }
}

void eq_set_band(equaliser_t* eq, uint32_t index, eq_type_t type, float f0, float Q, float gain_db)
{
    if (index >= EQ_MAX_BANDS) return;
    eq_band_config_t* bcfg = &eq->bands[index];
    bcfg->type = type;
    bcfg->f0 = f0;
    bcfg->Q = Q;
    bcfg->db_gain = gain_db;

    eq_update_coeffs(&eq->biquads[index], bcfg, eq->sample_rate);
}

void eq_enable_band(equaliser_t* eq, uint32_t index)
{
    (eq->bands + index)->enabled=true;
}

void eq_disable_band(equaliser_t* eq, uint32_t index)
{
    (eq->bands + index)->enabled=false;
}

static void eq_update_coeffs(biquad_t* bq, eq_band_config_t* bcfg, uint32_t sample_rate)
{
    float A = powf(10.0f, bcfg->db_gain/40.0f);
    if (A < 0.0001f) A = 0.0001f;
    float w0 = 2.0f * PI * bcfg->f0 / sample_rate;
    float alpha = sinf(w0) * 0.5f / bcfg->Q;
    switch (bcfg->type)
    {
        case EQ_FILTER_LOW_PASS:
        {
            float a0 = 1.0f + alpha;
            float a0inv = 1.0f/a0;
            bq->a1 = (-2.0f * cosf(w0)) * a0inv;
            bq->a2 = (1.0f - alpha) * a0inv;
            bq->b0 = (1.0f - cosf(w0)) * 0.5f * a0inv;
            bq->b1 = (1.0f - cosf(w0)) * a0inv;
            bq->b2 = (1.0f - cosf(w0)) * 0.5f * a0inv;
            break;
        }
        case EQ_FILTER_HIGH_PASS:
        {
            float a0 = 1.0f + alpha;
            float a0inv = 1.0f/a0;
            bq->a1 = (-2.0f * cosf(w0)) * a0inv;
            bq->a2 = (1.0f - alpha) * a0inv;
            bq->b0 = (1.0f + cosf(w0)) * 0.5f * a0inv;
            bq->b1 = -(1.0f + cosf(w0)) * a0inv;
            bq->b2 = (1.0f + cosf(w0)) * 0.5f * a0inv;
            break;
        }
        case EQ_FILTER_PEAKING:
        {
            float a0 = 1.0f + alpha/A;
            float a0inv = 1.0f/a0;
            bq->a1 = (-2.0f * cosf(w0)) * a0inv;
            bq->a2 = (1.0f - alpha/A) * a0inv;
            bq->b0 = (1.0f + alpha*A) * a0inv;
            bq->b1 = (-2.0f* cosf(w0)) * a0inv;
            bq->b2 = (1.0f - alpha*A) * a0inv;
            break;
        }
        case EQ_FILTER_LOW_SHELF:
        {
            float a0 = (A+1.0f) + (A-1.0f)*cosf(w0) + 2.0f*sqrtf(A)*alpha;
            float a0inv = 1.0f/a0;
            bq->a1 = -2.0f*( (A-1.0f) + (A+1.0f)*cosf(w0)) * a0inv;
            bq->a2 = ((A+1.0f) + (A-1.0f)*cosf(w0) - 2.0f*sqrtf(A)*alpha) * a0inv;
            bq->b0 = A*( (A+1.0f) - (A-1.0f)*cosf(w0) + 2.0f*sqrtf(A)*alpha )* a0inv;
            bq->b1 = 2.0f*A*( (A-1.0f) - (A+1.0f)*cosf(w0)) * a0inv;
            bq->b2 = A*( (A+1.0f) - (A-1.0f)*cosf(w0) - 2.0f*sqrtf(A)*alpha ) * a0inv;
            break;
        }
        case EQ_FILTER_HIGH_SHELF:
        {
            float a0 = (A+1.0f) - (A-1.0f)*cosf(w0) + 2.0f*sqrtf(A)*alpha;
            float a0inv = 1.0f/a0;
            bq->a1 = 2.0f*( (A-1.0f) - (A+1.0f)*cosf(w0)) * a0inv;
            bq->a2 = ((A+1.0f) - (A-1.0f)*cosf(w0) - 2.0f*sqrtf(A)*alpha) * a0inv;
            bq->b0 = A*( (A+1.0f) + (A-1.0f)*cosf(w0) + 2.0f*sqrtf(A)*alpha )* a0inv;
            bq->b1 = -2.0f*A*( (A-1.0f) + (A+1.0f)*cosf(w0)) * a0inv;
            bq->b2 = A*( (A+1.0f) + (A-1.0f)*cosf(w0) - 2.0f*sqrtf(A)*alpha ) * a0inv;
            break;
        }
        default:
            break;
    }
}


