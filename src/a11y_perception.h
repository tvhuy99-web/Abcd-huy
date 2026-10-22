#pragma once
#ifndef CATA_SRC_A11Y_PERCEPTION_H
#define CATA_SRC_A11Y_PERCEPTION_H
#include <string>
#include <vector>
#include "coords_fwd.h"
namespace a11y
{
enum class observation_source { unknown, remembered, visible };
struct tile_observation {
    observation_source source = observation_source::unknown;
    std::string terrain;
    std::string furniture;
    std::vector<std::string> fields;
    std::vector<std::string> known_traps;
    std::vector<std::string> seen_creatures;
    std::vector<std::string> seen_items;
    std::string vehicle;
    std::string heard_marker;
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
