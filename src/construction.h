#pragma once
#ifndef CATA_SRC_CONSTRUCTION_H
#define CATA_SRC_CONSTRUCTION_H

#include <functional>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "coords_fwd.h"
#include "game_constants.h"
#include "item.h"
#include "translation.h"
#include "type_id.h"

class Character;
class temp_crafting_inventory;
struct point;

namespace catacurses
{
class window;
}
class JsonObject;
class nc_color;

struct partial_con {
    int counter = 0;
    std::list<item> components;
    construction_id id = construction_id( -1 );
};

struct construction {

        construction_category_id category;

        construction_group_str_id group;

        translation pre_note;

        std::set<std::string> pre_terrain;

        std::string post_terrain;


        std::optional<item_group_id> byproduct_item_group;



        std::map<std::string, bool> pre_flags;


        std::set<std::string> post_flags;


        std::map<skill_id, int> required_skills;

        std::vector<std::pair<requirement_id, int>> reqs_using;
        requirement_id requirements;

        bool is_blacklisted() const;


        construction_str_id id;
        void load( const JsonObject &jo, std::string_view );
        void check() const;
        void finalize();


        int time = 0;

        bool was_loaded = false;

        bool vehicle_start = false;


        std::vector<bool ( * )( const tripoint_bub_ms & )> pre_specials;

        void ( *do_turn_special )( const tripoint_bub_ms &, Character & );

        std::vector<void ( * )( const tripoint_bub_ms &, Character & )> post_specials;

        void ( *explain_failure )( const tripoint_bub_ms & );

        bool pre_is_furniture = false;

        bool post_is_furniture = false;


        int adjusted_time() const;
        int print_time( const catacurses::window &w, const point &, int width, nc_color col ) const;
        std::vector<std::string> get_folded_time_string( int width ) const;

        float time_scale() const;


        bool on_display = true;


        bool dark_craftable = false;


        bool strict = false;

        float activity_level = MODERATE_EXERCISE;
    private:
        std::string get_time_string() const;
};

const std::vector<construction> &get_constructions();

void place_construction( std::vector<construction_group_str_id> const &groups );
void load_construction( const JsonObject &jo, const std::string &src );
void reset_constructions();
construction_id construction_menu( bool blueprint );
bool has_pre_flags( const construction &con, furn_id const &f, ter_id const &t );
bool can_construct( const construction &con, const tripoint_bub_ms &p );
bool player_can_build( Character &you, const temp_crafting_inventory &inv, const construction &con,
                       bool can_construct_skip = false );
std::vector<const construction *> constructions_by_group( const construction_group_str_id &group );
std::vector<const construction *> constructions_by_filter(
    std::function<bool( construction const & )>
    const &filter );
void check_constructions();
void finalize_constructions();
void prompt_partial_construction( Character &you, tripoint_bub_ms const &examp );
std::vector<construction_id> find_build_sequence( const std::string &target_id,
        std::function<bool( construction const & )> const &filter,
        std::function<bool( construction const & )> const &can_build );
#endif
