#pragma once
#ifndef CATA_SRC_VEHICLE_AUTODRIVE_TILES_H
#define CATA_SRC_VEHICLE_AUTODRIVE_TILES_H

#include "coordinates.h"

class Character;
class map;
class vehicle;

namespace vehicle_autodrive
{

// Shared source of truth for the native pathfinder and the accessible
// driving collision guard. Consumers may describe the blocker differently,
// but must never decide independently whether a map tile is drivable.
enum class tile_blocker {
    none,
    map_boundary,
    other_vehicle,
    creature,
    visible_trap,
    furniture,
    terrain
};

// These capabilities are cached by the native pathfinder. Accessible manual
// driving takes the same values directly from the live vehicle.
struct mobility_profile {
    bool land_ok = false;
    bool water_ok = false;
    bool air_ok = false;
    // Reader scans detect live creatures beyond sight just as native reader
    // autodrive does; ordinary autodrive retains its visibility restriction.
    bool unrestricted_creatures = false;
};

tile_blocker classify_tile( const vehicle &driven_veh, map &here,
                            const tripoint_bub_ms &pt, const Character &driver,
                            const mobility_profile &mobility );

} // namespace vehicle_autodrive

#endif // CATA_SRC_VEHICLE_AUTODRIVE_TILES_H
