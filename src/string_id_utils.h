#pragma once
#ifndef CATA_SRC_STRING_ID_UTILS_H
#define CATA_SRC_STRING_ID_UTILS_H

#include "string_id.h"
#include <algorithm>
#include <vector>









template<typename Col,
         typename El = std::decay_t<decltype( *std::declval<const Col &>().begin() )>,
         typename K =  std::decay_t<typename El::first_type>,
         typename V = std::decay_t<typename El::second_type>,
         std::enable_if_t<std::is_same_v<K, string_id<typename K::value_type>>, int> = 0>
std::vector<std::pair<K, V>> sorted_lex( Col col )
{
    std::vector<std::pair<K, V>> ret;
    ret.insert( ret.begin(), col.begin(), col.end() );
    std::sort( ret.begin(), ret.end(), []( const auto & a, const auto & b ) {
        return typename K::LexCmp()( a.first, b.first );
    } );
    return ret;
}






template<typename Col,
         typename El = std::decay_t<decltype( *std::declval<const Col &>().begin() )>,
         std::enable_if_t<std::is_same_v<El, string_id<typename El::value_type>>, int> = 0>
std::vector<El> sorted_lex( const Col &col )
{
    std::vector<El> ret;
    ret.insert( ret.begin(), col.begin(), col.end() );
    std::sort( ret.begin(), ret.end(), typename El::LexCmp() );
    return ret;
}

#endif
