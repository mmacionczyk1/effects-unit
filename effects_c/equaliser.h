#ifndef EQUALISER_H_
#define EQUALISER_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef EQ_MAX_BANDS
    #define EQ_MAX_BANDS 8
#endif


typedef struct
{
    float a1, a2;
    float b0, b1, b2;
    float s1, s2;
} biquad_t;

typedef enum
{
    EQ_FILTER_LOW_PASS,
    EQ_FILTER_HIGH_PASS,
    EQ_FILTER_PEAKING,
    EQ_FILTER_LOW_SHELF,
    EQ_FILTER_HIGH_SHELF,
} eq_type_t;

typedef struct
{
    eq_type_t type;
    float f0;
    float Q;
    float db_gain;  
    bool enabled;
} eq_band_config_t;

typedef struct
{
    uint32_t sample_rate;
    eq_band_config_t bands[EQ_MAX_BANDS];
    biquad_t biquads[EQ_MAX_BANDS];
    uint32_t sample_size;
} equaliser_t;

void eq_reset(equaliser_t* eq);
void eq_process(equaliser_t* eq, float* x, float* y);
void eq_set_band(equaliser_t* eq, uint32_t index, eq_type_t type, float f0, float Q, float gain_db);
void eq_enable_band(equaliser_t* eq, uint32_t index);
void eq_disable_band(equaliser_t* eq, uint32_t index);
#endif
