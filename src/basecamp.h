#pragma once
#ifndef CATA_SRC_BASECAMP_H
#define CATA_SRC_BASECAMP_H

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "craft_command.h"
#include "game_constants.h"
#include "item_components.h"
#include "item_location.h"
#include "map.h"
#include "mapgendata.h"
#include "memory_fast.h"
#include "mission_companion.h"
#include "point.h"
#include "requirements.h"
#include "stomach.h"
#include "temp_crafting_inventory.h"
#include "translation.h"
#include "type_id.h"
#include "units_fwd.h"

class Character;
class JsonObject;
class JsonOut;
class basecamp;
class character_id;
class faction;
class inventory_filter_preset;
class item;
class npc;
class recipe;
enum class farm_ops;
struct MonsterGroupResult;

const int work_day_hours = 10;
const int work_day_rest_hours = 8;
const int work_day_idle_hours = 6;

struct expansion_data {
    std::string type;
    std::vector<itype_id> available_pseudo_items;
    std::map<std::string, int> provides;
    std::map<std::string, int> in_progress;
    tripoint_abs_omt pos;

    int cur_level = 0;

};

using npc_ptr = shared_ptr_fast<npc>;
using comp_list = std::vector<npc_ptr>;

namespace base_camps
{

enum tab_mode : int {
    TAB_MAIN,
    TAB_N,
    TAB_NE,
    TAB_E,
    TAB_SE,
    TAB_S,
    TAB_SW,
    TAB_W,
    TAB_NW
};

struct direction_data {

    std::string id;

    tab_mode tab_order;

    translation bracket_abbr;

    translation tab_title;
};


extern const std::map<point_rel_omt, direction_data> all_directions;

point_rel_omt direction_from_id( const std::string &id );

constexpr point_rel_omt base_dir;
const std::string prefix = "faction_base_";
const std::string id = "FACTION_CAMP";
const int prefix_len = 13;
std::string faction_encode_short( const std::string &type );
std::string faction_encode_abs( const expansion_data &e, int number );
std::string faction_decode( std::string_view full_type );
time_duration to_workdays( const time_duration &work_time );
int max_upgrade_by_type( const std::string &type );
}


struct basecamp_resource {
    itype_id fake_id;
    itype_id ammo_id;
    int available = 0;
    int consumed = 0;
};

struct basecamp_fuel {
    itype_id ammo_id;
    int available = 0;
};

struct basecamp_upgrade {
    std::string bldg;
    mapgen_arguments args;
    translation name;
    bool avail = false;
    bool in_progress = false;
};

struct expansion_salt_water_pipe_segment {
    tripoint_abs_omt point;
    bool started;
    bool finished;
};

struct expansion_salt_water_pipe {
    point_rel_omt expansion;
    point_rel_omt connection_direction;
    std::vector<expansion_salt_water_pipe_segment> segments;
};

class basecamp_map
{
        friend basecamp;
    private:
        std::unique_ptr<map> map_;
    public:
        basecamp_map() = default;
        basecamp_map( const basecamp_map & );
        basecamp_map &operator=( const basecamp_map & );
};

class basecamp
{
    public:
        basecamp();
        basecamp( const std::string &name_, const tripoint_abs_omt &omt_pos );
        basecamp( const std::string &name_, const tripoint_abs_ms &bb_pos_,
                  const std::vector<point_rel_omt> &directions_,
                  const std::map<point_rel_omt, expansion_data> &expansions_ );
        inline bool is_valid() const {
            return !name.empty() && omt_pos != tripoint_abs_omt();
        }
        inline int board_x() const {
            return bb_pos.x();
        }
        inline int board_y() const {
            return bb_pos.y();
        }
        inline tripoint_abs_omt camp_omt_pos() const {
            return omt_pos;
        }
        inline const std::string &camp_name() const {
            return name;
        }
        tripoint_abs_ms get_bb_pos() const {
            return bb_pos;
        }
        tripoint_abs_ms get_bb_pos_abs() const {
            return bb_pos;
        }
        void validate_bb_pos( const tripoint_abs_ms &new_abs_pos ) {
            if( bb_pos.raw() == tripoint::zero ) {
                bb_pos = new_abs_pos;
            }
        }
        void set_bb_pos( const tripoint_abs_ms &new_abs_pos ) {
            bb_pos = new_abs_pos;
        }
        void set_by_radio( bool access_by_radio );

