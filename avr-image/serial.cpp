/**************************************************************************
 *   serial.cpp  --  This file is part of GBCR.                           *
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

#include "serial.h"

/*
 * SerialPort()
 *
 * set register settings
 *
 * Run this function at the beginning of main() to set the serial connection
 *
 */
SerialPort::SerialPort(){
    this->f_cpu = F_CPU;
    this->set_baud(57600);

    // high and low bits
    UBRR0H = (this->baud_rate_calc >> 8);
    UBRR0L = this->baud_rate_calc; 

    //transmit and receive enable
    UCSR0B = (1 << TXEN0)| (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
}

void SerialPort::set_baud(long _baud) {
    this->baud = _baud;
    this->baud_rate_calc =  ((this->f_cpu / 16 / this->baud) - 1);
}

/*
 * serial_send()
 *
 * send a single character over the serial connection
 *
 */
void SerialPort::serial_send(char data){
    while (( UCSR0A & (1<<UDRE0))  == 0){};
    UDR0 = data;
}

/*
 * serial_send_line()
 *
 * send a string of characters over the serial connection
 *
 */
void SerialPort::serial_send_line(const char str[], long len) {
    for(long i=0; i < len; i++) {
        serial_send(str[i]);
    }
}

/*
 * serial_receive()
 *
 * wait until received
 *
 */
char SerialPort::serial_receive (void) {
    while(((UCSR0A) & (1 << RXC0)) == 0) {};  // wait while data is being received
    return UDR0;
}
