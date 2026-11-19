#pragma once
#ifndef CATA_SRC_MAP_H
#define CATA_SRC_MAP_H

#include <array>
#include <bitset>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "active_item_cache.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_small_literal_vector.h"
#include "cata_type_traits.h"
#include "cata_utility.h"
#include "colony.h"
#include "coords_fwd.h"
#include "creature.h"
#include "enums.h"
#include "game_constants.h"
#include "item.h"
#include "item_stack.h"
#include "level_cache.h"
#include "lightmap.h"
#include "line.h"
#include "lru_cache.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "maptile_fwd.h"
#include "point.h"
#include "rng.h"
#include "tile_tint.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "vpart_position.h"

#if defined(TILES)
#include "cata_tiles.h"
#endif

struct scent_block;

namespace catacurses
{
class window;
}
class Character;
class Creature;
class basecamp;
class character_id;
class computer;
class field;
class field_entry;
class item_location;
class mapgendata;
class monster;
class relic_procgen_data;
class submap;
class vehicle;
class zone_data;
struct fragment_cloud;
struct partial_con;
struct spawn_data;
struct trap;
template<typename Tripoint>
class tripoint_range;

enum class special_item_type : int;
class npc_template;
class tileray;
class vpart_reference;
struct MonsterGroupResult;
struct mongroup;
struct projectile;
struct veh_collision;

struct wrapped_vehicle {
    tripoint_bub_ms pos;
    vehicle *v;
};

using VehicleList = std::vector<wrapped_vehicle>;
class map;

class weather_generator;

enum class ter_furn_flag : int;
struct pathfinding_cache;
struct pathfinding_settings;
struct pathfinding_target;
template<typename T>
struct weighted_int_list;
struct field_proc_data;

class PathfindingFlags;

class map_stack : public item_stack
{
    private:
        tripoint_bub_ms location;
        map *myorigin;
    public:
        map_stack( cata::colony<item> *newstack, tripoint_bub_ms newloc, map *neworigin ) :
            item_stack( newstack ), location( newloc ), myorigin( neworigin ) {}
        void insert( map &, const item &newitem ) override;
        void insert( const item &newitem );
        iterator erase( const_iterator it ) override;
        int count_limit() const override {
            return MAX_ITEM_IN_SQUARE;
        }
        units::volume max_volume() const override;
};

struct visibility_variables {

    bool variables_set = false;
    bool u_sight_impaired = false;
    bool u_is_boomered = false;

    int g_light_level = 0;
    int u_clairvoyance = 0;
    float vision_threshold = 0.0f;
    std::optional<field_type_id> clairvoyance_field;
};



struct visibility_inputs {
    tripoint_abs_ms pos;
    uint64_t fov_generation = 0;
    uint64_t lightmap_generation = 0;
    uint64_t sight_revision = 0;

    uint64_t field_revision = 0;
    uint64_t forced = 0;
    uint64_t aim_generation = 0;
    float vision_threshold = -1.0f;
    int clairvoyance = -1;
    int unimpaired_range = -1;
    int g_light_level = -1;
    bool boomered = false;
    bool aiming = false;

    bool operator==( const visibility_inputs &o ) const {
        return pos == o.pos && fov_generation == o.fov_generation &&
               lightmap_generation == o.lightmap_generation && sight_revision == o.sight_revision &&
               field_revision == o.field_revision &&
               forced == o.forced && aim_generation == o.aim_generation &&
               vision_threshold == o.vision_threshold && clairvoyance == o.clairvoyance &&
               unimpaired_range == o.unimpaired_range && g_light_level == o.g_light_level &&
               boomered == o.boomered && aiming == o.aiming;
    }
};

struct bash_params {

    const std::map<damage_type_id, int> &strength;

    bool silent = false;

    bool destroy = false;

    bool bash_floor = false;






    float roll = 0.0f;

    bool did_bash = false;

    bool can_bash = false;

    bool success = false;

    bool bashed_solid = false;






    bool bashing_from_above = false;
};


struct drawsq_params {
    private:
        tripoint_bub_ms view_center = tripoint_bub_ms::invalid;
        ter_str_id ter_override = ter_str_id::NULL_ID();
        furn_str_id furn_override = furn_str_id::NULL_ID();
        bool do_highlight = false;
        bool do_show_items = true;
        bool do_low_light = false;
        bool do_bright_light = false;
        bool do_memorize = false;
        bool do_output = true;

    public:
        drawsq_params() = default;






        constexpr drawsq_params &highlight( bool v ) {
            do_highlight = v;
            return *this;
        }
        constexpr bool highlight() const {
            return do_highlight;
        }







        constexpr drawsq_params &show_items( bool v ) {
            do_show_items = v;
            return *this;
        }
        constexpr bool show_items() const {
            return do_show_items;
        }







        constexpr drawsq_params &low_light( bool v ) {
            do_low_light = v;
            return *this;
        }
        constexpr bool low_light() const {
            return do_low_light;
        }







        constexpr drawsq_params &bright_light( bool v ) {
            do_bright_light = v;
            return *this;
        }
        constexpr bool bright_light() const {
            return do_bright_light;
        }







        constexpr drawsq_params &memorize( bool v ) {
            do_memorize = v;
            return *this;
        }
        constexpr bool memorize() const {
            return do_memorize;
        }








        constexpr drawsq_params &output( bool v ) {
            do_output = v;
            return *this;
        }
        constexpr bool output() const {
            return do_output;
        }







        constexpr drawsq_params &center( const tripoint_bub_ms &p ) {
            view_center = p;
            return *this;
        }
        constexpr drawsq_params &center_at_avatar() {
            view_center = tripoint_bub_ms::invalid;
            return *this;
        }
        tripoint_bub_ms center() const;







        drawsq_params &terrain_override( const ter_str_id &id ) {
            ter_override = id;
            return *this;
        }
        drawsq_params &furniture_override( const furn_str_id &id ) {
            furn_override = id;
            return *this;
        }
        const ter_str_id &terrain_override() const;
        const furn_str_id &furniture_override() const;

};




// NOLINTNEXTLINE(cata-xy)
struct sprite_screen_bounds {
    int x = 0, y = 0, w = 0, h = 0;
    bool valid = false;
    // NOLINTNEXTLINE(cata-large-inline-function,cata-xy)
    void expand( int rx, int ry, int rw, int rh ) {
        if( !valid ) {
            x = rx;
            y = ry;
            w = rw;
            h = rh;
            valid = true;
        } else {
            const int nx = x < rx ? x : rx; // NOLINT(cata-combine-locals-into-point)
            const int ny = y < ry ? y : ry;
            const int x2 = ( x + w > rx + rw ) ? x + w : rx + rw;
            const int y2 = ( y + h > ry + rh ) ? y + h : ry + rh;
            x = nx;
            y = ny;
            w = x2 - nx;
            h = y2 - ny;
        }
    }
};





struct tint_sprite_record {
    int sprite_index;
    struct { // NOLINT(cata-xy)
        int x, y, w, h;
    } destination;
    double angle;
    int flip;
};

struct tile_render_info {
    struct common {
        const tripoint_bub_ms pos;

        int height_3d = 0;





        sprite_screen_bounds bounds;
        small_literal_vector<tint_sprite_record, 4> tint_sprites;
        bool needs_tint = false;
        tile_tint tint_color;

        common( const tripoint_bub_ms &pos, const int height_3d )
            : pos( pos ), height_3d( height_3d ) {}
    };

    struct vision_effect {
        visibility_type vis;

        explicit vision_effect( const visibility_type vis )
            : vis( vis ) {}
    };

    struct sprite {
        lit_level ll;
        std::array<bool, 5> invisible;

        sprite( const lit_level ll, const std::array<bool, 5> &inv )
            : ll( ll ), invisible( inv ) {}
    };

    common com;
    std::variant<vision_effect, sprite> var;

    tile_render_info( const common &com, const vision_effect &var )
        : com( com ), var( var ) {}

    tile_render_info( const common &com, const sprite &var )
        : com( com ), var( var ) {}
};


enum class los_trace : int {

    optical,

    physical,
};




























class map
{
        friend class teleport;
        friend class editmap_ui;
        friend std::list<item> map_cursor::remove_items_with( const std::function<bool( const item & )> &,
                int );


        friend void field_processor_fd_fire( const tripoint_bub_ms &, field_entry &, field_proc_data & );
        friend void field_processor_spread_gas( const tripoint_bub_ms &, field_entry &, field_proc_data & );
        friend void field_processor_wandering_field( const tripoint_bub_ms &, field_entry &,
                field_proc_data & );
        friend void field_processor_fd_fire_vent( const tripoint_bub_ms &, field_entry &,
                field_proc_data & );
        friend void field_processor_fd_flame_burst( const tripoint_bub_ms &, field_entry &,
                field_proc_data & );
        friend void field_processor_fd_incendiary( const tripoint_bub_ms &, field_entry &,
                field_proc_data & );
        friend void field_processor_monster_spawn( const tripoint_bub_ms &, field_entry &,
                field_proc_data & );



        friend class jmapgen_monster;
        friend class monster;


        friend class map_meddler;
        friend void clear_fields( int zlevel );

    protected:
        map( int mapsize, bool zlev );
    public:

        map() : map( MAPSIZE, true ) { }
        virtual ~map();



        void temp_based_phase_change_at( const tripoint_bub_ms &p, const class weather_generator &wgen );


        bool has_original_terrain_at( const tripoint_bub_ms &p ) const;
        ter_id get_original_terrain_at( const tripoint_bub_ms &p ) const;
        void set_original_terrain_at( const tripoint_bub_ms &p, const ter_id &t );
        void clear_original_terrain_at( const tripoint_bub_ms &p );

        map &operator=( const map & ) = delete;
        // NOLINTNEXTLINE(performance-noexcept-move-constructor)
        map &operator=( map && );









        void set_transparency_cache_dirty( int zlev );







