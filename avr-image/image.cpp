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

// _ds, _pl, _cp, _ce
ShiftRegisterPISO sri(&PORTC, &DDRC, PINC5, PINC3, PINC4, PINC2, PINC1);

#define GBWR PINB3
#define GBRD PINB4

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
        uint8_t input = sri.read_8bit();
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
    // set write low
    PORTB &= ~(1 << GBWR);    // low
    PORTB |= (1 << GBRD);     // high

    // write address
    sro.write_16bit(0x2100);
    _delay_ms(5);

    // write bank
    sri.write_8bit(bank_addr);
    _delay_ms(5);

    // set write high and read low to
    // disable write and enable read
    PORTB |= (1 << GBWR);     // high
    PORTB &= ~(1 << GBRD);    // low
    _delay_ms(5);
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
    } else if(strncmp(cmd, "CHNGBANK", 8) == 0) {
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

    while(1) {
        get_command();
    }

    return 1;
}
