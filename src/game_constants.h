#pragma once
#ifndef CATA_SRC_GAME_CONSTANTS_H
#define CATA_SRC_GAME_CONSTANTS_H

#include <map>
#include <string>
#include "calendar.h"
#include "units.h"


constexpr int EVEN_MINIMUM_TERM_WIDTH = 80;
constexpr int EVEN_MINIMUM_TERM_HEIGHT = 24;
constexpr int MAXIMUM_TERM_WIDTH = 960;
constexpr int MAXIMUM_TERM_HEIGHT = 270;
constexpr int HP_HEIGHT = 14;
constexpr int HP_WIDTH = 7;
constexpr int MINIMAP_HEIGHT = 7;
constexpr int MINIMAP_WIDTH = MINIMAP_HEIGHT;
constexpr int MONINFO_HEIGHT = 12;
constexpr int MONINFO_WIDTH = 48;
constexpr int MESSAGES_HEIGHT = 8;
constexpr int MESSAGES_WIDTH = 48;
constexpr int LOCATION_HEIGHT = 1;
constexpr int LOCATION_WIDTH = 48;
constexpr int STATUS_HEIGHT = 4;
constexpr int STATUS_WIDTH = 55;

constexpr int EXPLOSION_MULTIPLIER = 7;

constexpr int fov_3d_z_range = 10;


constexpr int MAX_ITEM_IN_SQUARE = 4096;

constexpr int MAX_ITEM_IN_VEHICLE_STORAGE = MAX_ITEM_IN_SQUARE;

constexpr units::volume DEFAULT_TILE_VOLUME = units::from_liter( 1000 );
constexpr units::volume MAX_ITEM_VOLUME = DEFAULT_TILE_VOLUME;

constexpr int MAX_WORN_PER_TYPE = 2;





constexpr int PICKUP_RANGE = 6;


constexpr int MAX_HANDLING_COST = 400;

constexpr int INVENTORY_HANDLING_PENALTY = 100;

constexpr int MAP_HANDLING_PENALTY = 80;

constexpr int VEHICLE_HANDLING_PENALTY = 80;


constexpr int PLUTONIUM_CHARGES = 500;


constexpr int VEH_PART_DMG_REDUCTION_FROM_DURABILITY_CAP = 20;


namespace temperatures
{

constexpr units::temperature hot = units::from_fahrenheit( 100 );


constexpr units::temperature normal = units::from_fahrenheit( 70 );


constexpr units::temperature fridge = units::from_fahrenheit( 37 );


constexpr units::temperature cold = units::from_fahrenheit( 40 );


constexpr units::temperature freezer = units::from_celsius( -5 );


constexpr units::temperature freezing = units::from_celsius( 0 );


constexpr units::temperature boiling = units::from_celsius( 100 );
}


constexpr int MAX_AIM_COST = 10;


constexpr int MIN_SKILL = 0;

constexpr int MAX_SKILL = 10;


constexpr double accuracy_headshot = 0.1;
constexpr double accuracy_critical = 0.2;
constexpr double accuracy_goodhit  = 0.5;
constexpr double accuracy_standard = 0.8;
constexpr double accuracy_grazing  = 1.0;



constexpr double MAX_RECOIL = 3000;


constexpr int MELEE_STAT = 5;


constexpr int BIO_CQB_LEVEL = 5;


constexpr int HORDE_VISIBILITY_SIZE = 3;


constexpr time_duration time_between_npc_OM_moves = 5_minutes;





constexpr units::temperature SPRING_TEMPERATURE = units::from_fahrenheit( 65 );





constexpr int SIMPLEX_NOISE_RANDOM_SEED_LIMIT = 32768;

constexpr float MIN_MANIPULATOR_SCORE = 0.1f;

constexpr float MAX_MOVECOST_MODIFIER = 100.0f;











constexpr float SLEEP_EXERCISE = 0.85f;
constexpr float NO_EXERCISE = 1.0f;
constexpr float LIGHT_EXERCISE = 2.0f;
constexpr float MODERATE_EXERCISE = 4.0f;
constexpr float BRISK_EXERCISE = 6.0f;
constexpr float ACTIVE_EXERCISE = 8.0f;
constexpr float EXTRA_EXERCISE = 10.0f;

const std::map<std::string, float> activity_levels_map = {
    { "SLEEP_EXERCISE", SLEEP_EXERCISE },
    { "NO_EXERCISE", NO_EXERCISE },
    { "LIGHT_EXERCISE", LIGHT_EXERCISE },
    { "MODERATE_EXERCISE", MODERATE_EXERCISE },
    { "BRISK_EXERCISE", BRISK_EXERCISE },
    { "ACTIVE_EXERCISE", ACTIVE_EXERCISE },
    { "EXTRA_EXERCISE", EXTRA_EXERCISE }
};

const std::map<float, std::string> activity_levels_str_map = {
    { SLEEP_EXERCISE, "SLEEP_EXERCISE" },
    { NO_EXERCISE, "NO_EXERCISE" },
    { LIGHT_EXERCISE, "LIGHT_EXERCISE" },
    { MODERATE_EXERCISE, "MODERATE_EXERCISE" },
    { BRISK_EXERCISE, "BRISK_EXERCISE" },
    { ACTIVE_EXERCISE, "ACTIVE_EXERCISE" },
    { EXTRA_EXERCISE, "EXTRA_EXERCISE" }
};


namespace character_weight_category
{
constexpr float emaciated = 1.0f;
constexpr float underweight = 2.0f;
constexpr float normal = 3.0f;
constexpr float overweight = 5.0f;
constexpr float obese = 10.0f;
constexpr float very_obese = 15.0f;
constexpr float morbidly_obese = 20.0f;
}


namespace character_health_category
{

constexpr int very_bad = -100;
constexpr int bad = -50;
constexpr int fine = -10;
constexpr int good = 10;
constexpr int very_good = 50;
constexpr int great = 100;
}

#endif