        void set_transparency_cache_dirty( const tripoint_bub_ms &p, bool field = false );
        void set_seen_cache_dirty( const tripoint_bub_ms &change_location );


        void set_seen_cache_dirty( int zlevel );
        void set_outside_cache_dirty( int zlev );
        void set_floor_cache_dirty( int zlev );

        void bump_geometry_revision( int zlev );

        bool cast_reached( const tripoint_bub_ms &p ) const;
        void set_lightmap_cache_dirty( int zlev );
        void set_lightmap_cache_dirty_below( int zlev );
        void set_pathfinding_cache_dirty( int zlev );
        void set_pathfinding_cache_dirty( const tripoint_bub_ms &p );


        void invalidate_map_cache( int zlev );


        bool memory_cache_dec_is_dirty( const tripoint_bub_ms &p ) const;

        bool memory_cache_ter_is_dirty( const tripoint_bub_ms &p ) const;

        void memory_cache_dec_set_dirty( const tripoint_bub_ms &p, bool value ) const;

        void memory_cache_ter_set_dirty( const tripoint_bub_ms &p, bool value ) const;

        void memory_clear_vehicle_points( const vehicle &veh ) const;






        bool has_potential_los( const tripoint_bub_ms &from, const tripoint_bub_ms &to ) const;




        void on_vehicle_moved( int smz );


        void mark_turn_light_dirty();

        struct apparent_light_info {
            bool obstructed;
            bool abs_obstructed;
            float apparent_light;
        };


        static apparent_light_info apparent_light_helper( const level_cache &map_cache,
                const tripoint_bub_ms &p );






        lit_level apparent_light_at( const tripoint_bub_ms &p, const visibility_variables &cache ) const;
        visibility_type get_visibility( lit_level ll,
                                        const visibility_variables &cache ) const;


        std::tuple<maptile, maptile, maptile> get_wind_blockers( const int &winddirection,
                const tripoint_bub_ms &pos );












        void draw( const catacurses::window &w, const tripoint_bub_ms &center );








        void drawsq( const catacurses::window &w, const tripoint_bub_ms &p,
                     const drawsq_params &params ) const;













        void save();













        void load( const tripoint_abs_sm &w, bool update_vehicles,
                   bool pump_events = false );






        void shift( const point_rel_sm &s );






        void vertical_shift( int newz );

        void clear_spawns();
        void clear_traps();

        const_maptile maptile_at( const tripoint_bub_ms &p ) const;
        maptile maptile_at( const tripoint_bub_ms &p );
    private:

        const_maptile maptile_at_internal( const tripoint_bub_ms &p ) const;
        maptile maptile_at_internal( const tripoint_bub_ms &p );
        std::pair<tripoint_bub_ms, maptile> maptile_has_bounds( const tripoint_bub_ms &p,
                bool bounds_checked );
        std::array<std::pair<tripoint_bub_ms, maptile>, 8> get_neighbors( const tripoint_bub_ms &p );
        void spread_gas( field_entry &cur, const tripoint_bub_ms &p, int percent_spread,
                         const time_duration &outdoor_age_speedup, scent_block &sblk,
                         const oter_id &om_ter );
        void create_hot_air( const tripoint_bub_ms &p, int intensity );
        bool gas_can_spread_to( field_entry &cur, const maptile &dst );
        void gas_spread_to( field_entry &cur, maptile &dst, const tripoint_bub_ms &p );
        int burn_body_part( Character &you, field_entry &cur, const bodypart_id &bp, int scale );
    public:
















        int move_cost( const tripoint_bub_ms &p, const vehicle *ignored_vehicle = nullptr,
                       bool ignore_fields = false ) const;
        int move_cost( const point_bub_ms &p, const vehicle *ignored_vehicle = nullptr,
                       bool ignore_fields = false ) const {
            return move_cost( tripoint_bub_ms( p, abs_sub.z() ), ignored_vehicle, ignore_fields );
        }
        bool impassable( const tripoint_bub_ms &p ) const;
        bool impassable( const point_bub_ms &p ) const {
            return !passable( p );
        }
        bool passable( const tripoint_bub_ms &p ) const;
        bool passable( const point_bub_ms &p ) const {
            return passable( tripoint_bub_ms( p, abs_sub.z() ) );
        }




        bool passable_through( const tripoint_bub_ms &p ) const;

        bool passable_skip_fields( const tripoint_bub_ms &p ) const;
        bool is_wall_adjacent( const tripoint_bub_ms &center ) const;

        bool is_open_air( const tripoint_bub_ms &p ) const;

        bool try_fall( const tripoint_bub_ms &p, Creature *c );




        int move_cost_ter_furn( const tripoint_bub_ms &p ) const;
        bool impassable_ter_furn( const tripoint_bub_ms &p ) const;
        bool passable_ter_furn( const tripoint_bub_ms &p ) const;






        int combined_movecost( const tripoint_bub_ms &from, const tripoint_bub_ms &to,
                               const vehicle *ignored_vehicle = nullptr,
                               int modifier = 0, bool flying = false, bool via_ramp = false,
                               bool ignore_fields = false ) const;






        bool valid_move( const tripoint_bub_ms &from, const tripoint_bub_ms &to,
                         bool bash = false, bool flying = false, bool via_ramp = false ) const;






        double ranged_target_size( const tripoint_bub_ms &p ) const;







        bool sees( const tripoint_bub_ms &F, const tripoint_bub_ms &T, int range,
                   bool with_fields = true, los_trace trace = los_trace::optical ) const;
    private:










        bool sees( const tripoint_bub_ms &F, const tripoint_bub_ms &T, int range, int &bresenham_slope,
                   bool with_fields = true, bool allow_cached = true,
                   los_trace trace = los_trace::optical ) const;
        point sees_cache_key( const tripoint_bub_ms &from, const tripoint_bub_ms &to ) const;
    public:





        int obstacle_coverage( const tripoint_bub_ms &loc1, const tripoint_bub_ms &loc2 ) const;
        int ledge_coverage( const Creature &viewer, const tripoint_bub_ms &target_p ) const;


        float eye_level( const Creature &viewer ) const;
        int ledge_coverage( const tripoint_bub_ms &viewer_p, const tripoint_bub_ms &target_p,
                            const float &eye_level = 1.0f ) const;



        int coverage( const tripoint_bub_ms &p ) const;









        bool clear_path( const tripoint_bub_ms &f, const tripoint_bub_ms &t, int range,
                         int cost_min, int cost_max ) const;










        std::vector<tripoint_bub_ms> reachable_flood_steps( const tripoint_bub_ms &f, int range,
                int cost_min = 1, int cost_max = 100 ) const;







        std::vector<tripoint_bub_ms> find_clear_path( const tripoint_bub_ms &source,
                const tripoint_bub_ms &destination, bool empty_on_fail = false ) const;





        bool accessible_items( const tripoint_bub_ms &t ) const;







        void for_each_reachable_item( const tripoint_bub_ms &center, int radius,
                                      const Character *ch,
                                      const std::function<void( const item & )> &fn );





        void for_each_visible_item( const tripoint_bub_ms &center, int radius,
                                    const Character *ch,
                                    const std::function<void( const item & )> &fn );






        std::vector<tripoint_bub_ms> get_dir_circle( const tripoint_bub_ms &f,
                const tripoint_bub_ms &t ) const;









        std::vector<tripoint_bub_ms> route( const tripoint_bub_ms &f, const pathfinding_target &target,
                                            const pathfinding_settings &settings,
        const std::function<bool( const tripoint_bub_ms & )> &avoid = []( const tripoint_bub_ms & ) {
            return false;
        } ) const;







        std::vector<tripoint_bub_ms> route( const Creature &who, const pathfinding_target &target ) const;


        bool can_pass( const tripoint_bub_ms &cur, const tripoint_bub_ms &p,
                       const pathfinding_settings &settings ) const;



        std::vector<tripoint_bub_ms> straight_route( const tripoint_bub_ms &f,
                const tripoint_bub_ms &t ) const;
    private:


        int cost_to_pass( const tripoint_bub_ms &cur, const tripoint_bub_ms &p,
                          const pathfinding_settings &settings,
                          PathfindingFlags p_special ) const;



        int cost_to_avoid( const tripoint_bub_ms &cur, const tripoint_bub_ms &p,
                           const pathfinding_settings &settings,
                           PathfindingFlags p_special ) const;

        int extra_cost( const tripoint_bub_ms &cur, const tripoint_bub_ms &p,
                        const pathfinding_settings &settings,
                        PathfindingFlags p_special ) const;


        void resolve_off_map_grid_generation();


        void resolve_appliance_grid_power();
    public:


        VehicleList get_vehicles();
        bool place_vehicle( std::unique_ptr<vehicle> &&new_vehicle );
        void add_vehicle_to_cache( vehicle * );
        void clear_vehicle_point_from_cache( vehicle *veh, const tripoint_bub_ms &pt );

        void clear_vehicle_level_caches();
        void remove_vehicle_from_cache( vehicle *veh, int zmin = -OVERMAP_DEPTH,
                                        int zmax = OVERMAP_HEIGHT );
        void reset_vehicles_sm_pos();

        void rebuild_vehicle_level_caches();
        void clear_vehicle_list( int zlev );
        void update_vehicle_list( const submap *to, int zlev );

        bool check_vehicle_zones( int zlev );
        std::vector<zone_data *> get_vehicle_zones( int zlev );
        void register_vehicle_zone( vehicle *, int zlev );
        bool deregister_vehicle_zone( zone_data &zone ) const;



        std::set<tripoint_bub_ms> get_moving_vehicle_targets( const Creature &z, int max_range );


        std::unique_ptr<vehicle> detach_vehicle( vehicle *veh );
        void destroy_vehicle( vehicle *veh );

        void vehmove();

        bool vehproceed( VehicleList &vehicle_list );


        VehicleList get_vehicles( const tripoint_bub_ms &start, const tripoint_bub_ms &end );





