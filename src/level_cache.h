#pragma once
#ifndef CATA_SRC_LEVEL_CACHE_H
#define CATA_SRC_LEVEL_CACHE_H

#include <array>
#include <bitset>
#include <cstdint>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "lightmap.h"
#include "map_scale_constants.h"
#include "mdarray.h"
#include "shadowcasting.h"

// IWYU pragma: no_forward_declare four_quadrants
class vehicle;



uint64_t next_cache_generation();




struct level_cache_default_zero_members {
    cata::mdarray<four_quadrants, point_bub_ms> lm;


    cata::mdarray<four_quadrants, point_bub_ms> sun_lm;
    cata::mdarray<float, point_bub_ms> sm;


    cata::mdarray<light_color_rgb, point_bub_ms> light_color_cache;


    struct buffered_light_source {
        buffered_light_source() = default;
        float luminance;
        light_color_rgb color;
    };
    cata::mdarray<buffered_light_source, point_bub_ms> light_source_buffer;


    cata::mdarray<bool, point_bub_ms> outside_cache;







    cata::mdarray<bool, point_bub_ms> floor_cache;



    cata::mdarray<float, point_bub_ms>transparency_cache;





    std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X> transparent_cache_wo_fields;



    cata::mdarray<float, point_bub_ms> sight_cache;

    std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X> sight_cache_wo_fields;

    cata::mdarray<bool, point_bub_ms> vehicle_opaque_cache;


    cata::mdarray<bool, point_bub_ms> transparency_outside;
    cata::mdarray<bool, point_bub_ms> transparency_vehicle_opaque;




    cata::mdarray<float, point_bub_ms> vision_transparency_cache;



    cata::mdarray<float, point_bub_ms> seen_cache;





    cata::mdarray<float, point_bub_ms> camera_cache;



    std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X> ledge_hidden;


    cata::mdarray<lit_level, point_bub_ms> visibility_cache;
    std::bitset<MAPSIZE_X *MAPSIZE_Y> map_memory_cache_dec;
    std::bitset<MAPSIZE_X *MAPSIZE_Y> map_memory_cache_ter;
    std::bitset<MAPSIZE *MAPSIZE> field_cache;
};


static_assert( std::is_trivially_copyable_v<level_cache_default_zero_members> );

struct level_cache : level_cache_default_zero_members {
    public:

        level_cache();
        level_cache( const level_cache &other ) = default;

        void clear();

        std::bitset<MAPSIZE *MAPSIZE> transparency_cache_dirty;


        std::bitset<MAPSIZE *MAPSIZE> vision_transparency_dirty;


        std::vector<std::pair<point_bub_ms, float>> vision_observer_overrides;

        float built_sight_penalty = -1.0f;

        bool outside_rewritten = true;


        bool vehicle_opaque_any = false;
        bool transparency_vehicle_opaque_any = false;


        uint64_t sight_revision = 0;


        uint64_t geometry_revision = 0;

        uint64_t field_revision = 0;
        bool outside_cache_dirty = false;
        bool floor_cache_dirty = false;
        bool seen_cache_dirty = false;
        bool lightmap_dirty = true;



        uint64_t lightmap_generation = 0;

        uint64_t sun_revision = 0;

        bool light_full = false;


        bool light_changes_by_turn = false;

        light_color_rgb light_tint{};


        float sun_uniform = -1.0f;

        uint64_t lightmap_sun_revision = 0;


        uint64_t visibility_generation = 0;


        bool map_memory_sweep_pending = true;


        bool has_colored_lights = false;

        bool no_floor_gaps = false;


        float natural_light_level_cache = 0.0f;

        std::set<vehicle *> vehicle_list;
        std::set<vehicle *> zone_vehicles;

        bool get_veh_in_active_range() const;
        bool get_veh_exists_at( const tripoint_bub_ms &pt ) const;
        std::pair<vehicle *, int> get_veh_cached_parts( const tripoint_bub_ms &pt ) const;

        void set_veh_exists_at( const tripoint_bub_ms &pt, bool exists_at );
        void set_veh_cached_parts( const tripoint_bub_ms &pt, vehicle &veh, int part_num );

        void clear_vehicle_cache();
        void clear_veh_from_veh_cached_parts( const tripoint_bub_ms &pt, vehicle *veh );

    private:


        bool veh_cache_cleared = true;
        std::bitset<MAPSIZE_X *MAPSIZE_Y> veh_exists_at;
        std::unordered_map<tripoint_bub_ms, std::pair<vehicle *, int>> veh_cached_parts;
};
#endif
