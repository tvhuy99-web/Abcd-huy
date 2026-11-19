#pragma once
#ifndef CATA_SRC_MOD_TRACKER_H
#define CATA_SRC_MOD_TRACKER_H

#include <string>

#include "cata_type_traits.h"
#include "debug.h"
#include "demangle.h"
#include "type_id.h"

















class mod_error : public std::runtime_error
{
    public:
        explicit mod_error( const std::string &msg ) : std::runtime_error( msg ) {}
};

struct mod_tracker {

    template<typename T, typename = std::void_t<>>
    struct has_src_member : std::false_type {};

    template<typename T>
struct has_src_member<T, std::void_t<decltype( std::declval<T &>().src.emplace_back( std::declval<T &>().id, mod_id() ) )>> :
    std::true_type {};


    template < typename T, typename std::enable_if_t < !has_src_member<T>::value > * = nullptr >
    static void assign_src( T &, const std::string_view ) {
    }


    template<typename T, typename std::enable_if_t<has_src_member<T>::value > * = nullptr>
    static void assign_src( T &def, const std::string_view src ) {


        if( !def.src.empty() && def.src.back().first != def.id ) {
            def.src.clear();
        }
        def.src.emplace_back( def.id, mod_id( src ) );
    }


    template < typename T, typename std::enable_if_t < !has_src_member<T>::value > * = nullptr >
    static void check_duplicate_entries( const T &n, const T & ) {
        static bool reported = false;

        if( reported ) {
            return;
        }
        reported = true;
        DebugLog( D_INFO, D_MAIN ) << __FILE__ << ":" << __LINE__ << ": " << "Tried check if '" <<
                                   n.id.str() << "' had a duplicate, but type '" << demangle( typeid( T ).name() ) <<
                                   "' does not track object sources";
    }


    template<typename T, typename std::enable_if_t<has_src_member<T>::value > * = nullptr>
    static void check_duplicate_entries( const T &n, const T &o ) {


        if( n.src.back() == o.src.back() ) {
            throw mod_error( string_format( "%s (%s) has two definitions from the same source (%s)!",
                                            n.id.str(), demangle( typeid( T ).name() ), n.src.back().second.str() ) );
        }
    }

};

#endif
