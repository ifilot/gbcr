/**************************************************************************
 *   serial.h  --  This file is part of GBCR.                             *
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

#ifndef _SERIAL_H
#define _SERIAL_H

#include <avr/io.h>
#include <string.h>

class SerialPort {
private:
    long baud;
    long f_cpu;
    long baud_rate_calc;

public:
    static SerialPort* get() {
        static SerialPort instance;
        return &instance;
    }

    void serial_send(char data);
    void serial_send_line(const char str[], long len);
    char serial_receive();

    void set_baud(long _baud);

private:
    SerialPort();

    // Singleton pattern
    SerialPort(SerialPort const&)      = delete;
    void operator=(SerialPort const&)  = delete;
};

#endif //_SERIAL_H