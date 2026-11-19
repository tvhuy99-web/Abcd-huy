#pragma once
#ifndef CATA_SRC_DISPLAY_H
#define CATA_SRC_DISPLAY_H

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <utility>

#include "bodypart.h"
#include "color.h"
#include "coordinates.h"
#include "point.h"
#include "type_id.h"

class Character;
class Creature;
class avatar;
class mood_face;
class time_point;
class translation;
class vehicle;
enum class cardinal_direction : int;


enum class bodygraph_var : int {
    hp = 0,
    temp,
    encumb,
    status,
    wet,
    last
};

struct disp_overmap_cache {
    private:
        tripoint_abs_omt _center;
        tripoint_abs_omt _mission;
        std::string _om_wgt_str;
        int _width;

    public:
        disp_overmap_cache();



        bool is_valid_for( const tripoint_abs_omt &center,
                           const tripoint_abs_omt &mission, int width ) const {
            return _center == center && _mission == mission && _width == width;
        }



        void rebuild( const tripoint_abs_omt &center, const tripoint_abs_omt &mission, int width,
                      const std::string &om_wgt_str ) {
            _center = center;
            _mission = mission;
            _om_wgt_str = om_wgt_str;
            _width = width;
        }


        const std::string &get_val() const {
            return _om_wgt_str;
        }


        void invalidate() {
            _center = tripoint_abs_omt::invalid;
        }
};

struct disp_bodygraph_cache {
    private:
        bodygraph_var _var;
        std::map<bodypart_id, int> _bp_cur_max;
        std::string _graph_id;
        std::string _graph_wgt_str;

    public:
        explicit disp_bodygraph_cache( bodygraph_var var );


        bool is_valid_for( const Character &u, const std::string &graph_id ) const;



        void rebuild( const Character &u, const std::string &graph_id, const std::string &bg_wgt_str );


        const std::string &get_val() const {
            return _graph_wgt_str;
        }
};




namespace display
{


std::string date_string();

std::string time_approx( const time_point &turn );

std::string time_approx();

std::string time_string( const Character &u );

std::string sundial_text_color( const Character &u, int width = 0 );

std::string sundial_time_text_color( const Character &u, int width );


std::string get_temp( const Character &u );


std::string activity_level_str( float level );

std::string activity_malus_str( const Character &u );

std::string weight_long_description( const Character &u );


vehicle *vehicle_driven( const Character &u );


std::string vehicle_azimuth_text( const Character &u );

std::pair<std::string, nc_color> vehicle_cruise_text_color( const Character &u );

std::pair<std::string, nc_color> vehicle_fuel_percent_text_color( const Character &u );

std::pair<std::string, nc_color> vehicle_battery_percent_text_color( const Character &u );


std::pair<translation, nc_color> weariness_text_color( size_t weariness );
std::pair<std::string, nc_color> weariness_text_color( const Character &u );
std::pair<std::string, nc_color> weary_malus_text_color( const Character &u );
std::pair<std::string, nc_color> activity_text_color( const Character &u );
std::pair<std::string, nc_color> thirst_text_color( const Character &u );
std::pair<std::string, nc_color> hunger_text_color( const Character &u );
std::pair<std::string, nc_color> weight_text_color( const Character &u );
std::pair<std::string, nc_color> health_text_color( const Character &u );
std::pair<std::string, nc_color> sleepiness_text_color( const Character &u );
std::pair<std::string, nc_color> pain_text_color( const Creature &c );
std::pair<std::string, nc_color> pain_text_color( const Character &u );
std::pair<std::string, nc_color> faction_text( const Character &u );

std::pair<std::string, nc_color> morale_face_color( const avatar &u );

std::pair<std::string, nc_color> morale_emotion( int morale_cur, const mood_face &face );


std::pair<std::string, nc_color> move_mode_letter_color( const Character &u );

std::pair<std::string, nc_color> move_count_and_mode_text_color( const avatar &u );

std::pair<std::string, nc_color> carry_weight_text_color( const avatar &u );

std::pair<std::string, nc_color> temp_text_color( const Character &u,
        const bodypart_str_id &bp = bodypart_str_id::NULL_ID() );
std::pair<std::string, nc_color> power_text_color( const Character &u );
std::pair<std::string, nc_color> power_balance_text_color( const avatar &u );

std::pair<std::string, nc_color> safe_mode_text_color( bool classic_mode );
std::pair<std::string, nc_color> wind_text_color( const Character &u );
std::pair<std::string, nc_color> snow_depth_text_color( const Character &u );
std::pair<std::string, nc_color> weather_text_color( const Character &u );


std::string colorized_compass_text( cardinal_direction dir, int width );
std::string colorized_compass_legend_text( int width, int max_height, int &height );


std::string colorized_bodygraph_text( const Character &u, const std::string &graph_id,
                                      bodygraph_var var, int width, int max_height, int &height );


nc_color get_bodygraph_bp_color( const Character &u, const bodypart_id &bid,
                                 bodygraph_var var );


nc_color bodytemp_color( const Character &u, const bodypart_id &bp );

nc_color limb_color( const Character &u, const bodypart_id &bp, bool bleed, bool bite,
                     bool infect );


nc_color encumb_color( int level );


std::pair<std::string, nc_color> carry_weight_value_color( const avatar &ava );


std::pair<std::string, nc_color> overmap_tile_symbol_color( const avatar &u,
        const tripoint_abs_omt &omt, bool edge_tile, bool &found_mi );

std::pair<std::string, nc_color> overmap_note_symbol_color( std::string_view note_text );

point mission_arrow_offset( const avatar &you, int width, int height );

std::string colorized_overmap_text( const avatar &u, int width, int height );

std::string overmap_position_text( const tripoint_abs_omt &loc );

std::string current_position_text( const tripoint_abs_omt &loc );



std::string weight_string( const Character &u );



std::string health_string( const Character &u );


void invalidate_overmap_cache();
}

#endif
