/**************************************************************************
 *   image.cpp  --  This file is part of GBCR.                            *
 *                                                                        *
 *   Copyright (C) 2017, Ivo Filot                                        *
 *                                                                        *
 *   Netris is free software: you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published    *
 *   by the Free Software Foundation, either version 3 of the License,    *
 *   or (at your option) any later version.                               *
 *                                                                        *
 *   Netris is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "serial.h"
#include "shift_register.h"

// serial, clock, latch
ShiftRegisterSIPO sro(&PORTB, &DDRB, PINB1, PINB0, PINB2);

ShiftRegisterUniversal sru(&PORTC, &DDRC, PINC0, PINC1, PINC2, PINC3, PINC4, PINC5);

#define GBWR  PINB3
#define GBRD  PINB4
#define GBREQ PIND5
#define LED1  PIND2
#define LED2  PIND3

void reset_pins() {
    PORTB |= (1 << GBWR);     // no write
    PORTB |= (1 << GBRD);     // no read
    PORTD |= (1 << GBREQ);    // no req

}

uint8_t read_byte(uint16_t addr) {
    sro.write_16bit(addr);

    PORTB &= ~(1 << GBRD);
    PORTD &= ~(1 << GBREQ);

    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");

    uint8_t input = sru.read_8bit();

    PORTB |= (1 << GBRD);
    PORTD |= (1 << GBREQ);

    return input;
}

/*
 * read_memory
 *
 * Read memory from cartridge. Results are communicated over SerialPort
 * as HEX characters (2 char per byte)
 *
 * @param addr - Starting address
 * @param len  - Bytes to read
 *
 */
void read_memory(uint16_t addr, uint16_t len) {
    char buf[10];
    uint8_t val;

    sprintf(buf, "ADDR%04X", addr);
    SerialPort::get()->serial_send_line(buf, 8);
    sprintf(buf, "SIZE%04X", len);
    SerialPort::get()->serial_send_line(buf, 8);

    uint16_t pos  = addr;

    PORTD |= (1 << LED2); // enable led2 (operation)
    while(pos < (uint16_t)addr + len) {
        val = read_byte(pos);
        sprintf(buf, "%02X", val);
        SerialPort::get()->serial_send_line(buf, 2);
        pos++;
    }
    PORTD &= ~(1 << LED2); // disable led2 (done)

    // reset shift registers to 0
    sro.write_16bit(0);
}

/*
 * write_byte
 *
 * routine to select memory bank
 */
void write_byte(uint16_t addr, uint8_t byte) {
    // write address
    sro.write_16bit(addr);
    sru.write_8bit(byte);

    // write pulse
    PORTB &= ~(1 << GBWR);

    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");

    PORTB |= (1 << GBWR);
}

/*
 * write_ram
 *
 * @param len  - Bytes to write
 *
 */
void write_ram(uint16_t len) {
    uint16_t it  = 0;

    PORTD |= (1 << LED2); // enable led2 (operation)
    while(it < len) {
        char c = SerialPort::get()->serial_receive();
        SerialPort::get()->serial_send(c);
        write_byte(0xA000 + it, c);
        it++;
    }
    PORTD &= ~(1 << LED2); // disable led2 (done)

    // reset shift registers to 0
    sro.write_16bit(0);
}

/*
 * char2hex4
 *
 * Helper function that converts 4 char hex addr to 16 bit unsigned int
 *
 * @param c - Pointer to c-string
 *
 */
uint16_t char2hex4(char* c) {
    char hv[5] = {'\0', '\0', '\0', '\0', '\0'};

    for(int i=0; i<4; i++) {
        hv[i] = *c;
        c++;
    }

    return strtoul(hv, NULL, 16);
}

/*
 * char2hex2
 *
 * Helper function that converts 2 char hex addr to 8 bit unsigned int
 *
 * @param c - Pointer to c-string
 *
 */
uint8_t char2hex2(char* c) {
    char hv[3] = {'\0', '\0', '\0'};

    for(int i=0; i<2; i++) {
        hv[i] = *c;
        c++;
    }

    return strtoul(hv, NULL, 16);
}

/*
 * get_command
 *
 * Get input command over serial;
 *
 * Extend this function to accept more commands
 *
 */
void get_command() {
    char cmd[12];
    int cnt = 0;
    while(cnt < 12) {
        char c = SerialPort::get()->serial_receive();
        if(c != 0) {
            cmd[cnt] = c;
            SerialPort::get()->serial_send(cmd[cnt]);
            cnt++;
        }
    }

    // command list
    //
    // READ XXXX XXXX --> read instruction
    // WRBY XXXX XXXX --> write single byte at specified address
    // WRIT ERAM XXXX --> write instruction

    if(strncmp(cmd, "READ", 4) == 0) {
        uint16_t addr = char2hex4(&cmd[4]);
        uint16_t len  = char2hex4(&cmd[8]);
        read_memory(addr, len);
    } else if(strncmp(cmd, "WRBY", 4) == 0) {
        uint16_t addr = char2hex4(&cmd[4]);
        uint8_t value = char2hex2(&cmd[10]);
        write_byte(addr, value);
    } else if(strncmp(cmd, "WRITERAM", 8) == 0) {
        uint16_t len = char2hex4(&cmd[8]);
        write_ram(len);
    }
}

/*
 * setup
 *
 * run this function at the start of the main routine
 *
 */
void setup() {
    // set pins for WR and RD on GB
    DDRB |= (1 << GBWR);  // output
    DDRB |= (1 << GBRD);  // output

    // enable PORTD pins
    DDRD |= (1 << LED1);   // output
    DDRD |= (1 << LED2);   // output
    DDRD |= (1 << GBREQ);  // output

    // set write high and read low to
    // disable write and enable read
    PORTB |= (1 << GBWR);     // high
    PORTB &= ~(1 << GBRD);    // low

    // set high to set active
    PORTD |= (1 << LED1);     // high
}

/*
 * main routine
 *
 * Should stay in while loop indefinitely
 *
 */
int main(void) {
    // setup GB pins
    setup();

    // setup serial connection
    SerialPort::get();

    // set address to 0
    sro.write_16bit(0);
    reset_pins();

    while(1) {
        get_command();
    }

    return 1;
}
