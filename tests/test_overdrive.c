#include "./unity/unity.h"
#include "../effects_c/overdrive.h"
#include <math.h>
#include <string.h>
#include <stdint.h>

#define BLOCK_SIZE   256u
#define SAMPLE_RATE  48000u
#define FLOAT_TOL    1e-6f

#ifndef PI
#define PI 3.1415926535f
#endif

static void gen_sine(float* buf, uint32_t n, float freq, float amp)
{
    for (uint32_t i = 0; i < n; i++)
        buf[i] = amp * sinf(2.0f * PI * freq * (float)i / (float)SAMPLE_RATE);
}

static float rms(const float* buf, uint32_t n)
{
    float s = 0.0f;
    for (uint32_t i = 0; i < n; i++) s += buf[i] * buf[i];
    return sqrtf(s / (float)n);
}

static float peak(const float* buf, uint32_t n)
{
    float p = 0.0f;
    for (uint32_t i = 0; i < n; i++)
        if (fabsf(buf[i]) > p) p = fabsf(buf[i]);
    return p;
}

static void warmup(overdrive_config_t* cfg)
{
    float in[BLOCK_SIZE]  = {0};
    float out[BLOCK_SIZE] = {0};
    for (int k = 0; k < 8; k++)
        overdrive_process(cfg, in, out);
}

void setUp(void)
{
    overdrive_config_t cfg = { .gain = 1.0f, .driving_function = DRIVE_SOFT, .sample_size = BLOCK_SIZE };
    float zero[BLOCK_SIZE] = {0};
    float out[BLOCK_SIZE];
    for (int k = 0; k < 16; k++)
        overdrive_process(&cfg, zero, out);
}

void tearDown(void) {}

void test_soft_unity_gain_passes_quiet_signal(void)
{
    overdrive_config_t cfg = { .gain = 1.0f, .driving_function = DRIVE_SOFT, .sample_size = BLOCK_SIZE };
    warmup(&cfg);

    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 1000.0f, 0.01f);

    for (int k = 0; k < 8; k++)
        overdrive_process(&cfg, in, out);

    float rms_in = rms(in, BLOCK_SIZE);
    float rms_out = rms(out, BLOCK_SIZE);

    TEST_ASSERT_FLOAT_WITHIN(rms_in * 0.2f, rms_in, rms_out);
}

void test_hard_clip_limits_peak_to_one(void)
{
    overdrive_config_t cfg = { .gain = 10.0f, .driving_function = DRIVE_HARD, .sample_size = BLOCK_SIZE };
    warmup(&cfg);

    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 1000.0f, 1.0f);

    for (int k = 0; k < 8; k++)
        overdrive_process(&cfg, in, out);

    TEST_ASSERT_LESS_THAN_FLOAT(1.2f, peak(out, BLOCK_SIZE));
}

void test_hard_clip_output_bounded_negative(void)
{
    overdrive_config_t cfg = { .gain = 20.0f, .driving_function = DRIVE_HARD, .sample_size = BLOCK_SIZE };
    warmup(&cfg);

    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    for (uint32_t i = 0; i < BLOCK_SIZE; i++) in[i] = -5.0f;

    for (int k = 0; k < 16; k++)
        overdrive_process(&cfg, in, out);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, out[BLOCK_SIZE - 1]);
}

void test_hard_clip_output_bounded_positive(void)
{
    overdrive_config_t cfg = { .gain = 20.0f, .driving_function = DRIVE_HARD,
                                .sample_size = BLOCK_SIZE };
    warmup(&cfg);

    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    for (uint32_t i = 0; i < BLOCK_SIZE; i++) in[i] = 5.0f;

    for (int k = 0; k < 16; k++)
        overdrive_process(&cfg, in, out);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, out[BLOCK_SIZE - 1]);
}

void test_soft_clip_lower_peak_than_hard(void)
{
    float in[BLOCK_SIZE], out_hard[BLOCK_SIZE], out_soft[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 1000.0f, 1.0f);

    overdrive_config_t cfg_hard = { .gain = 5.0f, .driving_function = DRIVE_HARD, .sample_size = BLOCK_SIZE };
    overdrive_config_t cfg_soft = { .gain = 5.0f, .driving_function = DRIVE_SOFT, .sample_size = BLOCK_SIZE };
    warmup(&cfg_hard);
    warmup(&cfg_soft);

    for (int k = 0; k < 8; k++) overdrive_process(&cfg_hard, in, out_hard);
    for (int k = 0; k < 8; k++) overdrive_process(&cfg_soft, in, out_soft);

    TEST_ASSERT_LESS_THAN_FLOAT(peak(out_hard, BLOCK_SIZE), peak(out_soft, BLOCK_SIZE));
}

void test_soft_clip_symmetric(void)
{
    overdrive_config_t cfg = { .gain = 2.0f, .driving_function = DRIVE_SOFT, .sample_size = BLOCK_SIZE };
    warmup(&cfg);

    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 1000.0f, 1.0f);

    for (int k = 0; k < 8; k++)
        overdrive_process(&cfg, in, out);

    float max_val = -1e9f, min_val = 1e9f;
    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
    {
        if (out[i] > max_val) max_val = out[i];
        if (out[i] < min_val) min_val = out[i];
    }

    TEST_ASSERT_FLOAT_WITHIN(0.05f, max_val, -min_val);
}

