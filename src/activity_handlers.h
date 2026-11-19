#pragma once
#ifndef CATA_SRC_ACTIVITY_HANDLERS_H
#define CATA_SRC_ACTIVITY_HANDLERS_H

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "coordinates.h"
#include "point.h"
#include "requirements.h"
#include "type_id.h"
#include "units_fwd.h"

class Character;
class item;
class item_location;
class map;
class player_activity;

template<typename Point, typename Container>
std::vector<Point> get_sorted_tiles_by_distance( const Point &center, const Container &tiles )
{
    const auto cmp = [center]( const Point & a, const Point & b ) {
        const int da = rl_dist( center, a );
        const int db = rl_dist( center, b );

        return da < db;
    };

    std::vector<Point> sorted( tiles.begin(), tiles.end() );
    std::sort( sorted.begin(), sorted.end(), cmp );

    return sorted;
}

std::vector<tripoint_bub_ms> route_adjacent( const Character &you, const tripoint_bub_ms &dest );

enum class requirement_check_result : int {
    SKIP_LOCATION = 0,
    SKIP_LOCATION_NO_ZONE,
    SKIP_LOCATION_NO_SKILL,
    SKIP_LOCATION_BLOCKING,
    SKIP_LOCATION_UNKNOWN_ACTIVITY,
    SKIP_LOCATION_NO_LOCATION,
    SKIP_LOCATION_NO_MATCH,
    CAN_DO_LOCATION,
    RETURN_EARLY
};


enum class do_activity_reason : int {
    CAN_DO_CONSTRUCTION,
    CAN_DO_FETCH,
    NO_COMPONENTS,
    DONT_HAVE_SKILL,
    NO_ZONE,
    NO_VEHICLE,
    ALREADY_DONE,
    UNKNOWN_ACTIVITY,
    NEEDS_CLEARING,
    NEEDS_HARVESTING,
    NEEDS_PLANTING,
    NEEDS_TILLING,
    NEEDS_FERTILIZING,
    BLOCKING_TILE,
    NEEDS_BOOK_TO_LEARN,
    NEEDS_CHOPPING,
    NEEDS_TREE_CHOPPING,
    NEEDS_BIG_BUTCHERING,
    NEEDS_BUTCHERING,
    NEEDS_CUT_HARVESTING,
    ALREADY_WORKING,
    NEEDS_VEH_DECONST,
    NEEDS_VEH_REPAIR,
    WOULD_PREVENT_VEH_FLYING,
    NEEDS_MINING,
    NEEDS_MOP,
    NEEDS_FISHING,
    NEEDS_CRAFT,
    NEEDS_DISASSEMBLE,
    REFUSES_THIS_WORK

};


const std::vector<std::string> do_activity_reason_string = {
    "CAN_DO_CONSTRUCTION",
    "CAN_DO_FETCH",
    "NO_COMPONENTS",
    "DONT_HAVE_SKILL",
    "NO_ZONE",
    "NO_VEHICLE",
    "ALREADY_DONE",
    "UNKNOWN_ACTIVITY",
    "NEEDS_CLEARING",
    "NEEDS_HARVESTING",
    "NEEDS_PLANTING",
    "NEEDS_TILLING",
    "BLOCKING_TILE",
    "NEEDS_BOOK_TO_LEARN",
    "NEEDS_CHOPPING",
    "NEEDS_TREE_CHOPPING",
    "NEEDS_BIG_BUTCHERING",
    "NEEDS_BUTCHERING",
    "NEEDS_CUT_HARVESTING",
    "ALREADY_WORKING",
    "NEEDS_VEH_DECONST",
    "NEEDS_VEH_REPAIR",
    "WOULD_PREVENT_VEH_FLYING",
    "NEEDS_MINING",
    "NEEDS_MOP",
    "NEEDS_FISHING",
    "NEEDS_CRAFT",
    "NEEDS_DISASSEMBLE",
    "REFUSES_THIS_WORK"
};

struct activity_reason_info {

    do_activity_reason reason;

    bool can_do;

    std::optional<construction_id> con_idx;

    activity_reason_info( do_activity_reason reason_, bool can_do_,
                          const std::optional<construction_id> &con_idx_ = std::nullopt ) :
        reason( reason_ ),
        can_do( can_do_ ),
        con_idx( con_idx_ )
    { }
    activity_reason_info( do_activity_reason reason_, bool can_do_, const requirement_data &req ):
        reason( reason_ ),
        can_do( can_do_ ),
        req( req )
    { }

    const requirement_data req;

    static activity_reason_info ok( const do_activity_reason &reason_ ) {
        return activity_reason_info( reason_, true );
    }

    static activity_reason_info build( const do_activity_reason &reason_, bool can_do_,
                                       const construction_id &con_idx_ ) {
        return activity_reason_info( reason_, can_do_, con_idx_ );
    }

    static activity_reason_info fail( const do_activity_reason &reason_ ) {
        return activity_reason_info( reason_, false );
    }
};

int get_auto_consume_moves( Character &you, bool food );
bool try_fuel_fire( Character &you,
                    std::optional<tripoint_bub_ms> fire_target = std::nullopt );

enum class item_drop_reason : int {
    deliberate,
    too_large,
    too_heavy,
    tumbling
};

void put_into_vehicle_or_drop( Character &you, item_drop_reason, const std::list<item> &items );
void put_into_vehicle_or_drop( Character &you, item_drop_reason, const std::list<item> &items,
                               map *here, const tripoint_bub_ms &where, bool force_ground = false );
std::vector<item_location> put_into_vehicle_or_drop_ret_locs( Character &you, item_drop_reason,
        const std::list<item> &items, tripoint_bub_ms dest = tripoint_bub_ms::invalid );
std::vector<item_location> put_into_vehicle_or_drop_ret_locs( Character &you, item_drop_reason,
        const std::list<item> &items, map *here, const tripoint_bub_ms &where,
        bool force_ground = false );
std::vector<item_location> drop_on_map( Character &you, item_drop_reason reason,
                                        const std::list<item> &items,
                                        map *here, const tripoint_bub_ms &where );

void repair_item_finish( player_activity *act, Character *you, bool no_menu );

namespace activity_handlers
{

bool resume_for_multi_activities( Character &you );


void fill_liquid_do_turn( player_activity *act, Character *you );
void repair_item_do_turn( player_activity *act, Character *you );
void travel_do_turn( player_activity *act, Character *you );


extern const std::map< activity_id, std::function<void( player_activity *, Character * )> >
do_turn_functions;


void repair_item_finish( player_activity *act, Character *you );

int move_cost( const item &it, const tripoint_bub_ms &src, const tripoint_bub_ms &dest );
int move_cost_cart( const item &it, const tripoint_bub_ms &src, const tripoint_bub_ms &dest,
                    const units::volume &capacity );
int move_cost_inv( const item &it, const tripoint_bub_ms &src, const tripoint_bub_ms &dest );

void clean_may_activity_occupancy_items_var( Character &you );
void clean_may_activity_occupancy_items_var_if_is_avatar_and_no_activity_now( Character &you );

extern const std::map< activity_id, std::function<void( player_activity *, Character * )> >
finish_functions;

}

#endif
