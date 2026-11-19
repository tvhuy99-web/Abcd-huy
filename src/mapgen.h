#pragma once
#ifndef CATA_SRC_MAPGEN_H
#define CATA_SRC_MAPGEN_H

#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cata_variant.h"
#include "coordinates.h"
#include "dialogue_helpers.h"
#include "enum_bitset.h"
#include "flexbuffer_json.h"
#include "jmapgen_flags.h"
#include "mapgen_parameter.h"
#include "mapgen_primitives.h"
#include "memory_fast.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"
#include "weighted_list.h"

// IWYU pragma: no_forward_declare jmapgen_flags
class map;
class mapgendata;
class mission;
class tinymap;
struct mapgen_arguments;
struct oter_t;
template <typename E> struct enum_traits;
template <typename Id> class mapgen_value;

using building_gen_pointer = void ( * )( mapgendata & );





class mapgen_function
{
    public:
        dbl_or_var weight;
    protected:
        explicit mapgen_function( dbl_or_var w ) : weight( std::move( w ) ) { }
        explicit mapgen_function( int w ) : weight( w ) { }
    public:
        virtual ~mapgen_function() = default;
        virtual void setup() { }
        virtual void finalize_parameters() { }
        virtual void check() const { }
        virtual void check_consistent_with( const oter_t & ) const { }
        virtual bool expects_predecessor() const {
            return false;
        }
        virtual void generate( mapgendata & ) = 0;
        virtual mapgen_parameters get_mapgen_params( mapgen_parameter_scope ) const {
            return {};
        }
};



class mapgen_function_builtin : public virtual mapgen_function
{
    public:
        building_gen_pointer fptr;
        explicit mapgen_function_builtin( building_gen_pointer ptr,
                                          int w = 1000 ) : mapgen_function( w ),
            fptr( ptr ) { }
        explicit mapgen_function_builtin( building_gen_pointer ptr,
                                          dbl_or_var w ) : mapgen_function( std::move( w ) ),
            fptr( ptr ) { }
        void generate( mapgendata &mgd ) override;
};






enum class mapgen_phase {
    removal,
    terrain,
    furniture,
    default_,
    nested_mapgen,
    transform,
    faction_ownership,
    zones,
    last
};

template<>
struct enum_traits<mapgen_phase> {
    static constexpr mapgen_phase last = mapgen_phase::last;
};

inline bool operator<( const mapgen_phase l, const mapgen_phase r )
{
    return static_cast<int>( l ) < static_cast<int>( r );
}

enum jmapgen_setmap_op {
    JMAPGEN_SETMAP_OPTYPE_POINT = 0,
    JMAPGEN_SETMAP_TER,
    JMAPGEN_SETMAP_FURN,
    JMAPGEN_SETMAP_TRAP,
    JMAPGEN_SETMAP_RADIATION,
    JMAPGEN_SETMAP_TRAP_REMOVE,
    JMAPGEN_SETMAP_CREATURE_REMOVE,
    JMAPGEN_SETMAP_CREATURE_KILL,
    JMAPGEN_SETMAP_ITEM_REMOVE,
    JMAPGEN_SETMAP_FIELD_REMOVE,
    JMAPGEN_SETMAP_BASH,
    JMAPGEN_SETMAP_BURN,
    JMAPGEN_SETMAP_VARIABLE,
    JMAPGEN_SETMAP_OPTYPE_LINE = 100,
    JMAPGEN_SETMAP_LINE_TER,
    JMAPGEN_SETMAP_LINE_FURN,
    JMAPGEN_SETMAP_LINE_TRAP,
    JMAPGEN_SETMAP_LINE_RADIATION,
    JMAPGEN_SETMAP_LINE_TRAP_REMOVE,
    JMAPGEN_SETMAP_LINE_CREATURE_REMOVE,
    JMAPGEN_SETMAP_LINE_CREATURE_KILL,
    JMAPGEN_SETMAP_LINE_ITEM_REMOVE,
    JMAPGEN_SETMAP_LINE_FIELD_REMOVE,
    JMAPGEN_SETMAP_OPTYPE_SQUARE = 200,
    JMAPGEN_SETMAP_SQUARE_TER,
    JMAPGEN_SETMAP_SQUARE_FURN,
    JMAPGEN_SETMAP_SQUARE_TRAP,
    JMAPGEN_SETMAP_SQUARE_RADIATION,
    JMAPGEN_SETMAP_SQUARE_TRAP_REMOVE,
    JMAPGEN_SETMAP_SQUARE_CREATURE_REMOVE,
    JMAPGEN_SETMAP_SQUARE_CREATURE_KILL,
    JMAPGEN_SETMAP_SQUARE_ITEM_REMOVE,
    JMAPGEN_SETMAP_SQUARE_FIELD_REMOVE
};

