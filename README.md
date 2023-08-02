# FXDSP Plugins

A collection of effects for the [FXDSP](https://github.com/NuclearLighthouseStudios/FXDSP)
DSP guitar effects pedal.


## License

Unless otherwise noted all code is licensed under CC-BY.


## Building and flashing

* Run `make` in the top directory to build all effects.
* Hold down the footswitch of your pedal while plugging in the power to go into bootloader mode
* Plug the pedal into your computer via USB
* Run `make dfu` in the directory of the effect you want to flash to the pedal


## Effects

### Barry's Satan Maximizer

A port of Steve Harris' LADSPA plugin of the same name.

* Left potentiometer controls the gain (knee point)
* Right potentiometer controls the tone (envelope time)

Licensed under GPL, just like the original.


### Freeverb

A port of the original Freeverb code by Jezar at Dreampoint.

* Left potentiometer controls the loudness of the reverb signal
* Right potentiometer controls the reverb length
* Holding down the footswitch enables freeze mode

The original freeverb code is in the public domain.


### Generation Loss

A delay effect that compresses it's delay using the GSM 06.10 Full Rate audio codec,
leading to successive repetitions becoming more and more laden with compression artifacts.

* Left potentiometer controls the delay time
* Right potentiometer controls the feedback
* Holding down the footswitch temporarily overrides the feedback to 100%

The GSM codec code is Copyright by Jutta Degener and Carsten Bormann.


### Interface

Not an effect by itself, but a small program that turns the FXDSP into a USB
audio and MIDI interface.

The knobs and the foot switch are turned into midi CC messages.
True bypass and the LED can also be controlled via midi CC.

CC numbers and and knob mappings can be adjusted in `config.ini`.


### Longlines

A simple effect which just adds GSM compression to your signal.
Inspired by Steve Harris' GSM LADSPA effect.

* Left potentiometer controls the wet/dry mix
* Right potentiometer controls the error rate

The GSM codec code is Copyright by Jutta Degener and Carsten Bormann.


### uDelay

A delay effect which uses uLaw compression on it's delay buffer for longer delay times.

* Left potentiometer controls the delay time
* Right potentiometer controls the feedback
* Holding down the footswitch temporarily overrides the feedback to 100%

The uLaw compression code is provided by Sun Microsystems for unrestricted use.


### Voicemail

A looper which uses GSM 06.10 Full Rate compression for up 72 seconds of loop time.

* Left potentiometer controls the dry signal volume
* Right potentiometer controls the loop volume
* Tapping the footswitch switches into record mode
* Tapping it again toggles between playback and overdub mode
* Holding down the footswitch for 1 second clears the loop

The GSM codec code is Copyright by Jutta Degener and Carsten Bormann.