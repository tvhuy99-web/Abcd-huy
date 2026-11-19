#pragma once
#ifndef CATA_SRC_OVERMAP_H
#define CATA_SRC_OVERMAP_H

#include <stdint.h>
#include <algorithm>
#include <array>
#include <bitset>
#include <climits>
#include <cstdlib>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "basecamp.h"
#include "catacharset.h"
#include "cata_variant.h"
#include "city.h"
#include "colony.h"
#include "color.h"
#include "coordinates.h"
#include "cube_direction.h"
#include "enums.h"
#include "flat_set.h"
#include "hash_utils.h"
#include "horde_map.h"
#include "map_scale_constants.h"
#include "mapgendata.h"
#include "mdarray.h"
#include "memory_fast.h"
#include "mongroup.h"
#include "omdata.h"
#include "output.h"
#include "overmap_location.h"
#include "overmap_types.h" // IWYU pragma: keep
#include "point.h"
#include "rng.h"
#include "simple_pathfinding.h"
#include "type_id.h"

class JsonArray;
class JsonObject;
class JsonOut;
class JsonValue;
class cata_path;
class character_id;
class monster;
class npc;
class overmap_connection;
struct horde_entity;
struct pp_resolved_generator;
struct map_data_summary;
struct region_settings;
template <typename T> struct enum_traits;

struct om_note {
    std::string text;
    point_om_omt p;
    bool dangerous = false;
    int danger_radius = 0;
};

struct om_map_extra {
    map_extra_id id;
    point_om_omt p;
};


namespace om_lines
{
struct type {
    uint32_t symbol;
    size_t mapgen;
    MULTITILE_TYPE subtile;
    int rotation;
    std::string suffix;
};

const std::array<std::string, 5> mapgen_suffixes = { {
        "_straight", "_curved", "_end", "_tee", "_four_way"
    }
};

const std::array < type, 1 + om_direction::bits > all = { {
        { UTF8_getch( LINE_XXXX_S ), 4, unconnected,  0, "_isolated"  },
        { UTF8_getch( LINE_XOXO_S ), 2, end_piece,    2, "_end_south" },
        { UTF8_getch( LINE_OXOX_S ), 2, end_piece,    1, "_end_west"  },
        { UTF8_getch( LINE_XXOO_S ), 1, corner,       1, "_ne"        },
        { UTF8_getch( LINE_XOXO_S ), 2, end_piece,    0, "_end_north" },
        { UTF8_getch( LINE_XOXO_S ), 0, edge,         0, "_ns"        },
        { UTF8_getch( LINE_OXXO_S ), 1, corner,       0, "_es"        },
        { UTF8_getch( LINE_XXXO_S ), 3, t_connection, 1, "_nes"       },
        { UTF8_getch( LINE_OXOX_S ), 2, end_piece,    3, "_end_east"  },
        { UTF8_getch( LINE_XOOX_S ), 1, corner,       2, "_wn"        },
        { UTF8_getch( LINE_OXOX_S ), 0, edge,         1, "_ew"        },
        { UTF8_getch( LINE_XXOX_S ), 3, t_connection, 2, "_new"       },
        { UTF8_getch( LINE_OOXX_S ), 1, corner,       3, "_sw"        },
        { UTF8_getch( LINE_XOXX_S ), 3, t_connection, 3, "_nsw"       },
        { UTF8_getch( LINE_OXXX_S ), 3, t_connection, 0, "_esw"       },
        { UTF8_getch( LINE_XXXX_S ), 4, center,       0, "_nesw"      }
    }
};

const size_t size = all.size();
const size_t invalid = 0;

inline constexpr size_t rotate( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return line;
    }

    return ( ( line << static_cast<size_t>( dir ) ) |
             ( line >> ( om_direction::size - static_cast<size_t>( dir ) ) ) ) & om_direction::bits;
}

inline constexpr size_t set_segment( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return line;
    }
    return line | 1 << static_cast<int>( dir );
}

inline constexpr bool has_segment( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return false;
    }
    return static_cast<bool>( line & 1 << static_cast<int>( dir ) );
}

inline constexpr bool is_straight( size_t line )
{
    return line == 1
           || line == 2
           || line == 4
           || line == 5
           || line == 8
           || line == 10;
}

