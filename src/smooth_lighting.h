#pragma once
#ifndef CATA_SRC_SMOOTH_LIGHTING_H
#define CATA_SRC_SMOOTH_LIGHTING_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "map_scale_constants.h"
#include "point.h"

class JsonObject;
class map;
enum class lit_level : uint8_t;
struct ter_t;
struct light_color_rgb;

namespace smooth_lighting
{



struct light_cell {

    float light = 0.0f;

    bool detail = false;
};


bool needs_apparent_light( lit_level ll );



light_cell classify_light_cell( lit_level ll, float apparent_light, float vision_threshold );


float light_above_threshold( float apparent_light, float vision_threshold );


enum class quarter_turn : uint8_t {
    none,
    clockwise,
    counterclockwise,
};


constexpr int filter_reach = 2;



half_open_rectangle<point> lightmap_fill_area( const point &view_min, const point &view_max,
        const point &screen_min, const point &screen_max );




struct lightmap_extent {
    half_open_rectangle<point> area;
    int min_z = 0;
    int max_z = -1;
    bool covers( const tripoint_bub_ms &p ) const;
};


struct lightmap_fill_settings {
    half_open_rectangle<point> area;
    float vision_threshold = 0.0f;

    bool tint = false;

    bool masks = false;
    bool operator==( const lightmap_fill_settings &o ) const {
        return area.p_min == o.area.p_min && area.p_max == o.area.p_max &&
               vision_threshold == o.vision_threshold && tint == o.tint && masks == o.masks;
    }
};



struct layer_inputs {
    uint64_t lightmap_generation = 0;
    uint64_t visibility_generation = 0;
    uint64_t seen_generation = 0;
    uint64_t aim_generation = 0;

    bool aim_cone = false;
    bool operator==( const layer_inputs &o ) const {
        return lightmap_generation == o.lightmap_generation &&
               visibility_generation == o.visibility_generation &&
               seen_generation == o.seen_generation && aim_generation == o.aim_generation &&
               aim_cone == o.aim_cone;
    }
};





class lightmap_keys
{
    public:

        void begin_frame( const lightmap_fill_settings &settings );
        bool needs_fill( int z, const layer_inputs &inputs ) const;
        void mark_filled( int z, const layer_inputs &inputs );

        void forget( int z );
        void forget_all();
    private:
        std::optional<lightmap_fill_settings> settings_;
        std::array<std::optional<layer_inputs>, OVERMAP_LAYERS> filled_;
};



constexpr int reach_column = MAPSIZE_X;
constexpr int lightmap_width = 2 * MAPSIZE_X;
constexpr int lightmap_height = MAPSIZE_Y * OVERMAP_LAYERS;

constexpr int filter_window = 5;
static_assert( 2 * filter_reach + 1 <= filter_window, "reach mask covers the filter" );
static_assert( filter_window * filter_window <= 32, "reach mask holds one bit per cell" );

constexpr uint8_t texel_detail = 1;
constexpr uint8_t texel_barrier = 2;

constexpr float standing_marker = 0.5f;

struct lightmap_texel {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
    bool operator==( const lightmap_texel &o ) const {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
};
static_assert( sizeof( lightmap_texel ) == 4, "lightmap_texel is one RGBA32 texel" );

size_t light_index( const point &p );
size_t reach_index( const point &p );


lightmap_texel encode_light_texel( const light_cell &cell, const std::array<float, 3> &hue,
                                   bool barrier );

lightmap_texel encode_reach_texel( uint32_t mask );
uint32_t texel_reach( const lightmap_texel &t );

uint32_t reach_bit( const point &d );






std::array<float, 3> illumination_hue( const light_color_rgb &lc, float scalar_light );


struct barrier_grid {
    const lightmap_texel *texels = nullptr;
    int stride = 0;
    int width = 0;
    int height = 0;
    bool inside( const point &p ) const;

