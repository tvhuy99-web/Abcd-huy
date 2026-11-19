#pragma once
#ifndef CATA_SRC_MAPBUFFER_H
#define CATA_SRC_MAPBUFFER_H

#include <list>
#include <map>
#include <memory>

#include "coordinates.h"

class JsonArray;
class cata_path;
class submap;




class mapbuffer
{
    public:
        mapbuffer();
        ~mapbuffer();





        void save( bool delete_after_save = false );


        void clear();






        void clear_outside_reality_bubble();











        bool add_submap( const tripoint_abs_sm &p, std::unique_ptr<submap> &sm );

        bool add_submap( const tripoint_abs_sm &p, submap *sm );









        submap *lookup_submap( const tripoint_abs_sm &p );


        bool submap_exists( const tripoint_abs_sm &p );


        bool submap_exists_approx( const tripoint_abs_sm &p );

    private:
        using submap_map_t = std::map<tripoint_abs_sm, std::unique_ptr<submap>>;

    public:
        inline submap_map_t::iterator begin() {
            return submaps.begin();
        }
        inline submap_map_t::iterator end() {
            return submaps.end();
        }

    private:


        void remove_submap( const tripoint_abs_sm &addr );
        submap *unserialize_submaps( const tripoint_abs_sm &p );
        bool submap_file_exists( const tripoint_abs_sm &p );
        void deserialize( const JsonArray &ja );
        void save_quad(
            const cata_path &dirname, const cata_path &filename,
            const tripoint_abs_omt &om_addr, std::list<tripoint_abs_sm> &submaps_to_delete,
            bool delete_after_save );
        submap_map_t submaps; // NOLINT(cata-serialize)
};

extern mapbuffer MAPBUFFER;

#endif
