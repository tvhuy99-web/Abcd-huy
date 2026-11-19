#pragma once
#ifndef CATA_SRC_HORDE_ENTITY_H
#define CATA_SRC_HORDE_ENTITY_H

#include <memory>

#include "calendar.h"
#include "coordinates.h"
#include "type_id.h"
#include "monster.h"

struct mtype;





struct horde_entity {

    explicit horde_entity( const monster &original );

    explicit horde_entity( const mtype_id &original );

    const mtype *get_type() const;
    bool is_active() const;





    tripoint_abs_ms destination;

    int tracking_intensity = 0;

    time_point last_processed;

    int moves = 0;
    mtype_int_id type_id;





    std::unique_ptr<monster> monster_data;
};

#endif