size_t from_dir( om_direction::type dir );

}

struct om_vehicle {
    tripoint_om_omt p;
    std::string name;
};

enum class radio_type : int {
    MESSAGE_BROADCAST,
    WEATHER_RADIO
};

extern std::map<radio_type, std::string> radio_type_names;

constexpr int RADIO_MIN_STRENGTH = 120;
constexpr int RADIO_MAX_STRENGTH = 360;





constexpr int RIVER_BORDER = 10;
constexpr int RIVER_Z = 0;
constexpr int HIGHWAY_MAX_CONNECTIONS = 4;

struct radio_tower {

    point_om_sm pos;
    int strength;
    radio_type type;
    std::string message;
    int frequency;
    explicit radio_tower( const point_om_sm &p, int S = -1, const std::string &M = "",
                          radio_type T = radio_type::MESSAGE_BROADCAST ) :
        pos( p ), strength( S ), type( T ), message( M ) {
        frequency = rng( 0, INT_MAX );
    }
};

enum class om_vision_level : int8_t {
    unseen = 0,


    vague,


    outlines,


    details,

    full,
    last
};

template<>
struct enum_traits<om_vision_level> {
    static constexpr om_vision_level last = om_vision_level::last;
};

struct map_layer {
    cata::mdarray<oter_id, point_om_omt> terrain;
    cata::mdarray<om_vision_level, point_om_omt> visible;
    cata::mdarray<bool, point_om_omt> explored;
    cata::mdarray<std::shared_ptr<map_data_summary>, point_om_omt> map_cache;
    std::vector<om_note> notes;
    std::vector<om_map_extra> extras;
};

struct om_special_sectors {
    std::vector<point_om_omt> sectors;
    int sector_width;
};


struct overmap_special_placement {
    int instances_placed;
    const overmap_special *special_details;
};




struct overmap_river_node {
    const point_om_omt river_start;
    const point_om_omt river_end;
    point_om_omt control_p1 = point_om_omt::invalid;
    point_om_omt control_p2 = point_om_omt::invalid;
    const size_t size = 0;
    bool is_branch = false;
};


struct overmap_river_border {
    std::vector<point_om_omt> border_river_omt;
    std::vector<point_om_omt>
    border_river_nodes_omt;
    std::vector<const overmap_river_node *>
    border_river_nodes;
};


struct overmap_highway_node {
    std::array<point_om_omt, HIGHWAY_MAX_CONNECTIONS> highway_ends;
};


class overmap_special_batch
{
    public:
        explicit overmap_special_batch( const point_abs_om &origin ) : origin_overmap( origin ) {}
        overmap_special_batch( const point_abs_om &origin,
                               const std::vector<const overmap_special *> &specials ) :
            origin_overmap( origin ) {
            std::transform( specials.begin(), specials.end(),
            std::back_inserter( placements ), []( const overmap_special * elem ) {
                return overmap_special_placement{ 0, elem };
            } );
        }



        std::vector<overmap_special_placement>::iterator begin() {
            return placements.begin();
        }
        std::vector<overmap_special_placement>::const_iterator begin() const {
            return placements.begin();
        }
        std::vector<overmap_special_placement>::iterator end() {
            return placements.end();
        }
        std::vector<overmap_special_placement>::const_iterator end() const {
            return placements.end();
        }
        std::vector<overmap_special_placement>::iterator erase(
            std::vector<overmap_special_placement>::iterator pos ) {
            return placements.erase( pos );
        }
        bool empty() {
            return placements.empty();
        }

        point_abs_om get_origin() const {
            return origin_overmap;
        }

    private:
        std::vector<overmap_special_placement> placements;
        point_abs_om origin_overmap;
};


class overmap_feature_grid_node
{

        point_abs_om grid_pos = point_abs_om::invalid;

        point_abs_om offset_pos = point_abs_om::invalid;

