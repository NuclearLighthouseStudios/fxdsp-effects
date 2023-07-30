#include <math.h>
#include <string.h>
#include <limits.h>

#include <libwdsp.h>

#include "ringbuffer.h"
#include "resimpler.h"

#include "gsm/inc/gsm.h"

enum mode
{
	BYPASS,
	RECORD,
	OVERDUB,
	PLAY
};

static enum mode mode;

static gsm __CCMRAM encoder;
static gsm __CCMRAM decoder;

static gsm_frame __CCMRAM buf;
static gsm_signal __CCMRAM encode_samples[160] = { 0 };
static gsm_signal __CCMRAM decode_samples[160] = { 0 };
static int sample_count = 0;

static Resimpler<SAMPLE_RATE, 8000, 16, 64> __CCMRAM downsampler;
static Resimpler<8000, SAMPLE_RATE, 16, 64> __CCMRAM upsampler;

static constexpr int down_size = downsampler.max_output_size(BLOCK_SIZE);
static constexpr int up_size = upsampler.max_output_size(down_size);

static float __CCMRAM in_samples[BLOCK_SIZE];
static float __CCMRAM process_samples[down_size];
static float __CCMRAM out_samples[up_size];

static Ringbuffer<float, BLOCK_SIZE * 2> __CCMRAM out_buf;

#define MAX_LOOP_LENGTH 3600
static gsm_frame loop_buffer[MAX_LOOP_LENGTH];
static int loop_pos = 0;
static int loop_length = 0;

void wdsp_process(float **in_buffer, float **out_buffer)
{
	float dry_vol = io_analog_in(POT_1);
	float loop_vol = io_analog_in(POT_2);

	for (int i = 0; i < BLOCK_SIZE; i++)
		in_samples[i] = in_buffer[0][i] + in_buffer[1][i];

	int down_count = downsampler.process(in_samples, process_samples, BLOCK_SIZE);

	for (int i = 0; i < down_count; i++)
	{
		encode_samples[sample_count] = process_samples[i] * (float)INT16_MAX;
		process_samples[i] = (float)decode_samples[sample_count] / (float)INT16_MAX;

		sample_count++;
		if (sample_count >= 160)
		{
			sample_count = 0;

			switch (mode)
			{
				case PLAY:
					memcpy(buf, loop_buffer[loop_pos++], sizeof(buf));
					if (loop_pos >= loop_length)
						loop_pos = 0;

					gsm_decode(decoder, buf, decode_samples);
					break;

				case OVERDUB:
					memcpy(buf, loop_buffer[loop_pos], sizeof(buf));

					gsm_decode(decoder, buf, decode_samples);

					for (int i = 0; i < 160; i++)
					{
						int newsample = encode_samples[i] + decode_samples[i];
						if (newsample > SHRT_MAX) newsample = SHRT_MAX;
						if (newsample < SHRT_MIN) newsample = SHRT_MIN;
						encode_samples[i] = newsample;
					}

					gsm_encode(encoder, encode_samples, buf);

					memcpy(loop_buffer[loop_pos++], buf, sizeof(buf));
					if (loop_pos >= loop_length)
						loop_pos = 0;

					break;

				case RECORD:
					gsm_encode(encoder, encode_samples, buf);

					memcpy(loop_buffer[loop_pos++], buf, sizeof(buf));
					loop_length = loop_pos;
					if (loop_pos >= MAX_LOOP_LENGTH)
					{
						loop_pos = 0;
						mode = PLAY;
					}

				default:
					for (int i = 0; i < 160; i++)
						decode_samples[i] = 0;
					break;
			}
		}
	}

	int up_count = upsampler.process(process_samples, out_samples, down_count);

	out_buf.put(out_samples, up_count);

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
	#if CONFIG_EFFECT_MIXDOWN == true
		out_buffer[0][i] = (in_buffer[0][i] + in_buffer[1][i]) * dry_vol;
		out_buffer[1][i] = (in_buffer[0][i] + in_buffer[1][i]) * dry_vol;
	#else
		out_buffer[0][i] = in_buffer[0][i] * dry_vol;
		out_buffer[1][i] = in_buffer[1][i] * dry_vol;
	#endif
	}

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		float osamp = out_buf.get();
		out_buffer[0][i] += osamp * loop_vol;
		out_buffer[1][i] += osamp * loop_vol;
	}
}

void wdsp_idle(unsigned long int ticks)
{
	static bool button_state = false;
	static unsigned long int button_start;

	if (io_digital_in(BUTTON_1) != button_state)
	{
		button_state = io_digital_in(BUTTON_1);

		if (button_state == true)
		{
			button_start = ticks;

			switch (mode)
			{
				case BYPASS:
					loop_length = 0;
					loop_pos = 0;
					mode = RECORD;
					break;

				case RECORD:
					loop_pos = 0;
					mode = PLAY;
					break;

				case PLAY:
					mode = OVERDUB;
					break;

				case OVERDUB:
					mode = PLAY;
					break;

				default:
					break;
			}
		}
	}

	if ((button_state == true) && (mode != BYPASS))
	{
		if (ticks - button_start > 1000)
		{
			loop_length = 0;
			loop_pos = 0;
			mode = BYPASS;
		}
		else
			io_digital_out(LED_1, ticks & 0x40);
	}
	else
	{
		switch (mode)
		{
			case RECORD:
			case OVERDUB:
				io_digital_out(LED_1, ticks & 0x80);
				break;

			case PLAY:
				io_digital_out(LED_1, loop_pos > 5);
				break;

			default:
				io_digital_out(LED_1, false);
		}
	}

	io_digital_out(BYPASS_L, mode != BYPASS);
	io_digital_out(BYPASS_R, mode != BYPASS);
}

void wdsp_init(void)
{
	encoder = gsm_create();
	decoder = gsm_create();

	io_digital_out(BYPASS_L, false);
	io_digital_out(BYPASS_R, false);
}