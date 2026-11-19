#pragma once
#ifndef CATA_SRC_MAPGEN_PRIMITIVES_H
#define CATA_SRC_MAPGEN_PRIMITIVES_H

#include <cstdint>
#include <limits>
#include <map>

#include "cata_assert.h"
#include "coords_fwd.h"
#include "type_id.h"

class JsonObject;




struct jmapgen_int {
    int16_t val;
    int16_t valmax;
    explicit jmapgen_int( int v ) : val( v ), valmax( v ) {
        cata_assert( v <= std::numeric_limits<int16_t>::max() );
    }
    jmapgen_int( int v, int v2 ) : val( v ), valmax( v2 ) {
        cata_assert( v <= std::numeric_limits<int16_t>::max() );
        cata_assert( v2 <= std::numeric_limits<int16_t>::max() );
    }



    jmapgen_int( const JsonObject &jo, std::string_view tag );




    jmapgen_int( const JsonObject &jo, std::string_view tag, const int &def_val,
                 const int &def_valmax );

    int get() const;
};

struct spawn_data {
    std::map<itype_id, jmapgen_int> ammo;
    std::vector<point_rel_ms> patrol_points_rel_ms;

    void deserialize( const JsonObject &jo );
};

#endif