struct jmapgen_setmap {
    jmapgen_int x;
    jmapgen_int y;
    jmapgen_int z;
    jmapgen_int x2;
    jmapgen_int y2;
    jmapgen_setmap_op op;
    jmapgen_int val;
    int chance;
    jmapgen_int repeat;
    std::string string_val;
    jmapgen_setmap(
        jmapgen_int ix, jmapgen_int iy, jmapgen_int iz, jmapgen_int ix2, jmapgen_int iy2,
        jmapgen_setmap_op iop, jmapgen_int ival,
        int ione_in = 1, jmapgen_int irepeat = jmapgen_int( 1, 1 ), std::string istring_val = ""
    ) :
        x( ix ), y( iy ), z( iz ), x2( ix2 ), y2( iy2 ), op( iop ), val( ival ), chance( ione_in ),
        repeat( irepeat ), string_val( std::move( istring_val ) ) {}

    mapgen_phase phase() const;

    bool apply( const mapgendata &dat, const tripoint_rel_ms &offset ) const;





    ret_val<void> has_vehicle_collision( const mapgendata &dat, const tripoint_rel_ms &offset ) const;
};






















class jmapgen_piece
{
    protected:
        jmapgen_piece() : repeat( 1, 1 ) { }
    public:
        virtual bool is_nop() const {
            return false;
        }
        virtual mapgen_phase phase() const {
            return mapgen_phase::default_;
        }

        virtual void check( const std::string &, const mapgen_parameters &,
                            const jmapgen_int &, const jmapgen_int &, const jmapgen_int & ) const { }

        virtual void merge_parameters_into( mapgen_parameters &,
                                            const std::string & ) const {}


        virtual void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y,
                            const jmapgen_int &z,
                            const std::string &context ) const = 0;
        virtual ~jmapgen_piece() = default;
        jmapgen_int repeat;
        virtual ret_val<void> has_vehicle_collision( const mapgendata &,
                const tripoint_rel_ms & ) const {
            return ret_val<void>::make_success();
        }
};

class jmapgen_piece_with_has_vehicle_collision : public jmapgen_piece
{
    public:
        ret_val<void> has_vehicle_collision( const mapgendata &dat,
                                             const tripoint_rel_ms &p ) const override;
};





class jmapgen_place
{
    public:
        jmapgen_place() : x( 0, 0 ), y( 0, 0 ), z( 0, 0 ), repeat( 1, 1 ) { }
        explicit jmapgen_place( const tripoint_rel_ms &p ) : x( p.x() ), y( p.y() ), z( p.z() ), repeat( 1,
                    1 ) { }
        explicit jmapgen_place( const JsonObject &jsi );
        void offset( const tripoint_rel_ms & );
        jmapgen_int x;
        jmapgen_int y;
        jmapgen_int z;
        jmapgen_int repeat;
};




class map_key
{
    public:
        explicit map_key( const std::string & );
        explicit map_key( const JsonMember & );

        friend bool operator==( const map_key &l, const map_key &r ) {
            return l.str == r.str;
        }

        std::string str;
};

namespace std
{
template<>
struct hash<map_key> {
    size_t operator()( const map_key &k ) const noexcept {
        return hash<std::string> {}( k.str );
    }
};
}

template<typename T>
struct mapgen_constraint {
    mapgen_constraint( const std::string &name, const T &val )
        : parameter_name( name )
        , value( val )
    {}

    std::string parameter_name;
    T value;
};

