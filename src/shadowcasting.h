#pragma once
#ifndef CATA_SRC_SHADOWCASTING_H
#define CATA_SRC_SHADOWCASTING_H

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <string>
#include <type_traits>

#include "coords_fwd.h"
#include "lightmap.h"
#include "map_scale_constants.h"
#include "mdarray.h"

struct fragment_cloud;






enum class quadrant : int {
    NE,
    SE,
    SW,
    NW,
    default_ = NE
};

enum class vertical_direction {
    UP,
    DOWN,
    BOTH
};

struct four_quadrants {
    four_quadrants() = default;
    explicit constexpr four_quadrants( float v ) : values{{v, v, v, v}} {}

    std::array<float, 4> values;

    float &operator[]( quadrant q ) {
        return values[static_cast<int>( q )];
    }
    float operator[]( quadrant q ) const {
        return values[static_cast<int>( q )];
    }
    void fill( float v ) {
        values[0] = v;
        values[1] = v;
        values[2] = v;
        values[3] = v;
    }
    float max() const {
        return *std::max_element( values.begin(), values.end() );
    }
    std::string to_string() const;

    friend four_quadrants operator*( const four_quadrants &l, const four_quadrants &r ) {
        four_quadrants result;
        std::transform( l.values.begin(), l.values.end(), r.values.begin(),
                        result.values.begin(), std::multiplies<>() );
        return result;
    }

    friend four_quadrants elementwise_max( const four_quadrants &l, const four_quadrants &r ) {
        four_quadrants result;
        std::transform( l.values.begin(), l.values.end(), r.values.begin(),
        result.values.begin(), []( float l, float r ) {
            return std::max( l, r );
        } );
        return result;
    }

    friend four_quadrants elementwise_max( const four_quadrants &l, const float r ) {
        four_quadrants result( l );
        for( float &v : result.values ) {



            if( v < r ) {
                v = r;
            }
        }
        return result;
    }
};
static_assert( std::is_trivially_copyable_v<four_quadrants> );






inline float sight_calc( const float &numerator, const float &transparency, const int &distance )
{
    return numerator / std::exp( transparency * distance );
}
inline bool sight_check( const float &transparency, const float & )
{
    return transparency > LIGHT_TRANSPARENCY_SOLID;
}
inline void update_light( float &update, const float &new_value, quadrant )
{
    update = std::max( update, new_value );
}
inline void update_light_quadrants( four_quadrants &update, const float &new_value, quadrant q )
{
    update[q] = std::max( update[q], new_value );
}
inline float accumulate_transparency( const float &cumulative_transparency,
                                      const float &current_transparency, const int &distance )
{
    return ( ( distance - 1 ) * cumulative_transparency + current_transparency ) / distance;
}

template<typename T, typename Out, T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         void( *update_output )( Out &, const T &, quadrant ),
         T( *accumulate )( const T &, const T &, const int & )>
void castLightAll( cata::mdarray<Out, point_bub_ms> &output_cache,
                   const cata::mdarray<T, point_bub_ms> &input_array,
                   const point_bub_ms &offset, int offsetDistance = 0,
                   T numerator = 1.0 );

template<typename T>
using array_of_grids_of =
    std::conditional_t <
    std::is_const_v<T>,
    std::array<const cata::mdarray<std::remove_const_t<T>, point_bub_ms>*, OVERMAP_LAYERS>,
    std::array<cata::mdarray<T, point_bub_ms>*, OVERMAP_LAYERS>
    >;


template< typename T, T( *calc )( const T &, const T &, const int & ),
          bool( *check )( const T &, const T & ),
          T( *accumulate )( const T &, const T &, const int & ) >
void cast_zlight(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &origin, int offset_distance, T numerator,
    vertical_direction dir = vertical_direction::BOTH );

#endif
