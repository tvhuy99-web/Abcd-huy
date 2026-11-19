#pragma once
#ifndef CATA_SRC_ADVANCED_INV_PANE_H
#define CATA_SRC_ADVANCED_INV_PANE_H

#include <array>
#include <functional>
#include <string>
#include <vector>

#include "advanced_inv_area.h"
#include "advanced_inv_listitem.h"
#include "cursesdef.h"
#include "item_location.h"
#include "units_fwd.h"

class item;
struct advanced_inv_pane_save_state;

enum advanced_inv_sortby {
    SORTBY_NONE,
    SORTBY_NAME,
    SORTBY_WEIGHT,
    SORTBY_VOLUME,
    SORTBY_DENSITY,
    SORTBY_CHARGES,
    SORTBY_CATEGORY,
    SORTBY_DAMAGE,
    SORTBY_AMMO,
    SORTBY_SPOILAGE,
    SORTBY_PRICE,
    SORTBY_PRICEPERVOLUME,
    SORTBY_PRICEPERWEIGHT,
    SORTBY_STACKS
};




class advanced_inventory_pane
{
    private:
        aim_location area = NUM_AIM_LOCATIONS;
        aim_location prev_area = area;

        bool viewing_cargo = false;
        bool prev_viewing_cargo = false;
    public:

        void set_area( const advanced_inv_area &square, bool in_vehicle_cargo = false );
        void restore_area();
        aim_location get_area() const {
            return area;
        }
        bool prev_in_vehicle() const {
            return prev_viewing_cargo;
        }
        bool in_vehicle() const {
            return viewing_cargo;
        }
        advanced_inv_pane_save_state *save_state;
        void save_settings() const;
        void load_settings( int saved_area_idx,
                            const std::array<advanced_inv_area, NUM_AIM_LOCATIONS> &squares, bool is_re_enter );



        int index = 0;
        advanced_inv_sortby sortby = advanced_inv_sortby::SORTBY_NONE;
        catacurses::window window;
        std::vector<advanced_inv_listitem> items;



        bool recalc = false;
        item_location target_item_after_recalc;




        item_location container;



        aim_location container_base_loc = NUM_AIM_LOCATIONS;



        int other_cont = -1;

        void add_items_from_area( advanced_inv_area &square, bool vehicle_override = false );



        void fix_index();




        bool is_filtered( const advanced_inv_listitem &it ) const;



        bool is_filtered( const item &it ) const;




        void scroll_by( int offset );





        void scroll_page( int linesPerPage, int offset );




        void scroll_category( int offset );
        void scroll_to_start();
        void scroll_to_end();




        advanced_inv_listitem *get_cur_item_ptr();



        units::volume free_volume( const advanced_inv_area &square ) const;



        units::mass free_weight_capacity() const;



        void set_filter( const std::string &new_filter );
        std::string get_filter() const {
            return filter;
        };
    private:



        std::string filter;
        std::function<bool( const item & )> filter_function;

        void mod_index( int offset );
};
#endif