    bool barrier( const point &p ) const;
};




uint32_t reach_mask( const barrier_grid &grid, const point &at );



bool encode_lightmap_layer( const map &here, int z, const lightmap_fill_settings &settings,
                            std::vector<lightmap_texel> &out );



struct lightmap_view {
    const lightmap_texel *texels = nullptr;
    int width = 0;
    int height = 0;
    int rows_per_level = 0;
    int reach_column = 0;
};


struct lit_coords {
    float x = 0.0f;
    float y = 0.0f;
    float column = 0.0f;
    float row = 0.0f;
};



struct lit_vertex {
    float x = 0.0f;
    float y = 0.0f;
    lit_coords light;
    float u = 0.0f;
    float v = 0.0f;
};



struct lit_quad_params {
    std::array<float, 4> screen = {};
    std::array<float, 4> uv = {};
    bool flip_horizontal = false;
    bool flip_vertical = false;
    quarter_turn turn = quarter_turn::none;
    bool iso = false;
    float tile_width = 0.0f;
    float tile_height = 0.0f;

    float ground_x = 0.0f;
    float ground_y = 0.0f;
    tripoint_bub_ms pos;
    bool standing = false;
};



std::array<lit_vertex, 4> lit_quad( const lit_quad_params &p );

struct sample_params {
    bool per_tile = false;
    bool iso = false;
};


struct lit_sample {

    float light = 0.0f;

    std::array<float, 3> hue = { 1.0f, 1.0f, 1.0f };

    float in_sight = 0.0f;

    float visible = 0.0f;

    float weight = 0.0f;
};


lit_sample reference_sample( const lightmap_view &view, const sample_params &params,
                             const lit_coords &coords );



struct filter_result {
    float light = 0.0f;

    std::array<float, 3> chroma = { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f };

    float in_sight = 0.0f;

    float weight = 0.0f;
};


filter_result reference_filter( const lightmap_view &view, const point &own, int level, float lx,
                                float ly );
lit_sample finish_filter( const filter_result &r );



constexpr int prefilter_grid = 10;

constexpr float prefilter_error_budget = 2.0f;




struct prefilter_layout {

    half_open_rectangle<point> area;

    int first_level = 0;
    int levels = 0;
    int grid = prefilter_grid;

    point size() const;
    point block_origin( const point &cell, int level ) const;

    bool holds( const point &cell, int level ) const;
    bool operator==( const prefilter_layout &other ) const;
};

bool prefilter_fits( const prefilter_layout &layout, int max_texture_size, bool format_supported );


point prefilter_texture_size( const point &requested, const point &current, int max_texture_size );



std::optional<half_open_rectangle<point>> seen_box( const lightmap_texel *level, int stride,
                                       const half_open_rectangle<point> &area );


prefilter_layout prefilter_layout_for( const std::optional<half_open_rectangle<point>> &seen,
                                       const half_open_rectangle<point> &clip, int first_level, int levels );


float half_round( float x );



struct prefilter_lookup {
    std::array<point, 4> texels;
    float wx = 0.0f;
    float wy = 0.0f;
};
prefilter_lookup prefilter_lookup_texels( const prefilter_layout &layout, const point &cell,
        int level, float lx, float ly );



struct prefilter_table {
    prefilter_layout layout;
    std::vector<std::array<float, 4>> texels;

    bool quantize_weights = false;
};
prefilter_table build_prefilter_table( const lightmap_view &view, const prefilter_layout &layout );

lit_sample reference_prefiltered_sample( const prefilter_table &table, const lightmap_view &view,
        const sample_params &params, const lit_coords &coords );


constexpr float full_color_light = 0.75f;


constexpr float tint_mix = 0.4f;


constexpr float shadow_shade = 0.625f;

constexpr float night_floor = 0.55f;


constexpr float overexpose_start = 0.85f;


struct look_params {

