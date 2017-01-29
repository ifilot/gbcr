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
ShiftRegisterPISO sri(&PORTC, &DDRC, PINC5, PINC3, PINC4, PINC2);

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
        sro.output_16bit(pos);
        uint8_t input = sri.input_8bit();
        sprintf(buf, "%02X", input);
        SerialPort::get()->serial_send_line(buf, 2);
        pos++;
    }

    // reset shift registers to 0
    sro.output_16bit(0);
}

/*
 * char2hex
 *
 * Helper function that converts 4 char hex addr to 16 bit unsigned int
 *
 * @param c - Pointer to c-string
 *
 */
uint16_t char2hex(char* c) {
    char hv[5] = {'\0', '\0', '\0', '\0', '\0'};

    for(int i=0; i<4; i++) {
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

    uint16_t addr = char2hex(&cmd[4]);
    uint16_t len  = char2hex(&cmd[8]);

    read_memory(addr, len);
}

/*
 * main routine
 *
 * Should stay in while loop indefinitely
 *
 */
int main(void) {
    SerialPort::get();

    // set address to 0
    sro.output_16bit(0);

    while(1) {
        get_command();
    }

    return 1;
}