        std::array<point_abs_om, HIGHWAY_MAX_CONNECTIONS> adjacent_features =
        { point_abs_om::invalid, point_abs_om::invalid, point_abs_om::invalid, point_abs_om::invalid };
    public:
        overmap_feature_grid_node() = default;
        explicit overmap_feature_grid_node( point_abs_om grid_pos ) : grid_pos( grid_pos ) {};
        point_abs_om get_grid_pos() const {
            return grid_pos;
        }
        point_abs_om get_offset_pos() const {
            return offset_pos;
        }
        void set_offset_pos( const point_abs_om &pos ) {
            offset_pos = pos;
        }
        point_abs_om get_adjacent_pos( int dir ) {
            return adjacent_features[dir];
        }
        void set_adjacent_pos( const point_abs_om &pos, int dir ) {
            adjacent_features[dir] = pos;
        }
        void serialize( JsonOut &out ) const;
        void deserialize( const JsonObject &obj );
};









class overmap_feature_grid
{

    protected:
        std::map<std::string, overmap_feature_grid_node> feature_grid;
        point_abs_om grid_origin = point_abs_om::invalid;
        int row_separation = 0; // NOLINT(cata-serialize)
        int column_separation = 0; // NOLINT(cata-serialize)
        int max_offset_variance = 0; // NOLINT(cata-serialize)
    public:
        overmap_feature_grid() = default;
        virtual ~overmap_feature_grid() = default;
        void set_grid_origin( const point_abs_om &p );
        point_abs_om get_grid_origin() const;

        std::vector<overmap_feature_grid_node> find_grid_adjacent_features( const point_abs_om
                &generated_om_pos );
        void generate_feature_point( const point_abs_om &generated_om_pos );

        void set_feature_grid( const std::map<std::string, overmap_feature_grid_node> &grid ) {
            feature_grid = grid;
        }
        bool feature_point_exists( const point_abs_om &intersection_om ) const;
        overmap_feature_grid_node get_feature_point(
            const point_abs_om &p ) const {
            return feature_grid.at( p.to_string_writable() );
        }
        void set_feature_point( const point_abs_om &p,
                                const overmap_feature_grid_node &intersection ) {
            feature_grid[p.to_string_writable()] = intersection;
        }

        void clear();
        void serialize( JsonOut &out ) const;
        void deserialize( const JsonObject &obj );





        std::vector<point_abs_om> find_feature_point_bounds( const point_abs_om &generated_om_pos );

        virtual void generate_offset( overmap_feature_grid_node &node ) = 0;
};

class highway_intersection_grid : public overmap_feature_grid
{
    public:

        void set_options();
        void generate_offset( overmap_feature_grid_node &node ) override;
    private:
        std::unordered_map<point_abs_om, bool> om_has_lake_cache;
};







struct intrahighway_node {
    pf::directed_node<tripoint_om_omt> path_node =
        pf::directed_node<tripoint_om_omt>( tripoint_om_omt::invalid, om_direction::type::invalid );
    overmap_special_id placed_special = overmap_special_id::NULL_ID();
    bool is_segment = true;
    bool is_ramp = false;
    bool is_interchange = false;

    bool ramp_down = false;
    explicit intrahighway_node( tripoint_om_omt pos, om_direction::type dir,
                                overmap_special_id placed_spec, bool is_seg = true ) {
        path_node = pf::directed_node<tripoint_om_omt>( pos, dir );
        placed_special = placed_spec;
        is_segment = is_seg;
    }


    om_direction::type get_effective_dir() const {
        return is_segment ?
               om_direction::type( static_cast<int>( path_node.dir ) % 2 ) :
               om_direction::type( static_cast<int>( path_node.dir ) );
    }
};

using Highway_path = std::vector<intrahighway_node>;

template<typename Tripoint>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::hash<pos_dir<Tripoint>> {
    std::size_t operator()( const pos_dir<Tripoint> &p ) const {
        cata::tuple_hash h;
        return h( std::make_tuple( p.p, p.dir ) );
    }
};

class overmap
{
    public:
        overmap( const overmap & ) = default;
        overmap( overmap && ) = default;
        explicit overmap( const point_abs_om &p );
        ~overmap();

        overmap &operator=( const overmap & ) = default;




        void populate( overmap_special_batch &enabled_specials );
        void populate();

        const point_abs_om &pos() const {
            return loc;
        }
        int get_urbanity() const {
            return urbanity;
        }

        static bool omt_lake_noise_threshold( const point_abs_omt &origin, const point_om_omt &offset,
                                              double noise_threshold );