namespace debug_palettes
{

void debug_view_all_palettes();
}

class mapgen_palette
{
    public:
        palette_id id;






        using placing_map =
            std::unordered_map<map_key, std::vector< shared_ptr_fast<const jmapgen_piece>>>;

        std::unordered_set<map_key> keys_with_terrain;
        placing_map format_placings;

        template<typename PieceType>




        void load_place_mapings( const JsonObject &jo, const std::string &member_name,
                                 placing_map &format_placings, const std::string &context );

        void check() const;

        const mapgen_parameters &get_parameters() const {
            return parameters;
        }




        static mapgen_palette load_temp( const JsonObject &jo, std::string_view src,
                                         const std::string &context );




        static void load( const JsonObject &jo, std::string_view src );




        static const mapgen_palette &get( const palette_id &id );

        static void check_definitions();

        static void reset();
    private:
        mapgen_parameters parameters;




        std::vector<mapgen_value<std::string>> palettes_used;

        static mapgen_palette load_internal(
            const JsonObject &jo, std::string_view src, const std::string &context,
            bool require_id, bool allow_recur );

        struct add_palette_context {
            add_palette_context( const std::string &ctx, mapgen_parameters * );

            std::string context;
            std::vector<palette_id> ancestors;
            mapgen_parameters *top_level_parameters;
            const mapgen_parameters *current_parameters;
            std::vector<mapgen_constraint<palette_id>> constraints;
        };









        void add( const mapgen_value<std::string> &rh, const add_palette_context & );
        void add( const palette_id &rh, const add_palette_context & );
        void add( const mapgen_palette &rh, const add_palette_context & );
};

using terrain_coord_map = std::map<point_rel_ms, std::set<ter_str_id>>;

struct jmapgen_objects {

        jmapgen_objects( const tripoint_rel_ms &offset, const point_rel_ms &mapsize,
                         const point_rel_ms &tot_size );

        bool check_bounds( const jmapgen_place &place, const JsonObject &jso );

        void add( const jmapgen_place &place, const shared_ptr_fast<const jmapgen_piece> &piece );






        template<typename PieceType>
        void load_objects( const JsonArray &parray, std::string_view context );





        template<typename PieceType>
        void load_objects( const JsonObject &jsi, const std::string &member_name,
                           const std::string &context );

        void finalize();
        void check( const std::string &context, const mapgen_parameters & ) const;

        void merge_parameters_into( mapgen_parameters &, const std::string &outer_context ) const;

        void add_placement_coords_to( std::unordered_set<point_rel_ms> & ) const;

        bool has_terrain_placements( const mapgen_parameters &params ) const;
        bool any_nested_has_unguarded_terrain_for_furniture(
            const mapgen_parameters &params, int depth_limit ) const;
        void collect_terrain_coords( std::set<point_rel_ms> &coords,
                                     const mapgen_parameters &params, int depth_limit ) const;
        void collect_terrain_data( terrain_coord_map &data,
                                   const mapgen_parameters &params, int depth_limit ) const;
        void collect_furniture_coords( std::set<point_rel_ms> &coords,
                                       const mapgen_parameters &params, int depth_limit ) const;
        void check_nested_overlaps(
            const std::string &context,
            const mapgen_parameters &parameters,
            const std::set<point_rel_ms> &extra_furn_positions,
            const terrain_coord_map &parent_explicit_terrain,
            const mapgen_value<ter_id> *fill_ter ) const;

        void apply( const mapgendata &dat, mapgen_phase, const std::string &context ) const;
        void apply( const mapgendata &dat, mapgen_phase, const tripoint_rel_ms &offset,
                    const std::string &context ) const;





        ret_val<void> has_vehicle_collision( const mapgendata &dat, const tripoint_rel_ms &offset ) const;

        void collect_terrain_coords_for_sibling( std::set<point_rel_ms> &coords,
                const mapgen_parameters &params ) const;
    private:



        using jmapgen_obj = std::pair<jmapgen_place, shared_ptr_fast<const jmapgen_piece> >;
        std::vector<jmapgen_obj> objects;
        tripoint_rel_ms m_offset;
        point_rel_ms mapgensize;
        point_rel_ms total_size;
};

