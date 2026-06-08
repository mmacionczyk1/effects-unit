#include "./unity/unity.h"
#include "../effects_c/equaliser.h"
#include <math.h>
#include <string.h>

#define SAMPLE_RATE 48000u
#define BLOCK_SIZE 256u
#define FLOAT_TOL 1e-5f
#define DB_TOL 0.1f

#ifndef PI
#define PI 3.1415926535f
#endif

static equaliser_t eq;

static float signal_power(const float* buf, uint32_t n)
{
    float sum = 0.0f;
    for (uint32_t i = 0; i < n; i++) sum += buf[i] * buf[i];
    return sum / (float)n;
}

static void gen_sine(float* buf, uint32_t n, float freq, uint32_t sr, float amp)
{
    for (uint32_t i = 0; i < n; i++)
        buf[i] = amp * sinf(2.0f * PI * freq * (float)i / (float)sr);
}

static float measure_rms_steady(equaliser_t* e, float freq, float amp)
{
    float in[BLOCK_SIZE];
    float out[BLOCK_SIZE];

    for (int warm = 0; warm < 8; warm++)
    {
        gen_sine(in, BLOCK_SIZE, freq, e->sample_rate, amp);
        eq_process(e, in, out);
    }
    gen_sine(in, BLOCK_SIZE, freq, e->sample_rate, amp);
    eq_process(e, in, out);

    return sqrtf(signal_power(out, BLOCK_SIZE));
}

void setUp(void)
{
    memset(&eq, 0, sizeof(equaliser_t));
    eq.sample_rate = SAMPLE_RATE;
    eq.sample_size = BLOCK_SIZE;
    eq_reset(&eq);

    for (uint32_t i = 0; i < EQ_MAX_BANDS; i++)
        eq_disable_band(&eq, i);
}

void tearDown(void) {}

void test_eq_reset_sets_unity_gain(void)
{
    eq_reset(&eq);
    for (uint32_t i = 0; i < EQ_MAX_BANDS; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 1.0f, eq.biquads[i].b0);
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, eq.biquads[i].b1);
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, eq.biquads[i].b2);
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, eq.biquads[i].a1);
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, eq.biquads[i].a2);
    }
}

void test_eq_reset_clears_state(void)
{
    eq.biquads[0].s1 = 99.0f;
    eq.biquads[0].s2 = -3.14f;
    eq_reset(&eq);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, eq.biquads[0].s1);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, eq.biquads[0].s2);
}


void test_enable_disable_band(void)
{
    eq_enable_band(&eq, 0);
    TEST_ASSERT_TRUE(eq.bands[0].enabled);

    eq_disable_band(&eq, 0);
    TEST_ASSERT_FALSE(eq.bands[0].enabled);
}

void test_process_no_active_bands_copies_input(void)
{
    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 1000.0f, SAMPLE_RATE, 1.0f);
    memset(out, 0, sizeof(out));

    eq_process(&eq, in, out);

    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, in[i], out[i]);
}

void test_process_in_place_no_active_bands(void)
{
    float buf[BLOCK_SIZE];
    gen_sine(buf, BLOCK_SIZE, 440.0f, SAMPLE_RATE, 0.5f);

    float expected[BLOCK_SIZE];
    memcpy(expected, buf, sizeof(buf));

    eq_process(&eq, buf, buf);

    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, expected[i], buf[i]);
}

void test_set_band_out_of_range_does_nothing(void)
{
    eq_set_band(&eq, EQ_MAX_BANDS, EQ_FILTER_LOW_PASS, 1000.0f, 0.707f, 0.0f);
    eq_set_band(&eq, EQ_MAX_BANDS + 5, EQ_FILTER_PEAKING,  500.0f,  1.0f,   6.0f);
    TEST_PASS();
}

void test_set_band_peaking_zero_gain_unity(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_PEAKING, 1000.0f, 1.0f, 0.0f);
    biquad_t* bq = &eq.biquads[0];
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, bq->b0);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, bq->a1, bq->b1);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, bq->a2, bq->b2);
}