        static bool guess_has_lake( const point_abs_om &p, double noise_threshold, int tile_count );

        void save() const;






        tripoint_om_omt find_random_omt( const std::pair<std::string, ot_match_type> &target,
                                         std::optional<city> target_city = std::nullopt ) const;
        tripoint_om_omt find_random_omt( const std::string &omt_base_type,
                                         ot_match_type match_type = ot_match_type::type,
                                         std::optional<city> target_city = std::nullopt ) const {
            return find_random_omt( std::make_pair( omt_base_type, match_type ), std::move( target_city ) );
        }






        std::vector<point_abs_omt> find_terrain( std::string_view term, int zlevel ) const;

        void ter_set( const tripoint_om_omt &p, const oter_id &id );

        const oter_id &ter( const tripoint_om_omt &p ) const;

        const oter_id &ter_unsafe( const tripoint_om_omt &p ) const;
        std::optional<mapgen_arguments> *mapgen_args( const tripoint_om_omt & );


        std::vector<pp_resolved_generator> *pp_decisions( const tripoint_om_omt & );
        std::string *join_used_at( const om_pos_dir & );
        std::vector<oter_id> predecessors( const tripoint_om_omt & );
        void set_seen( const tripoint_om_omt &p, om_vision_level val, bool force = false );
        om_vision_level seen( const tripoint_om_omt &p ) const;
        bool seen_more_than( const tripoint_om_omt &p, om_vision_level test ) const;
        bool &explored( const tripoint_om_omt &p );
        bool is_explored( const tripoint_om_omt &p ) const;

        bool has_note( const tripoint_om_omt &p ) const;
        bool is_marked_dangerous( const tripoint_om_omt &p ) const;
        const std::string &note( const tripoint_om_omt &p ) const;
        void add_note( const tripoint_om_omt &p, std::string message );
        void delete_note( const tripoint_om_omt &p );
        void mark_note_dangerous( const tripoint_om_omt &p, int radius, bool is_dangerous );
        int note_danger_radius( const tripoint_om_omt &p ) const;

        bool has_extra( const tripoint_om_omt &p ) const;
        const map_extra_id &extra( const tripoint_om_omt &p ) const;
        void add_extra( const tripoint_om_omt &p, const map_extra_id &id );
        void add_extra_note( const tripoint_om_omt &p, bool force_add = false );
        void delete_extra( const tripoint_om_omt &p );




        bool passable( const tripoint_om_ms &p );
        std::shared_ptr<map_data_summary> get_omt_summary( const tripoint_om_omt &p );
        void set_passable( const tripoint_om_ms &p, bool new_passable );
        void set_passable( const tripoint_abs_omt &p, const std::bitset<24 * 24> &new_passable );
    private:
        void set_passable( const tripoint_abs_omt &p, std::shared_ptr<map_data_summary> new_passable );
        void set_passable( const tripoint_abs_omt &p, string_id<map_data_summary> new_passable );
        std::optional<tripoint_om_ms> find_open_space_in_submap( const tripoint_om_ms &submap_origin,
                point_rel_ms &cursor );
    public:

        horde_entity &spawn_monster( const tripoint_abs_ms &p, mtype_id id );

        void spawn_monsters( const tripoint_om_sm &p, std::vector<monster> &monsters );

        void spawn_mongroup( const tripoint_om_sm &p, const mongroup_id &type, int count );
        horde_entity *entity_at( const tripoint_om_ms &p );
        std::vector<std::unordered_map<tripoint_abs_ms, horde_entity>*> hordes_at(
            const tripoint_om_omt &p, int filter );





        const scent_trace &scent_at( const tripoint_abs_omt &loc ) const;



        void set_scent( const tripoint_abs_omt &loc, const scent_trace &new_scent );





        static bool inbounds( const tripoint_om_omt &p, int clearance = 0 );
        static bool inbounds( const point_om_omt &p, int clearance = 0 ) {
            return inbounds( tripoint_om_omt( p, 0 ), clearance );
        }
        bool inbounds( const tripoint_abs_ms &p );







        std::vector<point_abs_omt> find_notes( int z, const std::string &text );






        std::vector<point_abs_omt> find_extras( int z, const std::string &text );






