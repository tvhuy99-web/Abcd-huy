#pragma once
#ifndef CATA_SRC_NPC_TRIP_H
#define CATA_SRC_NPC_TRIP_H

#include <optional>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "point.h"


enum class npc_trip_kind : int {

    stand_on,

    within_range,

    legacy_nearest_passable
};

struct npc_trip_spec {
    tripoint_bub_ms source;
    npc_trip_kind kind = npc_trip_kind::stand_on;
    int range = 0;
    bool no_bashing = true;

    bool operator==( const npc_trip_spec &o ) const {
        return source == o.source && kind == o.kind && range == o.range &&
               no_bashing == o.no_bashing;
    }
    static npc_trip_spec stand_on( const tripoint_bub_ms &spot, bool no_bashing ) {
        return { spot, npc_trip_kind::stand_on, 0, no_bashing };
    }
    static npc_trip_spec within_range( const tripoint_bub_ms &source, int range,
                                       bool no_bashing ) {
        return { source, npc_trip_kind::within_range, range, no_bashing };
    }
    static npc_trip_spec legacy_nearest_passable( const tripoint_bub_ms &source, bool no_bashing ) {
        return { source, npc_trip_kind::legacy_nearest_passable, 0, no_bashing };
    }
};

struct npc_trip {
    time_point turn = calendar::turn_zero;
    tripoint_bub_ms from;
    npc_trip_spec spec;
    bool feasible = false;

    tripoint_bub_ms dest;

    std::vector<tripoint_bub_ms> path;
};


struct npc_retained_trip {
    bool valid = false;
    npc_trip_spec spec;
};


std::optional<tripoint_bub_ms> nearest_passable( const tripoint_bub_ms &p,
        const tripoint_bub_ms &closest_to );

#endif