        std::string board_name() const;
        std::vector<point_rel_omt> directions; // NOLINT(cata-serialize)
        std::vector<std::vector<ui_mission_id>> hidden_missions;
        std::vector<tripoint_abs_omt> fortifications;
        std::vector<expansion_salt_water_pipe *> salt_water_pipes;


        void set_name( const std::string &new_name );
        void query_new_name( bool force = false );


        void remove_camp( bool remove_from_overmap = true ) const;

        void abandon_camp();
        void scan_pseudo_items();
        void add_expansion( const std::string &terrain, const tripoint_abs_omt &new_pos );
        void add_expansion( const std::string &bldg, const tripoint_abs_omt &new_pos,
                            const point_rel_omt &dir );
        void define_camp( const tripoint_abs_omt &p, std::string_view camp_type,
                          bool player_founded = true );

        std::string expansion_tab( const point_rel_omt &dir ) const;

        bool point_within_camp( const tripoint_abs_omt &p ) const;

        bool has_provides( const std::string &req, const expansion_data &e_data, int level = 0 ) const;
        bool has_provides( const std::string &req, const std::optional<point_rel_omt> &dir = std::nullopt,
                           int level = 0 ) const;
        void update_resources( const std::string &bldg );
        void update_provides( const std::string &bldg, expansion_data &e_data );
        void update_in_progress( const std::string &bldg, const point_rel_omt &dir );


        static basecamp *any_claim_at( tripoint_abs_omt there );



        std::string next_upgrade( const point_rel_omt &dir, int offset = 1 ) const;
        std::vector<basecamp_upgrade> available_upgrades( const point_rel_omt &dir );


        int recruit_evaluation() const;
        int recruit_evaluation( int &sbase, int &sexpansions, int &sfaction, int &sbonus ) const;

        bool validate_sort_points();

        expansion_data parse_expansion( std::string_view terrain,
                                        const tripoint_abs_omt &new_pos );



        bool set_sort_points();





        nutrients camp_food_supply( nutrients change );

        nutrients camp_food_supply( int change );

        nutrients camp_food_supply( const time_duration &total_time, float exertion_level = NO_EXERCISE,
                                    const time_duration &travel_time = 0_hours );

        void feed_workers( const std::vector<std::reference_wrapper <Character>> &workers, nutrients food,
                           bool is_player_meal = false );

        void feed_workers( Character &worker, nutrients food, bool is_player_meal = false );
        void player_eats_meal();
        item make_fake_food( const nutrients &to_use ) const;


        bool distribute_food( bool player_command = true );
        std::string name_display_of( const mission_id &miss_id );
        void handle_hide_mission( const point_rel_omt &dir );
        void handle_reveal_mission( const point_rel_omt &dir );
        bool has_water() const;

        int camp_food_supply_days( float exertion_level ) const;

        int time_to_food( time_duration total_time, float work_exertion_level = NO_EXERCISE,
                          time_duration travel_time = 0_hours ) const;

        int camp_discipline( int change = 0 ) const;

        int camp_morale( int change = 0 ) const;

        bool allowed_access_by( Character &guy, bool water_request = false ) const;



        void handle_takeover_by( faction_id new_owner, bool violent_takeover );


        std::map<recipe_id, translation> recipe_deck( const point_rel_omt &dir ) const;

        std::map<recipe_id, translation> recipe_deck( const std::string &bldg ) const;

        std::unordered_set<recipe_id> recipe_deck_all() const;
        int recipe_batch_max( const recipe &making ) const;
        void form_crafting_inventory();
        void form_crafting_inventory( map &target_map );
        std::list<item> use_charges( const itype_id &fake_id, int &quantity );








        void search_results( int skill, const item_group_id &, int attempts, int difficulty );









        void hunting_results( int skill, const mission_id &miss_id, int attempts, int difficulty );
        void make_corpse_from_group( const std::vector<MonsterGroupResult> &group );
        inline const tripoint_abs_ms &get_dumping_spot() const {
            return dumping_spot;
        }
        inline const std::vector<tripoint_abs_ms> &get_liquid_dumping_spot() const {
            return liquid_dumping_spots;
        }