void test_zero_gain_gives_silence(void)
{
    overdrive_config_t cfg = { .gain = 0.0f, .driving_function = DRIVE_SOFT, .sample_size = BLOCK_SIZE };
    warmup(&cfg);

    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 1000.0f, 1.0f);

    for (int k = 0; k < 8; k++)
        overdrive_process(&cfg, in, out);

    TEST_ASSERT_LESS_THAN_FLOAT(1e-4f, rms(out, BLOCK_SIZE));
}

void test_higher_gain_more_distortion(void)
{
    float in[BLOCK_SIZE], out_low[BLOCK_SIZE], out_high[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 1000.0f, 1.0f);

    overdrive_config_t cfg_low = { .gain = 1.0f, .driving_function = DRIVE_HARD, .sample_size = BLOCK_SIZE };
    overdrive_config_t cfg_high = { .gain = 10.0f, .driving_function = DRIVE_HARD, .sample_size = BLOCK_SIZE };
    warmup(&cfg_low);
    warmup(&cfg_high);

    for (int k = 0; k < 8; k++) overdrive_process(&cfg_low,  in, out_low);
    for (int k = 0; k < 8; k++) overdrive_process(&cfg_high, in, out_high);

    TEST_ASSERT_GREATER_THAN_FLOAT(rms(out_low, BLOCK_SIZE), rms(out_high, BLOCK_SIZE));
}

void test_lp_attenuates_high_freq(void)
{
    overdrive_config_t cfg = { .gain = 1.0f, .driving_function = DRIVE_SOFT, .sample_size = BLOCK_SIZE };

    float in[BLOCK_SIZE], out[BLOCK_SIZE];

    warmup(&cfg);
    gen_sine(in, BLOCK_SIZE, 1000.0f, 0.1f);
    for (int k = 0; k < 8; k++) overdrive_process(&cfg, in, out);
    float rms_1k = rms(out, BLOCK_SIZE);

    warmup(&cfg);
    gen_sine(in, BLOCK_SIZE, 20000.0f, 0.1f);
    for (int k = 0; k < 8; k++) overdrive_process(&cfg, in, out);
    float rms_20k = rms(out, BLOCK_SIZE);

    TEST_ASSERT_GREATER_THAN_FLOAT(rms_20k * 3.0f, rms_1k);
}

void test_lp_passes_low_freq(void)
{
    overdrive_config_t cfg = { .gain = 1.0f, .driving_function = DRIVE_SOFT, .sample_size = BLOCK_SIZE };
    warmup(&cfg);

    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 200.0f, 0.1f);
    for (int k = 0; k < 8; k++) overdrive_process(&cfg, in, out);

    TEST_ASSERT_GREATER_THAN_FLOAT(0.06f, rms(out, BLOCK_SIZE));
}

void test_in_place_hard_same_as_separate(void)
{
    overdrive_config_t cfg = { .gain = 3.0f, .driving_function = DRIVE_HARD, .sample_size = BLOCK_SIZE };

    float in_a[BLOCK_SIZE], in_b[BLOCK_SIZE];
    float out_sep[BLOCK_SIZE];

    gen_sine(in_a, BLOCK_SIZE, 1000.0f, 1.0f);
    memcpy(in_b, in_a, sizeof(in_a));

    warmup(&cfg);
    overdrive_process(&cfg, in_a, out_sep);

    warmup(&cfg);
    overdrive_process(&cfg, in_b, in_b);

    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, out_sep[i], in_b[i]);
}

void test_zero_input_gives_zero_output(void)
{
    overdrive_config_t cfg = { .gain = 10.0f, .driving_function = DRIVE_HARD, .sample_size = BLOCK_SIZE };
    warmup(&cfg);

    float in[BLOCK_SIZE] = {0};
    float out[BLOCK_SIZE] = {0};
    overdrive_process(&cfg, in, out);

    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, out[i]);
}

void test_no_nan_or_inf_hard(void)
{
    overdrive_config_t cfg = { .gain = 1000.0f, .driving_function = DRIVE_HARD, .sample_size = BLOCK_SIZE };
    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 19000.0f, 1.0f);

    for (int k = 0; k < 4; k++)
        overdrive_process(&cfg, in, out);

    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
    {
        TEST_ASSERT_FALSE(isnan(out[i]));
        TEST_ASSERT_FALSE(isinf(out[i]));
    }
}

void test_no_nan_or_inf_soft(void)
{
    overdrive_config_t cfg = { .gain = 1000.0f, .driving_function = DRIVE_SOFT, .sample_size = BLOCK_SIZE };
    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 19000.0f, 1.0f);

    for (int k = 0; k < 4; k++)
        overdrive_process(&cfg, in, out);

    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
    {
        TEST_ASSERT_FALSE(isnan(out[i]));
        TEST_ASSERT_FALSE(isinf(out[i]));
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_soft_unity_gain_passes_quiet_signal);

    RUN_TEST(test_hard_clip_limits_peak_to_one);
    RUN_TEST(test_hard_clip_output_bounded_negative);
    RUN_TEST(test_hard_clip_output_bounded_positive);

    RUN_TEST(test_soft_clip_lower_peak_than_hard);
    RUN_TEST(test_soft_clip_symmetric);

    RUN_TEST(test_zero_gain_gives_silence);
    RUN_TEST(test_higher_gain_more_distortion);

    RUN_TEST(test_lp_attenuates_high_freq);
    RUN_TEST(test_lp_passes_low_freq);

    RUN_TEST(test_in_place_hard_same_as_separate);
    RUN_TEST(test_zero_input_gives_zero_output);

    RUN_TEST(test_no_nan_or_inf_hard);
    RUN_TEST(test_no_nan_or_inf_soft);

    return UNITY_END();
}