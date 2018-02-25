# WinAVR cross-compiler toolchain is used here
CC = avr-gcc
CXX = avr-g++
OBJCOPY = avr-objcopy
DUDE = avrdude
F_CPU = 16000000
FUSEL = 0xFF
FUSEH = 0xDE
EFUSE = 0xFD

# fuse settings
# 0xFF: external clock source 8+ MHz
# 0xDE: SPIEN & BOOTRST enables
# 0xFD: brownout at 2.7 V

# update the lines below to match your configuration
CFLAGS = -std=c++11 -Wall -O1 -mmcu=atmega328p -DF_CPU=$(F_CPU) -DDEBUG_LEVEL=0 -fno-threadsafe-statics
OBJFLAGS = -j .text -j .data -O ihex
DUDEFLAGS = -p atmega328p -c usbasp

# Object files for the firmware (usbdrv/oddebug.o not strictly needed I think)
OBJECTS = image.o serial.o shift_register.o

# By default, build the firmware and command-line client, but do not flash
all: image.hex

# With this, you can flash the firmware by just typing "make flash" on command-line
flash: image.hex
	sudo $(DUDE) $(DUDEFLAGS) -U flash:w:$<

# rule for programming fuse bits:
fuse:
	@[ "$(FUSEH)" != "" -a "$(FUSEL)" != "" ] || \
	{ echo "*** Edit Makefile and choose values for FUSEL and FUSEH!"; exit 1; }
	sudo $(DUDE) $(DUDEFLAGS) -U hfuse:w:$(FUSEH):m -U lfuse:w:$(FUSEL):m -U efuse:w:$(EFUSE):m -v

# Housekeeping if you want it
clean:
	$(RM) *.o *.hex *.elf

# From .elf file to .hex
%.hex: %.elf
	$(OBJCOPY) $(OBJFLAGS) $< $@

# Main.elf requires additional objects to the firmware
image.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

# Without this dependency, .o files will not be recompiled upon some changes
$(OBJECTS):

# From C source to .o object file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# From CPP source to .o object file
%.o: %.cpp $(HEADERS)
	$(CXX) $(CFLAGS) -c $< -o $@

# From assembler source to .o object file
%.o: %.asm
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
