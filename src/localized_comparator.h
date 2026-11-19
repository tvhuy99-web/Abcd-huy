#pragma once
#ifndef CATA_SRC_LOCALIZED_COMPARATOR_H
#define CATA_SRC_LOCALIZED_COMPARATOR_H

#include "translation.h"








struct localized_comparator {
    template<typename T, typename U>
    bool operator()( const std::pair<T, U> &l, const std::pair<T, U> &r ) const {
        if( ( *this )( l.first, r.first ) ) {
            return true;
        }
        if( ( *this )( r.first, l.first ) ) {
            return false;
        }
        return ( *this )( l.second, r.second );
    }

    template<typename Head, typename... Tail>
    bool operator()( const std::tuple<Head, Tail...> &l,
                     const std::tuple<Head, Tail...> &r ) const {
        if( ( *this )( std::get<0>( l ), std::get<0>( r ) ) ) {
            return true;
        }
        if( ( *this )( std::get<0>( r ), std::get<0>( l ) ) ) {
            return false;
        }
        constexpr std::make_index_sequence<sizeof...( Tail )> Ints{};
        return ( *this )( tie_tail( l, Ints ), tie_tail( r, Ints ) );
    }

    template<typename T>
    bool operator()( const T &l, const T &r ) const {
        return l < r;
    }

    bool operator()( const std::string &, const std::string & ) const;
    bool operator()( const translation &, const translation & ) const;

    template<typename Head, typename... Tail, size_t... Ints>
    auto tie_tail( const std::tuple<Head, Tail...> &t, std::index_sequence<Ints...> ) const {
        return std::tie( std::get < Ints + 1 > ( t )... );
    }
};

constexpr localized_comparator localized_compare{};

#endif
