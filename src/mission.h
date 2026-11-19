#pragma once
#ifndef CATA_SRC_MISSION_H
#define CATA_SRC_MISSION_H

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character_id.h"
#include "coordinates.h"
#include "dialogue_helpers.h"
#include "enums.h"
#include "npc_favor.h"
#include "point.h"
#include "translation.h"
#include "type_id.h"

class Creature;
class JsonArray;
class JsonObject;
class JsonOut;
class avatar;
class item;
class mission;
class npc;
class overmapbuffer;
struct const_dialogue;
struct dialogue;
struct oter_type_t;
template<typename T> struct enum_traits;

namespace debug_menu
{
class mission_debug;
}

enum mission_origin {
    ORIGIN_NULL = 0,
    ORIGIN_GAME_START,
    ORIGIN_OPENER_NPC,
    ORIGIN_ANY_NPC,
    ORIGIN_SECONDARY,
    ORIGIN_COMPUTER,
    NUM_ORIGIN
};

template<>
struct enum_traits<mission_origin> {
    static constexpr mission_origin last = mission_origin::NUM_ORIGIN;
};

enum mission_goal {
    MGOAL_NULL = 0,
    MGOAL_GO_TO,
    MGOAL_GO_TO_TYPE,
    MGOAL_FIND_ITEM,
    MGOAL_FIND_ANY_ITEM,
    MGOAL_FIND_ITEM_GROUP,
    MGOAL_FIND_MONSTER,
    MGOAL_FIND_NPC,
    MGOAL_ASSASSINATE,
    MGOAL_KILL_MONSTER,
    MGOAL_KILL_MONSTERS,
    MGOAL_KILL_MONSTER_TYPE,
    MGOAL_KILL_NEMESIS,
    MGOAL_RECRUIT_NPC,
    MGOAL_RECRUIT_NPC_CLASS,
    MGOAL_COMPUTER_TOGGLE,
    MGOAL_KILL_MONSTER_SPEC,
    MGOAL_TALK_TO_NPC,
    MGOAL_CONDITION,
    NUM_MGOAL
};

template<>
struct enum_traits<mission_goal> {
    static constexpr mission_goal last = mission_goal::NUM_MGOAL;
};

struct mission_place {

    static bool never( const tripoint_abs_omt & ) {
        return false;
    }
    static bool always( const tripoint_abs_omt & ) {
        return true;
    }
    static bool near_town( const tripoint_abs_omt & );
};




struct mission_start {
    static void standard( mission * );
    static void kill_nemesis( mission * );
    static void place_npc_software( mission * );
    static void place_deposit_box( mission * );
    static void find_safety( mission * );
    static void place_book( mission * );
    static void reveal_refugee_center( mission * );
};


struct mission_end {

    static void standard( mission * ) {}

};

struct mission_fail {

    static void standard( mission * ) {}
};

struct mission_target_params {
    str_or_var overmap_terrain;
    ot_match_type overmap_terrain_match_type = ot_match_type::type;
    mission *mission_pointer = nullptr;

    bool origin_u = true;
    std::optional<tripoint_rel_omt> offset;
    std::optional<str_or_var> replaceable_overmap_terrain;
    std::optional<str_or_var> overmap_special;
    std::optional<dbl_or_var> reveal_radius;
    std::optional<var_info> target_var;
    dbl_or_var min_distance;

    bool must_see = false;
    bool cant_see = false;
    bool random = false;
    bool create_if_necessary = true;
    dbl_or_var search_range;
    std::optional<dbl_or_var> z;
    npc *guy = nullptr;
};

namespace mission_util
{
tripoint_abs_omt random_house_in_closest_city();
bool reveal_road( const tripoint_abs_omt &source, const tripoint_abs_omt &dest,
                  overmapbuffer &omb );
tripoint_abs_omt reveal_om_ter( tripoint_abs_omt origin, const std::string &omter, int reveal_rad,
                                bool must_see,
                                int target_z = 0 );
tripoint_abs_omt target_om_ter( tripoint_abs_omt origin, const std::string &omter, int reveal_rad,
                                mission *miss,
                                bool must_see, int target_z = 0 );
tripoint_abs_omt target_om_ter_random(
    const std::string &omter, int reveal_rad, mission *miss, bool must_see, int range,
    tripoint_abs_omt loc = tripoint_abs_omt::invalid );
void set_reveal( const std::string &terrain,
                 std::vector<std::function<void( mission *miss )>> &funcs );
void set_reveal_any( const JsonArray &ja,
                     std::vector<std::function<void( mission *miss )>> &funcs );
mission_target_params parse_mission_om_target( const JsonObject &jo );
std::optional<tripoint_abs_omt> assign_mission_target( const mission_target_params
        &params, dialogue &d );
tripoint_abs_omt get_om_terrain_pos( const mission_target_params &params, dialogue &d );
void set_assign_om_target( const JsonObject &jo,
                           std::vector<std::function<void( mission *miss )>> &funcs );
bool set_update_mapgen( const JsonObject &jo,
                        std::vector<std::function<void( mission *miss )>> &funcs );
bool load_funcs( const JsonObject &jo, std::vector<std::function<void( mission *miss )>> &funcs );
}

struct mission_type {
    public:

        mission_type_id id = mission_type_id( "MISSION_NULL" );
        std::vector<std::pair<mission_type_id, mod_id>> src;
    private:

        translation name = to_translation( "Bugged mission type" );
    public:
        translation description;

        mission_goal goal = mission_goal::MGOAL_NULL;

        int difficulty = 0;

        int value = 0;


        duration_or_var deadline;

        bool urgent = false;

        bool has_generic_rewards = true;

