#include "./unity/unity.h"
#include "../effects_c/bitcrusher.h"
#include <math.h>
#include <string.h>

static bitcrusher_config_t make_cfg(uint32_t bit_depth, uint32_t dsf, float mix, float dither)
{
    bitcrusher_config_t c = {0};
    c.bit_depth = bit_depth;
    c.downsample_factor = dsf;
    c.mix_parameter = mix;
    c.dither_level = dither;
    c.rng_state = 0xDEADBEEF;
    c.counter = 0;
    c.tmp_sample = 0.0f;
    return c;
}

void setUp(void){}
void tearDown(void){}

void test_mix_zero_is_passthrough(void)
{
    float in[8] = { 0.1f, -0.2f, 0.5f, -0.9f, 0.0f, 1.0f, -1.0f, 0.3f };
    float out[8] = { 0 };

    bitcrusher_config_t cfg = make_cfg(8, 1, 0.0f, 1.0f);
    cfg.sample_size = 8;
    process_bitcrusher(&cfg, in, out);

    for (int i = 0; i < 8; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, in[i], out[i]);
}

void test_silence_no_dither_outputs_silence(void)
{
    float in[8] = { 0 };
    float out[8] = { 0 };

    bitcrusher_config_t cfg = make_cfg(8, 1, 1.0f, 0.0f);
    cfg.sample_size = 8;
    process_bitcrusher(&cfg, in, out);

    for (int i = 0; i < 8; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, out[i]);
}

void test_downsampling_holds_sample_for_dsf_frames(void)
{
    const uint32_t DSF = 4;
    float in[8], out[8];

    for (int i = 0; i < 8; i++) in[i] = 0.6f;

    bitcrusher_config_t cfg = make_cfg(12, DSF, 1.0f, 0.0f);
    cfg.sample_size = 8;
    process_bitcrusher(&cfg, in, out);

    for (uint32_t i = 1; i < DSF; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, out[0], out[i]);
    for (uint32_t i = DSF + 1; i < 2 * DSF; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, out[DSF], out[i]);
}

void test_bit_depth_2_outputs_are_discrete(void)
{
    const float valid[3] = { -1.0f, 0.0f, 1.0f };
    float in[8] = { 0.9f, 0.4f, 0.1f, -0.1f, -0.4f, -0.9f, 0.0f, 0.6f };
    float out[8] = { 0 };

    bitcrusher_config_t cfg = make_cfg(2, 1, 1.0f, 0.0f);
    cfg.sample_size = 8;
    process_bitcrusher(&cfg, in, out);

    for (int i = 0; i < 8; i++) 
    {
        int found = 0;
        for (int j = 0; j < 3; j++)
            if (fabsf(out[i] - valid[j]) < 1e-5f) { found = 1; break; }
        TEST_ASSERT_TRUE_MESSAGE(found, "output nie należy do zbioru {-1, 0, 1}");
    }
}

void test_mix_full_clamps_output_to_unity(void)
{
    float in[4] = { 5.0f, -5.0f, 100.0f, -100.0f };
    float out[4] = { 0 };

    bitcrusher_config_t cfg = make_cfg(8, 1, 1.0f, 0.0f);
    cfg.sample_size = 4;
    process_bitcrusher(&cfg, in, out);

    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_LESS_OR_EQUAL_FLOAT( 1.0f + 1e-5f, out[i]);
        TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(-1.0f - 1e-5f, out[i]);
    }
}

void test_state_preserved_across_calls(void)
{
    const uint32_t DSF = 4;
    float in[8];
    float out_single[8] = { 0 };
    float out_split[8]  = { 0 };

    for (int i = 0; i < 8; i++) in[i] = 0.5f;

    bitcrusher_config_t cfg1 = make_cfg(12, DSF, 1.0f, 0.0f);
    cfg1.sample_size = 8;
    process_bitcrusher(&cfg1, in, out_single);

    bitcrusher_config_t cfg2 = make_cfg(12, DSF, 1.0f, 0.0f);
    cfg2.sample_size = 4;
    process_bitcrusher(&cfg2, in,     out_split);
    process_bitcrusher(&cfg2, in + 4, out_split + 4);

    for (int i = 0; i < 8; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, out_single[i], out_split[i]);
}

void test_high_bit_depth_is_near_transparent(void)
{
    float in[8] = { 0.1f, -0.2f, 0.5f, -0.9f, 0.0f, 0.7f, -0.6f, 0.3f };
    float out[8] = { 0 };

    bitcrusher_config_t cfg = make_cfg(16, 1, 1.0f, 0.0f);
    cfg.sample_size = 8;
    process_bitcrusher(&cfg, in, out);

    const float tolerance = 1.0f / 32767.0f;
    for (int i = 0; i < 8; i++)
        TEST_ASSERT_FLOAT_WITHIN(tolerance, in[i], out[i]);
}

void test_dsf_one_quantizes_every_sample(void)
{
    float in[4] = { 0.1f, 0.4f, 0.7f, 0.9f };
    float out_dsf1[4] = { 0 };
    float out_dsf4[4] = { 0 };

    bitcrusher_config_t cfg1 = make_cfg(4, 1, 1.0f, 0.0f);
    cfg1.sample_size = 4;
    process_bitcrusher(&cfg1, in, out_dsf1);

    bitcrusher_config_t cfg4 = make_cfg(4, 4, 1.0f, 0.0f);
    cfg4.sample_size = 4;
    process_bitcrusher(&cfg4, in, out_dsf4);

    for (int i = 1; i < 4; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, out_dsf4[0], out_dsf4[i]);

    int any_different = 0;
    for (int i = 1; i < 4; i++)
        if (fabsf(out_dsf1[i] - out_dsf1[0]) > 1e-5f) { any_different = 1; break; }
    TEST_ASSERT_TRUE_MESSAGE(any_different, "DSF=1 nie kwantyzuje każdej próbki");
}

void test_initial_counter_offset_respected(void)
{
    float in[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
    float out[4] = { 0 };

    bitcrusher_config_t cfg = make_cfg(12, 4, 1.0f, 0.0f);
    cfg.counter = 2;
    cfg.tmp_sample = 0.25f;
    cfg.sample_size = 4;
    process_bitcrusher(&cfg, in, out);

    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.25f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.25f, out[1]);

    const float expected = roundf(0.5f * 2047.0f) / 2047.0f;
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, expected, out[2]);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, expected, out[3]);
}

void test_mix_half_blends_wet_dry(void)
{
    float in[1]  = { 0.5f };
    float out[1] = { 0.0f };

    bitcrusher_config_t cfg = make_cfg(2, 1, 0.5f, 0.0f);
    cfg.sample_size = 1;
    process_bitcrusher(&cfg, in, out);

    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.75f, out[0]);
}


int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mix_zero_is_passthrough);
    RUN_TEST(test_silence_no_dither_outputs_silence);
    RUN_TEST(test_downsampling_holds_sample_for_dsf_frames);
    RUN_TEST(test_bit_depth_2_outputs_are_discrete);
    RUN_TEST(test_mix_full_clamps_output_to_unity);
    RUN_TEST(test_state_preserved_across_calls);
    RUN_TEST(test_high_bit_depth_is_near_transparent);
    RUN_TEST(test_dsf_one_quantizes_every_sample);
    RUN_TEST(test_initial_counter_offset_respected);
    RUN_TEST(test_mix_half_blends_wet_dry);
    return UNITY_END();
}