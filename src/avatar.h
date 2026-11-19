#pragma once
#ifndef CATA_SRC_AVATAR_H
#define CATA_SRC_AVATAR_H

#include <array>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "character_id.h"
#include "coordinates.h"
#include "enums.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "magic_teleporter_list.h"
#include "mdarray.h"
#include "memory_fast.h"
#include "point.h"
#include "type_id.h"
#include "units.h"

class JsonObject;
class JsonOut;
class advanced_inv_area;
class advanced_inv_listitem;
class advanced_inventory_pane;
class cata_path;
class diary;
class map_memory;
class memorized_tile;
class mission;
class monster;
class npc;
class talker;
struct bionic;
struct mtype;

namespace debug_menu
{
class mission_debug;
}
enum class pool_type;



// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
struct monster_visible_info {

    std::vector<shared_ptr_fast<monster>> new_seen_mon;





    std::array<std::vector<npc *>, 9> unique_types;
    std::array<std::vector<std::pair<const mtype *, int>>, 9> unique_mons;


    std::array<bool, 8> dangerous = {};



    bool has_dangerous_creature_in_proximity = false;

    void remove_npc( npc *n );
};

struct point_of_interest {
    tripoint_abs_omt pos = tripoint_abs_omt::invalid;
    std::string text;
};

class avatar : public Character
{
    public:
        avatar();
        avatar( const avatar & ) = delete;
        // NOLINTNEXTLINE(performance-noexcept-move-constructor)
        avatar( avatar && );
        ~avatar() override;
        avatar &operator=( const avatar & ) = delete;
        // NOLINTNEXTLINE(performance-noexcept-move-constructor)
        avatar &operator=( avatar && );



        struct zone_sort_viewport_t {
            bool active = false;
            tripoint_abs_ms center;
            int target_zoom = 0;
            int saved_zoom = 0;
            tripoint_abs_ms bbox_min;
            tripoint_abs_ms bbox_max;
        };
        zone_sort_viewport_t zone_sort_viewport; // NOLINT(cata-serialize)

