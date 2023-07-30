#ifndef RINGBUF_H
#define RINGBUF_H

template <typename type, unsigned int length>
class Ringbuffer
{
public:
	void put(type samples[], unsigned int nsamples);
	void put(type samples);
	void get(type samples[], unsigned int nsamples);
	type get(void);

	void clear(void);

	unsigned int size(void);

private:
	type buffer[length] = { 0 };
	unsigned int rpos = 0;
	unsigned int wpos = 0;
};

template <typename type, unsigned int length>
inline void Ringbuffer<type, length>::put(type samples[], unsigned int nsamples)
{
	for (int i = 0; i < (signed int)nsamples; i++)
	{
		buffer[wpos++] = samples[i];

		if (wpos >= length)
			wpos = 0;

		if (wpos == rpos)
		{
			rpos++;
			if (rpos >= length)
				rpos = 0;
		}
	}
}

template <typename type, unsigned int length>
inline void Ringbuffer<type, length>::put(type sample)
{
	buffer[wpos++] = sample;

	if (wpos >= length)
		wpos = 0;

	if (wpos == rpos)
	{
		rpos++;
		if (rpos >= length)
			rpos = 0;
	}
}

template <typename type, unsigned int length>
inline void Ringbuffer<type, length>::get(type samples[], unsigned int nsamples)
{
	for (int i = 0; i < nsamples; i++)
	{
		if (rpos != wpos)
		{
			samples[i] = buffer[rpos++];
			if (rpos >= length)
				rpos = 0;
		}
		else
			samples[i] = 0;
	}
}

template <typename type, unsigned int length>
inline type Ringbuffer<type, length>::get(void)
{
	type sample;

	if (rpos != wpos)
	{
		sample = buffer[rpos++];
		if (rpos >= length)
			rpos = 0;
	}
	else
		sample = 0;

	return sample;
}

template <typename type, unsigned int length>
inline void Ringbuffer<type, length>::clear(void)
{
	rpos = wpos = 0;
}

template <typename type, unsigned int length>
inline unsigned int Ringbuffer<type, length>::size(void)
{
	if (wpos >= rpos)
		return wpos - rpos;
	else
		return length - (rpos - wpos);
}

#endif