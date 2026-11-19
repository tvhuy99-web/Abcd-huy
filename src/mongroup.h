#pragma once
#ifndef CATA_SRC_MONGROUP_H
#define CATA_SRC_MONGROUP_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "enums.h"
#include "io_tags.h"
#include "mapgen_primitives.h"
#include "monster.h"
#include "point.h"
#include "type_id.h"

class JsonObject;
class JsonOut;

class overmap;
struct MonsterGroupEntry;
template <typename T> struct enum_traits;

using FreqDef = std::vector<MonsterGroupEntry>;
using FreqDef_iter = FreqDef::iterator;

struct MonsterGroupEntry {
    mtype_id mtype;
    mongroup_id group;
    int frequency;
    int cost_multiplier;
    int pack_minimum;
    int pack_maximum;
    spawn_data data;
    std::vector<std::string> conditions;
    time_duration starts;
    time_duration ends;
    holiday event;
    bool lasts_forever() const {
        return ends <= 0_turns;
    }
    bool is_group() const {
        return group != mongroup_id();
    }

    MonsterGroupEntry( const mtype_id &new_id, int new_freq, int new_cost, int new_pack_min,
                       int new_pack_max, const spawn_data &new_data, const time_duration &new_starts,
                       const time_duration &new_ends, holiday new_event )
        : mtype( new_id )
        , frequency( new_freq )
        , cost_multiplier( new_cost )
        , pack_minimum( new_pack_min )
        , pack_maximum( new_pack_max )
        , data( new_data )
        , starts( new_starts )
        , ends( new_ends )
        , event( new_event ) {
    }

    MonsterGroupEntry( const mongroup_id &new_id, int new_freq, int new_cost, int new_pack_min,
                       int new_pack_max, const spawn_data &new_data, const time_duration &new_starts,
                       const time_duration &new_ends, holiday new_event )
        : group( new_id )
        , frequency( new_freq )
        , cost_multiplier( new_cost )
        , pack_minimum( new_pack_min )
        , pack_maximum( new_pack_max )
        , data( new_data )
        , starts( new_starts )
        , ends( new_ends )
        , event( new_event ) {
    }
};

struct MonsterGroupResult {
    mtype_id id;
    int pack_size;
    spawn_data data;

    MonsterGroupResult() : id( mtype_id::NULL_ID() ), pack_size( 0 ) {
    }

    MonsterGroupResult( const mtype_id &new_id, int new_pack_size, const spawn_data &new_data )
        : id( new_id ), pack_size( new_pack_size ), data( new_data ) {
    }
};




struct MonsterGroup {
    mongroup_id id;
    mtype_id defaultMonster;
    FreqDef monsters;
    bool IsMonsterInGroup( const mtype_id &id ) const;
    bool is_animal = false;


    bool replace_monster_group = false;

    mongroup_id new_monster_group;

    time_duration monster_group_time = 0_turns;


    bool is_safe = false;
    int freq_total = 0;
    std::map<holiday, int> event_freq;


    int event_adjusted_freq_total( holiday event = holiday::num_holiday ) const;
};






struct mongroup {

    mongroup_id type;





    std::vector<monster> monsters;




    tripoint_abs_sm abs_pos;




    unsigned int population = 1;


    point_abs_sm target;

    point_abs_sm nemesis_target;

    int interest = 0;


    bool dying = false;

    bool horde = false;

    enum class horde_behaviour : short {
        none,
        city,
        roam,
        nemesis,
        last
    };
    horde_behaviour behaviour = horde_behaviour::none;

    mongroup( const mongroup_id &ptype, const tripoint_abs_sm &ppos,
              unsigned int ppop )
        : type( ptype )
        , abs_pos( ppos )
        , population( ppop )
        , target( abs_pos.xy() ) {
    }
    mongroup( const std::string &ptype, const tripoint_abs_sm &ppos,
              unsigned int ppop, point_abs_sm ptarget, int pint, bool pdie, bool phorde ) :
        type( ptype ), abs_pos( ppos ), population( ppop ), target( ptarget ),
        interest( pint ), dying( pdie ), horde( phorde ) { }
    mongroup() = default;
    bool is_safe() const;
    bool empty() const;
    void clear();
    tripoint_om_sm rel_pos() const {
        return project_remain<coords::om>( abs_pos ).remainder_tripoint;
    }
    void set_target( const point_abs_sm &p ) {
        target = p;
    }
    void set_nemesis_target( const point_abs_sm &p ) {
        nemesis_target = p;
    }
    void wander( const overmap & );





    void inc_interest( int inc ) {
        interest += inc;
        if( interest > 100 ) {
            interest = 100;
        }
    }





    void dec_interest( int dec ) {
        interest -= dec;
        if( interest < 15 ) {
            interest = 15;
        }
    }




    void set_interest( int set ) {
        if( set < 15 ) {
            set = 15;
        }
        if( set > 100 ) {
            set = 100;
        }
        interest = set;
    }
    float avg_speed() const;

    template<typename Archive>
    void io( Archive & );
    using archive_type_tag = io::object_archive_tag;

    void deserialize( const JsonObject &jo );
    void deserialize_legacy( const JsonObject &jo );
    void serialize( JsonOut &json ) const;
};

template<>
struct enum_traits<mongroup::horde_behaviour> {
    static constexpr mongroup::horde_behaviour last = mongroup::horde_behaviour::last;
};

class MonsterGroupManager
{
    public:
        static void LoadMonsterGroup( const JsonObject &jo );
        static void LoadMonsterBlacklist( const JsonObject &jo );
        static void LoadMonsterWhitelist( const JsonObject &jo );
        static void FinalizeMonsterGroups();
        static std::vector<MonsterGroupResult> GetResultFromGroup( const mongroup_id &group,
                int *quantity = nullptr, bool *mon_found = nullptr, bool is_recursive = false,
                bool *returned_default = nullptr, bool use_pack_size = false );
        static bool IsMonsterInGroup( const mongroup_id &group, const mtype_id &monster );
        static bool isValidMonsterGroup( const mongroup_id &group );
        static const mongroup_id &Monster2Group( const mtype_id &monster );
        static std::vector<mtype_id> GetMonstersFromGroup( const mongroup_id &group, bool from_subgroups );
        static const MonsterGroup &GetMonsterGroup( const mongroup_id &group );




        static const mtype_id &GetRandomMonsterFromGroup( const mongroup_id &group );

        static void check_group_definitions();

        static void ClearMonsterGroups();

        static bool monster_is_blacklisted( const mtype_id &m );

        static bool is_animal( const mongroup_id &group );


        static std::map<mongroup_id, MonsterGroup> &Get_all_Groups();

    private:
        static std::map<mongroup_id, MonsterGroup> monsterGroupMap;
        using t_string_set = std::set<std::string>;
        static t_string_set monster_blacklist;
        static t_string_set monster_whitelist;
        static t_string_set monster_categories_blacklist;
        static t_string_set monster_categories_whitelist;
        static t_string_set monster_species_blacklist;
        static t_string_set monster_species_whitelist;

};

#endif
