#pragma once
#ifndef CATA_SRC_GAME_H
#define CATA_SRC_GAME_H

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "character_id.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "cursesdef.h"
#include "enums.h"
#include "global_vars.h"
#include "item_location.h"
#include "map_scale_constants.h"
#include "memory_fast.h"
#include "overmap_ui.h"
#include "pimpl.h"
#include "type_id.h"
#include "units_fwd.h"
#include "weather.h"


class game;

extern std::unique_ptr<game> g;

extern const int savegame_version;
extern int savegame_loading_version;

class input_context;

input_context get_default_mode_input_context();

enum quit_status {
    QUIT_NO = 0,
    QUIT_SUICIDE,
    QUIT_SAVED,
    QUIT_NOSAVED,
    QUIT_DIED,
    QUIT_WATCH,
};

enum safe_mode_type {
    SAFE_MODE_OFF = 0,
    SAFE_MODE_ON = 1,
    SAFE_MODE_STOP = 2,
};

class JsonObject;
class JsonValue;
class achievements_tracker;
class avatar;
class cata_path;
class creature_tracker;
class current_map;
class eoc_events;
class event_bus;
class faction_manager;
class field_entry;
class inventory_selector_preset;
class item;
class kill_tracker;
class live_view;
class map;
class memorial_logger;
class monster;
class npc;
class npc_template;
class overmap;
class power_network_manager;
class save_t;
class scenario;
class scent_map;
class spell_events;
class static_popup;
class stats_tracker;
class timed_event_manager;
class item_wakeup_manager;
class ui_adaptor;
class uilist;
class vehicle;
class viewer;
enum action_id : int;
struct special_game;
struct mtype;
struct visibility_variables;
template <typename Tripoint> class tripoint_range;

using item_filter = std::function<bool ( const item & )>;
using item_location_filter = std::function<bool ( const item_location & )>;

enum peek_act : int {
    PA_BLIND_THROW,
    PA_BLIND_THROW_WIELDED,
    PA_MOVE,

};

struct look_around_result {
    std::optional<tripoint_bub_ms> position;
    std::optional<peek_act> peek_action;
};
struct look_around_params {
    const bool show_window;
    tripoint_bub_ms &center;
    const tripoint_bub_ms &start_point;
    bool has_first_point;
    bool select_zone;
    bool peeking;
    bool change_lv;
};

struct w_map {
    int id;
    std::string name;
    bool toggle;
    catacurses::window win;
};

struct pulp_data {

    int mess_radius = 1;

    float nominal_pulp_power = 0.0f;

    float pulp_power = 0.0f;

    float pulp_effort = 0.0f;

    int time_to_pulp = 0;

    std::optional<proficiency_id> unknown_prof;

    bool can_pry_armor = false;

    bool can_cut_precisely = false;

    bool stomps_only = false;
    bool weapon_only = false;
    bool used_pry = false;
    bool couldnt_use_pry = false;
    std::string bash_tool;
    std::string cut_tool;
    std::string pry_tool;
};

bool is_valid_in_w_terrain( const point_rel_ms &p );
namespace turn_handler
{
bool cleanup_at_end();
}



// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class game
{
        friend class editmap_ui;
        friend class main_menu;
        friend class exosuit_interact;
        friend class swap_map;
        friend achievements_tracker &get_achievements();
        friend event_bus &get_event_bus();
        friend map &get_map();
        friend map &reality_bubble();
        friend creature_tracker &get_creature_tracker();
        friend Character &get_player_character();
        friend avatar &get_avatar();
        friend viewer &get_player_view();
        friend weather_manager &get_weather();
        friend const scenario *get_scenario();
        friend void set_scenario( const scenario *new_scenario );
        friend stats_tracker &get_stats();
        friend scent_map &get_scent();
        friend timed_event_manager &get_timed_events();
        friend item_wakeup_manager &get_item_wakeups();
        friend memorial_logger &get_memorial();
        friend bool turn_handler::cleanup_at_end();
        friend global_variables &get_globals();
    public:
        game();
        ~game();










        bool do_turn();


        void load_static_data();


        void load_core_data();


        bool is_core_data_loaded() const;






        bool check_mod_data( const std::vector<mod_id> &opts );


        void load_world_modfiles();






        void load_packs( const std::string &msg, const std::vector<mod_id> &packs );




        void on_options_changed();

    protected:

        void load_data_from_dir( const cata_path &path, const std::string &src );

        void load_mod_data_from_dir( const cata_path &path, const std::string &src );

        void load_mod_interaction_data_from_dir( const cata_path &path, const std::string &src );
    public:

        void setup( bool loading_saved_character = false );

        void serialize_json( std::ostream &fout );
        void unserialize( std::istream &fin, const cata_path &path );
        void unserialize( std::string fin );
        void unserialize_dimension_data( const cata_path &file_name, std::istream &fin );
        void unserialize_dimension_data( const JsonValue &jv );
        void unserialize_master( const cata_path &file_name, std::istream &fin );
        void unserialize_master( const JsonValue &jv );
    private:
        void unserialize_impl( const JsonObject &data );
    public:


        bool save();


        std::vector<std::string> list_active_saves();
        void write_memorial_file( std::string sLastWords );
        void start_calendar();
        shared_ptr_fast<ui_adaptor> create_or_get_main_ui_adaptor();
        void invalidate_main_ui_adaptor() const;
        void mark_main_ui_adaptor_resize() const;
        void draw( ui_adaptor &ui );
        void draw_ter( bool draw_sounds = true );
        void draw_ter( const tripoint_bub_ms &center, bool looking = false, bool draw_sounds = true );

        void try_activate_zone_sort_viewport();

        class draw_callback_t
        {
            public:
                explicit draw_callback_t( const std::function<void()> &cb );
                ~draw_callback_t();
                void operator()();
                friend class game;
            private:
                std::function<void()> cb;
                bool added = false;
        };







        void add_draw_callback( const shared_ptr_fast<draw_callback_t> &cb );
    private:
        bool is_looking = false; // NOLINT(cata-serialize)
        std::vector<weak_ptr_fast<draw_callback_t>> draw_callbacks; // NOLINT(cata-serialize)

    public:

        void init_draw_async_anim_curses( const tripoint_bub_ms &p, const std::string &ncstr,
                                          const nc_color &nccol );
        void draw_async_anim_curses();
        bool void_async_anim_curses();
    protected:
        std::map<tripoint_bub_ms, std::pair <std::string, nc_color>>
                async_anim_layer_curses; // NOLINT(cata-serialize)

    public:
        void init_draw_blink_curses( const tripoint_bub_ms &p, const std::string &ncstr,
                                     const nc_color &nccol );
        void draw_blink_curses();
        void void_blink_curses();
        bool has_blink_curses();
        bool blink_active_phase = true; // NOLINT(cata-serialize)
    protected:
        std::map<tripoint_bub_ms, std::pair <std::string, nc_color>>
                blink_layer_curses; // NOLINT(cata-serialize)

    public:


        void draw_panels( bool force_draw = false );







        std::optional<tripoint_rel_ms> get_veh_dir_indicator_location( bool next ) const;
        void draw_veh_dir_indicator( bool next );






        void vertical_move( int z, bool force, bool peeking = false );
        void start_hauling( const tripoint_bub_ms &pos );







        bool travel_to_dimension( dimension_id dimension_destination,
                                  const std::vector<npc *> &npc_travellers,
                                  const std::vector<item_location> &item_travellers,
                                  std::optional<tripoint_bub_ms> item_travellers_location,
                                  vehicle *veh = nullptr );



        dimension_id get_dimension_prefix() {
            return dimension_prefix;
        }




        std::optional<tripoint_bub_ms> find_stairs( const map &mp, int z_after,
                const tripoint_bub_ms &pos );
        std::optional<tripoint_bub_ms> find_or_make_stairs( map &mp, int z_after, bool &rope_ladder,
                bool peeking, const tripoint_bub_ms &pos );



        std::optional<tripoint_bub_ms> point_selection_menu( const std::vector<tripoint_bub_ms> &pts,
                bool up = true );



        bool vertical_shift( int z_after );

        void vertical_notes( int z_before, int z_after );

        void use_computer( const tripoint_bub_ms &p );





        template<typename T = Creature>
        T * critter_by_id( const character_id &id );






        template<typename T = Creature>
        shared_ptr_fast<T> shared_from( const T &critter );

























        monster *place_critter_at( const mtype_id &id, const tripoint_bub_ms &p );
        monster *place_critter_at( const shared_ptr_fast<monster> &mon, const tripoint_bub_ms &p );
        monster *place_critter_around( const mtype_id &id, const tripoint_bub_ms &center, int radius );
        monster *place_critter_around( const shared_ptr_fast<monster> &mon, const tripoint_bub_ms &center,
                                       int radius, bool forced = false );
        monster *place_critter_within( const mtype_id &id, const tripoint_range<tripoint_bub_ms> &range );
        monster *place_critter_within( const shared_ptr_fast<monster> &mon,
                                       const tripoint_range<tripoint_bub_ms> &range );


        monster *place_critter_at_or_within( const shared_ptr_fast<monster> &mon, map *here,
                                             const tripoint_bub_ms &center, const tripoint_range<tripoint_bub_ms> &range );






        size_t num_creatures() const;

        bool update_zombie_pos( const monster &critter, const tripoint_abs_ms &old_pos,
                                const tripoint_abs_ms &new_pos );
        void remove_zombie( const monster &critter );

        void clear_zombies();

        bool spawn_hallucination( const tripoint_bub_ms &p );

        bool spawn_hallucination( const tripoint_bub_ms &p, const mtype_id &mt,
                                  std::optional<time_duration> lifespan );

        bool spawn_npc( const tripoint_bub_ms &p, const string_id<npc_template> &npc_class,
                        std::string &unique_id,
                        std::vector<trait_id> &traits, std::optional<time_duration> lifespan );

        bool find_nearby_spawn_point( const tripoint_bub_ms &target, const mtype_id &mt, int min_radius,
                                      int max_radius, tripoint_bub_ms &point, bool outdoor_only, bool indoor_only,
                                      bool open_air_allowed = false );
        bool find_nearby_spawn_point( const tripoint_bub_ms &target, int min_radius,
                                      int max_radius, tripoint_bub_ms &point, bool outdoor_only, bool indoor_only,
                                      bool open_air_allowed = false );

        bool swap_critters( Creature &, Creature & );

    private:
        friend class monster_range;
        friend class Creature_range;

        template<typename T>
        class non_dead_range
        {
            public:
                std::vector<weak_ptr_fast<T>> items;

                class iterator
                {
                    private:
                        bool valid();
                    public:
                        std::vector<weak_ptr_fast<T>> &items;
                        typename std::vector<weak_ptr_fast<T>>::iterator iter;
                        shared_ptr_fast<T> current;

                        iterator( std::vector<weak_ptr_fast<T>> &i,
                                  const typename std::vector<weak_ptr_fast<T>>::iterator t ) : items( i ), iter( t ) {
                            while( iter != items.end() && !valid() ) {
                                ++iter;
                            }
                        }
                        iterator( const iterator & ) = default;
                        iterator &operator=( const iterator & ) = default;

                        bool operator==( const iterator &rhs ) const {
                            return iter == rhs.iter;
                        }
                        bool operator!=( const iterator &rhs ) const {
                            return !operator==( rhs );
                        }
                        iterator &operator++() {
                            do {
                                ++iter;
                            } while( iter != items.end() && !valid() );
                            return *this;
                        }
                        T &operator*() const {
                            return *current;
                        }
                };
                iterator begin() {
                    return iterator( items, items.begin() );
                }
                iterator end() {
                    return iterator( items, items.end() );
                }
                void push_back( T &new_item ) {
                    items.push_back( new_item );
                }
        };

        class monster_range : public non_dead_range<monster>
        {
            public:
                explicit monster_range( game &game_ref );
        };

        class npc_range : public non_dead_range<npc>
        {
            public:
                explicit npc_range( game &game_ref );
        };

        class Creature_range : public non_dead_range<Creature>
        {
            private:
                shared_ptr_fast<Character> u;

            public:
                explicit Creature_range( game &game_ref );
        };

    public:







        Creature_range all_creatures();

        monster_range all_monsters();

        npc_range all_npcs();





        std::vector<Creature *> get_creatures_if( const std::function<bool( const Creature & )> &pred );
        std::vector<Character *> get_characters_if( const std::function<bool( const Character & )> &pred );
        std::vector<npc *> get_npcs_if( const std::function<bool( const npc & )> &pred );
        std::vector<vehicle *> get_vehicles_if( const std::function<bool( const vehicle & )> &pred );





        Creature *get_creature_if( const std::function<bool( const Creature & )> &pred );


        bool is_empty( const tripoint_bub_ms &p );
        bool is_empty( map *here, const tripoint_abs_ms &p );

        bool is_in_sunlight( const tripoint_bub_ms &p );
        bool is_in_sunlight( map *here, const tripoint_bub_ms &p );

        bool is_sheltered( const tripoint_bub_ms &p );
        bool is_sheltered( map *here, const tripoint_bub_ms &p );











        bool revive_corpse( const tripoint_bub_ms &p, item &it );

        bool revive_corpse( const tripoint_bub_ms &p, item &it, int radius );

        void assign_revive_form( item &it, tripoint_bub_ms p );

        void save_cyborg( item *cyborg, const tripoint_bub_ms &couch_pos, Character &installer );

        bool cancel_activity_query( const std::string &text );


        bool cancel_activity_or_ignore_query( distraction_type type, const std::string &text );
        bool portal_storm_query( distraction_type type, const std::string &text );

        void moving_vehicle_dismount( const tripoint_bub_ms &dest_loc );


        vehicle *remoteveh();

        void setremoteveh( vehicle *veh );


        int assign_mission_id();

        npc *find_npc( character_id id );

        npc *find_npc_by_unique_id( const std::string &unique_id );

        void load_npcs();
        void load_npcs( map *here );



        void on_witness_theft( const item &target );
    private:







        void unload_npcs();
    public:

        void reload_npcs();
        void remove_npc( character_id const &id );
        const kill_tracker &get_kill_tracker() const;
        void clear_kill_tracker() const;
        stats_tracker &stats();
        achievements_tracker &achievements();

        void add_npc_follower( const character_id &id );

        void remove_npc_follower( const character_id &id );

        std::set<character_id> get_follower_list();

        void validate_npc_followers();
        void validate_mounted_npcs();

        void validate_linked_vehicles();

        void validate_camps();

        void catch_a_monster( monster *fish, const tripoint_bub_ms &pos, Character *p,
                              const time_duration &catch_duration );






        std::unordered_set<tripoint_abs_ms> get_fishable_locations_abs( int distance,
                const tripoint_bub_ms &fish_pos );
        std::unordered_set<tripoint_bub_ms> get_fishable_locations_bub( int distance,
                const tripoint_bub_ms &fish_pos );







        std::vector<monster *> get_fishable_monsters( std::unordered_set<tripoint_abs_ms>
                &fishable_locations );
        std::vector<monster *> get_fishable_monsters( std::unordered_set<tripoint_bub_ms>
                &fishable_locations );


        void water_affect_items( Character &ch ) const;




        bool fling_creature( Creature *c, const units::angle &dir, float flvel,
                             bool controlled = false, bool intentional = false );
        float natural_light_level( int zlev ) const;



        unsigned char light_level( int zlev ) const;
        void reset_light_level();
        character_id assign_npc_id();
        int64_t assign_item_uid();
        Creature *is_hostile_nearby();
        Creature *is_hostile_very_close( bool dangerous = false );
        field_entry *is_in_dangerous_field();


        point_rel_sm update_map( Character &p, bool z_level_changed = false );
        point_rel_sm update_map( int &x, int &y, bool z_level_changed = false );
        void update_overmap_seen();

        void peek();
        void peek( const tripoint_bub_ms &p );
        std::optional<tripoint_bub_ms> look_debug();

        bool check_zone( const zone_type_id &type, const tripoint_bub_ms &where ) const;

        bool check_near_zone( const zone_type_id &type, const tripoint_bub_ms &where ) const;
        bool is_zones_manager_open() const;
        void set_zones_manager_open( bool zm_open ) {
            zones_manager_open = zm_open;
        };










        std::optional<std::vector<tripoint_bub_ms>> safe_route_to( Character &who,
                const tripoint_bub_ms &target, int threshold,
                const std::function<void( const std::string &msg )> &report,
                bool require_visible_destination = true ) const;


        std::optional<tripoint_bub_ms> look_around();














        look_around_result look_around( bool show_window, tripoint_bub_ms &center,
                                        const tripoint_bub_ms &start_point, bool has_first_point, bool select_zone, bool peeking,
                                        bool is_moving_zone = false, const tripoint_bub_ms &end_point = tripoint_bub_ms::zero,
                                        bool change_lv = true );
        look_around_result look_around( look_around_params );


        void pre_print_all_tile_info( const tripoint_bub_ms &lp, const catacurses::window &w_info,
                                      int &line, int last_line, const visibility_variables &cache );


        void print_all_tile_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                                  std::string_view area_name, int column,
                                  int &line, int last_line, const visibility_variables &cache );

        void draw_look_around_cursor( const tripoint_bub_ms &lp, const visibility_variables &cache );




        void draw_trail_to_square( const tripoint_rel_ms &t, bool bDrawX );

        enum inventory_item_menu_position {
            RIGHT_TERMINAL_EDGE,
            LEFT_OF_INFO,
            RIGHT_OF_INFO,
            LEFT_TERMINAL_EDGE,
        };
        int inventory_item_menu( item_location locThisItem,
        const std::function<int()> &startx = []() {
            return 0;
        },
        const std::function<int()> &width = []() {
            return 50;
        },
        inventory_item_menu_position position = RIGHT_OF_INFO );


        item_location inv_map_splice( const item_filter &filter, const std::string &title, int radius = 0,
                                      const std::string &none_message = "" );
        item_location inv_map_splice( const inventory_selector_preset &preset, const std::string &title,
                                      int radius = 0, const std::string &none_message = "" );
        item_location inv_map_splice( const item_location_filter &filter, const std::string &title,
                                      int radius = 0, const std::string &none_message = "" );

        bool has_gametype() const;
        special_game_type gametype() const;

        void toggle_fullscreen();
        void toggle_pixel_minimap() const;
        void toggle_language_to_en();
        bool is_tileset_isometric() const;
        void reload_tileset();
        void temp_exit_fullscreen();
        void reenter_fullscreen();
        void zoom_in_overmap();
        void zoom_out_overmap();
        void set_overmap_zoom( int level );
        void zoom_in();
        void zoom_out();
        void reset_zoom();
        void set_zoom( int level );
        int get_zoom() const;
        int get_moves_since_last_save() const;
        int get_user_action_counter() const;
        void handle_progress_ui();






        bool take_screenshot( std::string_view file_path ) const;




        bool take_screenshot() const;









        void load_map( const tripoint_abs_sm &pos_sm, bool pump_events = false );

        static void legacy_migrate_npctalk_var_prefix( global_variables::impl_t &map_of_vars );



        overmap &get_cur_om() const;


        std::vector<npc *> allies();

        void set_driving_view_offset( const point_rel_ms &p );




        void calc_driving_offset( vehicle *veh = nullptr );



        void open_gate( const tripoint_bub_ms &p );







        void knockback( const tripoint_bub_ms &s, const tripoint_bub_ms &t, int force, int stun,
                        int dam_mult );
        void knockback( std::vector<tripoint_bub_ms> &traj, int stun, int dam_mult );


        void draw_bullet( const tripoint_bub_ms &t, int i, const std::vector<tripoint_bub_ms> &trajectory,
                          char bullet );
        void draw_hit_mon( const tripoint_bub_ms &p, const monster &m, bool dead = false );
        void draw_hit_player( const Character &p, int dam ) const;
        void draw_line( const tripoint_bub_ms &p, const tripoint_bub_ms &center_point,
                        const std::vector<tripoint_bub_ms> &points, bool noreveal = false );
        void draw_line( const tripoint_bub_ms &p, const std::vector<tripoint_bub_ms> &points );
        void draw_weather( const weather_printable &wPrint ) const;
        void draw_sct() const;
        void draw_zones( const tripoint_bub_ms &start, const tripoint_bub_ms &end,
                         const tripoint_rel_ms &offset ) const;


        void draw_critter( const Creature &critter, const tripoint_bub_ms &center );
        void draw_cursor( const tripoint_bub_ms &p ) const;


        void draw_cursor_unobscuring( const tripoint_bub_ms &p ) const;


        void draw_highlight( const tripoint_bub_ms &p );

        void draw_async_anim( const tripoint_bub_ms &p, std::string_view tile_id,
                              const std::string &ncstr = "",
                              const nc_color &nccol = c_black );
        void draw_radiation_override( const tripoint_bub_ms &p, int rad );
        void draw_terrain_override( const tripoint_bub_ms &p, const ter_id &id );
        void draw_furniture_override( const tripoint_bub_ms &p, const furn_id &id );
        void draw_graffiti_override( const tripoint_bub_ms &p, bool has );
        void draw_trap_override( const tripoint_bub_ms &p, const trap_id &id );
        void draw_field_override( const tripoint_bub_ms &p, const field_type_id &id );
        void draw_item_override( const tripoint_bub_ms &p, const itype_id &id, const mtype_id &mid,
                                 bool hilite );
        void draw_vpart_override( const tripoint_bub_ms &p, const vpart_id &id, int part_mod,
                                  const units::angle &veh_dir, bool hilite, const point_rel_ms &mount );
        void draw_monster_override( const tripoint_bub_ms &p, const mtype_id &id, int count,
                                    bool more, Creature::Attitude att );

        bool is_in_viewport( const tripoint_bub_ms &p, int margin = 0 ) const;




        bool check_safe_mode_allowed( bool repeat_safe_mode_warnings = true );
        void set_safe_mode( safe_mode_type mode );


        void exam_appliance( vehicle &veh, const point_rel_ms &cp = point_rel_ms::zero );


        void exam_vehicle( vehicle &veh, const point_rel_ms &cp = point_rel_ms::zero );


        bool load( const std::string &world );


        bool npc_menu( npc &who );


        bool phasing_move( const tripoint_bub_ms &dest, bool via_ramp = false );

        bool phasing_move_enchant( const tripoint_bub_ms &dest, int phase_distance = 0 );
        bool can_move_furniture( tripoint_bub_ms fdest, const tripoint_rel_ms &dp );

        bool walk_move( const tripoint_bub_ms &dest, bool via_ramp = false, bool furniture_move = false );
        void on_move_effects();
    private:

        bool load( const save_t &name );
        void load_master();
