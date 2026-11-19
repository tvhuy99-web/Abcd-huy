#pragma once
#include <cmath>
#include <string>
#include <type_traits>
#include <utility>
#ifndef CATA_SRC_UNITS_UTILITY_H
#define CATA_SRC_UNITS_UTILITY_H

#include "units.h"





template<typename T, typename U>
T divide_round_up( units::quantity<T, U> num, units::quantity<T, U> den )
{
    return divide_round_up( num.value(), den.value() );
}






units::angle normalize( units::angle a, units::angle mod = 360_degrees );




units::angle angle_delta( units::angle a, units::angle b );




int angle_to_dir4( units::angle direction );


int angle_to_dir8( units::angle direction );

template<typename T, typename U, std::enable_if_t<std::is_floating_point_v<T>>* = nullptr>
units::quantity<T, U> round_to_multiple_of( units::quantity<T, U> val, units::quantity<T, U> of )
{
    int multiple = std::lround( val / of );
    return multiple * of;
}

struct lat_long {
    units::angle latitude;
    units::angle longitude;
};









const char *weight_units();









const char *volume_units_abbr();









const char *volume_units_long();








double convert_weight( const units::mass &weight );







int convert_length( const units::length &length );
std::string length_units( const units::length &length );
std::string length_to_string( const units::length &length, bool compact = false );







double convert_length_approx( const units::length &length, bool &display_as_integer );
std::string length_units_approx( const units::length &length );
std::string length_to_string_approx( const units::length &length );


double convert_length_cm_in( const units::length &length );


std::string weight_to_string( const units::mass &weight, bool compact = false,
                              bool remove_trailing_zeroes = false );





std::pair<std::string, std::string> weight_to_string( const
        units::quantity<int, units::mass_in_microgram_tag> &weight );




double convert_volume( int volume );





double convert_volume( int volume, int *out_scale );


std::string vol_to_string( const units::volume &vol, bool compact = false,
                           bool remove_trailing_zeroes = false );


std::string unit_to_string( const units::volume &unit, bool compact = false,
                            bool remove_trailing_zeroes = false );
std::string unit_to_string( const units::mass &unit, bool compact = false,
                            bool remove_trailing_zeroes = false );
std::string unit_to_string( const units::length &unit, bool compact = false );


double round_with_places( double value, int decimal_places );
#endif










std::string three_digit_display( double value );
