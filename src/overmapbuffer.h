#pragma once
#ifndef CATA_SRC_OVERMAPBUFFER_H
#define CATA_SRC_OVERMAPBUFFER_H

#include <array>
#include <bitset>
#include <functional>
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

#include "cata_path.h"
#include "coordinates.h"
#include "enums.h"
#include "horde_map.h"
#include "map_scale_constants.h"
#include "memory_fast.h"
#include "overmap.h"
#include "overmap_types.h"
#include "overmap_worldgen.h"
#include "point.h"
#include "simple_pathfinding.h"
#include "type_id.h"

class JsonObject;
class JsonOut;
class JsonValue;
class basecamp;
class character_id;
class monster;
class npc;
class overmap_special;
class vehicle;
struct pp_resolved_generator;
enum class cube_direction : int;
enum class oter_travel_cost_type : int;
namespace om_direction
{
enum class type : int;
}
struct horde_entity;
struct map_data_summary;
struct mapgen_arguments;
struct mongroup;
struct region_settings;

struct overmap_path_params {
    std::map<oter_travel_cost_type, int> travel_cost_per_type;
    bool avoid_danger = true;
    bool only_known_by_player = true;
    bool allow_diagonal = false;

    void set_cost( const oter_travel_cost_type &type, int v ) {
        travel_cost_per_type.emplace( type, v );
    }
    int get_cost( const oter_travel_cost_type &type ) const {
        auto it = travel_cost_per_type.find( type );
        return it != travel_cost_per_type.end() ? it->second : -1;
    }
    static constexpr int standard_cost = 10;
    static overmap_path_params for_player();
    static overmap_path_params for_npc();
    static overmap_path_params for_land_vehicle( float offroad_coeff, bool tiny, bool amphibious );
    static overmap_path_params for_watercraft();
    static overmap_path_params for_aircraft();
    static overmap_path_params flatten_pathfinding_costs( overmap_path_params orig );
};

struct radio_tower_reference {

    radio_tower *tower;

    point_abs_sm abs_sm_pos;

    int signal_strength;
    explicit operator bool() const {
        return tower != nullptr;
    }
};

struct city_reference {
    static const city_reference invalid;

    const struct city *city;

    tripoint_abs_sm abs_sm_pos;

    int distance;

    explicit operator bool() const {
        return city != nullptr;
    }

    int get_distance_from_bounds() const;
};

struct camp_reference {
    static const camp_reference invalid;

    basecamp *camp;

    tripoint_abs_sm abs_sm_pos;

    int distance;

    explicit operator bool() const {
        return camp != nullptr;
    }

    int get_distance_from_bounds() const;
};

struct overmap_with_local_coords {
    overmap *om;
    tripoint_om_omt local;

    bool operator!() const {
        return !om;
    }

    explicit operator bool() const {
        return !!om;
    }
};














struct omt_find_params {
    std::vector<std::pair<std::string, ot_match_type>> types;
    int search_range = 0;
    int min_distance = 0;
    bool must_see = false;
    bool cant_see = false;
    bool existing_only = false;
    int min_z = -OVERMAP_DEPTH;
    int max_z = OVERMAP_HEIGHT;
    std::optional<overmap_special_id> om_special = std::nullopt;
};




struct unique_special_deck_state {
    int successes = 0;
    int deck_size = 0;
    int successes_remain = 0;
    int cards_remain = 0;
    int to_place = 0;

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &json );
};





struct overmap_global_state {

    std::unordered_set<overmap_special_id> placed_unique_specials;


    std::unordered_map<overmap_special_id, int> unique_special_count;


    std::unordered_map<overmap_special_id, unique_special_deck_state> unique_special_decks;

    int overmap_count = 0;

    int major_river_count = 0;

    highway_intersection_grid highway_intersections;

    std::unordered_map<tripoint_abs_om, region_settings_id> placed_regions;

    void clear();
    void reset();
    void deserialize( const JsonObject &json );
    void serialize( JsonOut &json ) const;
};

class overmapbuffer
{
    public:
        overmapbuffer();

        bool externally_set_args = false;
        overmap_global_state global_state;
        dimension_region_layout regions;

        static std::string terrain_filename( const point_abs_om & );
        static cata_path player_filename( const point_abs_om & );





