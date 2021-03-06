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

#include <curses.h>
#include <string>
#include <menu.h>
#include <stdlib.h>
#include <string.h>
#include <tclap/CmdLine.h>
#include <boost/format.hpp>

#include "savegame.h"

#define WHITEONBLUE 1
#define BLACKONWHITE 2
#define WHITEONBLACK 3
#define REDONWHITE 4
#define WHITEONRED 5

/* DEFINE APP WIDE COLORS/ATTRIBS */
#define ATTRIBS  WA_BOLD
#define COLORS WHITEONBLUE

bool initialize_colors() {
    if(has_colors()) {
        start_color();
        init_pair(WHITEONBLUE, COLOR_WHITE, COLOR_BLUE);
        init_pair(BLACKONWHITE, COLOR_BLACK, COLOR_WHITE);
        init_pair(WHITEONBLACK, COLOR_WHITE, COLOR_BLACK);
        init_pair(REDONWHITE, COLOR_RED, COLOR_WHITE);
        init_pair(WHITEONRED, COLOR_WHITE, COLOR_RED);
        return true;
    } else {
        return(false);
    }
}

bool set_colors(int colorscheme) {
    if(has_colors()) {
        attrset(colorscheme);
        return(true);
    } else {
        return(false);
    }
}

void wclrscr(WINDOW * pwin) {
    int y, x, maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);
    for(y=0; y < maxy; y++) {
        for(x=0; x < maxx; x++) {
            mvwaddch(pwin, y, x, ' ');
        }
    }
}

void window_center_title(WINDOW *pwin, const char * title) {
    int x, maxy, maxx, stringsize;
    getmaxyx(pwin, maxy, maxx);
    stringsize = 4 + strlen(title);
    x = (maxx - stringsize)/2;
    mvwaddch(pwin, 0, x, ACS_RTEE);
    waddch(pwin, ' ');
    waddstr(pwin, title);
    waddch(pwin, ' ');
    waddch(pwin, ACS_LTEE);
}

int run_menu(WINDOW* wparent, int height, int width, int y, int x, const std::vector<std::string>& choices) {
    int c;              // the key pressed

    ITEM** my_items;    // list of items on this menu
    MENU* my_menu;      // the menu structure

    WINDOW *wui;
    WINDOW *wborder;

    int n_choices = choices.size();      // number of items on the menu
    int ss_choice;                       // subscript to run around the choices array
    int my_choice = -1;                  // the zero based numeric user choice

    // allocate item array and individual items
    my_items = (ITEM**)calloc(n_choices + 1, sizeof(ITEM*));
    for(ss_choice = 0; ss_choice < n_choices; ++ss_choice) {
        my_items[ss_choice] = new_item(choices[ss_choice].c_str(), NULL);
    }
    my_items[n_choices] = (ITEM*)NULL;

    // create the menu structure and display it
    my_menu = new_menu((ITEM**)my_items);

    // set up windows for menu border
    wborder = newwin(height, width, y, x);
    wattrset(wborder, COLOR_PAIR(WHITEONRED) | WA_BOLD);
    wclrscr(wborder);
    box(wborder, 0, 0);
    window_center_title(wborder, "Choose one");

    // set up windows for the menu's user interface
    wui = derwin(wborder, height-2, width-2, 2, 2);

    // associate windows with the menu
    set_menu_win(my_menu, wborder);
    set_menu_sub(my_menu, wui);

    // match colors
    set_menu_fore(my_menu, COLOR_PAIR(REDONWHITE));
    set_menu_back(my_menu, COLOR_PAIR(WHITEONRED) | WA_BOLD);

    // set up environment conducive to menuing
    keypad(wui, true);
    noecho();
    curs_set(0);

    // display the menu
    post_menu(my_menu);

    // refresh border
    touchwin(wborder);
    wrefresh(wborder);

    // handle user keystrokes
    while(my_choice == -1) {
        touchwin(wui);
        wrefresh(wui);
        c = getch();
        switch(c) {
            case KEY_DOWN:
                menu_driver(my_menu, REQ_DOWN_ITEM);
            break;
            case KEY_UP:
                menu_driver(my_menu, REQ_UP_ITEM);
            break;
            case KEY_NPAGE:
                menu_driver(my_menu, REQ_SCR_DPAGE);
            break;
            case KEY_PPAGE:
                menu_driver(my_menu, REQ_SCR_UPAGE);
            break;
            case 10: // enter
                my_choice = item_index(current_item(my_menu));
                pos_menu_cursor(my_menu);
            break;
        }
    }

    // free all allocated resources
    free_item(my_items[0]);
    free_item(my_items[1]);
    free_menu(my_menu);

    // free all allocated menu and menu items
    unpost_menu(my_menu);
    for(ss_choice = 0; ss_choice < n_choices; ++ss_choice) {
        free_item(my_items[ss_choice]);
    }
    free_menu(my_menu);

    // destroy menu window and border windows
    delwin(wui);
    delwin(wborder);

    // undo menu environment
    curs_set(1);

    // repaint calling screen
    touchwin(wparent);
    wrefresh(wparent);

    // return zero based numeric user choice
    return my_choice;
}

