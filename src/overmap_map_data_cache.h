#pragma once
#ifndef CATA_SRC_OVERMAP_MAP_DATA_CACHE_H
#define CATA_SRC_OVERMAP_MAP_DATA_CACHE_H

#include <bitset>
#include <memory>
#include <string>
#include <string_view>

#include "string_id.h"

class JsonObject;
struct map_data_summary;

namespace map_data_placeholders
{
void load( const JsonObject &jo, const std::string &src );
void reset();
std::shared_ptr<const map_data_summary> get_ptr( string_id<map_data_summary> id );
void finalize();
}










struct map_data_summary {
    map_data_summary() = default;
    explicit map_data_summary( std::bitset<24 * 24> const &new_passable,
                               bool placeholder_override = false ): placeholder( placeholder_override ),
        passable( new_passable ) {}
    void load( const JsonObject &jo, const std::string_view &src );

    string_id<map_data_summary> id;
    bool was_loaded = false;



    bool placeholder = false;



    std::bitset<24 * 24> passable;
};

#endif
