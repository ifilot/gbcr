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

#ifndef _GAMEBOY_CARTRIDGE
#define _GAMEBOY_CARTRIDGE

#include <boost/asio/serial_port.hpp>
#include <boost/asio.hpp>

#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <chrono>

class GameboyCartridge {
private:
    std::vector<uint8_t> header;
    std::vector<uint8_t> rom_data;
    std::vector<uint8_t> ram_data;

    uint8_t cartridge_type;
    uint8_t nrbanks;

    boost::asio::io_service io;
    boost::asio::serial_port port;

    const size_t BAUD_RATE = 57600;

    std::string port_url;

public:

    /**
     * @brief      default constructor
     *
     * @param[in]  _port_url  path to /dev/ttyUSBx
     */
    GameboyCartridge(const std::string& _port_url);

    /**
     * @brief      load cartridge information
     */
    void init();

    /**
     * @brief      load sram into cartridge from file
     *
     * @param[in]  input_file  Input file
     */
    void load_ram(const std::string& input_file);

    /**
     * @brief      read sram from cartridge
     *
     * @param[in]  output_file  The output file
     */
    void read_ram(const std::string& output_file);

    /**
     * @brief      read rom from cartridge
     *
     * @param[in]  output_file  The output file
     */
    void read_rom(const std::string& output_file);

    /**
     * @brief      Destroys the object.
     */
    ~GameboyCartridge();

private:

    /**
     * @brief      writes a command word to ATMEGA
     *
     * @param[in]  cmd   command word
     *
     */
    void write_command_word(const char* cmd);

    /**
     * @brief      enables or disables RAM banking
     *
     * @param[in]  enable  whether to enable or disable
     */
    void set_ram(bool enable);

    /**
     * @brief      change ram bank number
     *
     * @param[in]  bank_addr  ram bank number
     */
    void change_ram_bank(uint8_t bank_addr);

    /**
     * @brief      change rom bank number
     *
     * @param[in]  bank_addr  rom bank number
     */
    void change_rom_bank(uint8_t bank_addr);

    /*
     * read_memory
     *
     * @param _addr  - starting address
     * @param _len   - number of bytes to read
     * @param buffer - pointer to vector to store data
     *
     * return number of bytes read
     */
    size_t read_memory(uint16_t _addr, uint16_t _len, std::vector<uint8_t> *buffer, bool show_progress = false);

    /*
     * @brief      Print information from the ROM header to the screen
     */
    void print_header_details();

    /**
     * @brief      calculate number of rom banks
     *
     * @return     The number rom banks.
     */
    int get_number_rom_banks();

    /*
     * @brief      print_loadbar
     *
     * @param      x     - iterator
     * @param      n     - total number of iterations
     * @param      w     - width of the progress bar
     */
    void print_loadbar(unsigned int x, unsigned int n, unsigned int w = 35);

    /**
     * @brief      Writes a byte.
     *
     * @param[in]  addr  address to write byte to
     * @param[in]  val   byte value
     */
    void write_byte(uint16_t addr, uint8_t val);

    /**
     * @brief      Writes to file.
     *
     * @param[in]  data     The data
     * @param[in]  outfile  The outfile
     */
    void write_to_file(const std::vector<uint8_t>& data, const std::string& outfile);

    /**
     * @brief      Loads from file.
     *
     * @param      data   The data
     * @param[in]  input  The input
     */
    void load_from_file(std::vector<uint8_t>& data, const std::string& input);
};

#endif