        bool is_omt_generated( const tripoint_om_omt &loc ) const;


        bool is_river_node( const point_om_omt &p ) const;


        const overmap_river_node *get_river_node_at( const point_om_omt &p ) const;


        point_abs_omt global_base_point() const;


        const region_settings &get_settings() const {
            return *settings;
        }

        void clear_mon_groups();
        void clear_overmap_special_placements();
        void clear_cities();
        void clear_connections_out();
        void place_special_forced( const overmap_special_id &special_id, const tripoint_om_omt &p,
                                   om_direction::type dir );

        bool is_in_city( const tripoint_om_omt &p ) const;

        std::optional<int> distance_to_city( const tripoint_om_omt &p,
                                             int max_dist_to_check = OMAPX ) const;


        std::optional<int> approx_distance_to_city( const tripoint_om_omt &p,
                int max_dist_to_check = OMAPX ) const;
    private:

        std::unordered_set<point_om_omt> city_tiles;

        void flood_fill_city_tiles();
        std::multimap<tripoint_om_sm, mongroup> zg; // NOLINT(cata-serialize)
        std::map<point_abs_omt, basecamp> camps;
    public:

        bool mongroup_check( const mongroup &candidate ) const;


        std::vector<radio_tower> radios;
        std::map<int, om_vehicle> vehicles;
        std::vector<city> cities;
        std::vector<overmap_river_node> rivers;
        std::map<string_id<overmap_connection>, std::vector<tripoint_om_omt>> connections_out;
        void add_camp( const point_abs_omt &p, const basecamp &camp );
        std::optional<basecamp *> find_camp( const point_abs_omt &p );
        void clear_camps();
        const std::map<point_abs_omt, basecamp> &get_camps() const {
            return camps;
        }
        void remove_camp( const point_abs_omt &p );

        void insert_npc( const shared_ptr_fast<npc> &who );

        shared_ptr_fast<npc> erase_npc( const character_id &id );
        shared_ptr_fast<npc> find_npc( const character_id &id ) const;
        shared_ptr_fast<npc> find_npc_by_unique_id( const std::string &id ) const;
        const std::vector<shared_ptr_fast<npc>> &get_npcs() const {
            return npcs;
        }
        std::vector<shared_ptr_fast<npc>> get_npcs( const std::function<bool( const npc & )>
                                       &predicate )
                                       const;
        point_om_omt get_fallback_road_connection_point() const;
    private:
        friend class overmapbuffer;
        friend class overmap_test_helper;

        std::vector<shared_ptr_fast<npc>> npcs;



        bool nullbool = false; // NOLINT(cata-serialize)
        point_abs_om loc; // NOLINT(cata-serialize)

        std::optional<point_om_omt> fallback_road_connection_point; // NOLINT(cata-serialize)


        std::array<tripoint_om_omt, 4> highway_connections = {
            tripoint_om_omt::invalid, tripoint_om_omt::invalid,
            tripoint_om_omt::invalid, tripoint_om_omt::invalid
        };

        std::array<map_layer, OVERMAP_LAYERS> layer;
        std::unordered_map<tripoint_abs_omt, scent_trace> scents;




        std::unordered_map<tripoint_om_omt, overmap_special_id> overmap_special_placements;


        std::unordered_set<tripoint_om_omt> safe_at_worldgen; // NOLINT(cata-serialize)



        std::unordered_map<tripoint_om_omt, std::vector<oter_id>> predecessors_;




        cata::colony<std::optional<mapgen_arguments>> mapgen_arg_storage;
        std::unordered_map<tripoint_om_omt, std::optional<mapgen_arguments> *> mapgen_args_index;

        std::unordered_map<point_abs_omt, mapgen_arguments> omt_stack_arguments_map;



        cata::colony<std::vector<pp_resolved_generator>> pp_decision_storage;
        std::unordered_map<tripoint_om_omt, std::vector<pp_resolved_generator> *> pp_decisions_index;



        std::unordered_map<om_pos_dir, std::string> joins_used;

        region_settings_id settings;

        oter_id get_default_terrain( int z ) const;


        void init_layers();

        void open( overmap_special_batch &enabled_specials );
    public:

        std::optional<mapgen_arguments> get_existing_omt_stack_arguments(
            const point_abs_omt &p ) const;