        overmap &get( const point_abs_om & );
        void save();





        void reset();
        void clear();
        void create_custom_overmap( const point_abs_om &, overmap_special_batch &specials );





        const oter_id &ter( const tripoint_abs_omt &p );




        const oter_id &ter_existing( const tripoint_abs_omt &p );
        void ter_set( const tripoint_abs_omt &p, const oter_id &id );
        std::optional<mapgen_arguments> *mapgen_args( const tripoint_abs_omt & );


        std::vector<pp_resolved_generator> *pp_decisions( const tripoint_abs_omt & );
        std::string *join_used_at( const std::pair<tripoint_abs_omt, cube_direction> & );
        std::vector<oter_id> predecessors( const tripoint_abs_omt & );

        int highest_omt_point( tripoint_abs_omt loc );



        bool has_note( const tripoint_abs_omt &p );
        bool is_marked_dangerous( const tripoint_abs_omt &p );
        const std::string &note( const tripoint_abs_omt &p );
        int note_danger_radius( const tripoint_abs_omt &p );
        void add_note( const tripoint_abs_omt &, const std::string &message );
        void delete_note( const tripoint_abs_omt &p );
        void mark_note_dangerous( const tripoint_abs_omt &p, int radius, bool is_dangerous );
        bool has_extra( const tripoint_abs_omt &p );
        const map_extra_id &extra( const tripoint_abs_omt &p );
        void add_extra( const tripoint_abs_omt &p, const map_extra_id &id );
        void add_extra_note( const tripoint_abs_omt &p, bool force_add = false );
        void delete_extra( const tripoint_abs_omt &p );
        bool is_explored( const tripoint_abs_omt &p );
        void toggle_explored( const tripoint_abs_omt &p );

        bool distance_limit( int distance, const tripoint_abs_omt &origin_pos,
                             const tripoint_abs_omt &picked_pos );

        bool distance_limit_line( int distance, const tripoint_abs_omt &origin_pos,
                                  const tripoint_abs_omt &picked_pos );
        om_vision_level seen( const tripoint_abs_omt &p );
        bool seen_more_than( const tripoint_abs_omt &p, om_vision_level test );
        void set_seen( const tripoint_abs_omt &p, om_vision_level seen );
        bool has_camp( const tripoint_abs_omt &p );
        bool has_vehicle( const tripoint_abs_omt &p );
        bool has_horde( const tripoint_abs_omt &p );
        int get_horde_size( const tripoint_abs_omt &p, int filter = horde_map_flavors::active |
                            horde_map_flavors::idle | horde_map_flavors::dormant | horde_map_flavors::immobile );
        std::vector<om_vehicle> get_vehicle( const tripoint_abs_omt &p );
        std::string get_vehicle_ter_sym( const tripoint_abs_omt &omt );
        std::string get_vehicle_tile_id( const tripoint_abs_omt &omt );
        const region_settings &get_settings( const tripoint_abs_omt &p );








        std::array<std::array<scent_trace, 3>, 3> scents_near( const tripoint_abs_omt &origin );



        scent_trace scent_at( const tripoint_abs_omt &pos );






        void set_scent( const tripoint_abs_omt &loc, int strength );




        bool is_safe( const tripoint_abs_omt &p );



        bool is_in_city( const tripoint_abs_omt &p );










        void move_vehicle( vehicle *veh, const point_abs_ms &old_msp );



        void add_vehicle( vehicle *veh );



        void remove_camp( const point_abs_omt &p );



        void remove_vehicle( const vehicle *veh );



        void add_camp( const basecamp &camp );

        std::optional<basecamp *> find_camp( const point_abs_omt &p );

        void clear_camps( const point_abs_omt &p );











        std::vector<shared_ptr_fast<npc>> get_npcs_near( const tripoint_abs_sm &p, int radius );




        std::vector<shared_ptr_fast<npc>> get_companion_mission_npcs( int range = 100 );






        std::vector<shared_ptr_fast<npc>> get_npcs_near_omt( const tripoint_abs_omt &p, int radius );




        std::vector<shared_ptr_fast<npc>> get_npcs_near_player( int radius );