#if defined(__ANDROID__)
        void load_shortcuts( const cata_path &path );
#endif
        bool start_game();



        bool save_factions_missions_npcs();
        bool save_external_options_record();
        bool save_dimension_data();
        bool load_dimension_data();
        void reset_npc_dispositions();
        void serialize_dimension_data( std::ostream &fout );
        void serialize_master( std::ostream &fout );

        bool save_maps();
#if defined(__ANDROID__)
        void save_shortcuts( std::ostream &fout );
#endif

        void init_autosave();
        void create_starting_npcs();

        vehicle *place_vehicle_nearby(
            const vproto_id &id, const point_abs_omt &origin, int min_distance,
            int max_distance, const std::vector<std::string> &omt_search_types = {} );

        void list_surroundings();




        bool grabbed_move( const tripoint_rel_ms &dp, bool via_ramp, bool stairs_move = false );
        bool grabbed_veh_move_helper( const tripoint_rel_ms &dp, bool stairs_move );
        bool grabbed_veh_move_stairs( const tripoint_rel_ms &dp );
        bool grabbed_veh_move( const tripoint_rel_ms &dp );


        void examine( const tripoint_bub_ms &p, bool with_pickup = false );
        void examine( bool with_pickup = true );
        std::string get_fire_fuel_string( const tripoint_bub_ms &examp ) const;


        void pickup();

        void pickup_all();

        void unload_container( const std::optional<tripoint_bub_ms> &p =
                                   std::nullopt );
        void drop_in_direction( const tripoint_bub_ms &pnt );

        void butcher( const std::optional<tripoint_bub_ms> &p = std::nullopt );

        void reload( item_location &loc, bool prompt = false, bool empty = true );
    public:



        void control_vehicle( const std::optional<tripoint_bub_ms> &p =
                                  std::nullopt );






        bool warn_player_maybe_anger_local_faction( bool really_bad_offense = false,
                bool asking_for_public_goods = false );
        int grabbed_furn_move_time( const tripoint_rel_ms &dp );
        bool grabbed_furn_move( const tripoint_rel_ms &dp );

        void reload_item();
        void reload_wielded( bool prompt = false );

        void auto_reload_wielded( int steps_remaining = 256 );
        void reload_weapon( bool try_everything = true );
        void insert_item();
        void insert_item( drop_locations &targets );

        point_rel_sm place_player( const tripoint_bub_ms &dest, bool quick = false );
        void place_player_overmap( const tripoint_abs_omt &om_dest, bool move_player = true );
        void perhaps_add_random_npc( bool ignore_spawn_timers_and_rates );
        static void display_om_pathfinding_progress( size_t open_set, size_t known_size );

        unsigned int get_seed() const;


        void set_npcs_dirty();

        void set_critter_died();
        void mon_info_update( );
        void cleanup_dead();
        bool is_dangerous_tile( const tripoint_bub_ms &dest_loc ) const;
        std::vector<std::string> get_dangerous_tile( const tripoint_bub_ms &dest_loc,
                size_t max = 0 ) const;
        bool prompt_dangerous_tile( const tripoint_bub_ms &dest_loc,
                                    std::vector<std::string> *harmful_stuff = nullptr ) const;

        void pickup( const tripoint_bub_ms &p );
        void chat( const std::optional<tripoint_bub_ms> &p = std::nullopt );
    private:


        void print_fields_info( const tripoint_bub_ms &lp, const catacurses::window &w_look, int column,
                                int &line );
        void print_terrain_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                                 std::string_view area_name, int column, int &line, bool visible );
        void print_furniture_info( const tripoint_bub_ms &lp, const catacurses::window &w_look, int column,
                                   int &line, bool visible );
        void print_trap_info( const tripoint_bub_ms &lp, const catacurses::window &w_look, int column,
                              int &line );
        void print_part_con_info( const tripoint_bub_ms &lp, const catacurses::window &w_look, int column,
                                  int &line );
        void print_creature_info( const Creature *creature, const catacurses::window &w_look, int column,
                                  int &line, int last_line );
        void print_vehicle_info( const vehicle *veh, int veh_part, const catacurses::window &w_look,
                                 int column, int &line, int last_line );
        void print_visibility_info( const catacurses::window &w_look, int column, int &line,
                                    visibility_type visibility );
        void print_items_info( const tripoint_bub_ms &lp, const catacurses::window &w_look, int column,
                               int &line,
                               int last_line );
        void print_graffiti_info( const tripoint_bub_ms &lp, const catacurses::window &w_look, int column,
                                  int &line, int last_line );

        void print_debug_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                               int column, int &line );

        input_context get_player_input( std::string &action );








        void shift_monsters( const tripoint_rel_sm &shift );
    public:





        void despawn_monster( monster &critter );

        void despawn_nonlocal_monsters();
    private:

        void open_consume_item_menu();
        void move_or_drive( action_id &act, avatar &player_character, const item_location &weapon,
                            int desired_move_mode_cost );
        bool do_regular_action( action_id &act, avatar &player_character,
                                const std::optional<tripoint_bub_ms> &mouse_target );
        bool handle_action();
        bool try_get_right_click_action( action_id &act, const tripoint_bub_ms &mouse_target );
        bool try_get_left_click_action( action_id &act, const tripoint_bub_ms &mouse_target );

        void item_action_menu( item_location loc = item_location() );

        bool is_game_over();
        void bury_screen() const;
        void death_screen();
    public:







        bool disable_robot( const tripoint_bub_ms &p );

        void draw_pixel_minimap( const catacurses::window &w );
    private:


        void autosave();
    public:
        void quicksave();
        void quickload();
        void disp_NPCs();

        void list_missions();
    private:


        bool handle_mouseview( input_context &ctxt, std::string &action );


        void display_faction_epilogues();
        void disp_NPC_epilogues();

    public:


        std::optional<action_id> displaying_overlays; // NOLINT(cata-serialize)
        void display_scent();
        void display_visibility();
        void display_lighting();

    private:

        class debug_hour_timer
        {
            public:
                using IRLTimeMs = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;
                void toggle();
                void print_time();
            private:
                bool enabled = false;
                std::optional<IRLTimeMs> start_time = std::nullopt;
        } debug_hour_timer; // NOLINT(cata-serialize)







        Creature *is_hostile_within( int distance, bool dangerous = false );

        static std::string timestamp_now();
        void move_save_to_graveyard();
        bool save_player_data();
        bool save_achievements();


        pimpl<map> map_ptr; // NOLINT(cata-serialize)
        pimpl<::current_map> current_map_ptr; // NOLINT(cata-serialize)
        pimpl<avatar> u_ptr; // NOLINT(cata-serialize)
        pimpl<live_view> liveview_ptr; // NOLINT(cata-serialize)
        live_view &liveview; // NOLINT(cata-serialize)
        pimpl<scent_map> scent_ptr; // NOLINT(cata-serialize)
        pimpl<timed_event_manager> timed_event_manager_ptr; // NOLINT(cata-serialize)
        pimpl<item_wakeup_manager> item_wakeup_manager_ptr;
        pimpl<event_bus> event_bus_ptr; // NOLINT(cata-serialize)
        pimpl<stats_tracker> stats_tracker_ptr;
        pimpl<achievements_tracker> achievements_tracker_ptr;
        pimpl<kill_tracker> kill_tracker_ptr;
        pimpl<memorial_logger> memorial_logger_ptr; // NOLINT(cata-serialize)
        pimpl<spell_events> spell_events_ptr; // NOLINT(cata-serialize)
        pimpl<eoc_events> eoc_events_ptr; // NOLINT(cata-serialize)
        pimpl<power_network_manager> power_networks_ptr;

        map &m; // NOLINT(cata-serialize)

        ::current_map &current_map; // NOLINT(cata-serialize)
        avatar &u;
        scent_map &scent;

        const scenario *scen = nullptr; // NOLINT(cata-serialize)

        event_bus &events();
        timed_event_manager &timed_events; // NOLINT(cata-serialize)
        memorial_logger &memorial();

        global_variables global_variables_instance;
        std::unordered_map<std::string, point_abs_om> unique_npcs;
    public:
        void update_unique_npc_location( const std::string &id, point_abs_om loc );
        point_abs_om get_unique_npc_location( const std::string &id );
        bool unique_npc_exists( const std::string &id );
        void unique_npc_despawn( const std::string &id );
        std::vector<effect_on_condition_id> inactive_global_effect_on_condition_vector;
        queued_eocs queued_global_effect_on_conditions;

        spell_events &spell_events_subscriber();
        power_network_manager &power_networks();

        pimpl<creature_tracker> critter_tracker;
        pimpl<faction_manager> faction_manager_ptr; // NOLINT(cata-serialize)


        quit_status uquit; // NOLINT(cata-serialize)

        bool save_is_dirty; // NOLINT(cata-serialize)

        bool new_game = false; // NOLINT(cata-serialize)

        tripoint_bub_ms ter_view_p; // NOLINT(cata-serialize)
        catacurses::window w_terrain; // NOLINT(cata-serialize)
        catacurses::window w_overmap; // NOLINT(cata-serialize)
        catacurses::window w_omlegend; // NOLINT(cata-serialize)
        catacurses::window w_minimap; // NOLINT(cata-serialize)
        catacurses::window w_pixel_minimap; // NOLINT(cata-serialize)



        overmap_ui::overmap_draw_data_t overmap_data; // NOLINT(cata-serialize)




        point_rel_ms driving_view_offset;


        bool debug_pathfinding = false; // NOLINT(cata-serialize)


        npc *follower_path_to_show = nullptr; // NOLINT(cata-serialize)



        void display_toggle_overlay( action_id );

        bool display_overlay_state( action_id );

        void toggle_debug_hour_timer();

        Creature *displaying_visibility_creature; // NOLINT(cata-serialize)

        int displaying_lighting_condition = 0; // NOLINT(cata-serialize)

        bool show_panel_adm = false; // NOLINT(cata-serialize)
        bool fullscreen = false; // NOLINT(cata-serialize)
        bool was_fullscreen = false; // NOLINT(cata-serialize)
        bool auto_travel_mode = false;
        bool queue_screenshot = false; // NOLINT(cata-serialize)
        safe_mode_type safe_mode;



        time_duration turnssincelastmon = 0_turns;
    private:
        weather_manager weather; // NOLINT(cata-serialize)

    public:



        int mostseen = 0;
    private:
        shared_ptr_fast<Character> u_shared_ptr; // NOLINT(cata-serialize)

        catacurses::window w_terrain_ptr; // NOLINT(cata-serialize)
        catacurses::window w_minimap_ptr; // NOLINT(cata-serialize)


        std::string sFilter; // NOLINT(cata-serialize)
        std::string list_item_upvote; // NOLINT(cata-serialize)
        std::string list_item_downvote; // NOLINT(cata-serialize)

        bool safe_mode_warning_logged = false; // NOLINT(cata-serialize)
        bool bVMonsterLookFire = false;
        character_id next_npc_id; // NOLINT(cata-serialize)
        int next_mission_id = 0; // NOLINT(cata-serialize)
        int64_t next_item_uid = 1; // NOLINT(cata-serialize)

        std::chrono::seconds time_played_at_last_load; // NOLINT(cata-serialize)
        // NOLINTNEXTLINE(cata-serialize)
        std::chrono::time_point<std::chrono::steady_clock> time_of_last_load;
        int moves_since_last_save = 0; // NOLINT(cata-serialize)
        std::time_t last_save_timestamp = 0; // NOLINT(cata-serialize)

        mutable std::array<float, OVERMAP_LAYERS> latest_lightlevels; // NOLINT(cata-serialize)

        time_point remoteveh_cache_time; // NOLINT(cata-serialize)
        vehicle *remoteveh_cache; // NOLINT(cata-serialize)

        bool npcs_dirty = false; // NOLINT(cata-serialize)

        bool critter_died = false; // NOLINT(cata-serialize)

        bool first_redraw_since_waiting_started = true; // NOLINT(cata-serialize)

        bool zones_manager_open = false; // NOLINT(cata-serialize)

        std::unique_ptr<special_game> gamemode; // NOLINT(cata-serialize)


        int user_action_counter = 0; // NOLINT(cata-serialize)


        unsigned int seed = 0; // NOLINT(cata-serialize)


        std::vector<tripoint_bub_ms> destination_preview; // NOLINT(cata-serialize)

        // NOLINTNEXTLINE(cata-serialize)
        std::chrono::time_point<std::chrono::steady_clock> last_mouse_edge_scroll;
        tripoint_rel_ms last_mouse_edge_scroll_vector_terrain; // NOLINT(cata-serialize)
        tripoint_rel_omt last_mouse_edge_scroll_vector_overmap; // NOLINT(cata-serialize)
        std::pair<tripoint_rel_ms, tripoint_rel_ms> mouse_edge_scrolling( input_context &ctxt, int speed,
                const tripoint_rel_ms &last, bool iso );
        std::pair<tripoint_rel_omt, tripoint_rel_omt> mouse_edge_scrolling( input_context &ctxt, int speed,
                const tripoint_rel_omt &last, bool iso );

        weak_ptr_fast<ui_adaptor> main_ui_adaptor; // NOLINT(cata-serialize)

        std::unique_ptr<static_popup> wait_popup; // NOLINT(cata-serialize)
    public:
        void wait_popup_reset();









        tripoint_rel_ms mouse_edge_scrolling_terrain( input_context &ctxt );

        tripoint_rel_omt mouse_edge_scrolling_overmap( input_context &ctxt );


        void shift_destination_preview( const point_rel_ms &delta );




        enum class climb_maneuver {
            down,
            up,
            over_obstacle,
        };








        bool slip_down(
            climb_maneuver maneuver,
            climbing_aid_id aid = climbing_aid_id::NULL_ID(),
            bool show_chance_messages = true );








        float slip_down_chance(
            climb_maneuver maneuver,
            climbing_aid_id aid = climbing_aid_id::NULL_ID(),
            bool show_chance_messages = true );







        void climb_down( const tripoint_bub_ms &examp );

        void climb_down_menu_gen( const tripoint_bub_ms &examp, uilist &cmenu );
        bool climb_down_menu_pick( const tripoint_bub_ms &examp, int retval );
        void climb_down_using(
            const tripoint_bub_ms &examp,
            climbing_aid_id aid,
            bool deploy_affordance = false );

        pulp_data calculate_character_ability_to_pulp( const Character &you );
        pulp_data calculate_pulpability( const Character &you, const mtype &corpse_mtype );
        pulp_data calculate_pulpability( const Character &you, const mtype &corpse_mtype, pulp_data pd );
        bool can_pulp_corpse( const Character &you, const mtype &corpse_mtype );
        bool can_pulp_corpse( const pulp_data &pd );
        bool can_pulp_acid_corpse( const Character &you, const mtype &corpse_mtype );


        bool swapping_dimensions = false; // NOLINT (cata-serialize)
    private:

        dimension_id dimension_prefix;
};



units::temperature_delta get_heat_radiation( const tripoint_bub_ms &location );


int get_best_fire( const tripoint_bub_ms &location );

units::temperature_delta get_convection_temperature( const tripoint_bub_ms &location );

namespace cata_event_dispatch
{




void avatar_moves( const tripoint_abs_ms &old_abs_pos, const avatar &u, const map &m );
}

#endif
