#ifndef AUDIO_BUFFERS_H_
#define AUDIO_BUFFERS_H_

#include <stdint.h>

#define AUDIO_BUF_LEN 512
#define AUDIO_HALF_BUF_LEN (AUDIO_BUF_LEN / 2)

#define AUDIO_BUF_STEREO_LEN (AUDIO_BUF_LEN * 2)
#define AUDIO_HALF_BUF_STEREO_LEN (AUDIO_BUF_STEREO_LEN / 2)

float* get_f_output_buffer();
float* get_f_input_buffer();
int16_t* get_i16_output_buffer();
uint16_t* get_u16_input_buffer();


void convert_fi16_buf(float* f_buf, int16_t* i_buf, uint16_t n);
void convert_u16f_buf(uint16_t* u_buf, float* f_buf, uint16_t n);
void convert_fi16_stereo(float* f_left, float* f_right, int16_t* i_out, uint16_t n_samples);
void convert_adc_u16f_buf(uint16_t* u_buf, float* f_buf, uint16_t n);



#endif /* AUDIO_BUFFERS_H_ */
