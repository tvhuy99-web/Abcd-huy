#pragma once
#ifndef CATA_SRC_TRY_PARSE_INTEGER_H
#define CATA_SRC_TRY_PARSE_INTEGER_H

#include <string_view>

#include "coordinates.h"
#include "ret_val.h"









template<typename T>
ret_val<T> try_parse_integer( std::string_view, bool use_locale );

extern template ret_val<int> try_parse_integer<int>( std::string_view, bool use_locale );
extern template ret_val<long> try_parse_integer<long>( std::string_view, bool use_locale );
extern template ret_val<long long> try_parse_integer<long long>( std::string_view,
        bool use_locale );

ret_val<tripoint_abs_omt> try_parse_coordinate_abs( std::string_view s );

#endif
