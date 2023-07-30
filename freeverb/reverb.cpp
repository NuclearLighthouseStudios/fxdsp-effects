#include <libwdsp.h>

#include "revmodel.hpp"

revmodel reverb;

bool active = false;

float silence[BLOCK_SIZE] = { 0 };

void wdsp_process(float **in_buffer, float **out_buffer)
{
	if (active)
	{
	#if CONFIG_EFFECT_MIXDOWN == true
		float mix[BLOCK_SIZE];

		for (int i = 0; i < BLOCK_SIZE; i++)
			mix[i] = in_buffer[0][i] + in_buffer[1][i];

		reverb.processreplace(mix, mix, out_buffer[0], out_buffer[1], BLOCK_SIZE, 1);
	#else
		reverb.processreplace(in_buffer[0], in_buffer[1], out_buffer[0], out_buffer[1], BLOCK_SIZE, 1);
	#endif
	}
	else
	{
	#if CONFIG_EFFECT_MIXDOWN == true
		for (int i = 0; i < BLOCK_SIZE; i++)
		{
			out_buffer[0][i] = in_buffer[0][i] + in_buffer[1][i];
			out_buffer[1][i] = in_buffer[0][i] + in_buffer[1][i];
		}
	#else
		for (int i = 0; i < BLOCK_SIZE; i++)
		{
			out_buffer[0][i] = in_buffer[0][i];
			out_buffer[1][i] = in_buffer[1][i];
		}
	#endif

		reverb.processmix(silence, silence, out_buffer[0], out_buffer[1], BLOCK_SIZE, 1);
	}
}

void wdsp_idle(unsigned long int sys_ticks)
{
	static unsigned long int button_start;
	static bool button_state = false;
	static bool just_activated;

	reverb.setwet(io_analog_in(POT_1));
	reverb.setroomsize(io_analog_in(POT_2));
	reverb.setmode(io_digital_in(BUTTON_1));

	reverb.update();

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
	reverb.setdry(1.0f);

#if CONFIG_EFFECT_BYPASS == false
	io_digital_out(BYPASS_L, true);
	io_digital_out(BYPASS_R, true);
#endif
}