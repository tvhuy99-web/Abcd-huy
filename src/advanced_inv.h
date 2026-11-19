#pragma once
#ifndef CATA_SRC_ADVANCED_INV_H
#define CATA_SRC_ADVANCED_INV_H

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "advanced_inv_area.h"
#include "advanced_inv_pane.h"
#include "cursesdef.h"

class Character;
class advanced_inv_listitem;
class drop_or_stash_item_info;
class input_context;
class string_input_popup;
class ui_adaptor;
struct advanced_inv_save_state;

void create_advanced_inv();





void cancel_aim_processing();

class advanced_inventory
{
    public:
        advanced_inventory();
        ~advanced_inventory();

        void display();
        void temp_hide();

        void init();

        void process_action( const std::string &input_action );




        aim_location screen_relative_location( aim_location area );
        std::string get_location_key( aim_location area );

        advanced_inv_area &get_one_square( const aim_location &loc ) {
            return squares[loc];
        }




        enum side {
            left  = 0,
            right = 1,
            NUM_PANES = 2
        };

        void recalc_pane( side p );

        side get_src() {
            return src;
        }
        side get_dest() {
            return dest;
        }

        advanced_inventory_pane &get_pane( side side ) {
            return panes[side];
        }
    private:

        static constexpr int head_height = 5;
        bool move_all_items_and_waiting_to_quit = false;

        std::unique_ptr<ui_adaptor> ui;
        std::unique_ptr<string_input_popup> spopup;




        class reader_session;


        void swap_panes();


        catacurses::window minimap;
        catacurses::window mm_border;
        const int minimap_width  = 3;
        const int minimap_height = 3;
        void draw_minimap();
        void refresh_minimap();
        char get_minimap_sym( side p ) const;

        bool inCategoryMode = false;

        int linesPerPage = 0;
        int w_height = 0;
        int w_width = 0;

        int headstart = 0;
        int colstart = 0;

        bool recalc = false;
        bool always_recalc = false;



        side src;



        side dest;




        bool filter_edit = false;




        std::array<advanced_inventory_pane, NUM_PANES> panes;
        static const advanced_inventory_pane null_pane;
        std::array<advanced_inv_area, NUM_AIM_LOCATIONS> squares;

        catacurses::window head;

        bool exit = false;

        advanced_inv_save_state *save_state;




        input_context register_ctxt() const;



        void start_activity( aim_location destarea, aim_location srcarea,
                             advanced_inv_listitem *sitem, int &amount_to_move,
                             bool from_vehicle, bool to_vehicle ) const;




        bool action_move_item( advanced_inv_listitem *sitem,
                               advanced_inventory_pane &dpane, const advanced_inventory_pane &spane,
                               const std::string &action );

        void action_examine( advanced_inv_listitem *sitem, advanced_inventory_pane &spane );

        bool action_unload( advanced_inv_listitem *sitem, advanced_inventory_pane &spane,
                            advanced_inventory_pane &dpane );


        void save_settings( bool only_panes );
        void load_settings();





        void do_return_entry();


        bool is_processing() const;

        static std::string get_sortname( advanced_inv_sortby sortby );
        void print_items( side p, bool active );

        void redraw_pane( side p );
        void redraw_sidebar();

        bool move_all_items();



        bool fill_lists_with_pane_items( Character &player_character, advanced_inv_sortby sort_priority,
                                         advanced_inventory_pane &spane, advanced_inventory_pane &dpane,
                                         std::vector<drop_or_stash_item_info> &item_list,
                                         std::vector<drop_or_stash_item_info> &fav_list, bool forbid_buckets );



        int print_header( advanced_inventory_pane &pane, aim_location sel );









        bool get_square( const std::string &action, aim_location &ret );
        void change_square( aim_location changeSquare, advanced_inventory_pane &dpane,
                            advanced_inventory_pane &spane );




        bool show_sort_menu( advanced_inventory_pane &pane );








        bool query_destination( aim_location &def );











        bool query_charges( aim_location destarea, const advanced_inv_listitem &sitem,
                            const std::string &action, int &amount );
};

#endif
