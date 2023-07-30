TARGET = generationloss
BOARD = fxdsp

include ../WhateverDSP/libwdsp.mk

OBJECTS = generationloss.o gsm/lib/libgsm.a

gsm/lib/libgsm.a:
	$(MAKE) CC="$(CC) $(MCU)" DEBUG="" -C gsm lib/libgsm.a

generationloss.o: generationloss.cpp ringbuffer.h resimpler.h gsm/inc/gsm.h

$(TARGET): $(OBJECTS)

.PHONY: clean
clean:
	$(MAKE) -C gsm clean
	rm -f $(OBJECTS)