void test_lowpass_passes_low_freq(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_LOW_PASS, 4000.0f, 0.707f, 0.0f);
    eq_enable_band(&eq, 0);

    float rms = measure_rms_steady(&eq, 200.0f, 1.0f);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.65f, rms);
}

void test_lowpass_attenuates_high_freq(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_LOW_PASS, 500.0f, 0.707f, 0.0f);
    eq_enable_band(&eq, 0);

    float rms = measure_rms_steady(&eq, 16000.0f, 1.0f);
    TEST_ASSERT_LESS_THAN_FLOAT(0.05f, rms);
}

void test_highpass_passes_high_freq(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_HIGH_PASS, 500.0f, 0.707f, 0.0f);
    eq_enable_band(&eq, 0);

    float rms = measure_rms_steady(&eq, 16000.0f, 1.0f);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.65f, rms);
}

void test_highpass_attenuates_low_freq(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_HIGH_PASS, 4000.0f, 0.707f, 0.0f);
    eq_enable_band(&eq, 0);

    float rms = measure_rms_steady(&eq, 100.0f, 1.0f);
    TEST_ASSERT_LESS_THAN_FLOAT(0.05f, rms);
}


void test_peaking_boosts_center_freq(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_PEAKING, 1000.0f, 2.0f, 12.0f);
    eq_enable_band(&eq, 0);

    float rms = measure_rms_steady(&eq, 1000.0f, 1.0f);
    TEST_ASSERT_GREATER_THAN_FLOAT(1.5f, rms);
}

void test_peaking_cuts_center_freq(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_PEAKING, 1000.0f, 2.0f, -12.0f);
    eq_enable_band(&eq, 0);

    float rms = measure_rms_steady(&eq, 1000.0f, 1.0f);
    TEST_ASSERT_LESS_THAN_FLOAT(0.3f, rms);
}

void test_peaking_does_not_affect_far_frequencies(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_PEAKING, 1000.0f, 8.0f, 12.0f);
    eq_enable_band(&eq, 0);

    float rms = measure_rms_steady(&eq, 100.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.15f, 0.707f, rms);
}


void test_low_shelf_boosts_below_shelf(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_LOW_SHELF, 1000.0f, 0.707f, 6.0f);
    eq_enable_band(&eq, 0);

    float rms_low  = measure_rms_steady(&eq, 100.0f, 1.0f);
    float rms_high = measure_rms_steady(&eq, 10000.0f, 1.0f);

    TEST_ASSERT_GREATER_THAN_FLOAT(rms_high, rms_low);
}

void test_low_shelf_cuts_below_shelf(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_LOW_SHELF, 1000.0f, 0.707f, -6.0f);
    eq_enable_band(&eq, 0);

    float rms_low = measure_rms_steady(&eq, 100.0f, 1.0f);
    float rms_high = measure_rms_steady(&eq, 10000.0f, 1.0f);

    TEST_ASSERT_LESS_THAN_FLOAT(rms_high, rms_low);
}


void test_high_shelf_boosts_above_shelf(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_HIGH_SHELF, 4000.0f, 0.707f, 6.0f);
    eq_enable_band(&eq, 0);

    float rms_low = measure_rms_steady(&eq, 200.0f, 1.0f);
    float rms_high = measure_rms_steady(&eq, 16000.0f, 1.0f);

    TEST_ASSERT_GREATER_THAN_FLOAT(rms_low, rms_high);
}

void test_high_shelf_cuts_above_shelf(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_HIGH_SHELF, 4000.0f, 0.707f, -6.0f);
    eq_enable_band(&eq, 0);

    float rms_low  = measure_rms_steady(&eq, 200.0f, 1.0f);
    float rms_high = measure_rms_steady(&eq, 16000.0f, 1.0f);

    TEST_ASSERT_LESS_THAN_FLOAT(rms_low, rms_high);
}

