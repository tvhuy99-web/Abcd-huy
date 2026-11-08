#pragma once
#ifndef CATA_SRC_A11Y_PERCEPTION_H
#define CATA_SRC_A11Y_PERCEPTION_H
#include <string>
#include <vector>
#include "coords_fwd.h"
namespace a11y
{
enum class observation_source { unknown, remembered, visible };
// Door state derived only from the tile's own ter/furn ids (the live ids for
// a visible tile, the memorized ids for a remembered tile). Published only
// for tiles carrying the DOOR flag; none means no door claim is made.
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
    // Name of the vehicle part at this tile; visible tiles only, read from
    // the same vpart_position that supplied the vehicle name.
    std::string vehicle_part;
    std::string heard_marker;
    door_state door = door_state::none;
    bool impassable = false;
    bool route_feature = false;
};
// All callers run on the game thread after updating the game's visibility
// cache for this z-level. Unknown/memory branches never read live tile content.
tile_observation observe_tile( const tripoint_bub_ms &position );
// Read only the map memory at this coordinate; do not inspect live terrain.
tile_observation observe_memory_tile( const tripoint_bub_ms &position );
std::string describe_tile( const tile_observation &observation );
} // namespace a11y
#endif // CATA_SRC_A11Y_PERCEPTION_H
