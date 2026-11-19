#pragma once
#ifndef CATA_SRC_MISSION_COMPANION_H
#define CATA_SRC_MISSION_COMPANION_H

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "mapgendata.h"
#include "memory_fast.h"
#include "type_id.h"

class JsonOut;
class JsonValue;
class item;
class monster;
class npc;
class npc_template;
struct comp_rank;
template <typename E> struct enum_traits;

using npc_ptr = shared_ptr_fast<npc>;
using comp_list = std::vector<npc_ptr>;










enum mission_kind : int {
    No_Mission,


    Scavenging_Patrol_Job,
    Scavenging_Raid_Job,
    Hospital_Raid_Job,
    Menial_Job,
    Carpentry_Job,
    Forage_Job,
    Caravan_Commune_Center_Job,


    Camp_Distribute_Food,
    Camp_Determine_Leadership,
    Camp_Have_Meal,
    Camp_Hide_Mission,
    Camp_Reveal_Mission,
    Camp_Assign_Jobs,
    Camp_Assign_Workers,
    Camp_Abandon,
    Camp_Upgrade,
    Camp_Emergency_Recall,
    Camp_Crafting,
    Camp_Gather_Materials,
    Camp_Collect_Firewood,
    Camp_Menial,
    Camp_Survey_Field,
    Camp_Survey_Expansion,
    Camp_Cut_Logs,
    Camp_Clearcut,
    Camp_Setup_Hide_Site,
    Camp_Relay_Hide_Site,
    Camp_Foraging,
    Camp_Trapping,
    Camp_Hunting,
    Camp_OM_Fortifications,
    Camp_Recruiting,
    Camp_Scouting,
    Camp_Combat_Patrol,
    Camp_Plow,
    Camp_Plant,
    Camp_Harvest,

    last_mission_kind
};

template<>
struct enum_traits<mission_kind> {
    static constexpr mission_kind last = mission_kind::last_mission_kind;
};


std::string action_of( mission_kind kind );











struct mission_id {
    mission_kind id = No_Mission;
    std::string parameters;
    mapgen_arguments mapgen_args;
    std::optional<point_rel_omt> dir;

    void serialize( JsonOut & ) const;
    void deserialize( const JsonValue & );
};

bool is_equal( const mission_id &first, const mission_id &second );
void reset_miss_id( mission_id &miss_id );



struct ui_mission_id {
    mission_id id;
    bool ret;
};

bool is_equal( const ui_mission_id &first, const ui_mission_id &second );

struct mission_entry {
    ui_mission_id id;
    std::string name_display;
    std::string text;
    bool priority = false;
    bool possible = false;
};

class mission_data
{
    public:
        std::vector<std::vector<mission_entry>> entries;
        mission_entry cur_key;

        mission_data();







        void add( const ui_mission_id &id, const std::string &name_display = "",
                  const std::string &text = "",
                  bool priority = false, bool possible = true );
        void add_return( const mission_id &id, const std::string &name_display,
                         const std::string &text, bool possible = true );
        void add_start( const mission_id &id, const std::string &name_display,
                        const std::string &text, bool possible = true );
};

namespace talk_function
{








void companion_mission( npc &p );







bool display_and_choose_opts( mission_data &mission_key, const tripoint_abs_omt &omt_pos,
                              const std::string &role_id, const std::string &title,
                              const std::string &point_name = "" );











npc_ptr individual_mission( npc &p, const std::string &desc, const mission_id &miss_id,
                            bool group = false, const std::vector<item *> &equipment = {},
                            const std::map<skill_id, int> &required_skills = {}, bool silent_failure = false );
npc_ptr individual_mission( const tripoint_abs_omt &omt_pos, const std::string &role_id,
                            const std::string &desc, const mission_id &miss_id,
                            bool group = false, const std::vector<item *> &equipment = {},
                            const std::map<skill_id, int> &required_skills = {}, bool silent_failure = false,
                            const npc_ptr &preselected_choice = nullptr );


void caravan_return( npc &p, const std::string &dest, const mission_id &miss_id );
void caravan_depart( npc &p, const std::string &dest, const mission_id &miss_id );
int caravan_dist( const std::string &dest );

void field_plant( npc &p, const std::string &place );
void field_harvest( npc &p, const std::string &place );
bool scavenging_patrol_return( npc &p );
bool scavenging_raid_return( npc &p );
bool hospital_raid_return( npc &p );
bool labor_return( npc &p );
bool carpenter_return( npc &p );
bool forage_return( npc &p );


void companion_skill_trainer( npc &comp, const std::string &skill_tested = "",
                              time_duration time_worked = 1_hours, int difficulty = 1 );
void companion_skill_trainer( npc &comp, const skill_id &skill_tested,
                              time_duration time_worked = 1_hours, int difficulty = 1 );

bool companion_om_combat_check( const comp_list &group, const tripoint_abs_omt &om_tgt,
                                bool try_engage = false );
void force_on_force( const comp_list &defender, const std::string &def_desc,
                     const comp_list &attacker, const std::string &att_desc, int advantage );
bool force_on_force( const comp_list &defender, const std::string &def_desc,
                     const std::vector< monster * > &monsters_fighting,
                     const std::string &att_desc, int advantage );
int combat_score( const comp_list &group );
int combat_score( const std::vector< monster * > &group );
void attack_random( const comp_list &attacker, const comp_list &defender );
void attack_random( const std::vector< monster * > &group, const comp_list &defender );
void attack_random( const comp_list &attacker, const std::vector< monster * > &group );
npc_ptr temp_npc( const string_id<npc_template> &type );



comp_list companion_list( const npc &p, const mission_id &miss_id, bool contains = false );
comp_list companion_list( const tripoint_abs_omt &omt_pos, const std::string &role_id,
                          const mission_id &miss_id, bool contains = false );
comp_list companion_sort( comp_list available,
                          const std::map<skill_id, int> &required_skills = {} );
std::vector<comp_rank> companion_rank( const comp_list &available, bool adj = true );


npc_ptr companion_choose( const std::map<skill_id, int> &required_skills = {}, bool silent_failure =
                              false );
npc_ptr companion_choose_return( const npc &p, const mission_id &miss_id,
                                 const time_point &deadline, bool ignore_parameters = false );
npc_ptr companion_choose_return( const tripoint_abs_omt &omt_pos, const std::string &role_id,
                                 const mission_id &miss_id, const time_point &deadline,
                                 bool by_mission = true, bool ignore_parameters = false );
npc_ptr companion_choose_return( comp_list &npc_list );


void companion_return( npc &comp );

std::set<item> loot_building( const tripoint_abs_omt &site, const oter_str_id &looted_replacement );

}
#endif
