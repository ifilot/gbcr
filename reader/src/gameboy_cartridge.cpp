/**************************************************************************
 *   This file is part of GBCR.                                           *
 *                                                                        *
 *   Copyright (C) 2017, Ivo Filot                                        *
 *                                                                        *
 *   GBCR is free software: you can redistribute it and/or modify         *
 *   it under the terms of the GNU General Public License as published    *
 *   by the Free Software Foundation, either version 3 of the License,    *
 *   or (at your option) any later version.                               *
 *                                                                        *
 *   GBCR is distributed in the hope that it will be useful,              *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include "gameboy_cartridge.h"

/**
 * @brief      default constructor
 *
 * @param[in]  _port_url  path to /dev/ttyUSBx
 */
GameboyCartridge::GameboyCartridge(const std::string& _port_url) :
    port(io)    // initialize port upon construction
{
    this->port_url = _port_url;
    port.open(this->port_url.c_str());
    port.set_option(boost::asio::serial_port_base::baud_rate(this->BAUD_RATE));
}

/**
 * @brief      load cartridge information
 */
void GameboyCartridge::init() {
    // test simple read instruction
    std::cout << "Test cartridge connectivity..." << std::flush;
    std::vector<uint8_t> header;
    this->read_memory(0x0000, 0x0000, &this->header);
    std::cout << "DONE" << std::endl;

    // read the ROM header and extract valuable information
    std::cout << "Test reading cartridge header info..." << std::flush;
    this->read_memory(0x0000, 0x014F, &this->header);
    std::cout << "DONE" << std::endl;
    std::cout << "=========================================" << std::endl;
    this->print_header_details();

    std::cout << "=========================================" << std::endl;

    this->cartridge_type = this->header[0x0147];
    this->nrbanks = this->get_number_rom_banks();
}

/**
 * @brief      load sram into cartridge from file
 *
 * @param[in]  input_file  Input file
 */
void GameboyCartridge::load_ram(const std::string& input_file) {
    size_t bytes = 0;
    this->ram_data.clear();
    char c[2];
    char rec[2];
    c[1] = '\0';

    this->load_from_file(this->ram_data, input_file);
    std::cout << this->ram_data.size() << std::endl;

    if(this->cartridge_type == 0x13) {
        size_t size = 4 * 8 * 1024;
        this->set_ram(true);

        for(unsigned int i=0; i<4; i++) {
            this->change_ram_bank(i);

            // give instruction that payload is coming
            char cmd[13] = {'W', 'R', 'I', 'T', 'E', 'R', 'A', 'M', 'X', 'X', 'X', 'X','0'};
            sprintf(&cmd[8], "%04X", 0x2000);
            write_command_word(cmd);

            // deliver payload
            for(uint16_t j = 0; j<0x2000; j++) {
                c[0] = this->ram_data[bytes];

                boost::asio::write(port, boost::asio::buffer(&c[0], 1));
                boost::asio::read(port, boost::asio::buffer(&rec,1));

                if(c[0] != rec[0]) {
                    std::cerr << "An error occurred during data transfer, aborting!" << std::endl;
                    std::cerr << "Error encountered at " << __FILE__ << " (" << __LINE__ << ")" << std::endl;
                    std::cerr << "Send: " << cmd[i] << " | Receive: " << c[0] << "| i=" << i << std::endl;
                    exit(-1);
                }

                bytes++;
                print_loadbar(bytes, size);
            }

            std::cout << std::endl;
        }

        this->set_ram(false);
    }

    std::cout << bytes << " bytes loaded into cartridge." << std::endl;
}

/**
 * @brief      read sram from cartridge
 *
 * @param[in]  output_file  The output file
 */
