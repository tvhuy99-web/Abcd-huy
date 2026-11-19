#pragma once
#ifndef CATA_SRC_CUBE_DIRECTION_H
#define CATA_SRC_CUBE_DIRECTION_H

#include <cstddef>
#include <functional>

struct tripoint;
template <typename E> struct enum_traits;

namespace om_direction
{
enum class type : int;
}




enum class cube_direction : int {
    north,
    east,
    south,
    west,
    above,
    below,
    last
};

template<>
struct enum_traits<cube_direction> {
    static constexpr cube_direction last = cube_direction::last;
};

namespace std
{
template <> struct hash<cube_direction> {
    std::size_t operator()( const cube_direction &d ) const {
        return static_cast<std::size_t>( d );
    }
};
}

cube_direction operator+( cube_direction, om_direction::type );
cube_direction operator+( cube_direction, int i );
cube_direction operator-( cube_direction, om_direction::type );
cube_direction operator-( cube_direction, int i );

tripoint displace( cube_direction d );

#endif
