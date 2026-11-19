#pragma once
#ifndef CATA_SRC_WEATHER_H
#define CATA_SRC_WEATHER_H

#include <optional>
#include <string>
#include <string_view>

#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "coordinates.h"
#include "pimpl.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "weather_gen.h"
#include "weather_type.h"

class JsonObject;
class JsonOut;
class translation;









constexpr units::temperature BODYTEMP_FREEZING = 28_C;

constexpr units::temperature BODYTEMP_VERY_COLD = 31_C;

constexpr units::temperature BODYTEMP_COLD = 34_C;

constexpr units::temperature BODYTEMP_NORM = 37_C;

constexpr units::temperature BODYTEMP_HOT = 40_C;

constexpr units::temperature BODYTEMP_VERY_HOT = 43_C;

constexpr units::temperature BODYTEMP_SCORCHING = 46_C;


constexpr units::temperature_delta BODYTEMP_THRESHOLD = 1_C_delta;





constexpr float BODYWET_PERCENT_WET = 0.3f;

constexpr float BODYWET_PERCENT_SOAKED = 0.6f;


namespace irradiance
{

constexpr float minimal = 87;


constexpr float low = 422;


constexpr float moderate = 573;


constexpr float high = 707;


constexpr float very_high = 866;


constexpr float extreme = 906;
}

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

class Character;
class Creature;
class item;
struct rl_vec2d;
struct trap;

double precip_mm_per_hour( precip_class p );
void handle_weather_effects( const weather_type_id &w );






struct weather_printable {

    weather_type_id wtype;

    std::vector<std::pair<int, int> > vdrops;

    nc_color colGlyph;

    uint32_t cGlyph;
    std::string get_symbol() const {
        return utf32_to_utf8( cGlyph );
    }
};

struct weather_sum {
    int rain_amount = 0;
    float sunlight = 0.0f;
    float radiant_exposure = 0.0f;
    int wind_amount = 0;
};
bool is_creature_outside( const Creature &target );
bool can_creature_see_sky( const Creature &target );
void wet_character( Character &target, int amount );
weather_type_id get_bad_weather();
std::string get_shortdirstring( int angle );

std::string get_dirstring( int angle );

std::string weather_forecast( const point_abs_sm &abs_sm_pos );







std::string print_temperature( units::temperature temperature, int decimals = 0 );
std::string print_humidity( double humidity, int decimals = 0 );
std::string print_pressure( double pressure, int decimals = 0 );


units::temperature_delta get_local_windchill( units::temperature temperature, double humidity,
        double wind_mph );

int get_local_humidity( double humidity, const weather_type_id &weather, bool sheltered = false );


int get_local_windpower( int windpower, const oter_id &omter, const tripoint_abs_ms &location,
                         const int &winddirection,
                         bool sheltered = false );
weather_sum sum_conditions( const time_point &start,
                            const time_point &end,
                            const tripoint_abs_ms &location );







void retroactively_fill_from_funnel( item &it, const trap &tr, const time_point &start,
                                     const time_point &end, const tripoint_abs_ms &pos );

double funnel_charges_per_turn( double surface_area_mm2, double rain_depth_mm_per_hour );

rl_vec2d convert_wind_to_coord( int angle );

std::string get_wind_arrow( int );

std::string get_wind_desc( double );

nc_color get_wind_color( double );





ret_val<void> warm_enough_to_plant( const tripoint_bub_ms &pos, const itype_id &it );

bool is_wind_blocker( const tripoint_bub_ms &location );

weather_type_id current_weather( const tripoint_abs_ms &location,
                                 const time_point &t = calendar::turn );

void glare( const weather_type_id &w );




float incident_sunlight( const weather_type_id &wtype,
                         const time_point &t = calendar::turn );

float incident_moonlight( const weather_type_id &wtype,
                          const time_point &t = calendar::turn );


float incident_sun_irradiance( const weather_type_id &wtype,
                               const time_point &t = calendar::turn );

void weather_sound( const translation &sound_message, std::string_view sound_effect );

struct omt_snow_state {
    double depth_mm = 0;
    time_point last_update = calendar::turn;
};

class weather_manager
{
    public:
        weather_manager();

        void on_game_start();
        const weather_generator &get_cur_weather_gen() const;

        void update_weather();

        void update_snow_depth();

        double get_snow_depth_mm( const tripoint_abs_omt &omt_pos ) const;

        units::temperature temperature = 0_K;
        bool lightning_active = false;

        weather_type_id weather_id = WEATHER_NULL;
        int winddirection = 0;
        int windspeed = 0;

        bool weather_changed = false;


        std::optional<units::temperature> forced_temperature;

        pimpl<w_point> weather_precise;
        std::optional<int> wind_direction_override;
        std::optional<int> windspeed_override;
        weather_type_id weather_override;

        void set_nextweather( time_point t );

        time_point nextweather;

        std::unordered_map< tripoint_bub_ms, units::temperature > temperature_cache;

        std::unordered_map< tripoint_abs_omt, omt_snow_state > snow_depth_map;




        units::temperature get_temperature( const tripoint_bub_ms &location );




        units::temperature get_area_temperature( const tripoint_abs_omt &location ) const;
        void clear_temp_cache();
        static void serialize_all( JsonOut &json );
        static void unserialize_all( const JsonObject &w );
};

weather_manager &get_weather();
const weather_manager &get_weather_const();

#endif
