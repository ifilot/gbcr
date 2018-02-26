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

#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <tclap/CmdLine.h>

#include "gameboy_cartridge.h"

int main(int argc, char** argv) {
    try {

        TCLAP::CmdLine cmd("Read or write to gameboy cartridge.", ' ', "0.3");

        // output file
        TCLAP::ValueArg<std::string> arg_output_filename("o","output","Output file (i.e. rom.gb)",true,"__NONE__","filename");
        cmd.add(arg_output_filename);

        // port
        TCLAP::ValueArg<std::string> arg_port("p","port","Port (i.e. /dev/ttyUSB1)",true,"__NONE__","port");
        cmd.add(arg_port);

        // whether to store ram
        TCLAP::SwitchArg arg_ram("r","ram","ram",false);
        cmd.add(arg_ram);

        // whether to load ram
        TCLAP::SwitchArg arg_load("l","load","load",false);
        cmd.add(arg_load);

        cmd.parse(argc, argv);

        const std::string port_url = arg_port.getValue();
        const std::string filename = arg_output_filename.getValue();
        const bool ram = arg_ram.getValue();
        const bool load = arg_load.getValue();

        std::cout << "=========================================" << std::endl;
        std::cout << "GameBoyCartridgeReader v.0.3 by Ivo Filot" << std::endl;
        std::cout << "=========================================" << std::endl;

        GameboyCartridge gbc(port_url);
        gbc.init();

        if(ram && !load) {
            gbc.read_ram(filename);
        } else if(load) {
            gbc.load_ram(filename);
        } else {
            gbc.read_rom(filename);
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
