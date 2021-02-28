# Copyright 2020, Takashi TOYOSHIMA <toyoshim@gmail.com> All rights reserved.
# Use of this source code is governed by a BSD-style license that can be found
# in the LICENSE file.

# Default flags for JIT 1.00 PCB.
CFLAGS	= -Os -Wall -mmcu=$(MCU) -std=c++11 -I$(PWD)

CC	= avr-g++
OBJCOPY	= avr-objcopy
MCU	= atmega32a
LFLAGS	= -Wall -mmcu=$(MCU)
TARGET	= jit
OBJS	= Arduino.o JammaControl.o JVSIO.o JVSIOClient.o main.o

all: $(TARGET).hex

clean:
	rm -rf *.o *.elf *.hex *.bin

size: $(TARGET).elf
	$(SIZE) $(SFLAGS) $(TARGET).elf

program: $(TARGET).hex
	avrdude -C ~/opt/etc/avrdude.conf -c usbtiny -pm32 -U flash:w:$<
#	hidspx $<

# Fuse High - 1100 1001 - !OCDEN | !JTAGEN | SPIEN | CKOPT | !EESAVE | BOOT(APP)
# Fuse Low  - 1110 1111 - !BODLEVEL | !BODEN | SUT(10) | CLSEL(1111-EXT)
#   expects 16MHz external clock
fuse:
	hidspx -fl0xef -fh0xc9

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

JVSIO.o: ../jvsio/JVSIO.cpp ../jvsio/JVSIO.h *.h
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cpp ../jvsio/JVSIO.h *.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(TARGET).elf: $(OBJS)
	$(CC) $(LFLAGS) -o $@ $^
