#pragma once
#ifndef CATA_SRC_TIMED_EVENT_H
#define CATA_SRC_TIMED_EVENT_H

#include <list>
#include <string>

#include "calendar.h"
#include "coordinates.h"
#include "explosion.h"
#include "point.h"
#include "submap.h"

class JsonArray;
class JsonOut;

enum class timed_event_type : int {
    NONE,
    HELP,
    WANTED,
    ROBOT_ATTACK,
    SPAWN_WYRMS,
    AMIGARA,
    AMIGARA_WHISPERS,
    ROOTS_DIE,
    TEMPLE_OPEN,
    TEMPLE_FLOOD,
    TEMPLE_SPAWN,
    DIM,
    ARTIFACT_LIGHT,
    DSA_ALRP_SUMMON,
    CUSTOM_LIGHT_LEVEL,
    TRANSFORM_RADIUS,
    UPDATE_MAPGEN,
    REVERT_SUBMAP,
    OVERRIDE_PLACE,
    EXPLOSION,
    NUM_TIMED_EVENT_TYPES
};

struct timed_event {
    timed_event_type type = timed_event_type::NONE;

    time_point when = calendar::turn_zero;

    int faction_id = -1;

    tripoint_abs_sm map_point = tripoint_abs_sm::invalid;

    tripoint_abs_ms map_square = tripoint_abs_ms::invalid;

    int strength = -1;

    std::string string_id;

    std::string key;

    explosion_data expl_data;

    submap revert;
    timed_event( timed_event_type e_t, const time_point &w, int f_id, tripoint_abs_ms p, int s,
                 std::string key );
    timed_event( timed_event_type e_t, const time_point &w, int f_id, tripoint_abs_ms p, int s,
                 std::string s_id, std::string key );
    timed_event( timed_event_type e_t, const time_point &w, int f_id, tripoint_abs_ms p, int s,
                 std::string s_id, submap sr, std::string key );



    timed_event( timed_event_type e_t, const time_point &w, const tripoint_abs_ms &p,
                 explosion_data explos_data );


    void actualize();

    void per_turn();
};

class timed_event_manager
{
    private:
        std::list<timed_event> events;

    public:




        void add( timed_event_type type, const time_point &when, int faction_id = -1, int strength = -1,
                  const std::string &key = "" );




        void add( timed_event_type type, const time_point &when, int faction_id,
                  const tripoint_abs_ms &where, int strength = -1, const std::string &key = "" );
        void add( timed_event_type type, const time_point &when, int faction_id,
                  const tripoint_abs_ms &where, int strength, const std::string &string_id,
                  const std::string &key = "" );
        void add( timed_event_type type, const time_point &when, int faction_id,
                  const tripoint_abs_ms &where, int strength, const std::string &string_id, submap sr,
                  const std::string &key = "" );
        void add( timed_event_type type, const time_point &when, const tripoint_abs_ms &where,
                  explosion_data expl_data );

        bool queued( timed_event_type type ) const;


        timed_event *get( timed_event_type type );
        timed_event *get( timed_event_type type, const std::string &key );
        std::list<timed_event> const &get_all() const;
        void set_all( const std::string &key, time_duration time_in_future );


        void process();
        static void serialize_all( JsonOut &jsout );
        static void unserialize_all( const JsonArray &ja );
};

timed_event_manager &get_timed_events();
bool you_know_where_you_are();

#endif