void GameboyCartridge::read_ram(const std::string& output_file) {
    size_t bytes = 0;
    this->ram_data.clear();
    auto start = std::chrono::system_clock::now();

    if(this->cartridge_type == 0x13 || this->cartridge_type == 0x1B) {
        this->set_ram(true);

        for(unsigned int i=0; i<4; i++) {
            this->change_ram_bank(i);
            bytes += this->read_memory(0xA000, 0x2000, &this->ram_data, true);
            std::cout << std::endl;
        }

        this->set_ram(false);
    }

    // calculate time
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;

    // write ram to file
    this->write_to_file(this->ram_data, output_file);

    std::cout << "Done reading " << bytes << " bytes from RAM in " << elapsed_seconds.count() << " seconds." << std::endl;
}

/**
 * @brief      read rom from cartridge
 *
 * @param[in]  output_file  The output file
 */
void GameboyCartridge::read_rom(const std::string& output_file) {
    size_t bytes = 0;
    auto start = std::chrono::system_clock::now();

    if(this->cartridge_type == 0x00) {
        // read the complete ROM
        size_t bytes = this->read_memory(0x0000, 0x8000, &this->rom_data, true);
    } else {
        for(uint8_t i=1; i<this->nrbanks; i++) {
            this->change_rom_bank(i); // false suppress output
            if(i == 1) {
                // read the first 16kb + the first rom bank (total 32kb)
                std::cout << "Reading ROM BANKS 0+1... please wait" << std::endl;
                bytes += this->read_memory(0x0000, 0x8000, &this->rom_data, true);
            } else {
                std::cout << "Reading ROM BANK " << (int)i << "... please wait" << std::endl;
                bytes += this->read_memory(0x4000, 0x4000, &this->rom_data, true);
            }
            std::cout << std::endl;
        }
    }

    // calculate time
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;

    // write rom to file
    this->write_to_file(this->rom_data, output_file);

    std::cout << "Done reading " << bytes << " bytes from ROM in " << elapsed_seconds.count() << " seconds." << std::endl;
}

/**
 * @brief      Destroys the object.
 */
GameboyCartridge::~GameboyCartridge() {
    this->port.close();
}

/**
 * @brief      writes a command word to ATMEGA
 *
 * @param[in]  cmd   command word
 *
 */