        inline void set_dumping_spot( const tripoint_abs_ms &spot ) {
            dumping_spot = spot;
        }
        inline void set_liquid_dumping_spot( const std::vector<tripoint_abs_ms> &liquid_dumps ) {

            if( liquid_dumps.empty() ) {
                liquid_dumping_spots.clear();
                liquid_dumping_spots.emplace_back( dumping_spot );
                return;
            }
            liquid_dumping_spots.clear();
            liquid_dumping_spots = liquid_dumps;
        }
        void place_results( const item &result );


        void add_available_recipes( mission_data &mission_key, mission_kind kind,
                                    const point_rel_omt &dir );

        std::string recruit_description( int npc_count ) const;


        std::vector<std::string> gathering_description() const;


        std::vector<std::string> farm_description( const point_rel_omt &dir, size_t &plots_count,
                farm_ops operation );


        std::string craft_description( const recipe_id &itm );


        void get_available_missions( mission_data &mission_key, map &here );
        void get_available_missions_by_dir( mission_data &mission_key, const point_rel_omt &dir );
        void choose_new_leader();

        void reset_camp_workers();
        comp_list get_mission_workers( const mission_id &miss_id, bool contains = false );

        bool handle_mission( const ui_mission_id &miss_id );



        npc_ptr start_mission( const mission_id &miss_id, time_duration total_time,
                               bool must_feed, const std::string &desc, bool group,
                               const std::vector<item *> &equipment,
                               const skill_id &skill_tested, int skill_level,
                               float exertion_level, const time_duration &travel_time = 0_hours );
        npc_ptr start_mission( const mission_id &miss_id, time_duration total_time,
                               bool must_feed, const std::string &desc, bool group,
                               const std::vector<item *> &equipment,
                               const std::map<skill_id, int> &required_skills = {},
                               float exertion_level = 1.0f, const time_duration &travel_time = 0_hours,
                               const npc_ptr &preselected_choice = nullptr );
        comp_list start_multi_mission( const mission_id &miss_id,
                                       bool must_feed, const std::string &desc,

                                       const skill_id &skill_tested, int skill_level );
        comp_list start_multi_mission( const mission_id &miss_id,
                                       bool must_feed, const std::string &desc,

                                       const std::map<skill_id, int> &required_skills = {} );
        void start_upgrade( const mission_id &miss_id );
        std::string om_upgrade_description( const std::string &bldg, const mapgen_arguments &,
                                            bool trunc = false ) const;
        void start_menial_labor();
        void worker_assignment_ui();
        void job_assignment_ui();


        void start_crafting( const mission_id &miss_id );


        void start_cut_logs( const mission_id &miss_id, float exertion_level );
        void start_clearcut( const mission_id &miss_id, float exertion_level );
        void start_setup_hide_site( const mission_id &miss_id, float exertion_level );
        void start_relay_hide_site( const mission_id &miss_id, float exertion_level );

        void start_fortifications( const mission_id &miss_id, float exertion_level );

        bool common_salt_water_pipe_construction( const mission_id &miss_id,
                expansion_salt_water_pipe *pipe,
                int segment_number );
        void start_salt_water_pipe( const mission_id &miss_id );
        void continue_salt_water_pipe( const mission_id &miss_id );
        void start_combat_mission( const mission_id &miss_id, float exertion_level );
        void start_farm_op( const point_rel_omt &dir, const mission_id &miss_id,
                            float exertion_level );


        drop_locations give_basecamp_equipment( inventory_filter_preset &preset, const std::string &title,
                                                const std::string &column_title, const std::string &msg_empty ) const;
        drop_locations give_equipment( Character *pc, const inventory_filter_preset &preset,
                                       const std::string &msg, const std::string &title, units::volume &total_volume,
                                       units::mass &total_mass );
        drop_locations get_equipment( tinymap *target_bay, const tripoint_omt_ms &target, Character *pc,
                                      const inventory_filter_preset &preset,
                                      const std::string &msg, const std::string &title, units::volume &total_volume,
                                      units::mass &total_mass );