        bool was_loaded = false;


        talk_effect_fun_t::likely_rewards_t likely_rewards;


        std::vector<mission_origin> origins;
        itype_id item_id = itype_id::NULL_ID();
        item_group_id group_id = item_group_id::NULL_ID();
        itype_id container_id = itype_id::NULL_ID();
        bool remove_container = false;
        bool invisible_on_complete = false;
        itype_id empty_container = itype_id::NULL_ID();
        int item_count = 1;
        npc_class_id recruit_class = npc_class_id::NULL_ID();
        character_id target_npc_id;
        mtype_id monster_type = mtype_id::NULL_ID();
        species_id monster_species;
        int monster_kill_goal = -1;
        string_id<oter_type_t> target_id;
        mission_type_id follow_up = mission_type_id( "MISSION_NULL" );

        std::function<bool( const tripoint_abs_omt & )> place = mission_place::always;
        std::function<void( mission * )> start = mission_start::standard;
        std::function<void( mission * )> end = mission_end::standard;
        std::function<void( mission * )> fail = mission_fail::standard;

        std::map<std::string, translation> dialogue;


        std::function<bool( const_dialogue const & )> goal_condition;

        mission_type() = default;

        mission create( const character_id &npc_id ) const;




        static const mission_type *get( const mission_type_id &id );





        static mission_type_id get_random_id( mission_origin origin, const tripoint_abs_omt &p );



        static const std::vector<mission_type> &get_all();

        bool test_goal_condition( struct dialogue &d ) const;

        static void reset();
        static void load_mission_type( const JsonObject &jo, const std::string &src );
        static void finalize_all();
        static void check_consistency();

        bool parse_funcs( const JsonObject &jo, std::string_view src,
                          std::function<void( mission * )> &phase_func );
        bool load( const JsonObject &jo, std::string_view src );




        std::string tname() const;
};

class mission
{
    public:
        enum class mission_status : int {
            yet_to_start,
            in_progress,
            success,
            failure,
            num_mission_status
        };
    private:

        friend struct mission_type;

        friend struct mission_start;
        friend class debug_menu::mission_debug;

        const mission_type *type;
        mission_status status;

        unsigned int value;

        npc_favor reward;

        int uid;


        tripoint_abs_omt target;

        dimension_id dimension;

        itype_id item_id;

        int item_count = 0;

        string_id<oter_type_t> target_id;

        npc_class_id recruit_class;

        character_id target_npc_id;

        mtype_id monster_type;

        species_id monster_species;

        int monster_kill_goal = 0;

        int kill_count_to_reach = 0;


        time_point deadline;

        character_id npc_id;

        int good_fac_id = 0;
        int bad_fac_id = 0;

        int step = 0;

        mission_type_id follow_up;

        character_id player_id;
    public:

        std::string name() const;
        mission_type_id mission_id() const;
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );

        mission();


        bool has_deadline() const;
        time_point get_deadline() const;
        std::string get_description() const;
        dimension_id get_dimension() const;
        bool has_target() const;
        const tripoint_abs_omt &get_target() const;
        const mission_type &get_type() const;
        bool has_follow_up() const;
        mission_type_id get_follow_up() const;
        int get_value() const;
        int get_id() const;
        const itype_id &get_item_id() const;
        character_id get_npc_id() const;
        const talk_effect_fun_t::likely_rewards_t &get_likely_rewards() const;
        bool has_generic_rewards() const;
        void register_kill_needed() {
            monster_kill_goal++;
        };




        bool is_assigned() const;




        character_id get_assigned_player_id() const;





        void set_deadline( time_point new_deadline );
        void set_dimension( dimension_id dimension_prefix );
        void set_target( const tripoint_abs_omt &p );
        void set_target_npc_id( const character_id &npc_id );
        void set_assigned_player_id( const character_id &char_id );
        void update_world_missions_character( const character_id &old_char_id,
                                              const character_id &new_char_id );



        void assign( avatar &u );


        void fail();

        void wrap_up();

        void step_complete( int step );

        bool is_complete( const character_id &npc_id ) const;

        bool has_failed() const;

        bool in_progress() const;

        void process();

        void on_talk_with_npc( const character_id &npc_id );


        std::string dialogue_for_topic( const std::string &topic ) const;





        static mission *reserve_new( const mission_type_id &type, const character_id &npc_id );
        static mission *reserve_random( mission_origin origin, const tripoint_abs_omt &p,
                                        const character_id &npc_id );




        static mission *find( int id, bool ok_missing = false );



        static void clear_all();



        static void process_all();





        static void on_creature_death( Creature &poor_dead_dude );

        static bool on_creature_fusion( Creature &fuser, Creature &fused );



        static void serialize_all( JsonOut &json );
        static void unserialize_all( const JsonArray &ja );

        static std::vector<mission *> to_ptr_vector( const std::vector<int> &vec, bool ok_missing = false );
        static std::vector<int> to_uid_vector( const std::vector<mission *> &vec );


        static std::vector<mission *> get_all_active();
        void remove_active_world_mission( mission &cur_mission );
        static void add_existing( const mission &m );

        static mission_status status_from_string( const std::string &s );
        static std::string status_to_string( mission_status st );

    private:

        void set_target_to_mission_giver();

        static void get_all_item_group_matches(
            std::vector<item *> &items,
            item_group_id &grp_type,
            std::map<itype_id, int> &matches,
            const itype_id &required_container,
            const itype_id &actual_container,
            bool &specific_container_required );

};

template<>
struct enum_traits<mission::mission_status> {
    static constexpr mission::mission_status last = mission::mission_status::num_mission_status;
};

#endif
