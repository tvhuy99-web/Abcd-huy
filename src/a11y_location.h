#pragma once
#ifndef CATA_SRC_A11Y_LOCATION_H
#define CATA_SRC_A11Y_LOCATION_H

#include <cstddef>
#include <functional>
#include <optional>
#include <set>

#include "coordinates.h"
#include "point.h"

namespace a11y
{

// Presentation-only coordinates. Gameplay, travel and saves continue to use
// CDDA absolute map-square coordinates.
struct local_place_coordinates {
    int x = 0;
    int y = 0;
};

// A location can cover many overmap terrain cells. Never silently reset the
// origin at a 24x24 cell boundary. Returns nullopt if its extent cannot be
// identified without guessing or generating additional overmaps.
std::optional<local_place_coordinates> local_coordinates_at( const tripoint_abs_ms &pos );

namespace location_detail
{

// Pure road-graph logic, separately tested with synthetic roads. nullopt from
// road_at means an unknown/unloaded overmap neighbor, not empty road.
using road_probe = std::function<std::optional<bool>( const point & )>;

struct road_section {
    std::set<point> tiles;
    bool complete = true;
};

// Split the connected road network at real three-way/four-way junctions,
// rather than cutting it into arbitrary 24x24 squares.
road_section find_road_section( const point &origin, const road_probe &road_at,
                                std::size_t max_tiles = 2048 );

// A location's fixed northwest OMT corner is the origin for 1-based,
// east-increasing X and south-increasing Y, including negative world coords.
point local_from_omt_corner( const point &world_ms, const point &northwest_omt );

} // namespace location_detail
} // namespace a11y

#endif // CATA_SRC_A11Y_LOCATION_H
