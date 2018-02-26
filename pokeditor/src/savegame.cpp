/**************************************************************************
 *   This file is part of GBCR.                                           *
 *                                                                        *
 *   Copyright (C) 2018, Ivo Filot                                        *
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

#include "savegame.h"

SaveGame::SaveGame(const std::string& _filename) :
filename(_filename)
{
    this->load_from_file(this->data, this->filename);
}

std::string SaveGame::get_player_name() const {
    std::vector<uint8_t> player_name;
    std::string name;

    for(size_t i=0x2598; i<0x2598+11; i++) {
        player_name.push_back(this->data[i]);
    }

    // transfer back to characters
    for(uint8_t chr : player_name) {
        if(chr >= 0x80 && chr <= 0x99) {
            name += ((char)(chr - (0x80 - 0x41)));
        }
    }

    return name;
}

std::bitset<151> SaveGame::get_pokemon_seen() const {
    std::bitset<151> pokemon_seen;

    for(unsigned int i=0; i<151; i++) {
        if(this->data[0x25B6 + (i >> 3)] >> (i & 7) & 1) {
            pokemon_seen.set(i);
        } else {
            pokemon_seen.reset(i);
        }
    }

    return pokemon_seen;
}

uint8_t SaveGame::calculate_checksum() const {
    uint8_t checksum = 0;

    for(uint16_t i = 0x2598; i <= 0x3522; i++) {
        checksum += this->data[i];
    }

    return ~checksum;
}

/**
 * @brief      Loads from file.
 *
 * @param      data   The data
 * @param[in]  input  The input
 */
void SaveGame::load_from_file(std::vector<uint8_t>& data, const std::string& input) {
    std::ifstream in(input.c_str());
    char chr;

    while(in.get(chr)) {
        data.push_back((uint8_t)chr);
    }
}
