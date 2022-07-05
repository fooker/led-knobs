SOURCES = \
	main.c

TARGET = firmware

OBJECTS = $(SOURCES:%.c=%.o)

CFLAGS = -mmcu=atmega328p \
	-DF_CPU=16000000L \
	-Os \
	-funsigned-char \
	-funsigned-bitfields \
	-fpack-struct \
	-fshort-enums \
	-fno-unit-at-a-time \
	-std=gnu18 \
	-Wall \
	-Wextra \
#	-Werror

LDFLAGS = -Wl,-Map=$(TARGET).map,--cref

AVRDUDE_PROGRAMMER ?= arduino
AVRDUDE_PORT ?= /dev/ttyUSB0

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
NM = avr-nm
AVRDUDE = avrdude

all: $(TARGET).hex $(TARGET).lss $(TARGET).sym size

flash: all
	$(AVRDUDE) -p atmega328p -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) -b57600 -D -U flash:w:$(TARGET).hex 
# -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m -U efuse:w:0xFF:m

%.hex: %.elf
	$(OBJCOPY) -O ihex -j .data -j .text $< $@

%.lss: %.elf
	$(OBJDUMP) -h -S $< > $@

%.sym: %.elf
	$(NM) -n $< > $@

%.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

size: $(TARGET).elf
	avr-size -G $(TARGET).elf

clean:
	$(RM) $(TARGET).elf
	$(RM) $(TARGET).hex
	$(RM) $(TARGET).lss
	$(RM) $(TARGET).sym
	$(RM) $(TARGET).map
	$(RM) $(OBJECTS)

.PHONY: all flash size clean