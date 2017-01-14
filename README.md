GameBoyCartridgeReader
===============

## Introduction
The GameBoy Cartridge Reader tool copies data held in GB/GBA cartridges and save these into rom files.

## Working
GBCR has two programs, one image for the AVR microcontroller that is wired to the cartridge and one C++ program that communicates with the AVR microcontroller over a serial port.

## Compilation
Both programs can be compiled using `make`. To upload the image to your microcontroller, check the settings inside `Makefile` and type `make flash`.

## Usage
Extract a ROM from a cartridge by typing `gbcr <PORT> <ROM>`, where `<PORT>` is something like `/dev/ttyUSB0` and `<ROM>` is something like `rom.gb`.

## Limitations
Currently, the program only supports the simple 32kb regular GB roms.