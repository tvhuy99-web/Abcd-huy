#pragma once
#ifndef CATA_SRC_MAP_MEMORY_H
#define CATA_SRC_MAP_MEMORY_H

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "coordinates.h"
#include "map_scale_constants.h"
#include "mdarray.h"
#include "memory_fast.h"
#include "point.h" // IWYU pragma: keep
#include "type_id.h"

class JsonArray;
class JsonOut;
class JsonValue;

class memorized_tile
{
    public:
        char32_t symbol = 0;

        const std::string &get_ter_id() const;
        const std::string &get_dec_id() const;
        void set_ter_id( std::string_view id );
        void set_dec_id( std::string_view id );

        int get_ter_rotation() const;
        void set_ter_rotation( int rotation );
        int get_dec_rotation() const;
        void set_dec_rotation( int rotation );

        int get_ter_subtile() const;
        void set_ter_subtile( int subtile );
        int get_dec_subtile() const;
        void set_dec_subtile( int subtile );

        bool operator==( const memorized_tile &rhs ) const;
        bool operator!=( const memorized_tile &rhs ) const {
            return !( *this == rhs );
        }
    private:
        friend struct mm_submap;
        ter_str_id ter_id;
        std::string dec_id;
        int8_t ter_rotation = 0;
        int8_t dec_rotation = 0;
        int8_t ter_subtile = 0;
        int8_t dec_subtile = 0;
};


struct mm_submap {
    public:
        static const memorized_tile default_tile;

        mm_submap() = default;
        explicit mm_submap( bool make_valid );


        bool is_empty() const;

        bool is_valid() const;

        const memorized_tile &get_tile( const point_sm_ms &p ) const;
        void set_tile( const point_sm_ms &p, const memorized_tile &value );

        void serialize( JsonOut &jsout ) const;
        void deserialize( int version, const JsonArray &ja );

    private:
        // NOLINTNEXTLINE(cata-serialize)
        std::vector<memorized_tile> tiles;
        // NOLINTNEXTLINE(cata-serialize)
        bool valid = true;
};






struct mm_region {
    cata::mdarray<shared_ptr_fast<mm_submap>, point, MM_REG_SIZE, MM_REG_SIZE> submaps;

    mm_region();

    bool is_empty() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonValue &ja );
};




class map_memory
{
    private:




        struct coord_pair {
            tripoint_abs_sm sm;
            point_sm_ms loc;

            explicit coord_pair( const tripoint_abs_ms &p );
        };

    public:
        map_memory();


        bool is_valid() const;


        void load( const tripoint_abs_ms &pos );


        bool save( const tripoint_abs_ms &pos );


        void clear();







        bool prepare_region( const tripoint_abs_ms &p1, const tripoint_abs_ms &p2 );





        const memorized_tile &get_tile( const tripoint_abs_ms &pos ) const;





        void set_tile_terrain( const tripoint_abs_ms &pos, std::string_view id,
                               int subtile, int rotation );





        void set_tile_decoration( const tripoint_abs_ms &pos, std::string_view id,
                                  int subtile, int rotation );





        void set_tile_symbol( const tripoint_abs_ms &pos, char32_t symbol );






        void clear_tile_decoration( const tripoint_abs_ms &pos, std::string_view prefix = "" );

    private:
        std::map<tripoint_abs_sm, shared_ptr_fast<mm_submap>> submaps;

        mutable std::map<int, std::vector<shared_ptr_fast<mm_submap>>> cached;
        tripoint_abs_sm cache_pos;
        point cache_size;


        shared_ptr_fast<mm_submap> fetch_submap( const tripoint_abs_sm &sm_pos );

        shared_ptr_fast<mm_submap> find_submap( const tripoint_abs_sm &sm_pos );

        shared_ptr_fast<mm_submap> load_submap( const tripoint_abs_sm &sm_pos );

        shared_ptr_fast<mm_submap> allocate_submap( const tripoint_abs_sm &sm_pos );



        const mm_submap &get_submap( const tripoint_abs_sm &sm_pos ) const;
        mm_submap &get_submap( const tripoint_abs_sm &sm_pos );


        void clear_cache();
};

#endif
