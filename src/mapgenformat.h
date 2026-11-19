#pragma once
#ifndef CATA_SRC_MAPGENFORMAT_H
#define CATA_SRC_MAPGENFORMAT_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "coords_fwd.h"
#include "type_id.h"

class map;

namespace mapf
{
template<typename ID>
class format_effect;












void formatted_set_simple( map *m, const point_bub_ms &start, const char *cstr,
                           const format_effect<ter_id> &ter_b, const format_effect<furn_id> &furn_b );

template<typename ID>
class format_effect
{
    private:
        std::string characters;
        std::vector<ID> determiners;

    public:
        format_effect( const std::string &chars,
                       std::vector<ID> dets );

        ID translate( char c ) const;
};


















template<size_t N, typename ...Args>
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
inline format_effect<ter_id> ter_bind( const char ( &characters )[N], Args... ids )
{

    static_assert( N % 2 == 0, "list of characters to bind to must be odd, e.g. \"a b c\"" );
    static_assert( N / 2 == sizeof...( Args ),
                   "list of characters to bind to must match the size of the remaining arguments" );
    return format_effect<ter_id>( characters, { std::forward<Args>( ids )... } );
}

template<size_t N, typename ...Args>
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
inline format_effect<furn_id> furn_bind( const char ( &characters )[N], Args... ids )
{

    static_assert( N % 2 == 0, "list of characters to bind to must be odd, e.g. \"a b c\"" );
    static_assert( N / 2 == sizeof...( Args ),
                   "list of characters to bind to must match the size of the remaining arguments" );
    return format_effect<furn_id>( characters, { std::forward<Args>( ids )... } );
}


}

#endif
