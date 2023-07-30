TARGET = voicemail
BOARD = fxdsp

include ../WhateverDSP/libwdsp.mk

OBJECTS = voicemail.o gsm/lib/libgsm.a

gsm/lib/libgsm.a:
	$(MAKE) CC="$(CC) $(MCU)" DEBUG="" -C gsm lib/libgsm.a

voicemail.o: voicemail.cpp ringbuffer.h resimpler.h gsm/inc/gsm.h

$(TARGET): $(OBJECTS)

.PHONY: clean
clean:
	$(MAKE) -C gsm clean
	rm -f $(OBJECTS)
