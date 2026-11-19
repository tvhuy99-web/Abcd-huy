#pragma once
#ifndef CATA_TESTS_MAP_HELPERS_H
#define CATA_TESTS_MAP_HELPERS_H

#include <string>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "type_id.h"

class map;
class monster;
class submap;
class time_point;

monster &spawn_test_monster( const std::string &monster_type, const tripoint_bub_ms &start,
                             bool death_drops = true );
void build_test_map( const ter_id &terrain );
void build_water_test_map( const ter_id &surface, const ter_id &mid, const ter_id &bottom );
void player_add_headlamp();
void player_wear_blindfold();
void set_time_to_day();
void set_time( const time_point &time );

using los_pairs = std::vector<std::pair<tripoint_bub_ms, tripoint_bub_ms>>;


los_pairs los_pairs_around( const tripoint_bub_ms &center, int radius );


enum class vision_layers {


    scene_and_fov,

    light,

    all,
};



class vision_cache_oracle
{
    public:
        explicit vision_cache_oracle( los_pairs pairs );


        void prime() const;


        void check_matches_rebuild( vision_layers layers = vision_layers::all ) const;
        void check_matches_rebuild( vision_layers layers, int zlev ) const;
    private:
        los_pairs pairs_;
};


void check_stationary_build_is_noop();

class map_meddler
{
    public:
        static bool has_altered_submaps( map &m );
        static submap *unsafe_get_submap_at( tripoint_bub_ms &p, point_sm_ms &l );
};

#endif
