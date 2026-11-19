#pragma once
#ifndef CATA_SRC_RNG_H
#define CATA_SRC_RNG_H

#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "coords_fwd.h"
#include "units.h"

class map;
class time_duration;
template<typename Tripoint>
class tripoint_range;





void rng_set_engine_seed( unsigned int seed );

using cata_default_random_engine = std::minstd_rand0;
cata_default_random_engine::result_type rng_get_first_seed();
cata_default_random_engine &rng_get_engine();
unsigned int rng_bits();

int rng( int lo, int hi );
double rng_float( double lo, double hi );

template<typename U>
units::quantity<double, U> rng_float( units::quantity<double, U> lo,
                                      units::quantity<double, U> hi )
{
    return { rng_float( lo.value(), hi.value() ), U{} };
}

units::angle random_direction();

bool one_in( int chance );
bool one_turn_in( const time_duration &duration );
bool x_in_y( double x, double y );
int dice( int number, int sides );


int roll_remainder( double value );
inline int roll_remainder( float value )
{
    return roll_remainder( static_cast<double>( value ) );
}

int djb2_hash( const unsigned char *input );








std::vector<int> rng_sequence( size_t count, int lo, int hi, int seed = 42 );

double rng_normal( double lo, double hi );

inline double rng_normal( double hi )
{
    return rng_normal( 0.0, hi );
}

float normal_roll_chance( float center, float stddev, float difficulty );

double normal_roll( double mean, double stddev );

double chi_squared_roll( double trial_num );

double rng_exponential( double min, double mean );

inline double rng_exponential( double mean )
{
    return rng_exponential( 0.0, mean );
}

double exponential_roll( double lambda );


std::string random_string( size_t length );












template<typename C, typename D, typename V = typename C::value_type>
inline V random_entry( const C &container, D default_value )
{
    if( container.empty() ) {
        return default_value;
    }
    auto iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    return *iter;
}






template<typename C>
inline auto random_entry_opt( C &container ) ->
std::optional<decltype( std::ref( *container.begin() ) )>
{
    if( container.empty() ) {
        return std::nullopt;
    }
    auto iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    return std::ref( *iter );
}




template<typename C, typename V = typename C::value_type>
inline V random_entry( const C &container )
{
    if( container.empty() ) {
        return V();
    }
    typename C::const_iterator iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    return *iter;
}

template<typename ...T>
class is_std_array_helper : public std::false_type
{
};
template<typename T, std::size_t N>
class is_std_array_helper<std::array<T, N>> : public std::true_type
{
};
template<typename T>
class is_std_array : public is_std_array_helper<std::decay_t<T>>
{
};






template<typename C, typename V = typename C::value_type>
inline std::enable_if_t < !is_std_array<C>::value,
       const V & > random_entry_ref( const C &container )
{
    if( container.empty() ) {
        static const V default_value = V();
        return default_value;
    }
    auto iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    return *iter;
}
template<typename V, std::size_t N>
inline const V &random_entry_ref( const std::array<V, N> &container )
{
    static_assert( N > 0, "Need a non-empty array to get a random value from it" );
    return container[rng( 0, N - 1 )];
}




template<typename C, typename V = typename C::value_type>
inline V random_entry_removed( C &container )
{
    auto iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    V result = std::move( *iter );
    container.erase( iter );
    return result;
}


tripoint_range<tripoint_bub_ms> points_in_range_bub( const map &m );

tripoint_range<tripoint_bub_ms> points_in_level_range( const map &m, int z );

std::optional<tripoint_bub_ms> random_point( const tripoint_range<tripoint_bub_ms> &range,
        const std::function<bool( const tripoint_bub_ms & )> &predicate );

std::optional<tripoint_bub_ms> random_point( const map &m,
        const std::function<bool( const tripoint_bub_ms & )> &predicate );
std::optional<tripoint_bub_ms> random_point_on_level( const map &m, int z,
        const std::function<bool( const tripoint_bub_ms & )> &predicate );


int rng_map_coord( int border );
template<typename Point>

Point rng_map_point( int border = 0 )
{
    return { rng_map_coord( border ), rng_map_coord( border ) };
}

template<typename Tripoint>
Tripoint rng_map_point( int border, int z )
{
    return { rng_map_coord( border ), rng_map_coord( border ), z };
}


#endif
