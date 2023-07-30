TARGET = udelay
BOARD = fxdsp

include ../WhateverDSP/libwdsp.mk

OBJECTS = g711.o udelay.o

g711.o: g711.c g711.h
udelay.o: udelay.c g711.h filters.h

$(TARGET): $(OBJECTS)

.PHONY: clean
clean:
	rm -f $(OBJECTS)