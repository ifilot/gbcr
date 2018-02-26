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

#ifndef _SAVEGAME_H
#define _SAVEGAME_H

#include <vector>
#include <string>
#include <fstream>
#include <bitset>
#include <iostream>

#include "pokemon.h"

class SaveGame {
private:
    std::vector<uint8_t> data;
    std::string filename;

public:
    SaveGame(const std::string& _filename);

    std::string get_player_name() const;

    std::bitset<151> get_pokemon_seen() const;

    uint8_t calculate_checksum() const;

private:
    void load_from_file(std::vector<uint8_t>& data, const std::string& input);
};

#endif // _SAVEGAME_H