        optional_vpart_position veh_at( const tripoint_abs_ms &p ) const;
        optional_vpart_position veh_at( const tripoint_bub_ms &p ) const;
        vehicle *veh_at_internal( const tripoint_bub_ms &p, int &part_num );
        const vehicle *veh_at_internal( const tripoint_bub_ms &p, int &part_num ) const;

        void board_vehicle( const tripoint_bub_ms &p, Character *pl );


        void unboard_vehicle( const vpart_reference &, Character *passenger,
                              bool dead_passenger = false );

        void unboard_vehicle( const tripoint_bub_ms &p, bool dead_passenger = false );



        bool displace_vehicle( vehicle &veh, const tripoint_rel_ms &dp, bool adjust_pos = true,
                               const std::set<int> &parts_to_move = {} );



        void level_vehicle( vehicle &veh );

        bool displace_water( const tripoint_bub_ms &dp );




        float vehicle_wheel_traction( const vehicle &veh, bool ignore_movement_modifiers = false );




        float vehicle_wheel_traction( const vehicle &veh, const tripoint_bub_ms &at_origin,
                                      bool ignore_movement_modifiers = false );




        float vehicle_vehicle_collision( vehicle &veh, vehicle &veh2,
                                         const std::vector<veh_collision> &collisions );


        units::angle shake_vehicle( vehicle &veh, int velocity_before, const units::angle &direction );



        vehicle *move_vehicle( vehicle &veh, const tripoint_rel_ms &dp, const tileray &facing );


