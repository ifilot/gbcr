/**************************************************************************
 *   reader.cpp  --  This file is part of GBCR.                           *
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

#include <boost/asio/serial_port.hpp>
#include <boost/asio.hpp>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <iomanip>

boost::asio::io_service io;
boost::asio::serial_port port(io);

/*
 * print_loadbar
 *
 * simple progress bar to show
 *
 * @param x - iterator
 * @param n - total number of iterations
 * @param w - width of the progress bar
 */
static inline void print_loadbar(unsigned int x, unsigned int n, unsigned int w = 35) {
    if ( (x != n) && (x % (n/100+1) != 0) ) return;

    float ratio  =  x/(float)n;
    int   c      =  ratio * w;

    std::cout << std::setw(3) << (int)(ratio*100) << "% [";
    for (int x=0; x<c; x++) std::cout << "=";
    for (int x=c; x<w; x++) std::cout << " ";
    std::cout << "]\r" << std::flush;
}

int change_rom_bank(uint8_t bank_addr, bool output) {
    // create buffer
    char c[12];

    if(output) {
        std::cout << "Changing to ROM BANK: " << (int)bank_addr << "  " << std::endl;
    }

    char cmd[13] = {'C', 'H', 'A', 'N', 'G', 'E', 'B', 'A', 'N', 'K', '0', '0','0'};
    sprintf(&cmd[10], "%02X", bank_addr);
    for(unsigned int i=0; i<12; i++) {
        boost::asio::write(port, boost::asio::buffer(&cmd[i], 1));
        boost::asio::read(port, boost::asio::buffer(&c,1));
        if(cmd[i] != c[0]) {
            std::cerr << "An error occurred during data transfer, aborting!" << std::endl;
            std::cerr << "Error encountered at " << __FILE__ << " (" << __LINE__ << ")" << std::endl;
            std::cerr << "Send: " << cmd[i] << " | Receive: " << c[0] << "| i=" << i << std::endl;
            exit(-1);
        }
    }
}

/*
 * read_memory
 *
 * @param _addr  - starting address
 * @param _len   - number of bytes to read
 * @param buffer - pointer to vector to store data
 *
 * return number of bytes read
 */
size_t read_memory(uint16_t _addr, uint16_t _len, std::vector<uint8_t> *buffer, bool show_progress = false) {
    // create buffer
    char c[12];

    char cmd[12] = {'R', 'E', 'A', 'D', '0', '0', '0', '0', '0', '0', '0', '0'};
    sprintf(&cmd[4], "%04X%04X", _addr, _len);

    for(unsigned int i=0; i<12; i++) {
        boost::asio::write(port, boost::asio::buffer(&cmd[i], 1));
        boost::asio::read(port, boost::asio::buffer(&c,1));
        if(cmd[i] != c[0]) {
            std::cerr << "An error occurred during data transfer, aborting!" << std::endl;
            std::cerr << "Error encountered at " << __FILE__ << " (" << __LINE__ << ")" << std::endl;
            std::cerr << "Send: " << cmd[i] << " | Receive: " << c[0] << "| i=" << i << std::endl;
            exit(-1);
        }
    }

    // read addr line
    boost::asio::read(port, boost::asio::buffer(&c,8));
    c[8] = '\0';
    size_t addr = strtoul(&c[4], NULL, 16);

    // read size line
    boost::asio::read(port, boost::asio::buffer(&c,8));
    c[8] = '\0';
    size_t size = strtoul(&c[4], NULL, 16);

    size_t bytes = 0;
    while(bytes < size) {
        boost::asio::read(port, boost::asio::buffer(&c,2));
        c[2] = '\0';
        uint8_t v = strtoul(c, NULL, 16);
        buffer->push_back(v);
        bytes++;

        if(show_progress) {
            print_loadbar(bytes, size);
        }
    }

    return bytes;
}

/*
 * reader_header_details
 *
 * Print information from the ROM header to the screen
 *
 * @param buffer - pointer to vector where ROM header is stored
 *
 */
void print_header_details(const std::vector<uint8_t> &header) {
    std::string title;

    for(unsigned int i=0x0134; i<0x0143; i++) {
        title += (char)header[i];
    }

    std::cout << "Cartridge title: " << title << std::endl;

    std::cout << "Cartridge type:  ";
    switch(header[0x0147]) {
        case 0x00:
            std::cout << "ROM ONLY";
        break;
        case 0x01:
            std::cout << "MBC1";
        break;
        case 0x02:
            std::cout << "MBC1+RAM";
        break;
        default:
            std::cout << "Unknown type";
        break;
    }
    std::cout << std::endl;

    std::cout << "ROM Size:        ";
    switch(header[0x0148]) {
        case 0x00:
            std::cout << "32KByte (no ROM banking)";
        break;
        case 0x01:
            std::cout << "64KByte (4 banks)";
        break;
        case 0x02:
            std::cout << "128 KByte (8 banks)";
        break;
        case 0x03:
            std::cout << "256KByte 16 banks)";
        break;
        default:
            std::cout << "Unknown size";
        break;
    }
    std::cout << std::endl;
}

int get_number_rom_banks(uint8_t rom_flag) {
    switch(rom_flag) {
        case 0x00:
            return 0;
        break;
        case 0x01:
            return 4;
        break;
    }

    return 0;
}

int main(int argc, char** argv) {
    if(argc != 3) {
        std::cerr << "Usage: " << argv[0] << "<PORT> <DUMP.gb>" << std::endl;
        return -1;
    }

    std::string _port(argv[1]);
    std::string _file(argv[2]);

    // open serial port and set BAUD rate
    port.open(_port.c_str());
    port.set_option(boost::asio::serial_port_base::baud_rate(57600));

    std::cout << "=========================================" << std::endl;
    std::cout << "GameBoyCartridgeReader v.0.1 by Ivo Filot" << std::endl;
    std::cout << "=========================================" << std::endl;

    // test simple read instruction
    std::cout << "Test cartridge connectivity..." << std::flush;
    std::vector<uint8_t> header;
    read_memory(0x0000, 0x0000, &header);
    std::cout << "DONE" << std::endl;

    // read the ROM header and extract valuable information
    std::cout << "Test reading cartridge header info..." << std::flush;
    read_memory(0x0000, 0x014F, &header);
    std::cout << "DONE" << std::endl;
    std::cout << "=========================================" << std::endl;
    print_header_details(header);

    std::cout << "=========================================" << std::endl;

    std::vector<uint8_t> data; // hold complete data

    if(header[0x0148] == 0x00) {
        // read the complete ROM
        size_t bytes = read_memory(0x0000, 0x8000, &data, true);
        std::cout << "Done reading " << bytes << " bytes from ROM.        " << std::endl;
    } else {
        size_t bytes = 0;
        for(uint8_t i=1; i<4; i++) {
            change_rom_bank(i, true); // false suppress output
            std::cout << "Reading ROM BANK " << (int)i << "... please wait" << std::endl;
            if(i == 1) {
                // read the first 16kb + the first rom bank (total 32kb)
                bytes += read_memory(0x0000, 0x8000, &data, true);
            } else {
                bytes += read_memory(0x4000, 0x4000, &data, true);
            }
            std::cout << std::endl;
        }

        std::cout << "Done reading " << bytes << " bytes from ROM.        " << std::endl;
    }

    // close and clean-up
    port.close();

    // store ROM data into file
    std::ofstream out(_file.c_str());
    for(auto v: data) {
        out << v;
    }

    // end of program
    std::cout << "=========================================" << std::endl;
    std::cout << "End of program" << std::endl;

    return 0;
}
