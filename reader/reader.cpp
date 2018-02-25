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
#include <tclap/CmdLine.h>

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

int write_command_word(const char* cmd) {
    // create buffer
    char c[12];

    for(unsigned int i=0; i<12; i++) {
        std::cout << cmd[i];
    }
    std::cout << std::endl;

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

int change_rom_bank(uint8_t type, uint8_t bank_addr, bool output) {
    if(output) {
        std::cout << "Changing to ROM BANK: " << (int)bank_addr << "  " << std::endl;
    }

    switch(type) {
        case 0x13: {
            // write lower 5 bits
            char cmd1[13] = {'W', 'R', 'B', 'Y', '0', '0', '0', '0', 'X', 'X', 'X', 'X','0'};
            uint8_t low5bit = bank_addr & 31;
            sprintf(&cmd1[4], "%04X%04X", 0x2100, low5bit);
            write_command_word(cmd1);

            // write higher 2 bits
            char cmd2[13] = {'W', 'R', 'B', 'Y', '0', '0', '0', '0', 'X', 'X', 'X', 'X','0'};
            uint8_t higher2bit = bank_addr >> 5;
            sprintf(&cmd2[4], "%04X%04X", 0x4000, higher2bit);
            //write_command_word(cmd2);
        }
        break;
        default: {
            char cmd[13] = {'W', 'R', 'B', 'Y', '0', '0', '0', '0', 'X', 'X', 'X', 'X','0'};
            sprintf(&cmd[4], "%04X%04X", 0x2100, bank_addr);
            write_command_word(cmd);
        }
        break;
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

    write_command_word(cmd);

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
        case 0x03:
            std::cout << "MBC1+RAM+BATTERY";
        break;
        case 0x05:
            std::cout << "MBC2";
        break;
        case 0x06:
            std::cout << "MBC2+BATTERY";
        break;
        case 0x08:
            std::cout << "ROM+RAM";
        break;
        case 0x09:
            std::cout << "ROM+RAM+BATTERY";
        break;
        case 0x0F:
            std::cout << "MBC3+TIMER+BATTERY";
        break;
        case 0x10:
            std::cout << "MBC3+TIMER+RAM+BATTERY";
        break;
        case 0x11:
            std::cout << "MBC3";
        break;
        case 0x12:
            std::cout << "MBC3+RAM";
        break;
        case 0x13:
            std::cout << "MBC3+RAM+BATTERY";
        break;
        default:
            std::cout << "Unknown type: " << (int)header[0x147];
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
            std::cout << "256KByte (16 banks)";
        break;
        case 0x04:
            std::cout << "512KByte (32 banks)";
        break;
        case 0x05:
            std::cout << "1MByte (64 banks)";
        break;
        case 0x06:
            std::cout << "2MByte (128 banks)";
        break;
        case 0x07:
            std::cout << "4MByte (256 banks)";
        break;
        default:
            std::cout << "Unknown size: " << (int)header[0x148];
        break;
    }
    std::cout << std::endl;
}

/*
 * get_number_rom_banks
 *
 * Get number of ROM banks based on header byte
 *
 * @param rom_flag - rom size header byte
 *
 * @return number of ROM banks
 *
 */
int get_number_rom_banks(uint8_t rom_flag) {
    switch(rom_flag) {
        case 0x00:
            return 0;
        break;
        case 0x01:
            return 4;
        break;
        case 0x02:
            return 8;
        break;
        case 0x03:
            return 16;
        break;
        case 0x04:
            return 32;
        break;
        case 0x05:
            return 64;
        break;
        case 0x06:
            return 128;
        break;
        case 0x07:
            return 256;
        break;
    }

    return 0;
}

int main(int argc, char** argv) {
    try {

        TCLAP::CmdLine cmd("Perform pattern recognition on particle.", ' ', "0.1");

        // output file
        TCLAP::ValueArg<std::string> arg_output_filename("o","output","Output file (i.e. rom.gb)",true,"__NONE__","filename");
        cmd.add(arg_output_filename);

        // port
        TCLAP::ValueArg<std::string> arg_port("p","port","Port (i.e. /dev/ttyUSB1)",true,"__NONE__","port");
        cmd.add(arg_port);

        cmd.parse(argc, argv);

        const std::string port_url = arg_port.getValue();
        const std::string filename = arg_output_filename.getValue();

        // open serial port and set BAUD rate
        port.open(port_url.c_str());
        port.set_option(boost::asio::serial_port_base::baud_rate(57600));

        std::cout << "=========================================" << std::endl;
        std::cout << "GameBoyCartridgeReader v.0.2 by Ivo Filot" << std::endl;
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
            for(uint8_t i=1; i<get_number_rom_banks(header[0x148]); i++) {
                change_rom_bank(header[0x0147], i, true); // false suppress output
                if(i == 2) {
                    break;
                }
                if(i == 1) {
                    // read the first 16kb + the first rom bank (total 32kb)
                    std::cout << "Reading ROM BANKS 0+1... please wait" << std::endl;
                    bytes += read_memory(0x0000, 0x8000, &data, true);
                } else {
                    std::cout << "Reading ROM BANK " << (int)i << "... please wait" << std::endl;
                    bytes += read_memory(0x4000, 0x4000, &data, true);
                }
                std::cout << std::endl;
            }

            std::cout << "Done reading " << bytes << " bytes from ROM.        " << std::endl;
        }

        // close and clean-up
        port.close();

        // store ROM data into file
        std::ofstream out(filename.c_str());
        for(auto v: data) {
            out << v;
        }

        // end of program
        std::cout << "=========================================" << std::endl;
        std::cout << "End of program" << std::endl;

        return 0;

    } catch (TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() <<
                     " for arg " << e.argId() << std::endl;
        return -1;
    }
}