void GameboyCartridge::write_command_word(const char* cmd) {
    char c[12]; // create buffer

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

/**
 * @brief      enables or disables RAM banking
 *
 * @param[in]  enable  whether to enable or disable
 */
void GameboyCartridge::set_ram(bool enable) {
    if(enable) {
        std::cout << "Enable RAM BANK" << std::endl;
    } else {
        std::cout << "Disable RAM BANK" << std::endl;
    }

    char cmd[13] = {'W', 'R', 'B', 'Y', '0', '0', '0', '0', 'X', 'X', 'X', 'X','0'};

    if(enable) {
        sprintf(&cmd[4], "%04X%04X", 0x0000, 0x0A);
    } else {
        sprintf(&cmd[4], "%04X%04X", 0x0000, 0x00);
    }

    this->write_command_word(cmd);
}

/**
 * @brief      change ram bank number
 *
 * @param[in]  bank_addr  ram bank number
 */
void GameboyCartridge::change_ram_bank(uint8_t bank_addr) {
    std::cout << "Changing to RAM BANK: " << (int)bank_addr << "  " << std::endl;

    char cmd[13] = {'W', 'R', 'B', 'Y', '0', '0', '0', '0', 'X', 'X', 'X', 'X','0'};

    sprintf(&cmd[4], "%04X%04X", 0x4000, bank_addr);
    this->write_command_word(cmd);
}

/**
 * @brief      change rom bank number
 *
 * @param[in]  bank_addr  rom bank number
 */
void GameboyCartridge::change_rom_bank(uint8_t bank_addr) {
    std::cout << "Changing to ROM BANK: " << (int)bank_addr << "  " << std::endl;

    char cmd[13] = {'W', 'R', 'B', 'Y', '0', '0', '0', '0', 'X', 'X', 'X', 'X','0'};

    if(this->cartridge_type >= 5) {
        sprintf(&cmd[4], "%04X%04X", 0x2100, bank_addr);
        this->write_command_word(cmd);
    } else {
        sprintf(&cmd[4], "%04X%04X", 0x6000, 0x00);
        this->write_command_word(cmd);
        sprintf(&cmd[4], "%04X%04X", 0x4000, bank_addr >> 5);
        this->write_command_word(cmd);
        sprintf(&cmd[4], "%04X%04X", 0x2100, bank_addr & 0x1F);
        this->write_command_word(cmd);
    }
}

/*
 * @brief      read_memory
 *
 * @param      _addr          starting address
 * @param      _len           number of bytes to read
 * @param      buffer         pointer to vector to store data
 * @param[in]  show_progress  The show progress
 *
 * @return     number of bytes read
 */
size_t GameboyCartridge::read_memory(uint16_t _addr, uint16_t _len, std::vector<uint8_t> *buffer, bool show_progress) {
    // create buffer
    char c[12];

    char cmd[12] = {'R', 'E', 'A', 'D', '0', '0', '0', '0', '0', '0', '0', '0'};
    sprintf(&cmd[4], "%04X%04X", _addr, _len);

    this->write_command_word(cmd);

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
 * @brief      Print information from the ROM header to the screen
 */
void GameboyCartridge::print_header_details() {
    std::string title;

    for(unsigned int i=0x0134; i<0x0143; i++) {
        title += (char)this->header[i];
    }

    std::cout << "Cartridge title: " << title << std::endl;

    std::cout << "Cartridge type:  ";
    switch(this->header[0x0147]) {
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
        case 0x19:
            std::cout << "MBC5";
        break;
        case 0x1A:
            std::cout << "MBC5+RAM";
        break;
        case 0x1B:
            std::cout << "MBC5+RAM+BATTERY";
        break;
        default:
            std::cout << "Unknown type: " << (int)header[0x147];
        break;
    }
    std::cout << " (" << (int)this->header[0x0147] << ")";
    std::cout << std::endl;

    std::cout << "ROM Size:        ";
    switch(this->header[0x0148]) {
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
            std::cout << "Unknown size: " << (int)this->header[0x148];
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
int GameboyCartridge::get_number_rom_banks() {
    switch(this->header[0x0148]) {
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

/*
 * print_loadbar
 *
 * simple progress bar to show
 *
 * @param x - iterator
 * @param n - total number of iterations
 * @param w - width of the progress bar
 */
void GameboyCartridge::print_loadbar(unsigned int x, unsigned int n, unsigned int w) {
    if ( (x != n) && (x % (n/100+1) != 0) ) return;

    float ratio  =  x/(float)n;
    int   c      =  ratio * w;

    std::cout << std::setw(3) << (int)(ratio*100) << "% [";
    for (int x=0; x<c; x++) std::cout << "=";
    for (int x=c; x<w; x++) std::cout << " ";
    std::cout << "]\r" << std::flush;
}

/**
 * @brief      Writes a byte.
 *
 * @param[in]  addr  address to write byte to
 * @param[in]  val   byte value
 */
void GameboyCartridge::write_byte(uint16_t addr, uint8_t val) {
    char cmd[13] = {'W', 'R', 'B', 'Y', '0', '0', '0', '0', 'X', 'X', 'X', 'X','0'};

    sprintf(&cmd[4], "%04X%04X", addr, val);
    this->write_command_word(cmd);
}

/**
 * @brief      Writes to file.
 *
 * @param[in]  data     The data
 * @param[in]  outfile  The outfile
 */
void GameboyCartridge::write_to_file(const std::vector<uint8_t>& data, const std::string& outfile) {
    // store ROM data into file
    std::ofstream out(outfile.c_str());
    for(auto v: data) {
        out << v;
    }
    out.close();
}

/**
 * @brief      Loads from file.
 *
 * @param      data   The data
 * @param[in]  input  The input
 */
void GameboyCartridge::load_from_file(std::vector<uint8_t>& data, const std::string& input) {
    std::ifstream in(input.c_str());
    char chr;

    while(in.get(chr)) {
        data.push_back((uint8_t)chr);
    }
}
