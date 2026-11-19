#pragma once
#ifndef CATA_SRC_A11Y_PERCEPTION_H
#define CATA_SRC_A11Y_PERCEPTION_H
#include <array>
#include <string>
#include <vector>
#include "coords_fwd.h"
namespace a11y
{
enum class observation_source { unknown, remembered, visible };



enum class door_state { none, locked, closed, open };
struct tile_observation {
    observation_source source = observation_source::unknown;
    std::string terrain;
    std::string furniture;
    std::vector<std::string> fields;
    std::vector<std::string> known_traps;
    std::vector<std::string> seen_creatures;
    std::vector<std::string> special_perceptions;
    std::vector<std::string> seen_items;
    std::string vehicle;


    std::string vehicle_part;
    std::string heard_marker;
    door_state door = door_state::none;
    bool impassable = false;
    bool route_feature = false;
};



struct directional_sonar_result {
    std::array<int, 8> creatures;
    std::array<int, 8> items;
    directional_sonar_result() {
        creatures.fill( -1 );
        items.fill( -1 );
    }
};
directional_sonar_result scan_directional_sonar( const tripoint_bub_ms &origin );



tile_observation observe_tile( const tripoint_bub_ms &position );

tile_observation observe_memory_tile( const tripoint_bub_ms &position );
std::string describe_tile( const tile_observation &observation );
}
#endif
