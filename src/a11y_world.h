#pragma once
#ifndef CATA_SRC_A11Y_WORLD_H
#define CATA_SRC_A11Y_WORLD_H
#include <optional>
#include <string>
#include <vector>
#include "a11y_semantics.h"
#include "coordinates.h"
#include "point.h"
class input_context;
class map;
class vehicle;
namespace a11y
{
enum class world_scan_filter { all, known, noteworthy, heard };



struct unrestricted_tile {
    std::string terrain;
    std::string furniture;
    std::string vehicle;
    bool impassable = false;
    std::vector<std::string> fields;
    std::vector<std::string> traps;
    std::vector<std::string> creatures;
    std::vector<std::string> items;
};
unrestricted_tile scan_unrestricted_tile( const tripoint_bub_ms &position );





struct vehicle_obstacle {
    int distance = 0;
    std::string label;
};
std::optional<vehicle_obstacle> vehicle_obstacle_ahead( vehicle &veh, map &here,
        bool reverse = false );





semantic_snapshot world_snapshot( const input_context &context, int scan_radius, int scan_depth,
                                  world_scan_filter filter,
                                  std::optional<point> camera_offset = std::nullopt,
                                  std::optional<tripoint_rel_ms> explore_cursor = std::nullopt );
semantic_snapshot activity_snapshot( const input_context &context );

void announce_linear_scan( const point &delta );


void announce_vehicle_relative_scan( const std::string &relative );
void announce_player_coordinates();


void announce_area_transition();

void announce_status_changes();
void reset_status_monitor();


void set_weapon_auto_stow( bool enabled );
bool weapon_auto_stow_enabled();
}
#endif
