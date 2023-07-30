#include <math.h>
#include <libwdsp.h>

#include "g711.h"
#include "filters.h"

volatile extern unsigned long int sys_ticks;

bool active = false;

#define BUFSIZE 96000

uint8_t buffer[BUFSIZE];

int write_pos = 0;

float delay;

struct filter lp;
struct filter hp;

static inline int wrap(int index)
{
	index %= BUFSIZE;
	return (index < 0 ? index += BUFSIZE : index);
}

/* greets to wrl */
static inline float shape_tanh(const float x)
{
	/* greets to aleksey vaneev */
	const float ax = fabsf(x);
	const float x2 = x * x;

	return (x * (2.45550750702956f + 2.45550750702956f * ax + (0.893229853513558f + 0.821226666969744f * ax) * x2) /
		(2.44506634652299f + (2.44506634652299f + x2) * fabsf(x + 0.814642734961073f * x * ax)));
}

float delay_get(float delay)
{
	int index = (float)write_pos - delay;
	float frac = delay - floorf(delay);

	float s1 = (float)ulaw2linear(buffer[wrap(index)]) / (float)INT16_MAX;
	float s2 = (float)ulaw2linear(buffer[wrap(index + 1)]) / (float)INT16_MAX;

	return s1 * (1.0f - frac) + s2 * frac;
}

void wdsp_process(float **in_buffer, float **out_buffer)
{
	static float feedback = 0.0f;
	static float f_active = 0.0f;
	static float mod_phase = 0;

	float new_delay = powf(io_analog_in(POT_1), 2.0f) * (float)(BUFSIZE - 32) + 16.0f;

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		float mod = sinf(mod_phase);
		mod_phase += (0.875f * 2.0f * M_PI) / (float)SAMPLE_RATE;
		if (mod_phase > 2.0f * M_PI)
			mod_phase -= 2.0f * M_PI;

		if (fabsf(new_delay - delay) > 240.0f)
			delay = delay * 0.999f + new_delay * 0.001f;

		float out = delay_get(delay + mod * 8.0f);

		out = filter_lp_iir(out, &lp);
		out = filter_hp_iir(out, &hp);

		if (io_digital_in(BUTTON_1))
			feedback = feedback * 0.999f + 0.001f;
		else
			feedback = feedback * 0.99f + io_analog_in(POT_2) * 0.01f;

		out *= feedback * 1.1f;

		f_active = f_active * 0.99f + (float)active * 0.01f;

		float in = (in_buffer[0][i] + in_buffer[1][i]) * f_active + out;
		in = shape_tanh(in);

		int16_t in_short = in * (float)INT16_MAX;

		buffer[write_pos] = linear2ulaw(in_short);
		write_pos = (write_pos + 1) % BUFSIZE;

	#if CONFIG_EFFECT_MIXDOWN == true
		out_buffer[0][i] = in_buffer[0][i] + in_buffer[1][i] + out;
		out_buffer[1][i] = in_buffer[0][i] + in_buffer[1][i] + out;
	#else
		out_buffer[0][i] = in_buffer[0][i] + out;
		out_buffer[1][i] = in_buffer[1][i] + out;
	#endif
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
			button_start = sys_ticks;
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
				if (active && (sys_ticks - button_start < 500))
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
	delay = powf(io_analog_in(POT_1), 2.0f) * (float)(BUFSIZE - 32) + 16.0f;

	for (int i = 0; i < BUFSIZE; i++)
		buffer[i] = linear2ulaw(0);

	filter_init(&lp, 0.25f, 0.25f);
	filter_init(&hp, 0.025f, 0.1f);

#if CONFIG_EFFECT_BYPASS == false
	io_digital_out(BYPASS_L, true);
	io_digital_out(BYPASS_R, true);
#endif
}