    int memory_look = 0;
    bool blend_memory = false;
    std::array<float, 3> custom_dark = {};
    std::array<float, 3> custom_light = {};
    float custom_gamma = 1.0f;
};

constexpr int custom_look = 4;



std::array<float, 3> reference_memory_rgb( const look_params &look,
        const std::array<float, 3> &rgb );
std::array<float, 3> reference_lit_rgb( const look_params &look, const std::array<float, 3> &rgb,
                                        const lit_sample &s );
std::array<float, 3> reference_night_rgb( const look_params &look,
        const std::array<float, 3> &rgb, const lit_sample &s );


enum class lit_failure : uint8_t {
    texture_create,
    upload,
    sampler_create,
    shader_load,
    state_create,
    uniform_upload,
    probe_mismatch,

    prefilter,
};
const char *to_string( lit_failure f );


enum class failure_scope : uint8_t {
    all,
    filtered,
};


failure_scope scope_of( lit_failure f, bool frame_filtered );



class failure_policy
{
    public:


        explicit failure_policy( lit_failure retried = lit_failure::upload ) : retried_( retried ) {}

        static constexpr int upload_retries = 3;

        bool fail( lit_failure f );

        void succeed();

        void rebuilt( uint32_t generation );
        void reset();
        bool latched() const {
            return latched_.has_value();
        }
        std::optional<lit_failure> latched_by() const {
            return latched_;
        }
    private:
        lit_failure retried_;
        std::optional<lit_failure> latched_;
        int upload_streak_ = 0;
        uint32_t generation_ = 0;
};


enum class lighting_status : uint8_t {
    classic_by_option,
    classic_no_shader_path,
    classic_failed,
    classic_this_frame,
    smooth,
    smooth_filtered,


    smooth_filtered_unavailable,
};
const char *to_string( lighting_status s );



enum class lookup : uint8_t {
    hardware,
    manual,
};


enum class probe_group_result : uint8_t {
    passed,
    mismatch,

    unsafe,
    skipped,
};


struct lit_capability {

    bool unsafe = false;
    bool per_tile = false;
    std::optional<lookup> filtered;
};


lit_capability with_linear_sampler( lit_capability probed, bool linear_sampler );

lit_capability decide_lit_capability( probe_group_result per_tile,
                                      probe_group_result filtered_hardware, probe_group_result filtered_manual );


struct lit_mode {
    bool per_tile = true;
    lighting_status status = lighting_status::smooth;
};
lit_mode choose_lit_mode( bool want_filtered, const lit_capability &capability,
                          bool filtered_latched, bool prefilter_fits );


struct prefilter_inputs {
    uint64_t upload_generation = 0;
    prefilter_layout layout;
    lookup mode = lookup::hardware;
    uint32_t resource_generation = 0;
    bool operator==( const prefilter_inputs &other ) const;
};



class prefilter_cache
{
    public:
        bool needs_run( const prefilter_inputs &in ) const;
        void begin_write();
        void publish( const prefilter_inputs &in );
    private:
        std::optional<prefilter_inputs> published_;
};


enum class lit_frame_action : uint8_t {
    draw_lit,
    draw_classic,
    abort_frame,
};



bool lit_path_for( bool lighting_active, bool wants_scene_light, bool cell_covered );



constexpr float standing_rise = 0.125f;

struct sprite_footprint {


    point size;
    half_open_rectangle<point> opaque;

    bool flip_horizontal = false;
    bool flip_vertical = false;
    quarter_turn turn = quarter_turn::none;

    float pixelscale = 1.0f;


    int top = 0;
};


struct tile_geometry {
    int width = 0;
    int height = 0;
    bool iso = false;
};



bool sprite_stands( const sprite_footprint &f, const tile_geometry &g );


enum class light_anchor : uint8_t {

    ground,

    base,
};



std::optional<light_anchor> read_light_anchor( const JsonObject &entry );



light_anchor terrain_light_anchor( const ter_t &t );



std::optional<light_anchor> chosen_light_anchor( std::optional<light_anchor> tile_anchor,
        std::optional<light_anchor> default_anchor );

}

#endif