        npc_ptr companion_choose_return( const mission_id &miss_id, time_duration min_duration );
        npc_ptr companion_crafting_choose_return( const mission_id &miss_id );


        void finish_return( npc &comp, bool fixed_time, const std::string &return_msg,
                            const std::string &skill, int difficulty, bool cancel = false );

        npc_ptr mission_return( const mission_id &miss_id, time_duration min_duration,
                                bool fixed_time, const std::string &return_msg,
                                const std::string &skill, int difficulty );
        npc_ptr crafting_mission_return( const mission_id &miss_id, const std::string &return_msg,
                                         const std::string &skill, int difficulty );

        npc_ptr emergency_recall( const mission_id &miss_id );



        bool upgrade_return( const mission_id &miss_id );


        bool survey_field_return( const mission_id &miss_id );


        bool survey_return( const mission_id &miss_id );
        bool menial_return( const mission_id &miss_id );

        bool gathering_return( const mission_id &miss_id, time_duration min_time );
        void recruit_return( const mission_id &miss_id, int score );






        bool farm_return( const mission_id &miss_id, const point_rel_omt &dir );
        std::pair<size_t, std::string> farm_action( const point_rel_omt &dir, farm_ops op,
                const npc_ptr &comp = nullptr );
        void fortifications_return( const mission_id &miss_id );
        bool salt_water_pipe_swamp_return( const mission_id &miss_id,
                                           const comp_list &npc_list );
        bool salt_water_pipe_return( const mission_id &miss_id,
                                     const comp_list &npc_list );

        void combat_mission_return( const mission_id &miss_id );
        void validate_assignees();
        void add_assignee( character_id id );
        void remove_assignee( character_id id );
        std::vector<npc_ptr> get_npcs_assigned();
        void hide_mission( ui_mission_id id );
        void reveal_mission( ui_mission_id id );
        bool is_hidden( ui_mission_id id );

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
        void load_data( const std::string &data );
        inline const std::unordered_set<tripoint_abs_ms> &get_storage_tiles() const {
            return src_set;
        }
        inline void set_storage_tiles( const std::unordered_set<tripoint_abs_ms> &tiles ) {
            src_set = tiles;
        }
        void form_storage_zones( map &here, const tripoint_abs_ms &abspos );
        map &get_camp_map();
        void unload_camp_map();
        void set_owner( faction_id new_owner );
        faction_id get_owner();
    private:
        friend class basecamp_action_components;


        mutable faction_id owner = faction_id::NULL_ID();

        faction *fac() const;

        void reset_camp_resources( map &here );
        void add_resource( const itype_id &camp_resource );

        std::string name;

        tripoint_abs_omt omt_pos;
        std::vector<npc_ptr> assigned_npcs; // NOLINT(cata-serialize)

        tripoint_abs_ms bb_pos;
        std::map<point_rel_omt, expansion_data> expansions;
        comp_list camp_workers; // NOLINT(cata-serialize)
        basecamp_map camp_map; // NOLINT(cata-serialize)

        tripoint_abs_ms dumping_spot;

        std::vector<tripoint_abs_ms> liquid_dumping_spots;
        std::unordered_set<tripoint_abs_ms> src_set; // NOLINT(cata-serialize)
        std::set<itype_id> fuel_types; // NOLINT(cata-serialize)
        std::vector<basecamp_fuel> fuels; // NOLINT(cata-serialize)
        std::vector<basecamp_resource> resources; // NOLINT(cata-serialize)
        std::vector<std::vector<ui_mission_id>> temp_ui_mission_keys;   // NOLINT(cata-serialize)
        temp_crafting_inventory _inv; // NOLINT(cata-serialize)
        bool by_radio = false; // NOLINT(cata-serialize)
};

class basecamp_action_components
{
    public:
        basecamp_action_components( const recipe &making, const mapgen_arguments &, int batch_size,
                                    basecamp & );


        bool choose_components();
        void consume_components();
        item_components &consumed_components() {
            return consumed_components_;
        }
    private:
        const recipe &making_;
        const mapgen_arguments &args_;
        int batch_size_;
        basecamp &base_;
        std::vector<comp_selection<item_comp>> item_selections_;
        std::vector<comp_selection<tool_comp>> tool_selections_;
        item_components consumed_components_;
};

#endif
