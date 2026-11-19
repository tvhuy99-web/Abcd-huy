#pragma once
#ifndef CATA_SRC_A11Y_SCENE_H
#define CATA_SRC_A11Y_SCENE_H

#include <string>
#include <vector>

#include "point.h"

namespace a11y
{

// Explicit, on-demand, visibility-independent survey of the currently loaded
// world. It speaks one complete report through the existing Android reader.
void announce_detailed_place_survey();

namespace scene_detail
{

// Pure directional classifier shared by the report and unit tests. These are
// world compass directions; they do not turn with the avatar's facing.
std::string compass_direction( const point &delta );

// Bounds in map squares, inclusive; useful for nonrectangular site envelopes.
struct extent {
    int west = 0;
    int north = 0;
    int east = 0;
    int south = 0;

    int width() const {
        return east - west + 1;
    }
    int height() const {
        return south - north + 1;
    }
};
extent overmap_envelope( int west_omt, int north_omt, int east_omt, int south_omt );
// Absolute bounds of every loaded map-square, independent of avatar position.
extent loaded_envelope( int west, int north, int width, int height );

// Combine only edge-connected tiles. The representative is the closest tile
// to the player, with stable coordinate tie-breaking for consistent speech.
std::vector<point> representative_tiles( const std::vector<point> &tiles,
        const point &player );

} // namespace scene_detail
} // namespace a11y

#endif // CATA_SRC_A11Y_SCENE_H
