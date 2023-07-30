#include <math.h>
#include <stdlib.h>

#include <libwdsp.h>

#include "ringbuffer.h"
#include "resimpler.h"

#include "gsm/inc/gsm.h"

static bool active = false;

static gsm encoder;
static gsm decoder;

static gsm_frame buf;
static gsm_signal encode_samples[160] = { 0 };
static gsm_signal decode_samples[160] = { 0 };
static int sample_count = 0;

static Resimpler<SAMPLE_RATE, 8000, 16, 64> downsampler;
static Resimpler<8000, SAMPLE_RATE, 16, 64> upsampler;

static constexpr int down_size = downsampler.max_output_size(BLOCK_SIZE);
static constexpr int up_size = upsampler.max_output_size(down_size);

static float in_samples[BLOCK_SIZE];
static float process_samples[down_size];
static float out_samples[up_size];

static Ringbuffer<float, BLOCK_SIZE * 2> out_buf;

void wdsp_process(float **in_buffer, float **out_buffer)
{
	float wet = io_analog_in(POT_1);
	float errors = powf(io_analog_in(POT_2), 4.0f);
	float env = 0.0f;

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		in_samples[i] = in_buffer[0][i] + in_buffer[1][i];
		env = fmax(env, fabsf(in_buffer[0][i] + in_buffer[1][i]));
	}

	int down_count = downsampler.process(in_samples, process_samples, BLOCK_SIZE);

	for (int i = 0; i < down_count; i++)
	{
		encode_samples[sample_count] = process_samples[i] * (float)INT16_MAX;
		process_samples[i] = (float)decode_samples[sample_count] / (float)INT16_MAX;

		sample_count++;
		if (sample_count >= 160)
		{
			sample_count = 0;
			gsm_encode(encoder, encode_samples, buf);

			for (int j = 0; j < 33; j++)
				for (int k = 0; k < 8; k++)
					if ((float)random() / (float)RAND_MAX < errors * env)
						buf[j] ^= 0b1 << k;

			gsm_decode(decoder, buf, decode_samples);
		}
	}

	int up_count = upsampler.process(process_samples, out_samples, down_count);

	out_buf.put(out_samples, up_count);

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
	#if CONFIG_EFFECT_MIXDOWN == true
		out_buffer[0][i] = (in_buffer[0][i] + in_buffer[1][i]) * (1.0f - wet);
		out_buffer[1][i] = (in_buffer[0][i] + in_buffer[1][i]) * (1.0f - wet);
	#else
		out_buffer[0][i] = in_buffer[0][i] * (1.0f - wet);
		out_buffer[1][i] = in_buffer[1][i] * (1.0f - wet);
	#endif
	}

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		float osamp = out_buf.get();
		out_buffer[0][i] += osamp * wet;
		out_buffer[1][i] += osamp * wet;
	}
}

void wdsp_idle(unsigned long int ticks)
{
	static bool button_state = false;

	if (io_digital_in(BUTTON_1) != button_state)
	{
		button_state = io_digital_in(BUTTON_1);

		if (button_state == true)
		{
			if (!active)
			{
				active = true;
				io_digital_out(BYPASS_L, true);
				io_digital_out(BYPASS_R, true);
			}
			else
			{
				active = false;
				io_digital_out(BYPASS_L, false);
				io_digital_out(BYPASS_R, false);
			}
		}
	}

	io_digital_out(LED_1, active);
}

void wdsp_init(void)
{
	encoder = gsm_create();
	decoder = gsm_create();

	io_digital_out(BYPASS_L, false);
	io_digital_out(BYPASS_R, false);
}