        shared_ptr_fast<npc> find_npc( character_id id );




        void populate_followers_vec( std::vector<npc *> &followers, bool only_following = false,
                                     bool ignore_hallu = false ) const;
        void foreach_npc( const std::function<void( npc & )> &callback );
        shared_ptr_fast<npc> find_npc_by_unique_id( const std::string &unique_id );



        std::vector<shared_ptr_fast<npc>> get_overmap_npcs();




        shared_ptr_fast<npc> remove_npc( const character_id &id );




        void insert_npc( const shared_ptr_fast<npc> &who );









        std::vector<tripoint_abs_omt> find_all( const tripoint_abs_omt &origin,
                                                const omt_find_params &params );
        std::vector<tripoint_abs_omt> find_all(
            const tripoint_abs_omt &origin, const std::string &type,
            int dist, bool must_be_seen, ot_match_type match_type = ot_match_type::type,
            bool existing_overmaps_only = false,
            const std::optional<overmap_special_id> &om_special = std::nullopt );








        tripoint_abs_omt find_random( const tripoint_abs_omt &origin,
                                      const omt_find_params &params );
        tripoint_abs_omt find_random(
            const tripoint_abs_omt &origin, const std::string &type, int dist,
            bool must_be_seen, ot_match_type match_type = ot_match_type::type,
            bool existing_overmaps_only = false,
            const std::optional<overmap_special_id> &om_special = std::nullopt );










        bool reveal( const point_abs_omt &center, int radius, int z );
        bool reveal( const tripoint_abs_omt &center, int radius );
        bool reveal( const tripoint_abs_omt &center, int radius,
                     const std::function<bool( const oter_id & )> &filter );
        pf::simple_path<tripoint_abs_omt> get_travel_path(
            const tripoint_abs_omt &src, const tripoint_abs_omt &dest, const overmap_path_params &params );
        bool reveal_route( const tripoint_abs_omt &source, const tripoint_abs_omt &dest,
                           int radius = 0, bool road_only = false );





        tripoint_abs_omt find_closest( const tripoint_abs_omt &origin,
                                       const omt_find_params &params );
        tripoint_abs_omt find_closest(
            const tripoint_abs_omt &origin, const std::string &type, int radius, bool must_be_seen,
            ot_match_type match_type = ot_match_type::type, bool existing_overmaps_only = false,
            const std::optional<overmap_special_id> &om_special = std::nullopt );
        tripoint_abs_omt find_existing_globally_unique( const tripoint_abs_omt &origin,
                const omt_find_params &params );









        overmap_with_local_coords get_existing_om_global( const point_abs_omt &p );
        overmap_with_local_coords get_existing_om_global( const tripoint_abs_omt &p );
        overmap_with_local_coords get_om_global( const point_abs_omt &p );
        overmap_with_local_coords get_om_global( const tripoint_abs_omt &p );


        region_settings_id get_overmap_region( tripoint_abs_om om_point );
        void print_region_layout();
        void init_region_layout();






        bool has( const point_abs_om &p );






        overmap *get_existing( const point_abs_om &p );





        bool is_omt_generated( const tripoint_abs_omt &loc );

        using t_point_with_note = std::pair<point_abs_omt, std::string>;
        using t_notes_vector = std::vector<t_point_with_note>;
        t_notes_vector get_all_notes( int z ) {
            return get_notes( z, {} );
        }
        t_notes_vector find_notes( int z, const std::string_view pattern ) {
            return get_notes( z, pattern );
        }
        using t_point_with_extra = std::pair<point_abs_omt, map_extra_id>;
        using t_extras_vector = std::vector<t_point_with_extra>;
        t_extras_vector get_all_extras( int z ) {
            return get_extras( z, {} );
        }
        t_extras_vector find_extras( int z, const std::string_view pattern ) {
            return get_extras( z, pattern );
        }
        bool passable( const tripoint_abs_ms &p );
        std::shared_ptr<map_data_summary> get_omt_summary( const tripoint_abs_omt &p );
        void set_passable( const tripoint_abs_ms &p, bool new_passable );
        void set_passable( const tripoint_abs_omt &p, const std::bitset<24 * 24> &new_passable );






        void signal_hordes( const tripoint_abs_sm &center, int sig_power );