void test_two_bands_chain_lp_hp(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_HIGH_PASS, 200.0f,  0.707f, 0.0f);
    eq_set_band(&eq, 1, EQ_FILTER_LOW_PASS, 8000.0f, 0.707f, 0.0f);
    eq_enable_band(&eq, 0);
    eq_enable_band(&eq, 1);

    float rms_pass = measure_rms_steady(&eq, 1000.0f, 1.0f);
    float rms_stop = measure_rms_steady(&eq, 50.0f, 1.0f);

    TEST_ASSERT_GREATER_THAN_FLOAT(0.5f, rms_pass);
    TEST_ASSERT_LESS_THAN_FLOAT(rms_pass * 0.5f, rms_stop);
}

void test_disabled_band_has_no_effect(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_LOW_PASS, 200.0f, 0.707f, 0.0f);

    float rms = measure_rms_steady(&eq, 16000.0f, 1.0f);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.5f, rms);
}

void test_dc_signal_lowpass(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_LOW_PASS, 1000.0f, 0.707f, 0.0f);
    eq_enable_band(&eq, 0);

    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    for (int k = 0; k < 64; k++)
    {
        for (uint32_t i = 0; i < BLOCK_SIZE; i++) in[i] = 1.0f;
        eq_process(&eq, in, out);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, out[BLOCK_SIZE - 1]);
}

void test_zero_input_gives_zero_output(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_PEAKING, 1000.0f, 1.0f, 6.0f);
    eq_enable_band(&eq, 0);

    float in[BLOCK_SIZE]  = {0};
    float out[BLOCK_SIZE] = {0};

    eq_process(&eq, in, out);

    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, out[i]);
}

void test_no_nan_or_inf_in_output(void)
{
    eq_set_band(&eq, 0, EQ_FILTER_PEAKING, 20000.0f, 0.1f, 24.0f);
    eq_enable_band(&eq, 0);

    float in[BLOCK_SIZE], out[BLOCK_SIZE];
    gen_sine(in, BLOCK_SIZE, 19000.0f, SAMPLE_RATE, 1.0f);

    for (int k = 0; k < 4; k++)
        eq_process(&eq, in, out);

    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
    {
        TEST_ASSERT_FALSE(isnan(out[i]));
        TEST_ASSERT_FALSE(isinf(out[i]));
    }
}

int main(void)
{
    UNITY_BEGIN();

    /* eq_reset */
    RUN_TEST(test_eq_reset_sets_unity_gain);
    RUN_TEST(test_eq_reset_clears_state);

    /* enable/disable */
    RUN_TEST(test_enable_disable_band);

    /* pass-through */
    RUN_TEST(test_process_no_active_bands_copies_input);
    RUN_TEST(test_process_in_place_no_active_bands);

    /* set_band */
    RUN_TEST(test_set_band_out_of_range_does_nothing);
    RUN_TEST(test_set_band_peaking_zero_gain_unity);

    /* low-pass */
    RUN_TEST(test_lowpass_passes_low_freq);
    RUN_TEST(test_lowpass_attenuates_high_freq);

    /* high-pass */
    RUN_TEST(test_highpass_passes_high_freq);
    RUN_TEST(test_highpass_attenuates_low_freq);

    /* peaking */
    RUN_TEST(test_peaking_boosts_center_freq);
    RUN_TEST(test_peaking_cuts_center_freq);
    RUN_TEST(test_peaking_does_not_affect_far_frequencies);

    /* low-shelf */
    RUN_TEST(test_low_shelf_boosts_below_shelf);
    RUN_TEST(test_low_shelf_cuts_below_shelf);

    /* high-shelf */
    RUN_TEST(test_high_shelf_boosts_above_shelf);
    RUN_TEST(test_high_shelf_cuts_above_shelf);

    /* wielopasmowe */
    RUN_TEST(test_two_bands_chain_lp_hp);
    RUN_TEST(test_disabled_band_has_no_effect);

    /* stabilność / brzegowe */
    RUN_TEST(test_dc_signal_lowpass);
    RUN_TEST(test_zero_input_gives_zero_output);
    RUN_TEST(test_no_nan_or_inf_in_output);

    return UNITY_END();
}