        void set( const tripoint_bub_ms &p, const ter_id &new_terrain, const furn_id &new_furniture );
        void set( const point_bub_ms &p, const ter_id &new_terrain, const furn_id &new_furniture ) {
            furn_set( p, new_furniture );
            ter_set( p, new_terrain );
        }
        std::string name( const tripoint_bub_ms &p );
        std::string name( const point_bub_ms &p ) {
            return name( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        std::string disp_name( const tripoint_bub_ms &p );




        std::string obstacle_name( const tripoint_bub_ms &p );
        bool has_furn( const tripoint_bub_ms &p ) const;
        bool has_furn( const point_bub_ms &p ) const {
            return has_furn( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        furn_id furn( tripoint_bub_ms p ) const;
        furn_id furn( const point_bub_ms &p ) const {
            return furn( tripoint_bub_ms( p, abs_sub.z() ) );
        }




        bool furn_set( const tripoint_bub_ms &p, const furn_id &new_furniture,
                       bool furn_reset = false, bool avoid_creatures = false, bool allow_on_open_air = false );
        bool furn_set( const point_bub_ms &p, const furn_id &new_furniture, bool avoid_creatures = false,
                       bool allow_on_open_air = false ) {
            return furn_set( tripoint_bub_ms( p, abs_sub.z() ), new_furniture, false, avoid_creatures,
                             allow_on_open_air );
        }
        void furn_clear( const tripoint_bub_ms &p ) {
            furn_set( p, furn_str_id( "f_clear" ) );
        };
        void furn_clear( const point_bub_ms &p ) {
            furn_clear( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        std::string furnname( const tripoint_bub_ms &p );
        bool can_move_furniture( const tripoint_bub_ms &pos, Character *you = nullptr ) const;


        ter_id ter( const tripoint_bub_ms &p ) const;
        ter_id ter( const point_bub_ms &p ) const {
            return ter( tripoint_bub_ms( p, abs_sub.z() ) );
        }

        void furniture_terrain_emit_fields();

        int get_map_damage( const tripoint_bub_ms &p ) const;
        void set_map_damage( const tripoint_bub_ms &p, int dmg );








        uint8_t get_known_connections( const tripoint_bub_ms &p,
                                       const std::bitset<NUM_TERCONN> &connect_group,
                                       const std::map<tripoint_bub_ms, ter_id> &override = {} ) const;

        uint8_t get_known_connections_f( const tripoint_bub_ms &p,
                                         const std::bitset<NUM_TERCONN> &connect_group,
                                         const std::map<tripoint_bub_ms, furn_id> &override = {} ) const;








        uint8_t get_known_rotates_to( const tripoint_bub_ms &p,
                                      const std::bitset<NUM_TERCONN> &rotate_to_group,
                                      const std::map<tripoint_bub_ms, ter_id> &override = {} ) const;

        uint8_t get_known_rotates_to_f( const tripoint_bub_ms &p,
                                        const std::bitset<NUM_TERCONN> &rotate_to_group,
                                        const std::map<tripoint_bub_ms, ter_id> &override = {},
                                        const std::map<tripoint_bub_ms, furn_id> &override_f = {} ) const;




        const harvest_id &get_harvest( const tripoint_bub_ms &p ) const;



        const std::set<std::string> &get_harvest_names( const tripoint_bub_ms &p ) const;
        ter_id get_ter_transforms_into( const tripoint_bub_ms &p ) const;

        bool ter_set( const tripoint_bub_ms &, const ter_id &new_terrain, bool avoid_creatures = false );
        bool ter_set( const point_bub_ms &p, const ter_id &new_terrain, bool avoid_creatures = false ) {
            return ter_set( tripoint_bub_ms( p, abs_sub.z() ), new_terrain, avoid_creatures );
        }

        void kill_creature( const tripoint_bub_ms &p, bool remove_corpse );

        std::string tername( const tripoint_bub_ms &p ) const;




        bool has_nearby_fire( const tripoint_bub_ms &p, int radius = 1 ) const;




        bool has_nearby_table( const tripoint_bub_ms &p, int radius = 1 ) const;



        bool has_nearby_chair( const tripoint_bub_ms &p, int radius = 1 ) const;



        bool has_nearby_ter( const tripoint_bub_ms &p, const ter_id &type, int radius = 1 ) const;








        bool sees_some_items( const tripoint_bub_ms &p, const Creature &who ) const;
        bool sees_some_items( const tripoint_bub_ms &p, const tripoint_bub_ms &from ) const;





        bool could_see_items( const tripoint_bub_ms &p, const Creature &who ) const;
        bool could_see_items( const tripoint_bub_ms &p, const tripoint_bub_ms &from ) const;



        bool has_items( const tripoint_bub_ms &p ) const;


        bool only_liquid_in_liquidcont( const tripoint_bub_ms &p );





        void examine( Character &you, const tripoint_bub_ms &pos ) const;




        bool is_harvestable( const tripoint_bub_ms &pos ) const;



        std::string features( const tripoint_bub_ms &p ) const;

        bool has_flag( const std::string &flag, const tripoint_bub_ms &p ) const;

        bool can_put_items( const tripoint_bub_ms &p ) const;

        bool can_put_items_ter_furn( const tripoint_bub_ms &p ) const;

        bool has_flag_ter( const std::string &flag, const tripoint_bub_ms &p ) const;
        bool has_flag_ter( const std::string &flag, const point_bub_ms &p ) const {
            return has_flag_ter( flag, tripoint_bub_ms( p, abs_sub.z() ) );
        }

        bool has_flag_furn( const std::string &flag, const tripoint_bub_ms &p ) const;
        bool has_flag_furn( const std::string &flag, const point_bub_ms &p ) const {
            return has_flag_furn( flag, tripoint_bub_ms( p, abs_sub.z() ) );
        }

        bool has_flag_ter_or_furn( const std::string &flag, const tripoint_bub_ms &p ) const;
        bool has_flag_ter_or_furn( const std::string &flag, const point_bub_ms &p ) const {
            return has_flag_ter_or_furn( flag, tripoint_bub_ms( p, abs_sub.z() ) );
        }


        bool has_flag( ter_furn_flag flag, const tripoint_bub_ms &p ) const;
        bool has_flag( ter_furn_flag flag, const point_bub_ms &p ) const {
            return has_flag( flag, tripoint_bub_ms( p, abs_sub.z() ) );
        }

        bool has_flag_ter( ter_furn_flag flag, const tripoint_bub_ms &p ) const;
        bool has_flag_ter( ter_furn_flag flag, const point_bub_ms &p ) const {
            return has_flag_ter( flag, tripoint_bub_ms( p, abs_sub.z() ) );
        }

        bool has_flag_furn( ter_furn_flag flag, const tripoint_bub_ms &p ) const;

        bool has_flag_ter_or_furn( ter_furn_flag flag, const tripoint_bub_ms &p ) const;



        bool on_matching_stairs( const tripoint_bub_ms &a, const tripoint_bub_ms &b ) const;



        bool is_bashable( const tripoint_bub_ms &p, bool allow_floor = false ) const;

        bool is_bashable_ter( const tripoint_bub_ms &p, bool allow_floor = false ) const;

        bool is_bashable_furn( const tripoint_bub_ms &p ) const;

        bool is_bashable_ter_furn( const tripoint_bub_ms &p, bool allow_floor = false ) const;

        int bash_strength( const tripoint_bub_ms &p, bool allow_floor = false ) const;

        int bash_resistance( const tripoint_bub_ms &p, bool allow_floor = false ) const;


        int bash_rating( const std::map<damage_type_id, int> &str, const tripoint_bub_ms &p,
                         bool allow_floor = false ) const;




        void make_rubble( const tripoint_bub_ms &p, const furn_id &rubble_type, bool items,
                          const ter_id &floor_type, bool overwrite = false );
        void make_rubble( const tripoint_bub_ms &p, const furn_id &rubble_type, bool items ) {
            make_rubble( p, rubble_type, items, ter_str_id( "t_dirt" ).id(), false );
        }
        void make_rubble( const tripoint_bub_ms &p ) {
            make_rubble( p, furn_str_id( "f_rubble" ), false, ter_str_id( "t_dirt" ).id(), false );
        }

        bool is_outside( const tripoint_bub_ms &p ) const;





        bool is_roofed( const tripoint_bub_ms &p ) const;






        bool is_divable( const tripoint_bub_ms &p ) const;
        bool is_water_shallow_current( const tripoint_bub_ms &p ) const;









        bool is_last_ter_wall( bool no_furn, const point_bub_ms &p,
                               const point_bub_ms &max, direction dir ) const;





        bool tinder_at( const tripoint_bub_ms &p );






        bool flammable_items_at( const tripoint_bub_ms &p, int threshold = 0 );

        bool is_flammable( const tripoint_bub_ms &p );
        point_bub_ms random_outdoor_tile() const;


        void draw_line_ter( const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z,
                            bool avoid_creature = false );
        void draw_line_furn( const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z,
                             bool avoid_creatures = false );
        void draw_fill_background( const ter_id &type );
        void draw_fill_background( ter_id( *f )() );
        void draw_fill_background( const weighted_int_list<ter_id> &f );

        void draw_square_ter( const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z,
                              bool avoid_creature = false );
        void draw_square_furn( const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z,
                               bool avoid_creatures = false );
        void draw_square_ter( ter_id( *f )(), const point_bub_ms &p1, const point_bub_ms &p2,
                              bool avoid_creatures = false );
        void draw_square_ter( const weighted_int_list<ter_id> &f, const point_bub_ms &p1,
                              const point_bub_ms &p2, bool avoid_creatures = false );
        void draw_rough_circle_ter( const ter_id &type, const point_bub_ms &p, int rad );
        void draw_rough_circle_furn( const furn_id &type, const point_bub_ms &p, int rad );
        void draw_circle_ter( const ter_id &type, const rl_vec2d &p, double rad );
        void draw_circle_ter( const ter_id &type, const point_bub_ms &p, int rad );
        void draw_circle_furn( const furn_id &type, const point_bub_ms &p, int rad );

        void add_corpse( const tripoint_bub_ms &p );



        void translate( const ter_id &from, const ter_id &to );


        void translate_radius( const ter_id &from, const ter_id &to, float radi, const tripoint_bub_ms &p,
                               bool same_submap = false, bool toggle_between = false );
        void transform_radius( const ter_furn_transform_id &transform, int radi,
                               const tripoint_abs_ms &p );
        void transform_line( const ter_furn_transform_id &transform, const tripoint_abs_ms &first,
                             const tripoint_abs_ms &second );
        bool close_door( const tripoint_bub_ms &p, bool inside, bool check_only );
        bool open_door( Creature const &u, const tripoint_bub_ms &p, bool inside, bool check_only = false );


        void batter( const tripoint_bub_ms &p, int power, int tries = 1, bool silent = false );

        void destroy( const tripoint_bub_ms &p, bool silent = false );

        void destroy_furn( const tripoint_bub_ms &, bool silent = false );

        void destroy_vehicle( const tripoint_bub_ms &, bool silent = false );
        void crush( const tripoint_bub_ms &p );
        double shoot( const tripoint_bub_ms &p, projectile &proj, bool hit_items );

        int collapse_check( const tripoint_bub_ms &p ) const;

        void collapse_at( const tripoint_bub_ms &p, bool silent, bool was_supporting = false,
                          bool destroy_pos = true );


        void smash_items( const tripoint_bub_ms &p, int power, const std::string &cause_message,
                          vehicle_part *vp_wheel = nullptr, vehicle *veh = nullptr );










        bash_params bash( const tripoint_bub_ms &p, const std::map<damage_type_id, int> &str,
                          bool silent = false, bool destroy = false, bool bash_floor = false,
                          const vehicle *bashing_vehicle = nullptr,
                          bool repair_missing_ground = true );
        bash_params bash( const tripoint_bub_ms &p, int str,
                          bool silent = false, bool destroy = false, bool bash_floor = false,
                          const vehicle *bashing_vehicle = nullptr,
                          bool repair_missing_ground = true );


        bool hit_with_acid( const tripoint_bub_ms &p );
        bool hit_with_fire( const tripoint_bub_ms &p );







        bool has_adjacent_furniture_with( const tripoint_bub_ms &p,
                                          const std::function<bool( const furn_t & )> &filter ) const;





        bool terrain_moppable( const tripoint_bub_ms &p );





        bool mop_spills( const tripoint_bub_ms &p );




        void decay_fields_and_scent( const time_duration &amount );


        std::string get_signage( const tripoint_bub_ms &p ) const;
        void set_signage( const tripoint_bub_ms &p, const std::string &message );
        void delete_signage( const tripoint_bub_ms &p );


        int get_radiation( const tripoint_bub_ms &p ) const;
        void set_radiation( const tripoint_bub_ms &p, int value );
        void set_radiation( const point_bub_ms &p, const int value ) {
            set_radiation( tripoint_bub_ms( p, abs_sub.z() ), value );
        }




        void adjust_radiation( const tripoint_bub_ms &p, int delta );
        void adjust_radiation( const point_bub_ms &p, const int delta ) {
            adjust_radiation( tripoint_bub_ms( p, abs_sub.z() ), delta );
        }


        units::temperature_delta get_temperature_mod( const tripoint_bub_ms &p ) const;

        void set_temperature_mod( const tripoint_bub_ms &p, units::temperature_delta temperature_mod );
        void set_temperature_mod( const point_bub_ms &p, units::temperature_delta new_temperature_mod ) {
            set_temperature_mod( tripoint_bub_ms( p, abs_sub.z() ), new_temperature_mod );
        }



        void check_submap_active_item_consistency();






        std::unordered_set<item_location> all_items( Character &who,
                accessor_flags flags = Access_EVERYTHING );
        std::unordered_set<item_location> all_items( const std::function<bool( const item & )> &filter,
                Character &who, accessor_flags flags = Access_EVERYTHING );

        map_stack i_at( const tripoint_bub_ms &p );
        map_stack i_at( const point_bub_ms &p ) {
            return i_at( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        item liquid_from( const tripoint_bub_ms &p ) const;
        void i_clear( const tripoint_bub_ms &p );
        void i_clear( const point_bub_ms &p ) {
            i_clear( tripoint_bub_ms( p, abs_sub.z() ) );
        }


        map_stack::iterator i_rem( const tripoint_bub_ms &p, const map_stack::const_iterator &it );
        void i_rem( const tripoint_bub_ms &p, item *it );
        void spawn_artifact( const tripoint_bub_ms &p, const relic_procgen_id &id, int max_attributes = 5,
                             int power_level = 1000, int max_negative_power = -2000, bool is_resonant = false );
        void spawn_item( const tripoint_bub_ms &p, const itype_id &type_id,
                         unsigned quantity = 1, int charges = 0,
                         const time_point &birthday = calendar::start_of_cataclysm, int damlevel = 0,
                         const std::set<flag_id> &flags = {}, const std::string &variant = "",
                         const std::string &faction = "" );
        void spawn_item( const point_bub_ms &p, const itype_id &type_id,
                         unsigned quantity = 1, int charges = 0,
                         const time_point &birthday = calendar::start_of_cataclysm, int damlevel = 0,
                         const std::set<flag_id> &flags = {}, const std::string &variant = "",
                         const std::string &faction = "" ) {
            spawn_item( tripoint_bub_ms( p, abs_sub.z() ), type_id, quantity, charges, birthday, damlevel,
                        flags,
                        variant, faction );
        }

        units::volume max_volume( const tripoint_bub_ms &p );
        units::volume free_volume( const tripoint_bub_ms &p );
        units::volume stored_volume( const tripoint_bub_ms &p );










        item_location add_item_or_charges_ret_loc( const tripoint_bub_ms &pos, item obj,
                bool overflow = true );
        item &add_item_or_charges( const tripoint_bub_ms &pos, item obj, bool overflow = true );
        item &add_item_or_charges( const tripoint_bub_ms &pos, item obj, int &copies_remaining,
                                   bool overflow = true );





        float item_category_spawn_rate( const item &itm );








        item &add_item( const tripoint_bub_ms &p, item new_item, int copies );
        item &add_item( const tripoint_bub_ms &p, item new_item );





        void make_active( item_location &loc );
        void make_active( tripoint_abs_sm const &loc );




        void update_lum( item_location &loc, bool add );














        std::list<item> use_amount_square( const tripoint_bub_ms &p, const itype_id &type, int &quantity,
                                           const std::function<bool( const item & )> &filter = return_true<item> );
        std::list<item> use_amount( const tripoint_bub_ms &origin, int range, const itype_id &type,
                                    int &quantity,
                                    const std::function<bool( const item & )> &filter = return_true<item>, bool select_ind = false );
        std::list<item> use_amount( const std::vector<tripoint_bub_ms> &reachable_pts, const itype_id &type,
                                    int &quantity,
                                    const std::function<bool( const item & )> &filter = return_true<item>, bool select_ind = false );
        std::list<item> use_charges( const tripoint_bub_ms &origin, int range, const itype_id &type,
                                     int &quantity, const std::function<bool( const item & )> &filter = return_true<item>,
                                     basecamp *bcp = nullptr, bool in_tools = false );
        std::list<item> use_charges( const std::vector<tripoint_bub_ms> &reachable_pts,
                                     const itype_id &type,
                                     int &quantity,
                                     const std::function<bool( const item & )> &filter = return_true<item>,
                                     basecamp *bcp = nullptr, bool in_tools = false );


        std::list<item_location> items_with( const tripoint_bub_ms &p,
                                             const std::function<bool( const item & )> &filter );








        units::energy consume_ups( const std::vector<tripoint_bub_ms> &reachable_pts, units::energy qty );
        units::energy consume_ups( const tripoint_bub_ms &origin, int range, units::energy qty );


        std::list<std::pair<tripoint_bub_ms, item *> > get_rc_items( const tripoint_bub_ms &p = { -1, -1, -1 } );


















        std::vector<item *> place_items(
            const item_group_id &group_id, int chance,
            const tripoint_bub_ms &p1, const tripoint_bub_ms &p2,
            bool ongrass, const time_point &turn, int magazine = 0, int ammo = 0,
            const std::string &faction = "" );
        std::vector<item *> place_items(
            const item_group_id &group_id, int chance, const point_bub_ms &p1, const point_bub_ms &p2,
            const int z_level,
            bool ongrass, const time_point &turn, int magazine = 0, int ammo = 0,
            const std::string &faction = "" ) {
            return place_items( group_id, chance, tripoint_bub_ms( p1, z_level ),
                                tripoint_bub_ms( p2, z_level ), ongrass, turn, magazine, ammo, faction );
        }








        std::vector<item *> put_items_from_loc(
            const item_group_id &group_id, const tripoint_bub_ms &p,
            const time_point &turn = calendar::start_of_cataclysm );


        std::vector<item *> spawn_items( const tripoint_bub_ms &p,
                                         const std::vector<item> &new_items );

        void create_anomaly( const tripoint_bub_ms &p, artifact_natural_property prop,
                             bool create_rubble = true );
        void create_anomaly( const point_bub_ms &cp, artifact_natural_property prop,
                             bool create_rubble = true ) {
            create_anomaly( tripoint_bub_ms( cp, abs_sub.z() ), prop, create_rubble );
        }


        void partial_con_set( const tripoint_bub_ms &p, const partial_con &con );
        void partial_con_remove( const tripoint_bub_ms &p );
        void partial_con_remove_no_vision_for_testing( const tripoint_bub_ms &p );
        partial_con *partial_con_at( const tripoint_bub_ms &p );

        void trap_set( const tripoint_bub_ms &p, const trap_id &type );

    private:
        void partial_con_remove_impl( const tripoint_bub_ms &p );
    public:

        const trap &tr_at( const tripoint_abs_ms &p ) const;
        const trap &tr_at( const tripoint_bub_ms &p ) const;

        bool can_see_trap_at( const tripoint_bub_ms &p, const Character &c ) const;

        void remove_trap( const tripoint_bub_ms &p );
        const std::vector<tripoint_bub_ms> &get_furn_field_locations() const;
        const std::vector<tripoint_bub_ms> &get_ter_field_locations() const;
        const std::vector<tripoint_bub_ms> &trap_locations( const trap_id &type ) const;








        void maybe_trigger_trap( const tripoint_bub_ms &pos, Creature &c, bool may_avoid ) const;

        void maybe_trigger_prox_trap( const tripoint_bub_ms &pos, Creature &c, bool may_avoid ) const;


        void create_burnproducts( const tripoint_bub_ms &p, const item &fuel,
                                  const units::mass &burned_mass );

        void process_fields();
        void process_fields_in_submap( submap *current_submap, const tripoint_bub_sm &submap_pos );



        void creature_in_field( Creature &critter );









        void creature_on_trap( Creature &critter, bool may_avoid = true ) const;






        const field &field_at( const tripoint_bub_ms &p ) const;



        field &field_at( const tripoint_bub_ms &p );




        time_duration get_field_age( const tripoint_bub_ms &p, const field_type_id &type ) const;




        int get_field_intensity( const tripoint_bub_ms &p, const field_type_id &type ) const;




        time_duration mod_field_age( const tripoint_bub_ms &p, const field_type_id &type,
                                     const time_duration &offset );





        int mod_field_intensity( const tripoint_bub_ms &p, const field_type_id &type, int offset );









        time_duration set_field_age( const tripoint_bub_ms &p, const field_type_id &type,
                                     const time_duration &age, bool isoffset = false );










        int set_field_intensity( const tripoint_bub_ms &p, const field_type_id &type, int new_intensity,
                                 bool isoffset = false );



        bool has_field_at( const tripoint_bub_ms &p, bool check_bounds = true ) const;
        bool has_field_at( const tripoint_bub_ms &p, const field_type_id &type ) const;


        std::optional<field_entry> get_impassable_field_at( const tripoint_bub_ms &p );
        std::vector<field_type_id> get_impassable_field_type_ids_at( const tripoint_bub_ms &p );





        field_entry *get_field( const tripoint_bub_ms &p, const field_type_id &type );
        const field_entry *get_field( const tripoint_bub_ms &p, const field_type_id &type ) const;
        bool dangerous_field_at( const tripoint_bub_ms &p );
        bool impassable_field_at( const tripoint_bub_ms &p );


        bool mopsafe_field_at( const tripoint_bub_ms &p );





        bool add_field(
            const tripoint_bub_ms &p, const field_type_id &type_id, int intensity = INT_MAX,
            const time_duration &age = 0_turns, bool hit_player = true,
            effect_source source = effect_source() );



        void remove_field( const tripoint_bub_ms &p, const field_type_id &field_to_remove );






        void delete_field( const tripoint_bub_ms &p, const field_type_id &field_to_remove );



        void clear_fields( const tripoint_bub_ms &p );




        const field_type_str_id &get_applicable_electricity_field( const tripoint_bub_ms &p ) const;

    private:


        void on_field_modified( const tripoint_bub_ms &p, const field_type &fd_type );

        template<typename Map>
        static cata::copy_const<Map, field_entry> *get_field_helper(
            Map &m, const tripoint_bub_ms &p, const field_type_id &type );

        std::pair<item *, tripoint_bub_ms> _add_item_or_charges( const tripoint_bub_ms &pos, item obj,
                int &copies_remaining, bool overflow = true );
    public:


        void add_splatter( const field_type_id &type, const tripoint_bub_ms &where, int intensity = 1 );
        void add_splatter_trail( const field_type_id &type, const tripoint_bub_ms &from,
                                 const tripoint_bub_ms &to );
        void add_splash( const field_type_id &type, const tripoint_bub_ms &center, int radius,
                         int intensity );

        void propagate_field( const tripoint_bub_ms &center, const field_type_id &type,
                              int amount, int max_intensity = 0 );







        void emit_field( const tripoint_bub_ms &pos, const emit_id &src, float mul = 1.0f );






        void scent_blockers( std::array<std::array<bool, MAPSIZE_X>, MAPSIZE_Y> &blocks_scent,
                             std::array<std::array<bool, MAPSIZE_X>, MAPSIZE_Y> &reduces_scent,
                             const point_bub_ms &min, const point_bub_ms &max );


        computer *computer_at( const tripoint_bub_ms &p );
        computer *add_computer( const tripoint_bub_ms &p, const std::string &name, int security );


        void add_camp( const tripoint_abs_omt &omt_pos, const std::string &name,
                       bool need_validate = true );
        void remove_submap_camp( const tripoint_bub_ms & );
        basecamp hoist_submap_camp( const tripoint_bub_ms &p );

        bool has_graffiti_at( const tripoint_bub_ms &p ) const;
        const std::string &graffiti_at( const tripoint_bub_ms &p ) const;
        void set_graffiti( const tripoint_bub_ms &p, const std::string &contents );
        void delete_graffiti( const tripoint_bub_ms &p );






        int climb_difficulty( const tripoint_bub_ms &p ) const;


    private:

        std::set<tripoint_bub_ms> support_cache_dirty;

        void support_dirty( const tripoint_bub_ms &p );
    public:




        bool has_floor( const tripoint_bub_ms &p ) const;
        bool has_floor_or_water( const tripoint_bub_ms &p ) const;

        bool supports_above( const tripoint_bub_ms &p ) const;
        bool has_floor_or_support( const tripoint_bub_ms &p ) const;
        bool has_vehicle_floor( const tripoint_bub_ms &p ) const;
    private:




        void drop_everything( const tripoint_bub_ms &p );
        void drop_furniture( const tripoint_bub_ms &p );
        void drop_items( const tripoint_bub_ms &p );
        void drop_vehicle( const tripoint_bub_ms &p );
        void drop_fields( const tripoint_bub_ms &p );
        void drop_creature( const tripoint_bub_ms &p );

    public:



        void process_falling();

        bool is_cornerfloor( const tripoint_bub_ms &p ) const;




        void generate( const tripoint_abs_omt &p, const time_point &when, bool save_results,
                       bool run_post_process = true );
        // Used when contents has been generated by 'generate' with save_results = false to dispose of

        void delete_unmerged_submaps();
        void place_spawns( const mongroup_id &group, int chance,
                           const point_bub_ms &p1, const point_bub_ms &p2, int z_level, float density,
                           bool individual = false, bool friendly = false,
                           const std::optional<std::string> &name = std::nullopt,
                           int mission_id = -1 );
        void place_gas_pump( const tripoint_bub_ms &p, int charges, const itype_id &fuel_type );
        void place_gas_pump( const tripoint_bub_ms &p, int charges );

        void place_toilet( const tripoint_bub_ms &p, int charges = 6 * 4 );
        void place_vending( const tripoint_bub_ms &p, const item_group_id &type, bool reinforced = false,
                            bool lootable = false, bool powered = false, bool networked = false );

        character_id place_npc( const point_bub_ms &p, const string_id<npc_template> &type );
        void apply_faction_ownership( const point_bub_ms &p1, const point_bub_ms &p2,
                                      const faction_id &id );
        void do_vehicle_caching( int z );


        void build_map_cache( int zlev, bool skip_lightmap = false );


        void rebuild_vision_caches_from_scratch( int zlev );



        const std::vector<int> &vision_levels() const;

        void build_obstacle_cache(
            const tripoint_bub_ms &start, const tripoint_bub_ms &end,
            cata::mdarray<fragment_cloud, point_bub_ms> &obstacle_cache );

















        vehicle *add_vehicle( const vproto_id &type, const tripoint_bub_ms &p, const units::angle &dir,
                              int init_veh_fuel = -1, veh_spawn_status init_veh_status = veh_spawn_status::DEFAULT_LIGHT_DMG,
                              bool merge_wrecks = true,
                              bool force_status = false );


        float light_transparency( const tripoint_bub_ms &p ) const;

        lit_level light_at( const tripoint_bub_ms &p ) const;

        float ambient_light_at( const tripoint_bub_ms &p ) const;




        bool is_transparent( const tripoint_bub_ms &p ) const;
        bool is_transparent_wo_fields( const tripoint_bub_ms &p ) const;

        bool is_sight_clear( const tripoint_bub_ms &p ) const;
        bool is_sight_clear_wo_fields( const tripoint_bub_ms &p ) const;










        bool pl_sees( const tripoint_bub_ms &t, int max_range ) const;

        std::set<vehicle *> dirty_vehicle_list;


        tripoint_abs_sm get_abs_sub() const;





        tripoint_abs_ms get_abs( const tripoint_bub_ms &p ) const;



        tripoint_bub_ms get_bub( const tripoint_abs_ms &p ) const;
        point_bub_ms get_bub( const point_abs_ms &p ) const {
            return get_bub( tripoint_abs_ms( p, abs_sub.z() ) ).xy();
        }
        bool inbounds( const tripoint_bub_ms &p ) const;
        bool inbounds( const tripoint_abs_ms &p ) const;
        bool inbounds( const tripoint_abs_sm &p ) const {
            return inbounds( project_to<coords::omt>( p ) );
        }
        bool inbounds( const tripoint_abs_omt &p ) const;
        bool inbounds( const point_bub_ms &p ) const {
            return inbounds( tripoint_bub_ms( p, 0 ) );
        }

        bool inbounds_z( const int z ) const {
            return z >= -OVERMAP_DEPTH && z <= OVERMAP_HEIGHT;
        }


        void clip_to_bounds( tripoint_bub_ms &p ) const;
        void clip_to_bounds( int &x, int &y ) const;
        void clip_to_bounds( int &x, int &y, int &z ) const;

        int getmapsize() const {
            return my_MAPSIZE;
        }






        void rotate( int turns );












        void mirror( bool mirror_horizontal, bool mirror_vertical );









        void spawn_monsters( bool ignore_sight, bool spawn_nonlocal = false );






        void handle_decayed_corpse( const item &it, const tripoint_abs_ms &pnt );






        void rotten_item_spawn( const item &item, const tripoint_bub_ms &p );
    private:
        void add_spawn( const mtype_id &type, int count, const tripoint_bub_ms &p,
                        bool friendly = false, int faction_id = -1, int mission_id = -1,
                        const std::optional<std::string> &name = std::nullopt );
        void add_spawn( const mtype_id &type, int count, const tripoint_bub_ms &p, bool friendly,
                        int faction_id, int mission_id, const std::optional<std::string> &name,
                        const spawn_data &data );
        void add_spawn( const MonsterGroupResult &spawn_details, const tripoint_bub_ms &p );


        void spawn_monsters_submap( const tripoint_rel_sm &gp, bool ignore_sight,
                                    bool spawn_nonlocal = false );

        void spawn_monsters_submap_group( const tripoint_rel_sm &gp, mongroup &group,
                                          bool ignore_sight );

    protected:
        void saven( const tripoint_bub_sm &grid );
        void loadn( const point_bub_sm &grid, bool update_vehicles );





        enum class reconcile_scope : uint8_t {
            full_rebuild,
            additive
        };







        void reconcile_loaded_items( reconcile_scope scope );




        void actualize( const tripoint_rel_sm &grid );





        void add_tree_tops( const tripoint_rel_sm &grid );




        void fill_funnels( const tripoint_bub_ms &p, const time_point &since );



        void grow_plant( const tripoint_bub_ms &p );






        void restock_fruits( const tripoint_bub_ms &p, const time_duration &time_since_last_actualize );






        void produce_sap( const tripoint_bub_ms &p, const time_duration &time_since_last_actualize );
    public:




        void cut_down_tree( tripoint_bub_ms p, point_rel_ms dir );

        void maybe_apply_field_effect( const std::vector<field_effect> &vfe, Creature &critter ) const;
    protected:



        void rad_scorch( const tripoint_bub_ms &p, const time_duration &time_since_last_actualize );
        void decay_cosmetic_fields( const tripoint_bub_ms &p,
                                    const time_duration &time_since_last_actualize );

        void player_in_field( Character &you );
        void monster_in_field( monster &z );




        void shift_traps( const point_rel_sm &shift );

        void on_unload( const tripoint_rel_sm &loc );
        void copy_grid( const tripoint_rel_sm &to, const tripoint_rel_sm &from );
        void draw_map( mapgendata &dat );



        bool build_transparency_cache( int zlev );
        bool build_vision_transparency_cache( int zlev );


        std::vector<std::pair<point_bub_ms, float>> observer_vision_overrides( int zlev ) const;


        void build_sunlight_cache();

        void update_sunlight();




        void ensure_light( int zlev ) const;

        void set_sunlight_only( int zlev ) const;
        void refresh_vision_levels();

        int lowest_vision_level( int zlev ) const;

        bool level_reached_by_cast( int zlev ) const;
    public:
        void build_outside_cache( int zlev );

        std::bitset<OVERMAP_LAYERS> get_inter_level_visibility( int origin_zlevel )const ;


        void build_floor_cache( int zlev );

        void build_floor_caches();


        void seen_cache_process_ledges( array_of_grids_of<float> &seen_caches,
                                        const array_of_grids_of<const bool> &floor_caches,
                                        const tripoint_bub_ms &origin, float eye_level = 1.0f ) const;

    protected:
        void generate_lightmap( int zlev );


        void build_seen_cache( const tripoint_bub_ms &origin, int target_z, int extension_range,
                               bool camera, int penalty, float eye_level );
        void apply_character_light( Character &p );

        int my_MAPSIZE;
        int my_HALF_MAPSIZE;
        bool zlevels;









        tripoint_abs_sm abs_sub;

        point_abs_ms abs_ms;



        void set_abs_sub( const tripoint_abs_sm &p );

        field &get_field( const tripoint_bub_ms &p );




        submap *getsubmap( size_t grididx );
        const submap *getsubmap( size_t grididx ) const;




        submap *unsafe_get_submap_at( const tripoint_bub_ms &p );
        const submap *unsafe_get_submap_at( const tripoint_bub_ms &p ) const;
        submap *get_submap_at( const tripoint_bub_ms &p );
        const submap *get_submap_at( const tripoint_bub_ms &p ) const;





        submap *unsafe_get_submap_at( const tripoint_bub_ms p, point_sm_ms &offset_p ) {
            tripoint_bub_sm sm;
            std::tie( sm, offset_p ) = project_remain<coords::sm>( p );
            return unsafe_get_submap_at( p );
        }
        const submap *unsafe_get_submap_at(
            const tripoint_bub_ms p, point_sm_ms &offset_p ) const {
            tripoint_bub_sm sm;
            std::tie( sm, offset_p ) = project_remain<coords::sm>( p );
            return unsafe_get_submap_at( p );
        }
        submap *get_submap_at( const tripoint_bub_ms &p, point_sm_ms &offset_p ) {
            offset_p.x() = p.x() % SEEX;
            offset_p.y() = p.y() % SEEY;
            return get_submap_at( p );
        }
        const submap *get_submap_at( const tripoint_bub_ms &p, point_sm_ms &offset_p ) const {
            offset_p.x() = p.x() % SEEX;
            offset_p.y() = p.y() % SEEY;
            return get_submap_at( p );
        }





        submap *get_submap_at_grid( const point_rel_sm &gridp ) {
            return getsubmap( get_nonant( gridp ) );
        }
        submap *get_submap_at_grid( const tripoint_rel_sm &gridp );
        const submap *get_submap_at_grid( const tripoint_rel_sm &gridp ) const;





        size_t get_nonant( const tripoint_rel_sm &gridp ) const;
        size_t get_nonant( const point_rel_sm &gridp ) const {
            return get_nonant( tripoint_rel_sm{ gridp.x(), gridp.y(), abs_sub.z()} );
        }





        void setsubmap( size_t grididx, submap *smap );
    private:




        int calc_max_populated_zlev();





        void invalidate_max_populated_zlev( int zlev );





        int move_cost_internal( const furn_t &furniture, const ter_t &terrain, const field &field,
                                const vehicle *veh, int vpart ) const;
        int bash_rating_internal( const std::map<damage_type_id, int> &str,
                                  const furn_t &furniture, const ter_t &terrain,
                                  bool allow_floor, const vehicle *veh, int part ) const;





        bool draw_maptile( const catacurses::window &w, const tripoint_bub_ms &p,
                           const const_maptile &tile, const drawsq_params &params ) const;



        void draw_from_above( const catacurses::window &w, const tripoint_bub_ms &p,
                              const const_maptile &tile, const drawsq_params &params ) const;

        int determine_wall_corner( const tripoint_bub_ms &p ) const;

        void apply_light_source( const tripoint_bub_ms &p, float luminance );



        void add_light_source( const tripoint_bub_ms &p, float luminance,
                               const light_color_rgb &color = {} );

        void apply_directional_light( const tripoint_bub_ms &p, int direction, float luminance,
                                      const light_color_rgb &color = {} );
        void apply_light_arc( const tripoint_bub_ms &p, const units::angle &angle, float luminance,
                              const units::angle &wideangle = 30_degrees,
                              const light_color_rgb &color = {} );
        void apply_light_ray( cata::mdarray<bool, point_bub_ms, MAPSIZE_X, MAPSIZE_Y> &lit,
                              const tripoint_bub_ms &s, const tripoint_bub_ms &e, float luminance );
        void add_light_from_items( const tripoint_bub_ms &p, const item_stack &items );
        void add_item_light_recursive( const tripoint_bub_ms &p, const item &it );
        std::unique_ptr<vehicle> add_vehicle_to_map( std::unique_ptr<vehicle> veh, bool merge_wrecks );



        void bash_ter_furn( const tripoint_bub_ms &p, bash_params &params,
                            bool repair_missing_ground = true );
        void bash_items( const tripoint_bub_ms &p, bash_params &params );
        void bash_vehicle( const tripoint_bub_ms &p, bash_params &params );
        void bash_field( const tripoint_bub_ms &p, bash_params &params );




        ter_str_id get_roof( const tripoint_bub_ms &p, bool allow_air ) const;

    public:


        void drop_bash_results( const map_data_common_t &ter_furn, const tripoint_bub_ms &p );

        void process_items();

        std::vector<item_reference> item_network_connections( vehicle *power_grid );
    private:

        void process_items_in_submap( submap &current_submap, const tripoint_rel_sm &gridp );
        void process_items_in_vehicles( submap &current_submap );
        void process_items_in_vehicle( vehicle &cur_veh, submap &current_submap );


        enum iteration_state {
            ITER_CONTINUE = 0,
            ITER_SKIP_SUBMAP,
            ITER_SKIP_ZLEVEL,
            ITER_FINISH
        };










        template<typename Functor>
        void function_over( const tripoint_bub_ms &start, const tripoint_bub_ms &end, Functor fun ) const;







        std::vector<submap *> grid;






        std::vector< std::vector<tripoint_bub_ms> > traplocs;



        std::vector<tripoint_bub_ms> field_furn_locs;



        std::vector<tripoint_bub_ms> field_ter_locs;



        static std::list<std::unique_ptr<level_cache>> free_cache_pool;
        struct level_cache_free {
            void operator()( level_cache *cache ) {
                free_cache_pool.emplace_back( cache );
            }
        };



        mutable std::array< std::unique_ptr<level_cache, level_cache_free>, OVERMAP_LAYERS > caches;

        mutable std::array< std::unique_ptr<pathfinding_cache>, OVERMAP_LAYERS > pathfinding_caches;



        std::set<tripoint_abs_sm> submaps_with_active_items;
        std::set<tripoint_abs_sm> submaps_with_active_items_dirty;




        using lru_cache_t = lru_cache<point, char>;

        mutable std::array<lru_cache_t, 4> skew_vision_caches;

        mutable uint64_t skew_vision_scene_stamp = 0;
        lru_cache_t &skew_vision_cache_for( los_trace trace, bool with_fields ) const;


        uint64_t last_scene_change = 0;


        bool scene_build_pending = true;

        void build_scene_caches();



        void ensure_scene_caches() const;


        std::vector<int> vision_parts_key( const tripoint_bub_ms &origin, int extension_range ) const;


        level_cache &get_cache( int zlev ) const {
            std::unique_ptr<level_cache, level_cache_free> &cache = caches[zlev + OVERMAP_DEPTH];
            if( !cache ) {
                cache = alloc_cache();
            }
            return *cache;
        }

        static std::unique_ptr<level_cache, level_cache_free> alloc_cache();

        level_cache *get_cache_lazy( int zlev ) const {
            return caches[zlev + OVERMAP_DEPTH].get();
        }

        pathfinding_cache &get_pathfinding_cache( int zlev ) const;

        visibility_variables visibility_variables_cache;
        uint64_t seen_cache_generation = 0;

        std::bitset<OVERMAP_LAYERS> camera_levels_written;


        mutable std::array<int8_t, OVERMAP_LAYERS> level_reached = {};
        mutable uint64_t level_reached_generation = 0;

        tripoint_abs_ms avatar_fov_pos;
        int avatar_fov_range = -1;
        float avatar_fov_eye_level = -1.0f;
        std::vector<int> avatar_fov_vision_parts;

        struct camera_fov_input {
            const monster *mon;
            tripoint_abs_ms pos;
            int range;
            int size;
            bool operator==( const camera_fov_input &o ) const {
                return mon == o.mon && pos == o.pos && range == o.range && size == o.size;
            }
        };
        std::vector<camera_fov_input> camera_fov_moncams;

        std::vector<float> sunlight_natural_light;
        std::vector<uint64_t> sunlight_scene_revisions;

        std::array<visibility_inputs, OVERMAP_LAYERS> visibility_keys;

        visibility_inputs visibility_variables_inputs;

        uint64_t visibility_force_generation = 0;

        std::vector<int> vision_levels_list;

        struct light_source_state {
            float light;
            tripoint_bub_ms pos;

            int dir;
            bool operator==( const light_source_state &o ) const {
                return light == o.light && pos == o.pos && dir == o.dir;
            }
        };
        std::vector<light_source_state> cached_light_sources;



        std::optional<std::pair<tripoint_abs_sm, int>> max_populated_zlev = std::nullopt;

        bool mapgen_in_progress = false;

        bool _main_requires_cleanup = false;
        std::optional<bool> _main_cleanup_override = std::nullopt;

    public:
        int supports_zlevels() const {
            return zlevels;
        }
        void queue_main_cleanup();
        bool is_main_cleanup_queued() const;
        void main_cleanup_override( bool over );
        const level_cache &get_cache_ref( int zlev ) const {
            return get_cache( zlev );
        }

        const pathfinding_cache &get_pathfinding_cache_ref( int zlev ) const;

        void update_pathfinding_cache( const tripoint_bub_ms &p ) const;
        void update_pathfinding_cache( int zlev ) const;

        void update_visibility_cache( int zlev );

        bool level_reads_as_blank( int zlev, int clairvoyance ) const;
        void invalidate_visibility_cache();
        const visibility_variables &get_visibility_variables_cache() const;

        uint64_t seen_generation() const {
            return seen_cache_generation;
        }

        void update_submaps_with_active_items();


        const std::set<tripoint_abs_sm> &get_submaps_with_active_items() const {
            return submaps_with_active_items;
        }

        tripoint_range<tripoint_bub_ms> points_in_rectangle(
            const tripoint_bub_ms &from, const tripoint_bub_ms &to ) const;

        tripoint_range<tripoint_bub_ms> points_in_radius(
            const tripoint_bub_ms &center, size_t radius, size_t radiusz = 0 ) const;





        tripoint_range<tripoint_bub_ms> points_on_zlevel() const;


        tripoint_range<tripoint_bub_ms> points_on_zlevel( int z ) const;

        std::list<item_location> get_active_items_in_radius( const tripoint_bub_ms &center, int radius );
        std::list<item_location> get_active_items_in_radius( const tripoint_bub_ms &center, int radius,
                special_item_type type );


        std::list<tripoint_bub_ms> find_furnitures_with_flag_in_radius( const tripoint_bub_ms &center,
                size_t radius,
                const std::string &flag,
                size_t radiusz = 0 ) const;

        std::list<tripoint_bub_ms> find_furnitures_with_flag_in_radius( const tripoint_bub_ms &center,
                size_t radius,
                ter_furn_flag flag,
                size_t radiusz = 0 ) const;

        std::list<Creature *> get_creatures_in_radius( const tripoint_bub_ms &center, size_t radius,
                size_t radiusz = 0 ) const;

        std::list<Creature *> get_creatures_in_radius_circ( const tripoint_bub_ms &center, size_t radius,
                size_t radiusz = 0 ) const;

        level_cache &access_cache( int zlev );
        const level_cache &access_cache( int zlev ) const;
        bool dont_draw_lower_floor( const tripoint_bub_ms &p ) const;

        bool has_haulable_items( const tripoint_bub_ms &pos );
        std::vector<item_location> get_haulable_items( const tripoint_bub_ms &pos );

#if defined(TILES)
        bool draw_points_cache_dirty = true;
        std::map<int, std::map<int, std::vector<tile_render_info>>> draw_points_cache;
        point_rel_ms prev_top_left;
        point_rel_ms prev_bottom_right;
        point prev_o;

        tripoint_abs_ms prev_memory_sweep_origin = tripoint_abs_ms::invalid;
        std::multimap<point, formatted_text> overlay_strings_cache;
        color_block_overlay_container color_blocks_cache;
#endif
};





map &get_map();









map &reality_bubble();

template<int SIZE, int MULTIPLIER>
void shift_bitset_cache( std::bitset<SIZE *SIZE> &cache, const point_rel_sm &s );

bool ter_furn_has_flag( const ter_t &ter, const furn_t &furn, ter_furn_flag flag );
bool generate_uniform( const tripoint_abs_sm &p, const ter_str_id &ter );
bool generate_uniform_omt( const tripoint_abs_sm &p, const oter_id &terrain_type );








class tinymap : private map
{
        friend class editmap_ui;
    protected:
        tinymap( int mapsize, bool zlev ) : map( mapsize, zlev ) {};


        void cut_down_tree( tripoint_omt_ms p, point_rel_ms dir ) {
            map::cut_down_tree( rebase_bub( p ), dir );
        };

    public:
        tinymap() : map( 2, false ) {}
        bool inbounds( const tripoint_omt_ms &p ) const {
            return map::inbounds( rebase_bub( p ) );
        }

        map *cast_to_map() {
            return this;
        }

        using map::save;
        void load( const tripoint_abs_omt &w, bool update_vehicles,
                   bool pump_events = false ) {
            map::load( project_to<coords::sm>( w ), update_vehicles, pump_events );
        };

        using map::is_main_cleanup_queued;
        using map::main_cleanup_override;
        using map::generate;
        using map::delete_unmerged_submaps;
        void place_spawns( const mongroup_id &group, int chance,
                           const point_omt_ms &p1, const point_omt_ms &p2, const int z_level, float density,
                           bool individual = false, bool friendly = false,
                           const std::optional<std::string> &name = std::nullopt,
                           int mission_id = -1 ) {
            map::place_spawns( group, chance, rebase_bub( p1 ), rebase_bub( p2 ), z_level, density, individual,
                               friendly,
                               name, mission_id );
        }
        using map::translate;
        ter_id ter( const tripoint_omt_ms &p ) const {
            return map::ter( rebase_bub( p ) );
        }
        bool ter_set( const tripoint_omt_ms &p, const ter_id &new_terrain, bool avoid_creatures = false ) {
            return map::ter_set( rebase_bub( p ), new_terrain, avoid_creatures );
        }
        bool has_flag_ter( ter_furn_flag flag, const tripoint_omt_ms &p ) const {
            return map::has_flag_ter( flag, rebase_bub( p ) );
        }
        void draw_line_ter( const ter_id &type, const point_omt_ms &p1, const point_omt_ms &p2,
                            bool avoid_creature = false ) {
            map::draw_line_ter( type, rebase_bub( p1 ), rebase_bub( p2 ), avoid_creature );
        }
        bool is_last_ter_wall( bool no_furn, const point_omt_ms &p,
                               const point_omt_ms &max, direction dir ) const {
            return map::is_last_ter_wall( no_furn, rebase_bub( p ), rebase_bub( max ), dir );
        }
        furn_id furn( const point_omt_ms &p ) const {
            return map::furn( rebase_bub( p ) );
        }
        furn_id furn( const tripoint_omt_ms &p ) const {
            return map::furn( rebase_bub( p ) );
        }
        bool has_furn( const tripoint_omt_ms &p ) const {
            return map::has_furn( rebase_bub( p ) );
        }
        bool has_furn( const point_omt_ms &p ) const {
            return map::has_furn( rebase_bub( p ) );
        }
        void set( const tripoint_omt_ms &p, const ter_id &new_terrain, const furn_id &new_furniture ) {
            map::set( rebase_bub( p ), new_terrain, new_furniture );
        }
        bool furn_set( const tripoint_omt_ms &p, const furn_id &new_furniture, bool furn_reset = false,
                       bool avoid_creatures = false ) {
            return map::furn_set( rebase_bub( p ), new_furniture, furn_reset, avoid_creatures );
        }
        void draw_line_furn( const furn_id &type, const point_omt_ms &p1, const point_omt_ms &p2,
                             bool avoid_creatures = false ) {
            map::draw_line_furn( type, rebase_bub( p1 ), rebase_bub( p2 ), avoid_creatures );
        }
        void draw_square_furn( const furn_id &type, const point_omt_ms &p1, const point_omt_ms &p2,
                               bool avoid_creatures = false ) {
            map::draw_square_furn( type, rebase_bub( p1 ), rebase_bub( p2 ), avoid_creatures );
        }
        bool has_flag_furn( ter_furn_flag flag, const tripoint_omt_ms &p ) const {
            return map::has_flag_furn( flag, rebase_bub( p ) );
        }
        computer *add_computer( const tripoint_omt_ms &p, const std::string &name, int security ) {
            return map::add_computer( rebase_bub( p ), name, security );
        }
        std::string name( const tripoint_omt_ms &p ) {
            return map::name( rebase_bub( p ) );
        }
        bool impassable( const tripoint_omt_ms &p ) const {
            return map::impassable( rebase_bub( p ) );
        }
        tripoint_range<tripoint_omt_ms> points_on_zlevel() const;
        tripoint_range<tripoint_omt_ms> points_on_zlevel( int z ) const;
        tripoint_range<tripoint_omt_ms> points_in_rectangle(
            const tripoint_omt_ms &from, const tripoint_omt_ms &to ) const;

        tripoint_range<tripoint_omt_ms> points_in_radius(
            const tripoint_omt_ms &center, size_t radius, size_t radiusz = 0 ) const;
        map_stack i_at( const tripoint_omt_ms &p ) {
            return map::i_at( rebase_bub( p ) );
        }
        void spawn_item( const tripoint_omt_ms &p, const itype_id &type_id,
                         unsigned quantity = 1, int charges = 0,
                         const time_point &birthday = calendar::start_of_cataclysm, int damlevel = 0,
                         const std::set<flag_id> &flags = {}, const std::string &variant = "",
                         const std::string &faction = "" ) {
            map::spawn_item( rebase_bub( p ), type_id, quantity, charges, birthday, damlevel, flags, variant,
                             faction );
        }
        std::vector<item *> spawn_items( const tripoint_omt_ms &p, const std::vector<item> &new_items ) {
            return map::spawn_items( rebase_bub( p ), new_items );
        }
        item &add_item( const tripoint_omt_ms &p, item new_item ) {
            return map::add_item( rebase_bub( p ), std::move( new_item ) );
        }
        item &add_item_or_charges( const point_omt_ms &p, const item &obj,
                                   bool overflow = true ) {
            return map::add_item_or_charges( tripoint_bub_ms( rebase_bub( p ), abs_sub.z() ), obj, overflow );
        }
        std::vector<item *> put_items_from_loc(
            const item_group_id &group_id, const tripoint_omt_ms &p,
            const time_point &turn = calendar::start_of_cataclysm ) {
            return map::put_items_from_loc( group_id, rebase_bub( p ), turn );
        }
        item &add_item_or_charges( const tripoint_omt_ms &pos, item obj, bool overflow = true ) {
            return map::add_item_or_charges( rebase_bub( pos ), std::move( obj ), overflow );
        }
        std::vector<item *> place_items(
            const item_group_id &group_id, int chance, const tripoint_omt_ms &p1, const tripoint_omt_ms &p2,
            bool ongrass, const time_point &turn, int magazine = 0, int ammo = 0,
            const std::string &faction = "" ) {
            return map::place_items( group_id, chance, rebase_bub( p1 ), rebase_bub( p2 ), ongrass, turn,
                                     magazine, ammo, faction );
        }
        void add_corpse( const tripoint_omt_ms &p ) {
            map::add_corpse( rebase_bub( p ) );
        }
        void i_rem( const tripoint_omt_ms &p, item *it ) {
            map::i_rem( rebase_bub( p ), it );
        }
        void i_clear( const tripoint_omt_ms &p ) {
            map::i_clear( rebase_bub( p ) );
        }
        bool add_field( const tripoint_omt_ms &p, const field_type_id &type_id, int intensity = INT_MAX,
                        const time_duration &age = 0_turns, bool hit_player = true ) {
            return map::add_field( rebase_bub( p ), type_id, intensity, age, hit_player );
        }
        void delete_field( const tripoint_omt_ms &p, const field_type_id &field_to_remove ) {
            map::delete_field( rebase_bub( p ), field_to_remove );
        }
        bool has_flag( ter_furn_flag flag, const tripoint_omt_ms &p ) const {
            return map::has_flag( flag, rebase_bub( p ) );
        }
        void destroy( const tripoint_omt_ms &p, bool silent = false ) {
            map::destroy( rebase_bub( p ), silent );
        }
        const trap &tr_at( const tripoint_omt_ms &p ) const {
            return map::tr_at( rebase_bub( p ) );
        }
        void trap_set( const tripoint_omt_ms &p, const trap_id &type ) {
            map::trap_set( rebase_bub( p ), type );
        }
        void set_signage( const tripoint_omt_ms &p, const std::string &message ) {
            map::set_signage( rebase_bub( p ), message );
        }
        void delete_signage( const tripoint_omt_ms &p ) {
            map::delete_signage( rebase_bub( p ) );
        }
        VehicleList get_vehicles() {
            return map::get_vehicles();
        }
        optional_vpart_position veh_at( const tripoint_omt_ms &p ) const {
            return map::veh_at( rebase_bub( p ) );
        }
        vehicle *add_vehicle( const vproto_id &type, const tripoint_omt_ms &p, const units::angle &dir,
                              int init_veh_fuel = -1,
                              veh_spawn_status init_veh_status = veh_spawn_status::DEFAULT_LIGHT_DMG,
                              bool merge_wrecks = true,
                              bool force_status = false ) {
            return map::add_vehicle( type, rebase_bub( p ), dir, init_veh_fuel, init_veh_status,
                                     merge_wrecks, force_status );
        }
        void add_splatter_trail( const field_type_id &type, const tripoint_omt_ms &from,
                                 const tripoint_omt_ms &to ) {
            map::add_splatter_trail( type, rebase_bub( from ), rebase_bub( to ) );
        }
        void collapse_at( const tripoint_omt_ms &p, bool silent,
                          bool was_supporting = false,
                          bool destroy_pos = true ) {
            map::collapse_at( rebase_bub( p ), silent, was_supporting, destroy_pos );
        }
        tripoint_abs_sm get_abs_sub() const {
            return map::get_abs_sub();
        }
        tripoint_abs_ms get_abs( const tripoint_omt_ms &p ) const {
            return map::get_abs( rebase_bub( p ) );
        }
        tripoint_omt_ms get_omt( const tripoint_abs_ms &p ) const {
            return rebase_omt( map::get_bub( p ) );
        };
        bool is_outside( const tripoint_omt_ms &p ) const {
            return map::is_outside( rebase_bub( p ) );
        }
        bool is_roofed( const tripoint_omt_ms &p ) const {
            return map::is_roofed( rebase_bub( p ) );
        }
        int get_radiation( const tripoint_omt_ms &p ) const {
            return map::get_radiation( rebase_bub( p ) );
        }
        bool has_graffiti_at( const tripoint_omt_ms &p ) const {
            return map::has_graffiti_at( rebase_bub( p ) );
        }
        field &field_at( const tripoint_omt_ms &p ) {
            return map::field_at( rebase_bub( p ) );
        }
        maptile maptile_at( const tripoint_omt_ms &p );
        bool sees_some_items( const tripoint_omt_ms &p, const tripoint_omt_ms &from ) const {
            return map::sees_some_items( rebase_bub( p ), rebase_bub( from ) );
        }

        using map::rotate;
        using map::mirror;

        using map::build_outside_cache;

    protected:
        using map::set_abs_sub;
        using map::setsubmap;
        using map::get_nonant;
        int get_my_MAPSIZE() {
            return my_MAPSIZE;
        }
};

class fake_map : public tinymap
{
    private:
        std::vector<std::unique_ptr<submap>> temp_submaps_;
    public:
        explicit fake_map( const ter_id &ter_type = ter_str_id( "t_dirt" ).id() );
        ~fake_map() override;
        static constexpr int fake_map_z = -OVERMAP_DEPTH;
};








class smallmap : public tinymap
{
    public:
        smallmap() : tinymap( 2, true ) {}

        void cut_down_tree( tripoint_omt_ms p, point_rel_ms dir ) {
            tinymap::cut_down_tree( p, dir );
        };
};

class small_fake_map : public smallmap
{
    private:
        std::vector<std::unique_ptr<submap>> temp_submaps_;
    public:
        explicit small_fake_map( const ter_id &ter_type = ter_str_id( "t_dirt" ).id() );
        ~small_fake_map() override;
};

#endif
