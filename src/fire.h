#pragma once
#ifndef CATA_SRC_FIRE_H
#define CATA_SRC_FIRE_H

#include "units.h"












struct fire_data {
    fire_data() = default;
    explicit fire_data( int intensity, bool is_contained = false ) : fire_intensity( intensity ),
        contained( is_contained )
    {}

    int fire_intensity = 0;

    float smoke_produced = 0.0f;

    float fuel_produced = 0.0f;

    bool contained = false;
};













struct mat_burn_data {

    bool immune = false;

    units::volume volume_per_turn = 0_ml;

    float fuel = 0.0f;

    float smoke = 0.0f;

    float burn = 0.0f;

    void deserialize( const JsonObject &jo );
};

#endif
