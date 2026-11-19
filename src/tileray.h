#pragma once
#ifndef CATA_SRC_TILERAY_H
#define CATA_SRC_TILERAY_H

#include <string>

#include "coordinates.h"
#include "units.h"






















class tileray
{
    private:
        point_rel_ms delta;
        int leftover = 0;
        point_rel_ms abs_d;
        units::angle direction = 0_degrees;
        point_rel_ms last_d;
        int steps = 0;
        bool infinite = false;
    public:
        tileray();
        explicit tileray( const point_rel_ms &ad );
        explicit tileray( const point_bub_ms &ad );
        explicit tileray( units::angle adir );

        void init( const point_rel_ms &ad );
        void init( const point_bub_ms &ad );
        void init( const units::angle &adir );

        int dx() const;
        int dy() const;
        units::angle dir() const;
        int quadrant() const;


        std::string to_string_azimuth_from_north() const;


        int ortho_dx( int od ) const;

        int ortho_dy( int od ) const;
        bool mostly_vertical() const;

        void advance( int num = 1 );
        void clear_advance();
        int get_steps() const;
};

#endif