        std::optional<cata_variant> get_existing_omt_stack_argument( const point_abs_omt &p,
                const std::string &param_name ) const;

        void add_omt_stack_argument( const point_abs_omt &p, const std::string &param_name,
                                     const cata_variant &value );







        horde_map hordes;


        void unserialize( const cata_path &file_name, std::istream &fin );
        void unserialize( std::istream &fin );
        void unserialize( const JsonObject &jsobj );

        void unserialize_omap( const JsonValue &jsin, const cata_path &json_path );

        void unserialize_view( const cata_path &file_name, std::istream &fin );
        void unserialize_view( const JsonObject &jsobj );

        void serialize( std::ostream &fout ) const;

        void serialize_view( std::ostream &fout ) const;
    private:
        void generate( const std::vector<const overmap *> &neighbor_overmaps,
                       overmap_special_batch &enabled_specials );
        bool generate_sub( int z );
        bool generate_over( int z );

        void generate_bridgeheads( const std::vector<point_om_omt> &bridge_points,
                                   oter_type_str_id bridge_type,
                                   const std::string &bridgehead_ground,
                                   const std::string &bridgehead_ramp );

        const city &get_nearest_city( const tripoint_om_omt &p ) const;
        const city &get_invalid_city() const;

        void signal_hordes( const tripoint_abs_ms &p, int sig_power );
        void alert_entity( const tripoint_om_ms &location, const tripoint_abs_ms &destination,
                           int intensity );
        void process_mongroups();
        void move_hordes();


        void signal_nemesis( const tripoint_abs_sm & );
        void move_nemesis();
        void place_nemesis( const tripoint_abs_omt & );
        bool remove_nemesis();


        float calculate_ocean_gradient( const point_om_omt &p, point_abs_om this_omt );




        void place_river( const std::vector<const overmap *> &neighbor_overmaps,
                          const overmap_river_node &initial_points, int river_scale = 1.0, bool major_river = false );
        void place_forests();
        void place_lakes( const std::vector<const overmap *> &neighbor_overmaps );
        void place_oceans( const std::vector<const overmap *> &neighbor_overmaps );
        void place_rivers( const std::vector<const overmap *> &neighbor_overmaps );

        void place_swamps();
        void place_forest_trails();
        void place_forest_trailheads();

        void place_roads( const std::vector<const overmap *> &neighbor_overmaps );




        std::vector<Highway_path> place_highways( const std::vector<const overmap *>
                &neighbor_overmaps );





        Highway_path place_highway_line(
            const tripoint_om_omt &sp1, const tripoint_om_omt &sp2,
            const om_direction::type &draw_direction, int base_z );





        void place_highway_lines_with_bends( Highway_path &highway_path,
                                             const std::vector<std::pair<tripoint_om_omt, om_direction::type>> &bend_points,
                                             const tripoint_om_omt &start_point, const tripoint_om_omt &end_point,
                                             om_direction::type direction, int base_z );




        Highway_path place_highway_reserved_path( const tripoint_om_omt &p1,
                const tripoint_om_omt &p2,
                int dir1, int dir2, int base_z );



        void place_highway_interchanges( std::vector<Highway_path> &highway_path );




        tripoint_om_omt find_highway_intersection_point( const overmap_special_id &special,
                const tripoint_om_omt &center, const om_direction::type &dir, int border ) const;

        bool check_intersection_inbounds( const overmap_special_id &special,
                                          const tripoint_om_omt &center, int border ) const;

        void highway_handle_special_z( Highway_path &highway_path, int base_z );


        std::pair<bool, std::bitset<HIGHWAY_MAX_CONNECTIONS>> highway_handle_oceans();


        bool highway_select_end_points( const std::vector<const overmap *> &neighbor_overmaps,
                                        std::array<tripoint_om_omt, HIGHWAY_MAX_CONNECTIONS> &end_points,
                                        std::bitset<HIGHWAY_MAX_CONNECTIONS> &neighbor_connections,
                                        const std::bitset<HIGHWAY_MAX_CONNECTIONS> &ocean_neighbors, int base_z );

        void highway_handle_ramps( Highway_path &path, int base_z );

