#pragma once
#ifndef CATA_SRC_MAGIC_TELEPORTER_LIST_H
#define CATA_SRC_MAGIC_TELEPORTER_LIST_H

#include <map>
#include <optional>
#include <set>
#include <string>

#include "coordinates.h"

class Character;
class JsonObject;
class JsonOut;

class teleporter_list
{
    private:

        std::map<tripoint_abs_omt, std::string> known_teleporters;


        std::optional<tripoint_abs_omt> choose_teleport_location();


        bool place_avatar_overmap( Character &you, const tripoint_abs_omt &omt_pt ) const;
    public:
        bool knows_translocator( const tripoint_abs_omt &omt_pos ) const;

        bool copy_translocator( const tripoint_abs_omt &omt_pos, const tripoint_abs_omt &omt_pos_new );

        bool remove_translocator( const tripoint_abs_omt &omt_pos );

        bool activate_teleporter( const tripoint_abs_omt &omt_pt, const tripoint_bub_ms &local_pt );
        void deactivate_teleporter( const tripoint_abs_omt &omt_pt, const tripoint_bub_ms &local_pt );



        void translocate( const std::set<tripoint_bub_ms> &targets );

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
};

#endif
