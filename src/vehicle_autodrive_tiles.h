#pragma once
#ifndef CATA_SRC_VEHICLE_AUTODRIVE_TILES_H
#define CATA_SRC_VEHICLE_AUTODRIVE_TILES_H

#include "coordinates.h"

class Character;
class map;
class vehicle;

namespace vehicle_autodrive
{




enum class tile_blocker {
    none,
    map_boundary,
    other_vehicle,
    creature,
    visible_trap,
    furniture,
    terrain
};



struct mobility_profile {
    bool land_ok = false;
    bool water_ok = false;
    bool air_ok = false;


    bool unrestricted_creatures = false;
};

tile_blocker classify_tile( const vehicle &driven_veh, map &here,
                            const tripoint_bub_ms &pt, const Character &driver,
                            const mobility_profile &mobility );

}

#endif
