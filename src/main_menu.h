#pragma once
#ifndef CATA_SRC_MAIN_MENU_H
#define CATA_SRC_MAIN_MENU_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "enums.h"
#include "input_context.h"
#include "input_enums.h"
#include "point.h"
#include "worldfactory.h"

class main_menu
{
    public:
        main_menu() : ctxt( "MAIN_MENU", keyboard_mode::keychar ) { }

        bool opening_screen();

        static std::string queued_world_to_load;
        static std::string queued_save_id_to_load;
    private:

        std::vector<std::string> mmenu_title;
        std::string mmenu_motd;
        std::string mmenu_credits;
        int mmenu_motd_len;
        int mmenu_credits_len;
        std::vector<std::string> vMenuItems;
        std::vector<std::string> vWorldSubItems;
        std::vector<std::string> vNewGameSubItems;
        std::vector<std::string> vNewGameHints;
        std::vector< std::vector<std::string> > vWorldHotkeys;
        std::vector<std::string> vSettingsSubItems;
        std::vector< std::vector<std::string> > vSettingsHotkeys;
        std::vector< std::vector<std::string> > vMenuHotkeys;
        std::vector< std::vector<std::string> > vNewGameHotkeys;
        std::string vdaytip;





        void init_strings();

        std::vector<std::string> load_file( const std::string &path,
                                            const std::string &alt_text ) const;


        void on_move() const;


        void on_error();



        bool load_game( std::string const &worldname, save_t const &savegame );
        bool new_character_tab();
        bool load_character_tab( const std::string &worldname );
        void world_tab( const std::string &worldname );




        void load_char_templates();



        input_context ctxt;
        int sel1 = 1;
        int sel2 = 1;
        size_t last_world_pos = 0;
        int sub_opt_off = 0;
        point LAST_TERM;
        catacurses::window w_open;
        point menu_offset;
        std::vector<std::string> templates;
        int extra_w = 0;
        std::vector<save_t> savegames;
        std::vector<std::pair<inclusive_rectangle<point>, std::pair<int, int>>> main_menu_sub_button_map;
        std::vector<std::pair<inclusive_rectangle<point>, int>> main_menu_button_map;












        std::vector<int> print_menu_items( const catacurses::window &w_in,
                                           const std::vector<std::string> &vItems, size_t iSel,
                                           point offset, int spacing = 1, bool main = false );








        void print_menu( const catacurses::window &w_open, int iSel, const point &offset, int sel_line );

        void display_text( const std::string &text, const std::string &title, int &selected );

        void display_sub_menu( int sel, const point &bottom_left, int sel_line );

        void init_windows();

        holiday get_holiday_from_time();

        holiday current_holiday = holiday::none;

        static std::string halloween_spider();
        std::string halloween_graves();
};


void show_game_settings_menu();

#endif
