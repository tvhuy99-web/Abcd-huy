#pragma once
#ifndef CATA_SRC_UISTATE_H
#define CATA_SRC_UISTATE_H

#include <list>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "editmap.h"
#include "enums.h"
#include "flat_set.h"
#include "item_location.h"
#include "json.h"
#include "omdata.h"
#include "type_id.h"

constexpr int DEFAULT_TILESET_ZOOM = 16;

class item;

struct advanced_inv_pane_save_state {
    public:
        int sort_idx = 1;
        std::string filter;
        int area_idx = 11;
        int selected_idx = 0;

        bool in_vehicle = false;
        item_location container;
        int container_base_loc;

        void serialize( JsonOut &json, const std::string &prefix ) const {
            json.member( prefix + "sort_idx", sort_idx );
            json.member( prefix + "filter", filter );
            json.member( prefix + "area_idx", area_idx );
            json.member( prefix + "selected_idx", selected_idx );
            json.member( prefix + "in_vehicle", in_vehicle );
            json.member( prefix + "container", container );
            json.member( prefix + "container_base_loc", container_base_loc );
        }

        void deserialize( const JsonObject &jo, const std::string &prefix ) {
            jo.read( prefix + "sort_idx", sort_idx );
            jo.read( prefix + "filter", filter );
            jo.read( prefix + "area_idx", area_idx );
            jo.read( prefix + "selected_idx", selected_idx );
            jo.read( prefix + "in_vehicle", in_vehicle );
            jo.read( prefix + "container", container );
            jo.read( prefix + "container_base_loc", container_base_loc );
        }
};

struct advanced_inv_save_state {
    public:
        aim_exit exit_code = aim_exit::none;
        aim_entry re_enter_move_all = aim_entry::START;
        int aim_all_location = 1;

        bool active_left = true;
        int last_popup_dest = 0;

        int saved_area = 11;
        int saved_area_right = 0;
        advanced_inv_pane_save_state pane;
        advanced_inv_pane_save_state pane_right;

        void serialize( JsonOut &json, const std::string &prefix ) const {
            json.member( prefix + "exit_code", exit_code );
            json.member( prefix + "re_enter_move_all", re_enter_move_all );
            json.member( prefix + "aim_all_location", aim_all_location );

            json.member( prefix + "active_left", active_left );
            json.member( prefix + "last_popup_dest", last_popup_dest );

            json.member( prefix + "saved_area", saved_area );
            json.member( prefix + "saved_area_right", saved_area_right );
            pane.serialize( json, prefix + "pane_" );
            pane_right.serialize( json, prefix + "pane_right_" );
        }

        void deserialize( const JsonObject &jo, const std::string &prefix ) {
            jo.read( prefix + "exit_code", exit_code );
            jo.read( prefix + "re_enter_move_all", re_enter_move_all );
            jo.read( prefix + "aim_all_location", aim_all_location );

            jo.read( prefix + "active_left", active_left );
            jo.read( prefix + "last_popup_dest", last_popup_dest );

            jo.read( prefix + "saved_area", saved_area );
            jo.read( prefix + "saved_area_right", saved_area_right );
            pane.area_idx = saved_area;
            pane_right.area_idx = saved_area_right;
            pane.deserialize( jo, prefix + "pane_" );
            pane_right.deserialize( jo, prefix + "pane_right_" );
        }
};

extern void save_inv_state( JsonOut &json );
extern void load_inv_state( const JsonObject &jo );

struct overmap_sidebar_uistate {
    bool quickref_header = true;
    bool layers_header = true;
    bool debug_header = true;

    void set_all( bool state ) {
        quickref_header = state;
        layers_header = state;
        debug_header = state;
    }
    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &jo );
};


struct consume_menu_uistate {


    std::vector<uint64_t> consume_menu_selections;

    std::vector<item_location> consume_menu_selected_items;

    std::string consume_menu_filter;

    bool collated = false;

    std::string consume_menu_comestype;

    bool empty() const {
        return consume_menu_selections.empty() && consume_menu_selected_items.empty() &&
               consume_menu_filter.empty();
    }
    void clear() {
        collated = false;
        consume_menu_selections.clear();
        consume_menu_selected_items.clear();
        consume_menu_filter.clear();

    }
    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &jo );
};

struct editmap_uistate {

    editmap_brush brush;

    editmap_mode mode = editmap_mode::EMM_DRAWING;
    bool advanced_info_toggle = false;
    bool blink = false;
    bool run_post_process = false;
    bool fast_scroll = false;

    std::array<bool, SELECTABLE_ACTIONS> selected; // NOLINT(cata-serialize)
    tinymap *tmpmap_ptr = nullptr; // NOLINT(cata-serialize)

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &jo );
};







// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class uistatedata
{

    private:
        enum side { left = 0, right = 1, NUM_PANES = 2 };
    public:
        int ags_pay_gas_selected_pump = 0;

        int wishitem_selected = 0; // NOLINT(cata-serialize)
        int wishmutate_selected = 0; // NOLINT(cata-serialize)
        int wishmonster_selected = 0; // NOLINT(cata-serialize)
        int iexamine_atm_selected = 0; // NOLINT(cata-serialize)

        int adv_inv_container_location = -1;
        int adv_inv_container_index = 0;
        itype_id adv_inv_container_type = itype_id::NULL_ID();
        itype_id adv_inv_container_content_type = itype_id::NULL_ID();
        bool adv_inv_container_in_vehicle = false;

        advanced_inv_save_state transfer_save;

        bool unload_auto_contain = true;
        std::optional<bool> hide_entries_override = std::nullopt;

        bool overmap_blinking = true;
        bool overmap_show_overlays = false;
        bool overmap_show_map_notes = true;
        bool overmap_show_land_use_codes = false;
        bool overmap_show_city_labels = true;
        bool overmap_show_hordes = true;
        bool overmap_show_revealed_omts = true;
        bool overmap_show_forest_trails = true;
        bool overmap_visible_weather = false;
        bool overmap_debug_weather = false;

        bool overmap_debug_mongroup = false;
        bool overmap_only_auto_travel = false;
        bool overmap_fast_scroll = false;

        int tileset_zoom = DEFAULT_TILESET_ZOOM;
        int overmap_tileset_zoom = DEFAULT_TILESET_ZOOM;

        overmap_sidebar_uistate overmap_sidebar_state;
        editmap_uistate editmap_state;

        consume_menu_uistate consume_uistate;


        bool distraction_noise = true;
        bool distraction_pain = true;
        bool distraction_attack = true;
        bool distraction_hostile_close = true;
        bool distraction_hostile_spotted = true;
        bool distraction_conversation = true;
        bool distraction_asthma = true;
        bool distraction_dangerous_field = true;
        bool distraction_weather_change = true;
        bool distraction_hunger = true;
        bool distraction_thirst = true;
        bool distraction_temperature = true;
        bool distraction_mutation = true;
        bool distraction_oxygen = true;
        bool distraction_withdrawal = true;
        bool distraction_craft_step_complete = true;
        bool distraction_all = true; // NOLINT(cata-serialize)
        bool numpad_navigation = false;







        int vmenu_item_sort = 0;
        int vmenu_monster_sort = 0;
        int vmenu_terfurn_sort = 0;
        std::set<itype_id> read_items;



        std::string list_item_filter; // NOLINT(cata-serialize)
        std::string list_item_downvote; // NOLINT(cata-serialize)
        std::string list_item_priority; // NOLINT(cata-serialize)
        std::string monster_filter; // NOLINT(cata-serialize)
        std::string terfurn_filter; // NOLINT(cata-serialize)
        surroundings_menu_tab_enum vmenu_tab = surroundings_menu_tab_enum::items;
        bool list_item_filter_active = false;
        bool list_item_downvote_active = false;
        bool list_item_priority_active = false;
        bool list_monster_filter_active = false;
        bool list_terfurn_filter_active = false;


        std::string construction_filter;
        construction_group_str_id last_construction = construction_group_str_id::NULL_ID();
        construction_category_id construction_tab = construction_category_id::NULL_ID();


        const oter_t *place_terrain = nullptr; // NOLINT(cata-serialize)
        const overmap_special *place_special = nullptr; // NOLINT(cata-serialize)
        om_direction::type omedit_rotation = om_direction::type::none; // NOLINT(cata-serialize)


        std::set<recipe_id> hidden_recipes;
        std::set<recipe_id> favorite_recipes;
        std::set<recipe_id> expanded_recipes;
        cata::flat_set<recipe_id> read_recipes;
        std::vector<recipe_id> recent_recipes;
        bool crafting_expand_details = false;
        bool crafting_expand_steps = false;

        bionic_ui_sort_mode bionic_sort_mode = bionic_ui_sort_mode::POWER;






        // NOLINTNEXTLINE(cata-serialize)
        std::map<std::string, std::vector<std::string>> input_history;

        std::map<ammotype, itype_id> lastreload;



        bool _testing_save = true; // NOLINT(cata-serialize)

        bool _really_testing_save = false; // NOLINT(cata-serialize)

        std::vector<std::string> &gethistory( const std::string &id ) {
            return input_history[id];
        }






        std::optional<std::function<void()>> open_menu; // NOLINT(cata-serialize)


        template<typename T>
        void serialize_array( JsonOut &json, const std::string_view name, T &data ) const {
            json.member( name );
            json.start_array();
            for( const auto &d : data ) {
                json.write( d );
            }
            json.end_array();
        }

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );
};
extern uistatedata uistate;

#endif
