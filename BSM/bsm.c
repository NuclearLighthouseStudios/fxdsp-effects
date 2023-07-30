#include <math.h>

#include <libwdsp.h>

static bool active = false;

#define BUFFER_SIZE 16
#define BUFFER_MASK 15

float env = 0.0f;
float buffer[BUFFER_SIZE] = { 0 };
int buffer_pos = 0;

/*
 * Code based on "Barry's Satan Maximiser" by Steve Harris,
 * the only good digital distortion in existence.
 * Licensed under GPL.
 * http://plugin.org.uk/swh-plugins/satanMaximiser
*/

#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

void wdsp_process(float **in_buffer, float **out_buffer)
{
	float knee_point = io_analog_in(POT_1) * -90.0f;
	float env_time = io_analog_in(POT_2) * 28.0f + 2.0f;

	float knee = DB_CO(knee_point);
	int delay = roundf(env_time * 0.5f);
	float env_tr = 1.0f / env_time;

	for (int pos = 0; pos < BLOCK_SIZE; pos++)
	{
		float input = (in_buffer[0][pos] + in_buffer[0][pos]) * 5.0f;

		if (fabs(input) > env)
			env = fabs(input);
		else
			env = fabs(input) * env_tr + env * (1.0f - env_tr);

		float env_sc;

		if (env <= knee)
			env_sc = 1.0f / knee;
		else
			env_sc = 1.0f / env;

		buffer[buffer_pos] = input;
		float output = buffer[(buffer_pos - delay) & BUFFER_MASK] * env_sc;
		buffer_pos = (buffer_pos + 1) & BUFFER_MASK;

		out_buffer[0][pos] = output * 0.2f;
		out_buffer[1][pos] = output * 0.2f;
	}
}

void wdsp_idle(unsigned long int sys_ticks)
{
	static bool button_state = false;

	if (io_digital_in(BUTTON_1) != button_state)
	{
		button_state = io_digital_in(BUTTON_1);

		if (button_state == true)
		{
			active = !active;
			io_digital_out(BYPASS_L, active);
			io_digital_out(BYPASS_R, active);
		}
	}

	io_digital_out(LED_1, active);
}
