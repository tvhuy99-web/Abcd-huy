#pragma once
#ifndef CATA_SRC_TELEPORT_H
#define CATA_SRC_TELEPORT_H
#include "coords_fwd.h"

class Creature;
class vehicle;
class zone_data;
class teleport
{
    public:



        static bool teleport_creature( Creature &critter, int min_distance = 2, int max_distance = 12,
                                       bool safe = false,
                                       bool add_teleglow = true );
        static bool teleport_to_point( Creature &critter, tripoint_bub_ms target, bool safe,
                                       bool add_teleglow, bool display_message = true, bool force = false, bool force_safe = false );
        static bool teleport_vehicle( vehicle &veh, const tripoint_abs_ms &dp, bool force = false );
        static bool teleport_zone( zone_data &zone, const tripoint_abs_ms &dp );

};

#endif