class mapgen_function_json_base
{
    public:
        void merge_non_nest_parameters_into( mapgen_parameters &,
                                             const std::string &outer_context ) const;
        bool check_inbounds( const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                             const JsonObject &jso ) const;
        size_t calc_index( const point_rel_ms &p ) const;
        ret_val<void> has_vehicle_collision( const mapgendata &dat, const tripoint_rel_ms &offset ) const;

        void add_placement_coords_to( std::unordered_set<point_rel_ms> & ) const;

        bool has_furniture_clearing_flags() const;
        bool reliably_clears_furniture() const;
        point_rel_ms get_mapgensize() const;
        bool has_unguarded_terrain_for_furniture( int depth_limit = 10 ) const;
        void collect_terrain_coords( std::set<point_rel_ms> &coords,
                                     int depth_limit = 10 ) const;
        void collect_terrain_data( terrain_coord_map &data,
                                   int depth_limit = 10 ) const;
        void collect_furniture_coords( std::set<point_rel_ms> &coords,
                                       int depth_limit = 10 ) const;
        void collect_setmap_terrain( terrain_coord_map &data ) const;

        virtual const mapgen_value<ter_id> *get_fill_ter() const {
            return nullptr;
        }

        const mapgen_parameters &get_parameters() const {
            return parameters;
        }


        const jmapgen_objects &jmapgen_objects_for_check() const {
            return objects;
        }

    private:
        JsonObject jsobj;
    protected:
        mapgen_function_json_base( JsonObject &&jsobj, const std::string &context );
        virtual ~mapgen_function_json_base();

        void setup_common();
        bool setup_common( const JsonObject &jo );
        void setup_setmap( const JsonArray &parray );

        virtual bool setup_internal( const JsonObject &jo ) = 0;
        virtual void setup_setmap_internal() { }
        void finalize_parameters_common();

        void check_common() const;

        mapgen_arguments get_args( const mapgendata &md, mapgen_parameter_scope ) const;

        std::string context_;
        enum_bitset<jmapgen_flags> flags_;
        bool is_ready;

        point_rel_ms mapgensize;
        tripoint_rel_ms m_offset;
        point_rel_ms total_size;
        std::vector<jmapgen_setmap> setmap_points;

        jmapgen_objects objects;

        mapgen_parameters parameters;
};

class mapgen_function_json : public mapgen_function_json_base, public virtual mapgen_function
{
    public:
        void setup() override;
        void finalize_parameters() override;
        void check() const override;
        void check_consistent_with( const oter_t & ) const override;
        bool expects_predecessor() const override;
        void generate( mapgendata & ) override;
        mapgen_parameters get_mapgen_params( mapgen_parameter_scope ) const override;
        mapgen_function_json( JsonObject &&jsobj, dbl_or_var w,
                              const std::string &context,
                              const point_rel_omt &grid_offset, const point_rel_omt &grid_total );
        ~mapgen_function_json() override = default;

        cata::value_ptr<mapgen_value<ter_id>> fill_ter;
        oter_id predecessor_mapgen;

        const mapgen_value<ter_id> *get_fill_ter() const override {
            return fill_ter.get();
        }

    protected:
        bool setup_internal( const JsonObject &jo ) override;

    private:
        jmapgen_int rotation;
        oter_str_id fallback_predecessor_mapgen_;
};

class update_mapgen_function_json : public mapgen_function_json_base
{
    public:
        update_mapgen_function_json( JsonObject &&jsobj, const std::string &context );
        ~update_mapgen_function_json() override = default;

        void setup();
        bool setup_update( const JsonObject &jo );
        void finalize_parameters();
        void check() const;

        ret_val<void> update_map(
            const tripoint_abs_omt &omt_pos, const mapgen_arguments &, const tripoint_rel_ms &offset,
            mission *miss, bool verify = false, bool mirror_horizontal = false,
            bool mirror_vertical = false, int rotation = 0 ) const;

        ret_val<void> update_map( const mapgendata &md,
                                  const tripoint_rel_ms &offset = tripoint_rel_ms::zero,
                                  bool verify = false ) const;