        void store( JsonOut &json ) const;
        void load( const JsonObject &data );
        void export_as_npc( const cata_path &path );
        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &data ) override;
        bool save_map_memory();
        void load_map_memory();
        void clear_map_memory();

        void ensure_portrait_valid() override;


        bool create( character_type type, const std::string &tempname = "" );

        void initialize( character_type type );
        bool load_template( const std::string &template_name, pool_type & );
        void save_template( const std::string &name, pool_type );
        void character_to_template( const std::string &name );

        bool is_avatar() const override {
            return true;
        }
        avatar *as_avatar() override {
            return this;
        }
        const avatar *as_avatar() const override {
            return this;
        }

        mfaction_id get_monster_faction() const override;

        void witness_thievery( item * ) override {}

        std::string get_save_id() const {
            return save_id.empty() ? name : save_id;
        }
        void set_save_id( const std::string &id ) {
            save_id = id;
        }




        void control_npc( npc &, bool debug = false );



        void control_npc_menu( bool debug = false );
        using Character::query_yn;
        bool query_yn( const std::string &mes ) const override;


        bool is_map_memory_valid() const;
        bool should_show_map_memory() const;
        void prepare_map_memory_region( const tripoint_abs_ms &p1, const tripoint_abs_ms &p2 );
        const memorized_tile &get_memorized_tile( const tripoint_abs_ms &p ) const;
        bool has_memory_at( const tripoint_abs_ms &p ) const;
        void memorize_terrain( const tripoint_abs_ms &p, std::string_view id,
                               int subtile, int rotation );
        void memorize_decoration( const tripoint_abs_ms &p, std::string_view id,
                                  int subtile, int rotation );
        void memorize_symbol( const tripoint_abs_ms &p, char32_t symbol );
        void memorize_clear_decoration( const tripoint_abs_ms &p, std::string_view prefix = "" );

        nc_color basic_symbol_color() const override;
        int print_info( const catacurses::window &w, int vStart, int vLines, int column ) const override;
        std::string display_name( bool possessive = false, bool capitalize_first = false ) const;


        void reset_stats() override;

        void reset_all_missions();

        std::vector<mission *> get_active_missions() const;
        std::vector<mission *> get_completed_missions() const;
        std::vector<mission *> get_failed_missions() const;
        std::vector<point_of_interest> get_points_of_interest() const;



        mission *get_active_mission() const;



        point_of_interest get_active_point_of_interest() const;




        tripoint_abs_omt get_active_mission_target() const;



        void set_active_mission( mission &cur_mission );



        void set_active_point_of_interest( const point_of_interest &active_point_of_interest );



        void on_mission_assignment( mission &new_mission );




        void on_mission_finished( mission &cur_mission );



        bool has_mission_id( const mission_type_id &miss_id );

        void remove_active_mission( mission &cur_mission );




        void add_point_of_interest( const point_of_interest &new_point_of_interest );
        void delete_point_of_interest( tripoint_abs_omt pos );


        diary *get_avatar_diary();


        void talk_to( std::unique_ptr<talker> talk_with, bool radio_contact = false,
                      bool is_computer = false, bool is_not_conversation = false,
                      const std::string &debug_topic = "",
                      const std::string &remote_name = "" );






        void disarm( npc &target );







        bool wield_contents( item &container, item *internal_item = nullptr, bool penalties = true,
                             int base_cost = INVENTORY_HANDLING_PENALTY );

        void try_to_sleep( const time_duration &dur );
        void set_pos_abs_only( const tripoint_abs_ms &loc );

        bool read( item_location &book, item_location ereader = {} );

        bool has_identified( const itype_id &item_id ) const override;
        void identify( const item &item ) override;
        void clear_identified();

        void clear_nutrition();

        void add_snippet( snippet_id snippet );
        bool has_seen_snippet( const snippet_id &snippet ) const;
        const std::set<snippet_id> &get_snippets();


        struct smash_result {
            bool can_smash;
            bool did_smash;
            bool success;
        };
        smash_result smash( tripoint_bub_ms &smashp );




        void longpull( const std::string &name );

        void wake_up() override;

        void grab( object_type grab_type, const tripoint_rel_ms &grab_point = tripoint_rel_ms::zero );
        object_type get_grab_type() const;

        void vomit();



        bool try_break_relax_gas( const std::string &msg_success, const std::string &msg_failure );
        void add_pain_msg( int val, const bodypart_id &bp ) const;





        void steal( npc &target );

        void reassign_item( item &it, int invlet );
        void reassign_item_cache( item &it, char invlet, bool remove_old );

        teleporter_list translocators;

        void upgrade_stat_prompt( const character_stat &stat_name );

        int free_upgrade_points() const;
        void power_bionics() override;
        void power_mutations() override;

        bionic *bionic_by_invlet( int ch );

        faction *get_faction() const override;
        bool is_ally( const Character &p ) const override;
        bool is_obeying( const Character &p ) const override;


        bool dialogue_by_radio = false;


        std::string dialogue_remote_name;

        std::string preferred_aiming_mode;


        bool cant_see( const tripoint_bub_ms &p ) const;


        void rebuild_aim_cache() const;


        void set_movement_mode( const move_mode_id &mode ) override;
        void reset_move_mode();
        void activate_crouch_mode();



        void set_desired_movement_mode( const move_mode_id &mode );
        move_mode_id get_desired_move_mode() const;

        void cycle_desired_move_mode();
        void cycle_desired_move_mode_reverse();

        bool is_walk_mode_desired() const;
        void set_walk_mode_desired();

        bool is_run_mode_desired() const;
        void toggle_run_mode_desired();

        bool is_crouch_mode_desired() const;
        void toggle_crouch_mode_desired();

        bool is_prone_mode_desired() const;
        void toggle_prone_mode_desired();


        bool wield( item &it );
        bool wield( item_location loc, bool remove_old = true );


        item::reload_option select_ammo( const item_location &base, bool prompt = false,
                                         bool empty = true ) override;


        std::vector<advanced_inv_listitem> get_AIM_inventory( const advanced_inventory_pane &pane,
                advanced_inv_area &square );

        using Character::invoke_item;
        bool invoke_item( item *, const tripoint_bub_ms &pt, int pre_obtain_moves ) override;
        bool invoke_item( item * ) override;
        bool invoke_item( item *, const std::string &, const tripoint_bub_ms &pt,
                          int pre_obtain_moves = -1 ) override;
        bool invoke_item( item *, const std::string & ) override;



        void update_cache_with_item( item &newit );

        void add_invlet_to_new_item( item &newit );
        void update_invlet( item &newit, const item *ignore_invlet_collision_with = nullptr );
        void assign_empty_invlet( item &it, bool force = false );
        char free_assigned_invlet( const itype_id &id );
        invlets_bitset allocated_invlets() const;
        bool invlet_is_assigned( char invlet ) const;
        itype_id get_itype_by_invlet( char invlet ) const;

        monster_visible_info &get_mon_visible() {
            return mon_visible;
        }

        const monster_visible_info &get_mon_visible() const {
            return mon_visible;
        }

        struct daily_calories {
            int spent = 0;
            int gained = 0;
            int ingested = 0;
            int total() const noexcept {
                return gained - spent;
            }
            std::map<float, int> activity_levels; // NOLINT(cata-serialize)

            daily_calories();

            void serialize( JsonOut &json ) const;
            void deserialize( const JsonObject &data );

            void save_activity( JsonOut &json ) const;
            void read_activity( const JsonObject &data );
        };



        void advance_daily_calories();
        int get_daily_spent_kcal( bool yesterday ) const;
        int get_daily_ingested_kcal( bool yesterday ) const;
        void add_ingested_kcal( int kcal );
        void update_cardio_acc() override;
        void add_spent_calories( int cal ) override;
        void add_gained_calories( int cal ) override;
        int get_daily_calories( unsigned days_ago, std::string const &type ) const;
        void log_activity_level( float level ) override;
        std::string total_daily_calories_string() const;

        std::set<character_id> get_followers() const;
        std::set<character_id> get_known_faction_representatives() const;

        int movecounter = 0;


        units::energy power_prev_turn = 0_kJ;

        units::energy power_balance = 0_kJ;


        time_point last_pocket_noise = time_point( 0 );

        vproto_id starting_vehicle = vproto_id::NULL_ID();
        std::vector<mtype_id> starting_pets;
        std::set<character_id> follower_ids;
        std::set<character_id> faction_representatives;

        mutable bool aim_cache_dirty = true;

        void mark_aim_cache_dirty();

        uint64_t aim_generation() const {
            return aim_cache_generation;
        }

        const mood_face_id &character_mood_face( bool clear_cache = false ) const;

        bool is_waiting_to_change_mode_mode();

    private:
        uint64_t aim_cache_generation = 0;
        std::map<char, itype_id> assigned_invlet;
        invlet_favorites invlet_cache;
        char find_usable_cached_invlet( const itype_id &item_type );

        npc &get_shadow_npc();


        std::string save_id;

        std::unique_ptr<map_memory> player_map_memory;

        friend class debug_menu::mission_debug;




        std::vector<mission *> active_missions;



        std::vector<mission *> completed_missions;



        std::vector<mission *> failed_missions;






        std::vector<point_of_interest> points_of_interest;

        mission *active_mission;

        point_of_interest active_point_of_interest;

        void update_active_mission();



        std::unique_ptr <diary> a_diary;




        std::list<daily_calories> calorie_diary;


        std::unordered_set<itype_id> items_identified;


        std::set<snippet_id> snippets_read;

        object_type grab_type;

        monster_visible_info mon_visible;





        std::unique_ptr<npc> shadow_npc;


        mutable cata::mdarray<bool, point_bub_ms> aim_cache;

        move_mode_id desired_move_mode;
};

avatar &get_avatar();
std::unique_ptr<talker> get_talker_for( avatar &me );
std::unique_ptr<talker> get_talker_for( avatar *me );

#endif
