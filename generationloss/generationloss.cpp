#include <math.h>
#include <string.h>
#include <limits.h>

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

#define DELAY_MAX 250
static gsm_frame delay_buffer[DELAY_MAX];
static int delay_pos = 0;

/* greets to wrl */
static inline float shape_tanh(const float x)
{
	/* greets to aleksey vaneev */
	const float ax = fabsf(x);
	const float x2 = x * x;

	return (x * (2.45550750702956f + 2.45550750702956f * ax + (0.893229853513558f + 0.821226666969744f * ax) * x2) /
		(2.44506634652299f + (2.44506634652299f + x2) * fabsf(x + 0.814642734961073f * x * ax)));
}

void wdsp_process(float **in_buffer, float **out_buffer)
{
	static float feedback = 0;
	static float delay = 0;
	static float f_active = 0;

	float new_delay = powf(io_analog_in(POT_1), 2.0f);

	if (fabs(delay - new_delay) > 1.25f / (float)DELAY_MAX)
		delay = new_delay;

	for (int i = 0; i < BLOCK_SIZE; i++)
		in_samples[i] = in_buffer[0][i] + in_buffer[1][i];

	int down_count = downsampler.process(in_samples, process_samples, BLOCK_SIZE);

	for (int i = 0; i < down_count; i++)
	{
		if (io_digital_in(BUTTON_1))
			feedback = feedback * 0.99f + 0.01f;
		else
			feedback = feedback * 0.9f + io_analog_in(POT_2) * 0.1f;

		f_active = f_active * 0.99f + (float)active * 0.01f;

		float out_sample = (float)decode_samples[sample_count] / (float)INT16_MAX;
		float in_sample = process_samples[i] * f_active + out_sample * feedback;
		in_sample = shape_tanh(in_sample);

		encode_samples[sample_count] = in_sample * (float)INT16_MAX;
		process_samples[i] = out_sample * feedback;

		sample_count++;
		if (sample_count >= 160)
		{
			sample_count = 0;

			int read_pos = delay_pos - (1 + delay * (DELAY_MAX - 2));
			if (read_pos < 0)
				read_pos += DELAY_MAX;

			memcpy(buf, delay_buffer[read_pos], sizeof(buf));
			gsm_decode(decoder, buf, decode_samples);

			gsm_encode(encoder, encode_samples, buf);
			memcpy(delay_buffer[delay_pos++], buf, sizeof(buf));
			if (delay_pos >= DELAY_MAX)
				delay_pos = 0;
		}
	}

	int up_count = upsampler.process(process_samples, out_samples, down_count);

	out_buf.put(out_samples, up_count);

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
	#if CONFIG_EFFECT_MIXDOWN == true
		out_buffer[0][i] = (in_buffer[0][i] + in_buffer[1][i]);
		out_buffer[1][i] = (in_buffer[0][i] + in_buffer[1][i]);
	#else
		out_buffer[0][i] = in_buffer[0][i];
		out_buffer[1][i] = in_buffer[1][i];
	#endif
	}

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		float osamp = out_buf.get();
		out_buffer[0][i] += osamp;
		out_buffer[1][i] += osamp;
	}
}

void wdsp_idle(unsigned long int ticks)
{
	static unsigned long int button_start;
	static bool button_state = false;
	static bool just_activated;

	if (io_digital_in(BUTTON_1) != button_state)
	{
		button_state = io_digital_in(BUTTON_1);

		if (button_state == true)
		{
			button_start = ticks;
			if (!active)
			{
				active = true;
				just_activated = true;
			#if CONFIG_EFFECT_BYPASS == true
				io_digital_out(BYPASS_L, true);
				io_digital_out(BYPASS_R, true);
			#endif
			}
		}
		else
		{
			if (just_activated)
				just_activated = false;
			else
				if (active && (ticks - button_start < 500))
				{
					active = false;
				#if CONFIG_EFFECT_BYPASS == true
					io_digital_out(BYPASS_L, false);
					io_digital_out(BYPASS_R, false);
				#endif
				}
		}
	}

	io_digital_out(LED_1, active);
}

void wdsp_init(void)
{
	encoder = gsm_create();
	decoder = gsm_create();

#if CONFIG_EFFECT_BYPASS == false
	io_digital_out(BYPASS_L, true);
	io_digital_out(BYPASS_R, true);
#endif
}