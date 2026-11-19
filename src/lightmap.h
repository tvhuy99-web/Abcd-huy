#pragma once
#ifndef CATA_SRC_LIGHTMAP_H
#define CATA_SRC_LIGHTMAP_H

#include <cmath> // IWYU pragma: keep
#include <ostream>
#include <string_view>

#include <stdint.h>

#include "map_scale_constants.h"
#include "point.h"
#include "type_id.h"

constexpr float LIGHT_SOURCE_LOCAL = 0.1f;
constexpr float LIGHT_SOURCE_BRIGHT = 10.0f;


constexpr float LIGHT_AMBIENT_MINIMAL = 3.7f;

constexpr float LIGHT_AMBIENT_LOW = 3.5f;

constexpr float LIGHT_AMBIENT_DIM = 5.0f;

constexpr float LIGHT_AMBIENT_LIT = 10.0f;













constexpr float LIGHT_TRANSPARENCY_SOLID = 0.0f;





constexpr float LIGHT_TRANSPARENCY_OPEN_AIR = 2.3f / MAX_VIEW_DISTANCE;


constexpr float VISIBILITY_FULL = 1.0f;

constexpr inline int LIGHT_RANGE( float b )
{
    if( b <= LIGHT_AMBIENT_LOW ) {
        return 0;
    }
    return static_cast<int>( -std::log( LIGHT_AMBIENT_LOW / b ) * ( 1.0 /
                             LIGHT_TRANSPARENCY_OPEN_AIR ) );
}



struct light_color_rgb {
    light_color_rgb() = default;
    float r;
    float g;
    float b;
    bool is_colored() const {
        return r > 0.0f || g > 0.0f || b > 0.0f;
    }
    light_color_rgb &operator+=( const light_color_rgb &rhs ) {
        r += rhs.r;
        g += rhs.g;
        b += rhs.b;
        return *this;
    }
    bool operator==( const light_color_rgb &rhs ) const {
        return r == rhs.r && g == rhs.g && b == rhs.b;
    }
    bool operator!=( const light_color_rgb &rhs ) const {
        return !( *this == rhs );
    }
    light_color_rgb operator*( float scale ) const {
        return { r * scale, g * scale, b * scale };
    }


    static light_color_rgb from_hsv( float h, float s, float v );
};
static_assert( std::is_trivially_default_constructible_v<light_color_rgb> );



light_color_rgb dawn_dusk_color_for_lightmap( dimension_id dimension );

enum class lit_level : uint8_t {
    DARK = 0,
    LOW,
    BRIGHT_ONLY,
    LIT,
    BRIGHT,
    MEMORIZED,
    BLANK
};

template<typename T>
constexpr inline bool operator>( const T &lhs, const lit_level &rhs )
{
    return lhs > static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator<=( const T &lhs, const lit_level &rhs )
{
    return !operator>( lhs, rhs );
}

template<typename T>
constexpr inline bool operator!=( const lit_level &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) != rhs;
}

inline std::ostream &operator<<( std::ostream &os, const lit_level &ll )
{
    return os << static_cast<int>( ll );
}


int trig_dist_2d( point delta );

#endif