        void alert_entity( const tripoint_abs_ms &location, const tripoint_abs_ms &destination,
                           int intensity );



        void clear_mongroups();



        void process_mongroups();




        void move_hordes();





        void signal_nemesis( const tripoint_abs_sm &p );



        void add_nemesis( const tripoint_abs_omt &p );




        void move_nemesis();



        void remove_nemesis();

        std::vector<mongroup *> monsters_at( const tripoint_abs_omt &p );




        std::vector<mongroup *> groups_at( const tripoint_abs_sm &p );





        void spawn_monster( const tripoint_abs_sm &p, bool spawn_nonlocal = false );



        horde_entity &spawn_monster( const tripoint_abs_ms &p, mtype_id id );




        void despawn_monster( const monster &critter );
        void spawn_mongroup( const tripoint_abs_sm &p, const mongroup_id &type, int count );
        horde_entity *entity_at( const tripoint_abs_ms &p );
        std::vector<std::unordered_map<tripoint_abs_ms, horde_entity>*> hordes_at(
            const tripoint_abs_omt &p, int filter = horde_map_flavors::active | horde_map_flavors::idle |
                    horde_map_flavors::dormant | horde_map_flavors::immobile );






        radio_tower_reference find_radio_station( int frequency );




        std::vector<radio_tower_reference> find_all_radio_stations();
        std::vector<camp_reference> get_camps_near( const tripoint_abs_sm &location, int radius );




        std::vector<city_reference> get_cities_near( const tripoint_abs_sm &location, int radius );





        city_reference closest_city( const tripoint_abs_sm &center );

        city_reference closest_known_city( const tripoint_abs_sm &center );


        std::string get_description_at( const tripoint_abs_sm &where, bool draw_origin = true );

        void display_description_at( const tripoint_abs_sm &where, bool draw_origin = true );













        std::optional<std::vector<tripoint_abs_omt>> place_special(
                    const overmap_special &special, const tripoint_abs_omt &origin,
                    om_direction::type dir, bool must_be_unexplored, bool force );










        bool place_special( const overmap_special_id &special_id, const tripoint_abs_omt &center,
                            int radius );

        int get_unique_special_count( const overmap_special_id &id );
        int get_overmap_count() const;


        unique_special_deck_state &get_deck_state(
            const overmap_special_id &id, int successes, int deck_size );

        void consume_deck_placement( const overmap_special_id &id );
        int get_major_river_count() const;
        void inc_major_river_count();

    private:






        bool is_findable_location( const tripoint_abs_omt &location, const omt_find_params &params );

        std::unordered_map< point_abs_om, std::unique_ptr< overmap > > overmaps;




        mutable std::set<point_abs_om> known_non_existing;

        overmap mutable *last_requested_overmap;







        t_notes_vector get_notes( int z, std::string_view pattern );






        t_extras_vector get_extras( int z, std::string_view pattern );
    public:





        bool check_ot( const std::string &otype, ot_match_type match_type,
                       const tripoint_abs_omt &p );
        bool check_overmap_special_type( const overmap_special_id &id, const tripoint_abs_omt &loc );
        std::optional<overmap_special_id> overmap_special_at( const tripoint_abs_omt & );
        std::optional<mapgen_arguments> get_existing_omt_stack_arguments( const point_abs_omt &p );





        bool check_ot_existing( const std::string &otype, ot_match_type match_type,
                                const tripoint_abs_omt &loc );
        bool check_overmap_special_type_existing( const overmap_special_id &id,
                const tripoint_abs_omt &loc );




        void add_unique_special( const overmap_special_id &id );



        void log_unique_special( const overmap_special_id &id );



        bool contains_unique_special( const overmap_special_id &id ) const;



        void deserialize_placed_unique_specials( const JsonValue &jsin );
    private:




        void fix_mongroups( overmap &new_overmap );




        void fix_nemesis( overmap &new_overmap );



        void fix_npcs( overmap &new_overmap );





        std::vector<overmap *> get_overmaps_near( const point_abs_sm &p, int radius );
        std::vector<overmap *> get_overmaps_near( const tripoint_abs_sm &location, int radius );
};

extern overmapbuffer overmap_buffer;

#endif
