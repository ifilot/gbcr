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

#define GBWR PINB3
#define GBRD PINB4

void output_3bit(uint8_t addr) {
    if(bit_is_set(addr, 0)) {
        DDRD |= (1 << PIND2);
    } else {
        DDRD &= ~(1 << PIND2);
    }

    if(bit_is_set(addr, 1)) {
        DDRD |= (1 << PIND3);
    } else {
        DDRD &= ~(1 << PIND3);
    }

    if(bit_is_set(addr, 2)) {
        DDRD |= (1 << PIND4);
    } else {
        DDRD &= ~(1 << PIND4);
    }
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

    sprintf(buf, "ADDR%04X", addr);
    SerialPort::get()->serial_send_line(buf, 8);
    sprintf(buf, "SIZE%04X", len);
    SerialPort::get()->serial_send_line(buf, 8);

    uint16_t pos  = addr;
    while(pos < (uint16_t)addr + len) {
        sro.write_16bit(pos);
        uint8_t input = sru.read_8bit();
        sprintf(buf, "%02X", input);
        SerialPort::get()->serial_send_line(buf, 2);
        pos++;
    }

    // reset shift registers to 0
    sro.write_16bit(0);
}

/*
 * change_mbr
 *
 * routine to select memory bank
 */
void change_mbr(uint8_t bank_addr) {
    output_3bit(bank_addr);

    // set write
    PORTB &= ~(1 << GBWR);    // write low
    PORTB |= (1 << GBRD);     // read high
    _delay_ms(5);

    // write address
    sro.write_16bit(0x2100);
    _delay_ms(5);

    // write bank
    sru.write_8bit(bank_addr);
    _delay_ms(5);

    // set read
    PORTB |= (1 << GBWR);     // write high
    PORTB &= ~(1 << GBRD);    // read low
    _delay_ms(5);

    // get address that was written
    uint8_t out = sru.shift_out();
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
    // CHAN GEBA NKXX --> change bank position

    if(strncmp(cmd, "READ", 4) == 0) {
        uint16_t addr = char2hex4(&cmd[4]);
        uint16_t len  = char2hex4(&cmd[8]);
        read_memory(addr, len);
    } else if(strncmp(cmd, "CHANGEBANK", 10) == 0) {
        uint8_t bank_addr = char2hex2(&cmd[10]);
        change_mbr(bank_addr);
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

    // set write high and read low to
    // disable write and enable read
    PORTB |= (1 << GBWR);     // high
    PORTB &= ~(1 << GBRD);    // low

    PORTD |= (1 << PIND2);
    PORTD |= (1 << PIND3);
    PORTD |= (1 << PIND4);

    DDRD &= ~(1 << PIND2);
    DDRD &= ~(1 << PIND3);
    DDRD &= ~(1 << PIND4);
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

    for(uint8_t i=0; i<8; i++) {
        output_3bit(i);
        _delay_ms(500);
    }
    output_3bit(0);

    // setup serial connection
    SerialPort::get();

    // set address to 0
    sro.write_16bit(0);

    while(1) {
        get_command();
    }

    return 1;
}
