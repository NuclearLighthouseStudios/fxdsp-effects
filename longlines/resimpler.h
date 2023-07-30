#ifndef RESIMPLER_H
#define RESIMPLER_H

#include <cmath>

template <unsigned int from, unsigned int to, unsigned int quality = 16, unsigned int memory = 64>
class Resimpler
{
public:
	constexpr unsigned int max_output_size(unsigned int nsamples)
	{
		return ceilf(nsamples * ratio) + (ratio >= 1.0f ? 1 : 0);
	}

	unsigned int process(float *in, float *out, unsigned int nsamples)
	{
		unsigned int outsamples = 0;

		while (position + step < nsamples - window_size)
		{
			float out_sample = 0.0f;

			int start = -window_size + 1;
			int end = window_size;
			int sample_index = position + start;

			float kernel_index = (sample_index - position) * kernel.scale;

			for (int i = start; i < end; i++)
			{
				int kernel_index_abs = fabsf(kernel_index);
				float kernel_frac = fabsf(kernel_index) - kernel_index_abs;

				float sample;
				if (sample_index >= 0)
					sample = in[sample_index];
				else
					sample = history[sample_index + hist_size];

				out_sample += sample * kernel.lut[kernel_index_abs] * (1.0f - kernel_frac);
				out_sample += sample * kernel.lut[kernel_index_abs + 1] * kernel_frac;

				kernel_index += kernel.scale;
				sample_index++;
			}

			out[outsamples++] = out_sample * scale;
			position += step;
		}

		position -= nsamples;

		int carryover = nsamples < hist_size ? nsamples : hist_size;

		if (carryover > 0)
			for (int i = 0; i < (signed int)hist_size - carryover; i++)
				history[i] = history[carryover + i];

		for (int i = -carryover; i < 0; i++)
			history[hist_size + i] = in[nsamples + i];

		return outsamples;
	}

private:
	static constexpr float ratio = (float)to / (float)from;
	static constexpr float scale = ratio >= 1 ? 1.0f : ratio;
	static constexpr float step = (float)from / (float)to;
	static constexpr float window_size = quality / scale;

	class Kernel
	{
	public:
		constexpr Kernel(void)
		{
			for (int i = 0; i < (signed int)size + 1; i++)
				lut[i] = lanczos(quality, (float)i / (float)size * (float)quality);
		};

		static constexpr unsigned int size = memory * quality;
		static constexpr float scale = size / window_size;
		float lut[size + 1] = { 0 };

	private:
		constexpr static inline float lanczos(float a, float x)
		{
			if (x == 0) return 1.0f;
			return (a * sinf(M_PI * x) * sinf(M_PI * x / a)) / (M_PI * M_PI * x * x);
		}
	};

	static constexpr Kernel kernel = Kernel();

	static constexpr unsigned int hist_size = ceilf(window_size) * 2 + ceilf(step);
	float history[hist_size] = { 0 };

	float position = 0;
};

#endif