        void place_highway_supported_special( const overmap_special_id &special,
                                              const tripoint_om_omt &placement,
                                              const om_direction::type &dir );

        void finalize_highways( std::vector<Highway_path> &paths );
        bool highway_intersection_exists( const point_abs_om &intersection_om ) const;




        std::optional<std::bitset<4>> is_highway_overmap() const;




        std::pair<std::vector<tripoint_om_omt>, om_direction::type>
        get_highway_segment_points( const pf::directed_node<tripoint_om_omt> &node ) const;

        void place_railroads( const std::vector<const overmap *> &neighbor_overmaps );

        void populate_connections_out_from_neighbors( const std::vector<const overmap *>
                &neighbor_overmaps );

        void log_unique_special( const overmap_special_id &id );
        bool contains_unique_special( const overmap_special_id &id ) const;









        overmap_river_border setup_adjacent_river( const point_rel_om &adjacent_om, int border );


        overmap_special_id pick_random_building_to_place( int town_dist, int town_size,
                const std::unordered_set<overmap_special_id> &placed_unique_buildings ) const;


        // NOLINTNEXTLINE(cata-serialize)
        int urbanity = 0;


        // NOLINTNEXTLINE(cata-serialize)
        float forest_size_adjust = 0.0f;
        // NOLINTNEXTLINE(cata-serialize)
        float forestosity = 0.0f;
        void calculate_urbanity();
        void calculate_forestosity();


        void place_cities();

        void build_cities();
        void place_building( const tripoint_om_omt &p, om_direction::type dir, const city &town,
                             std::unordered_set<overmap_special_id> &placed_unique_buildings );

        void build_city_street( const overmap_connection &connection, const point_om_omt &p, int cs,
                                om_direction::type dir, const city &town,
                                std::unordered_set<overmap_special_id> &placed_unique_buildings, int block_width = 2 );
        void place_ravines();


        pf::directed_path<point_om_omt> lay_out_connection(
            const overmap_connection &connection, const point_om_omt &source,
            const point_om_omt &dest, int z, bool must_be_unexplored ) const;
        pf::directed_path<point_om_omt> lay_out_street(
            const overmap_connection &connection, const point_om_omt &source,
            om_direction::type dir, size_t len );
    public:
        void build_connection(
            const overmap_connection &connection, const pf::directed_path<point_om_omt> &path, int z,
            cube_direction initial_dir = cube_direction::last );
        void build_connection( const point_om_omt &source, const point_om_omt &dest, int z,
                               const overmap_connection &connection, bool must_be_unexplored,
                               cube_direction initial_dir = cube_direction::last );
        void connect_closest_points( const std::vector<point_om_omt> &points, int z,
                                     const overmap_connection &connection );

        bool check_ot( const std::string &otype, ot_match_type match_type,
                       const tripoint_om_omt &p ) const;
        bool check_overmap_special_type( const overmap_special_id &id,
                                         const tripoint_om_omt &location ) const;
        std::optional<overmap_special_id> overmap_special_at( const tripoint_om_omt &p ) const;


        std::vector<tripoint_om_omt> get_border( const point_rel_om &direction, int z,
                int distance_corner );
        std::vector<tripoint_om_omt> get_border( om_direction::type direction, int z,
                int distance_corner );

        std::vector<tripoint_om_omt> get_neighbor_border( const point_rel_om &direction, int z,
                int distance_corner );
        void polish_river( const std::vector<const overmap *> &neighbor_overmaps );
        void build_river_shores( const std::vector<const overmap *> &neighbor_overmaps,
                                 const tripoint_om_omt &p );
        void river_meander( const point_om_omt &river_end, tripoint_om_omt &current_point,
                            int river_scale );

        om_direction::type random_special_rotation( const overmap_special &special,
                const tripoint_om_omt &p, bool must_be_unexplored ) const;

        static bool is_amongst_locations( const oter_id &oter,
                                          const cata::flat_set<string_id<overmap_location>> &locations );

        bool can_place_special( const overmap_special &special, const tripoint_om_omt &p,
                                om_direction::type dir, bool must_be_unexplored ) const;

        std::vector<tripoint_om_omt> place_special(
            const overmap_special &special, const tripoint_om_omt &p, om_direction::type dir,
            const city &cit, bool must_be_unexplored, bool force );