void draw_window(WINDOW* win, const std::string& title) {
    box(stdscr, ACS_VLINE, ACS_HLINE);

    int x, maxy, maxx;
    getmaxyx(win, maxy, maxx);

    x = (maxx - 4 - title.size()) / 2;
    mvwaddch(win, 0, x, ACS_RTEE);
    waddch(win, ' ');
    waddstr(win, title.c_str());
    waddch(win, ' ');
    waddch(win, ACS_LTEE);

    touchwin(stdscr);
    wrefresh(stdscr);
}

void draw_pokemon_list(WINDOW* wparent, const SaveGame& sg) {
    int maxy, maxx, mypadpos = 0;
    getmaxyx(wparent, maxy, maxx);

    WINDOW* wborder = newwin(maxy - 10, maxx - 10, 5, 5);
    wattrset(wborder, COLOR_PAIR(WHITEONRED) | WA_BOLD);
    wclrscr(wborder);
    box(wborder, 0, 0);
    window_center_title(wborder, "Pokemon seen");

    // refresh border
    touchwin(wborder);
    wrefresh(wborder);

    WINDOW* pad = newpad(151, maxx - 12);
    wattrset(pad, COLOR_PAIR(WHITEONRED) | WA_BOLD);

    auto pokemon_seen = sg.get_pokemon_seen();
    for(unsigned int i=0; i<151; i++) {
        std::string line = (boost::format("%03i %s - %s")
                            % (i+1)
                            % get_pokemon_name(i)
                            % (pokemon_seen.test(i) ? "SEEN" : "")
                           ).str();
        line.resize((maxx - 13), ' ');
        line.append("\n");
        wprintw(pad, line.c_str());
    }

    int c = 0;
    int maxpadpos = 151 - (maxy - 12);
    while(c != 10 ) {
        switch(c) {
            case KEY_UP:
                mypadpos--;
            break;
            case KEY_DOWN:
                mypadpos++;
            break;
            case KEY_PPAGE:
                mypadpos-= maxy - 7;
            break;
            case KEY_NPAGE:
                mypadpos+= maxy - 7;
            break;
        }

        mypadpos = std::min(mypadpos, maxpadpos);
        mypadpos = std::max(mypadpos, 0);
        prefresh(pad, mypadpos, 0, 6, 6, maxy - 7, maxx - 2);
        touchwin(pad);

        c = getch();
    }
}

int main(int argc, char *argv[]) {
    try {

        TCLAP::CmdLine cmd("Edit a pokemon save file.", ' ', "0.1");

        // output file
        TCLAP::ValueArg<std::string> arg_input_filename("i","input","Input file (i.e. rom.sav)",true,"__NONE__","filename");
        cmd.add(arg_input_filename);

        cmd.parse(argc, argv);

        const std::string input_filename = arg_input_filename.getValue();

        SaveGame sg(input_filename);

        std::vector<std::string> choices = {
            "Show pokemon seen",
            "Exit"
        };

        int choiceno = 0;

        initscr();
        cbreak();
        noecho();
        keypad(stdscr, true);
        initialize_colors();

        // set up standard screen
        wattrset(stdscr, COLOR_PAIR(WHITEONBLUE) | WA_BOLD);
        wclrscr(stdscr);
        draw_window(stdscr, "Pokemon Editor");
        mvwaddstr(stdscr, 1, 1, (std::string("Player: ") + sg.get_player_name()).c_str());
        mvwaddstr(stdscr, 2, 1, (boost::format("Pokemon seen: %i") % sg.get_pokemon_seen().count()).str().c_str());

        const int border = 10;
        int x, maxy, maxx;
        getmaxyx(stdscr, maxy, maxx);

        while(choiceno != choices.size() - 1) {
            choiceno = run_menu(stdscr, maxy - border, maxx - border, border / 2, border / 2, choices);

            switch(choiceno) {
                case 0:
                    draw_pokemon_list(stdscr, sg);
                break;
            }
        }

        endwin();

        return 0;

    } catch (TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() <<
                     " for arg " << e.argId() << std::endl;
        return -1;
    }
}

