#pragma once
#ifndef CATA_SRC_OVERMAP_TYPES_H
#define CATA_SRC_OVERMAP_TYPES_H

#include "calendar.h"

class scent_trace
{
    public:

        scent_trace() : creation_time( calendar::before_time_starts ), initial_strength( 0 ) {}
        scent_trace( const time_point &ct, int strength ) : creation_time( ct ),
            initial_strength( strength ) {}

        time_point creation_time;
        int initial_strength;
};

#endif
