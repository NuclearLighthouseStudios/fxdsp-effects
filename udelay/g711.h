/*
 * g711.h
 *
 * u-law, A-law and linear PCM conversions.
 * Source: http://www.speech.kth.se/cost250/refsys/latest/src/g711.h
 */

#ifndef _G711_H_
#define _G711_H_

#ifdef __cplusplus
extern "C" {
#endif

	extern uint8_t linear2alaw(int16_t pcm_val);
	extern int16_t alaw2linear(uint8_t a_val);
	extern uint8_t linear2ulaw(int16_t pcm_val);
	extern int16_t ulaw2linear(uint8_t u_val);
	extern uint8_t alaw2ulaw(uint8_t aval);
	extern uint8_t ulaw2alaw(uint8_t uval);

#ifdef __cplusplus
}
#endif

#endif /* _G711_H_ */