#include "audio_buffers.h"


static uint16_t u16_input_buffer[AUDIO_BUF_LEN] = {0};
static float f_input_buffer[AUDIO_BUF_LEN] = {0.0f};
static float f_output_buffer[AUDIO_BUF_LEN] = {0.0f};
static int16_t i16_output_buffer[AUDIO_BUF_STEREO_LEN] = {0};


float* get_f_output_buffer()
{
	return f_output_buffer;
}

float* get_f_input_buffer()
{
	return f_input_buffer;
}

int16_t* get_i16_output_buffer()
{
	return i16_output_buffer;
}

uint16_t* get_u16_input_buffer()
{
	return u16_input_buffer;
}


void convert_fi16_buf(float* f_buf, int16_t* i_buf, uint16_t n)
{
	for (uint16_t i = 0; i < n; i++)
	{
		float sample = f_buf[i];
		if (sample > 1.0f) sample = 1.0f;
		if (sample < -1.0f) sample = -1.0f;

		i_buf[i] = (int16_t)(sample * 32767.0f);
	}
}

void convert_u16f_buf(uint16_t* u_buf, float* f_buf, uint16_t n)
{
    const float inv_scale = 1.0f / 32767.5f;
    for (uint16_t i = 0; i < n; i++)
    {
        f_buf[i] = ((float)u_buf[i] - 32767.5f) * inv_scale;
    }
}

void convert_fi16_stereo(float* f_left, float* f_right, int16_t* i_out, uint16_t n_samples)
{
    for (uint16_t i = 0; i < n_samples; i++)
    {
        float sample_l = f_left[i];
        float sample_r = f_right[i];

        if (sample_l > 1.0f) sample_l = 1.0f;
        if (sample_l < -1.0f) sample_l = -1.0f;
        if (sample_r > 1.0f) sample_r = 1.0f;
        if (sample_r < -1.0f) sample_r = -1.0f;

        int16_t i16_sample_l = (int16_t)(sample_l * 32760.0f);
        int16_t i16_sample_r = (int16_t)(sample_r * 32760.0f);

        i_out[2*i] = i16_sample_l;
        i_out[2*i + 1] = i16_sample_r;
    }
}

void convert_adc_u16f_buf(uint16_t* u_buf, float* f_buf, uint16_t n)
{
	const float inv_scale = 1.0f / 2048.0f;
    for (uint16_t i = 0; i < n; i++)
    {
        f_buf[i] = ((float)u_buf[i] - 2048.0f) * inv_scale;
    }
}
