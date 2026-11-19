#pragma once
#ifndef CATA_SRC_WEATHER_GEN_H
#define CATA_SRC_WEATHER_GEN_H

#include <string>
#include <string_view>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "point.h"
#include "type_id.h"
#include "units.h"

class JsonObject;

struct w_point {
    units::temperature temperature = 0_K;
    double humidity = 0;
    double pressure = 0;
    double windpower = 0;
    std::string wind_desc;
    int winddirection = 0;
    season_effective_time time;
    tripoint_abs_ms location;
};

class weather_generator
{
    public:
        weather_generator_id id = weather_generator_id::NULL_ID();


        double base_temperature = 0;

        double base_humidity = 0;

        double base_pressure = 0;

        double base_wind = 0;

        int base_wind_distrib_peaks = 0;

        int summer_temp_manual_mod = 0;
        int spring_temp_manual_mod = 0;
        int autumn_temp_manual_mod = 0;
        int winter_temp_manual_mod = 0;
        int spring_humidity_manual_mod = 0;
        int summer_humidity_manual_mod = 0;
        int autumn_humidity_manual_mod = 0;
        int winter_humidity_manual_mod = 0;

        int base_wind_season_variation = 0;
        static int current_winddir;

        std::vector<std::string> weather_black_list;
        std::vector<std::string> weather_white_list;

        std::vector<weather_type_id> sorted_weather;
        weather_generator();






        w_point get_weather( const tripoint_abs_ms &, const time_point &, unsigned ) const;
        weather_type_id get_weather_conditions( const tripoint_abs_ms &, const time_point &,
                                                unsigned seed ) const;
        weather_type_id get_weather_conditions( const w_point & ) const;
        int get_wind_direction( season_type ) const;
        int convert_winddir( int ) const;
        units::temperature get_water_temperature() const;
        void test_weather( unsigned seed ) const;
        void sort_weather();
        units::temperature get_weather_temperature( const tripoint_abs_ms &, const time_point &,
                unsigned ) const;

        bool was_loaded = false;
        void load( const JsonObject &jo, std::string_view );
        void finalize();
        static void load_weather_generator( const JsonObject &jo, const std::string &src );
        static void reset();
        static void finalize_all();
};

#endif