        void debug_force_add_group( const mongroup &group );
        std::vector<std::reference_wrapper<mongroup>> debug_unsafe_get_groups_at( tripoint_abs_omt &loc );
    private:






        void place_specials( overmap_special_batch &enabled_specials );






        void place_specials_pass( overmap_special_batch &enabled_specials,
                                  om_special_sectors &sectors, bool place_optional, bool must_be_unexplored );







        bool place_special_attempt(
            overmap_special_batch &enabled_specials, const point_om_omt &sector, int sector_width,
            bool place_optional, bool must_be_unexplored );

        void place_mongroups();
        void place_radios();

        void add_mon_group( const mongroup &group );
        void add_mon_group( const mongroup &group, int radius );

        void spawn_mon_group( const mongroup &group, int radius );

        void load_monster_groups( const JsonArray &jsin );
        void load_legacy_monstergroups( const JsonArray &jsin );
        void save_monster_groups( JsonOut &jo ) const;
    public:
        static void load_oter_id_camp_migration( const JsonObject &jo );
        static void load_oter_id_migration( const JsonObject &jo );
        static void reset_oter_id_camp_migrations();
        static void reset_oter_id_migrations();
        static bool oter_id_should_have_camp( const oter_type_str_id &oter );
        static bool is_oter_id_obsolete( const std::string &oterid );
        void migrate_camps( const std::vector<tripoint_abs_omt> &points ) const;
        void migrate_oter_ids( const std::unordered_map<tripoint_om_omt, std::string> &points );
        oter_id get_or_migrate_oter( const std::string &oterid );
};



struct oter_display_lru {
    static constexpr size_t cache_size = 8;
    std::array<std::pair<oter_id, oter_t const *>, cache_size> cache;
    size_t cache_next = 0;

    std::pair<std::string, nc_color> get_symbol_and_color( const oter_id &cur_ter, om_vision_level );
};


struct oter_display_options {
    struct npc_coloring {
        nc_color color;
        size_t count = 0;
    };

    oter_display_options( tripoint_abs_omt orig, int sight_range ) :
        center( orig ), sight_points( sight_range ) {}



    bool blink = true;

    bool debug_scent = false;

    bool hilite_mission = false;

    bool hilite_pc = false;

    bool mission_inbounds = true;

    bool show_explored = true;
    bool showhordes = true;

    bool show_map_revealed = false;

    bool show_pc = true;

    bool show_weather = false;


    tripoint_abs_omt center;

    int sight_points;

    std::optional<tripoint_abs_omt> mission_target = std::nullopt;

    std::unordered_map<tripoint_abs_omt, npc_coloring> npc_color;

    std::unordered_set<tripoint_abs_omt> npc_path_route;
    std::unordered_map<point_abs_omt, int> player_path_route;


    std::string sZoneName;
    tripoint_abs_omt tripointZone = tripoint_abs_omt( -1, -1, -1 );


    mutable bool drawn_mission = false;
};


struct oter_display_args {

    explicit oter_display_args( om_vision_level vis ) : vision( vis ) {}


    om_vision_level vision;

    bool edge_tile = false;



    int los = -1;
    int los_sky = -1;
};




std::pair<std::string, nc_color> oter_symbol_and_color( const tripoint_abs_omt &omp,
        oter_display_args &args, const oter_display_options &opts, oter_display_lru *lru = nullptr );

bool is_river( const oter_id &ter );
bool is_water_body( const oter_id &ter );
bool is_water_body_not_shore( const oter_id &ter );
bool is_lake_or_river( const oter_id &ter );
bool is_ocean( const oter_id &ter );
bool is_road( const oter_id &ter );
bool is_highway( const oter_id &ter );
bool is_highway_reserved( const oter_id &ter );
bool is_highway_special( const oter_id &ter );








bool is_ot_match( const std::string &name, const oter_id &oter,
                  ot_match_type match_type );





om_special_sectors get_sectors( int sector_width );




std::string_view oter_no_dir( const oter_id &oter );




std::string_view oter_no_dir_or_connections( const oter_id &oter );





int oter_get_rotation( const oter_id &oter );




std::string oter_get_rotation_string( const oter_id &oter );
#endif