    protected:
        bool setup_internal( const JsonObject & ) override;
        cata::value_ptr<mapgen_value<ter_id>> fill_ter;
};

class mapgen_function_json_nested : public mapgen_function_json_base
{
    public:
        void setup();
        void finalize_parameters();
        void check() const;
        mapgen_function_json_nested( JsonObject &&jsobj, const std::string &context );
        ~mapgen_function_json_nested() override = default;

        void nest( const mapgendata &md, const tripoint_rel_ms &offset,
                   const std::string &outer_context ) const;
    protected:
        bool setup_internal( const JsonObject &jo ) override;

    private:
        jmapgen_int rotation;
};

class nested_mapgen
{
    public:
        const weighted_int_list<std::shared_ptr<mapgen_function_json_nested>> &funcs() const {
            return funcs_;
        }
        void add( const std::shared_ptr<mapgen_function_json_nested> &p, int weight );


        std::unordered_set<point_rel_ms> all_placement_coords() const;
    private:
        weighted_int_list<std::shared_ptr<mapgen_function_json_nested>> funcs_;
};

class update_mapgen
{
    public:
        const std::vector<std::unique_ptr<update_mapgen_function_json>> &funcs() const {
            return funcs_;
        }
        void add( std::unique_ptr<update_mapgen_function_json> &&p );
    private:
        std::vector<std::unique_ptr<update_mapgen_function_json>> funcs_;
};






std::shared_ptr<mapgen_function> load_mapgen_function( const JsonObject &jio,
        const std::string &id_base, const point_rel_omt &offset, const point_rel_omt &total );
void load_and_add_mapgen_function(
    const JsonObject &jio, const std::string &id_base, const point_rel_omt &offset,
    const point_rel_omt &total );




void load_mapgen( const JsonObject &jo );
void reset_mapgens();






int register_mapgen_function( const std::string &key );



bool has_mapgen_for( const std::string &key );



void check_mapgen_consistent_with( const std::string &key, const oter_t & );



bool has_update_mapgen_for( const update_mapgen_id & );



void calculate_mapgen_weights();
#if defined(__ANDROID__)


void set_android_deferred_oter_mapgen_setup( bool enabled );
#endif

void check_mapgen_definitions();


enum room_type {
    room_null,
    room_closet,
    room_lobby,
    room_chemistry,
    room_teleport,
    room_goo,
    room_cloning,
    room_vivisect,
    room_bionics,
    room_dorm,
    room_living,
    room_bathroom,
    room_kitchen,
    room_bedroom,
    room_backyard,
    room_study,
    room_split
};


bool connects_to( const oter_id &there, int dir );

void line( map *m, const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z );
void line( tinymap *m, const ter_id &type, const point_omt_ms &p1, const point_omt_ms &p2 );
void line_furn( map *m, const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2,
                int z );
void line_furn( tinymap *m, const furn_id &type, const point_omt_ms &p1, const point_omt_ms &p2 );
void fill_background( map *m, const ter_id &type );
void fill_background( map *m, ter_id( *f )() );
void square( map *m, const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2 );
void square( map *m, ter_id( *f )(), const point_bub_ms &p1, const point_bub_ms &p2 );
void square( map *m, const weighted_int_list<ter_id> &f, const point_bub_ms &p1,
             const point_bub_ms &p2 );
void square_furn( map *m, const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2 );
void square_furn( tinymap *m, const furn_id &type, const point_omt_ms &p1, const point_omt_ms &p2 );
void rough_circle( map *m, const ter_id &type, const point_bub_ms &, int rad );
void rough_circle_furn( map *m, const furn_id &type, const point_bub_ms &, int rad );
void circle( map *m, const ter_id &type, double x, double y, double rad );
void circle( map *m, const ter_id &type, const point_bub_ms &, int rad );
void circle_furn( map *m, const furn_id &type, const point_bub_ms &, int rad );
void add_corpse( map *m, const point_bub_ms & );

extern std::map<nested_mapgen_id, nested_mapgen> nested_mapgens;
extern std::map<update_mapgen_id, update_mapgen> update_mapgens;

#endif
