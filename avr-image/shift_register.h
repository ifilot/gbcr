/**************************************************************************
 *   shift_register.h  --  This file is part of GBCR.                     *
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

#ifndef _SHIFT_REGISTER_H
#define _SHIFT_REGISTER_H

#include <avr/io.h>
#include <util/delay.h>

/*
 * Class to control SIPO ShiftRegister such as the 74HC595
 */
class ShiftRegisterSIPO {
private:
    volatile uint8_t *port;
    volatile uint8_t *ddr;
    uint8_t ser;                // serial
    uint8_t clk;                // clock
    uint8_t rck;                // latch

public:
    ShiftRegisterSIPO(volatile uint8_t *_port, volatile uint8_t *_ddr, uint8_t _ser, uint8_t _clk, uint8_t _rck);
    void write_8bit(uint8_t state);
    void write_16bit(uint16_t state);
};

/*
 * Class to control PISO ShiftRegister such as the 74HC165
 */
class ShiftRegisterPISO {
private:
    volatile uint8_t *port;
    volatile uint8_t *ddr;

    uint8_t ds;                // serial data
    uint8_t pl;                // parallel load (active low)
    uint8_t cp;                // clock pulse
    uint8_t ce;                // clock enable (active low)
    uint8_t se;                // serial in

public:
    ShiftRegisterPISO(volatile uint8_t *_port, volatile uint8_t *_ddr, uint8_t _ds, uint8_t _pl, uint8_t _cp, uint8_t _ce, uint8_t _se);
    uint8_t read_8bit();
};

/*
 * Class to control PISO ShiftRegister such as the 74HC165
 */
class ShiftRegisterUniversal {
private:
    volatile uint8_t *port;
    volatile uint8_t *ddr;

    uint8_t s0;    //
    uint8_t s1;    //
    uint8_t ds0;   // serial input left
    uint8_t q7;    // serial output right
    uint8_t cp;    // clock pulse
    uint8_t oe1;   // output enable 1

public:
    ShiftRegisterUniversal(volatile uint8_t *_port, volatile uint8_t *_ddr, uint8_t _s0, uint8_t _s1, uint8_t _ds0, uint8_t _q7, uint8_t _cp, uint8_t _oe1);
    uint8_t read_8bit();
    uint8_t shift_out();
    void write_8bit(uint8_t state);
};

#endif
