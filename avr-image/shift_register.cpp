/**************************************************************************
 *   shift_register.cpp  --  This file is part of GBCR.                   *
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

#include "shift_register.h"

ShiftRegisterSIPO::ShiftRegisterSIPO(volatile uint8_t *_port, volatile uint8_t *_ddr, uint8_t _ser, uint8_t _clk, uint8_t _rck) {
    this->ddr =   _ddr;
    this->port =  _port;
    this->ser =   _ser;
    this->clk =   _clk;
    this->rck =   _rck;

    // enable ports
    *(this->ddr) |= (1 << this->ser);
    *(this->ddr) |= (1 << this->clk);
    *(this->ddr) |= (1 << this->rck);

    // set ports to default value
    *(this->port) &= ~(1 << this->ser);
    *(this->port) &= ~(1 << this->clk);
    *(this->port) |=  (1 << this->rck);
}

void ShiftRegisterSIPO::output_8bit(uint8_t state) {
    // clock low
    *(this->port) &= ~(1 << this->clk);
    // latch low
    *(this->port) &= ~(1 << this->rck);

    for(int i=7; i>=0; i--) {

        if(bit_is_set(state, i)) {
            *(this->port) |= (1 << this->ser);
        } else {
            *(this->port) &= ~(1 << this->ser);
        }

        *(this->port) |= (1 << this->clk);  // clock high
        *(this->port) &= ~(1 << this->clk); // clock low
    }


    *(this->port) |= (1 << this->rck);  // latch high
}

void ShiftRegisterSIPO::output_16bit(uint16_t state) {
    uint8_t hi = ((state >> 8) & 0xff);
    uint8_t lo = ((state >> 0) & 0xff);

    this->output_8bit(hi);
    this->output_8bit(lo);
}

ShiftRegisterPISO::ShiftRegisterPISO(volatile uint8_t *_port, volatile uint8_t *_ddr, uint8_t _ds, uint8_t _pl, uint8_t _cp, uint8_t _ce) {
    this->ddr  =  _ddr;
    this->port =  _port;
    this->ds   =  _ds;
    this->pl   =  _pl;
    this->cp   =  _cp;
    this->ce   =  _ce;

    // enable ports
    *(this->ddr) &= ~(1 << this->ds); // input
    *(this->ddr) |= (1 << this->pl);  // output
    *(this->ddr) |= (1 << this->cp);  // output
    *(this->ddr) |= (1 << this->ce);  // output

    *(this->port) &= ~(1 << this->ds);    // low
    *(this->port) &= ~(1 << this->cp);    // low
    *(this->port) &= ~(1 << this->ce);    // low
    *(this->port) |= (1 << this->pl);     // high
}

uint8_t ShiftRegisterPISO::input_8bit() {
    uint8_t input = 0;

    PORTC &= ~(1 << this->pl); // set parallel load low = enable
    PORTC |= (1 << this->ce);  // set clock enable high = disable
    PORTC &= ~(1 << this->ce); // enable clock
    PORTC |= (1 << this->pl);  // disable parallel load

    // read in data
    for(int i=7; i>=0; i--) {
        if(!(PINC & (1 << this->ds))) {
            input |= (1 << i);
        }
        
        // toggle clock high -> low (clock pulse)
        PORTC |= (1 << this->cp);
        PORTC &= ~(1 << this->cp);
    }

    return input;
}