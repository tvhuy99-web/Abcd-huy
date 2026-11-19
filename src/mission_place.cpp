#include "city.h"
#include "coordinates.h"
#include "mission.h" // IWYU pragma: associated
#include "overmapbuffer.h"


bool mission_place::near_town( const tripoint_abs_omt &pos_omt )
{
    const tripoint_abs_sm pos_sm = project_to<coords::sm>( pos_omt );
    const city_reference cref = overmap_buffer.closest_city( pos_sm );
    if( !cref ) {
        return false;
    }

    return cref.distance / 2 - cref.city->size <= 40;
}
