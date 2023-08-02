#include <libwdsp.h>

#include <sys/param.h>
#include <math.h>

const io_pin_idx inputs[] = { CONFIG_MIDI_INTERFACE_CC_INPUTS };
#define NUM_INPUTS (sizeof(inputs) / sizeof(inputs[0]))
const uint8_t controls[NUM_INPUTS] = { CONFIG_MIDI_INTERFACE_CC_CONTROLS };

void wdsp_process(float **in_buffer, float **out_buffer)
{
	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		out_buffer[0][i] = 0.0f;
		out_buffer[1][i] = 0.0f;
	}
}

void wdsp_idle(unsigned long int ticks)
{
	static float control_last[NUM_INPUTS] = { 0.0f };
	static bool button_state = false;

	if (button_state != io_digital_in(BUTTON_1))
	{
		button_state = io_digital_in(BUTTON_1);

		midi_message message;

		message.command = CONTROL_CHANGE;
		message.channel = CONFIG_MIDI_INTERFACE_CHANNEL;
		message.data.control.param = CONFIG_MIDI_INTERFACE_CC_FOOTSWITCH;
		message.data.control.value = button_state * 127;

		midi_send_message(&message);
	}

	for (int i = 0; i < NUM_INPUTS; i++)
	{
		float value = io_analog_in(inputs[i]);

		if (fabsf(value - control_last[i]) > 1.0f / 128.0f)
		{
			control_last[i] = value;

			midi_message message;

			message.command = CONTROL_CHANGE;
			message.channel = CONFIG_MIDI_INTERFACE_CHANNEL;
			message.data.control.param = controls[i];
			message.data.control.value = MIN((uint8_t)(value * 128.0f), 127);

			midi_send_message(&message);
		}
	}

	midi_message *message = midi_get_message();

	if ((message != NULL) && (message->channel == CONFIG_MIDI_INTERFACE_CHANNEL))
	{
		if (message->command == CONTROL_CHANGE)
		{
			switch (message->data.control.param)
			{
				case CONFIG_MIDI_INTERFACE_CC_FOOTSWITCH:
				{
					bool bypass = message->data.control.value < 64;

					io_digital_out(BYPASS_L, bypass);
					io_digital_out(BYPASS_R, bypass);
				}
				break;

				case CONFIG_MIDI_INTERFACE_CC_LED:
				{
					bool led = message->data.control.value > 64;

					io_digital_out(LED_1, led);
				}
				break;

				default:
			}
		}
	}
}

void wdsp_init(void)
{
	io_digital_out(BYPASS_L, true);
	io_digital_out(BYPASS_R, true);
}