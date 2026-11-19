#include <string_view>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "coordinates.h"
#include "game.h"
#include "level_cache.h"
#include "lightmap.h"
#include "line.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "mdarray.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "shadowcasting.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather_type.h"

static const dimension_id dimension_world_default( "default" );
static const dimension_id dimension_world_nether( "nether" );

static const field_type_str_id field_fd_test_green_glow( "fd_test_green_glow" );
static const ter_str_id ter_t_brick_wall( "t_brick_wall" );
static const ter_str_id ter_t_door_o( "t_door_o" );
static const ter_str_id ter_t_flat_roof( "t_flat_roof" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_test_blue_light( "t_test_blue_light" );
static const ter_str_id ter_t_test_red_light( "t_test_red_light" );
static const ter_str_id ter_t_wall( "t_wall" );
static const ter_str_id ter_test_t_utility_light( "test_t_utility_light" );
static const vpart_id vpart_test_red_headlight( "test_red_headlight" );
static const vproto_id vehicle_prototype_car( "car" );

static const time_point midnight = calendar::turn_zero + 0_hours;


static void rebuild_lightmap( int zlev )
{
    map &here = get_map();
    for( int z = -2; z <= OVERMAP_HEIGHT; z++ ) {
        here.invalidate_map_cache( z );
    }
    here.invalidate_visibility_cache();
    here.build_map_cache( zlev );
    here.invalidate_visibility_cache();
    here.update_visibility_cache( zlev );
    here.build_map_cache( zlev );
}


static void setup_dark_map()
{
    clear_avatar();
    clear_map_without_vision( -2, OVERMAP_HEIGHT );
    g->reset_light_level();
    calendar::turn = midnight;
    get_player_character().recalc_sight_limits();
}


static light_color_rgb get_light_color_at( const tripoint_bub_ms &pos )
{
    const map &here = get_map();
    const level_cache &cache = here.access_cache( pos.z() );
    return cache.light_color_cache[pos.x()][pos.y()];
}


static void place_ter_roofed( const tripoint_bub_ms &pos, const ter_str_id &ter )
{
    map &here = get_map();
    here.ter_set( pos, ter.id() );
    here.ter_set( pos + tripoint::above, ter_t_flat_roof.id() );
}

TEST_CASE( "colored_light_source_populates_cache", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_t_test_red_light );

    rebuild_lightmap( 0 );

    const light_color_rgb color = get_light_color_at( src );
    CHECK( color.r > 0.0f );
    CHECK( color.g == 0.0f );
    CHECK( color.b == 0.0f );
}

TEST_CASE( "colored_light_attenuates_with_distance", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 5;
    place_ter_roofed( src, ter_t_test_red_light );


    for( int dx = -4; dx <= 4; dx++ ) {
        for( int dy = -4; dy <= 4; dy++ ) {
            tripoint_bub_ms p = src + tripoint( dx, dy, 0 );
            if( p != src ) {
                place_ter_roofed( p, ter_t_floor );
            }
        }
    }

    rebuild_lightmap( 0 );



    const float r_at_1 = get_light_color_at( src + tripoint::east ).r;
    const float r_at_2 = get_light_color_at( src + tripoint::east * 2 ).r;
    const float r_at_3 = get_light_color_at( src + tripoint::east * 3 ).r;

    CHECK( r_at_1 > 0.0f );
    CHECK( r_at_1 > r_at_2 );
    CHECK( r_at_2 > r_at_3 );
    CHECK( r_at_3 > 0.0f );
}

TEST_CASE( "wall_blocks_colored_light", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_t_test_red_light );


    const tripoint_bub_ms wall_pos = src + tripoint::east;
    place_ter_roofed( wall_pos, ter_t_brick_wall );


    const tripoint_bub_ms target = src + tripoint::east * 2;
    place_ter_roofed( target, ter_t_floor );

    rebuild_lightmap( 0 );

    const light_color_rgb behind_wall = get_light_color_at( target );
    CHECK( behind_wall.r == 0.0f );
    CHECK( behind_wall.g == 0.0f );
    CHECK( behind_wall.b == 0.0f );
}

TEST_CASE( "two_colored_sources_both_contribute", "[light_color]" )
{

    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms center = get_player_character().pos_bub() + tripoint::east * 5;


    const tripoint_bub_ms red_pos = center + tripoint::west * 3;
    place_ter_roofed( red_pos, ter_t_test_red_light );


    const tripoint_bub_ms blue_pos = center + tripoint::east * 3;
    place_ter_roofed( blue_pos, ter_t_test_blue_light );


    for( int dx = -5; dx <= 5; dx++ ) {
        for( int dy = -2; dy <= 2; dy++ ) {
            tripoint_bub_ms p = center + tripoint( dx, dy, 0 );
            if( p != red_pos && p != blue_pos ) {
                place_ter_roofed( p, ter_t_floor );
            }
        }
    }

    rebuild_lightmap( 0 );

    const light_color_rgb mid = get_light_color_at( center );
    CHECK( mid.r > 0.0f );
    CHECK( mid.b > 0.0f );
}

TEST_CASE( "overlapping_same_color_sources_accumulate", "[light_color]" )
{



    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms center = get_player_character().pos_bub() + tripoint::east * 5;


    place_ter_roofed( center + tripoint::west * 2, ter_t_test_red_light );
    for( int dx = -3; dx <= 3; dx++ ) {
        for( int dy = -3; dy <= 3; dy++ ) {
            tripoint_bub_ms p = center + tripoint( dx, dy, 0 );
            if( get_map().ter( p ) != ter_t_test_red_light.id() ) {
                place_ter_roofed( p, ter_t_floor );
            }
        }
    }
    rebuild_lightmap( 0 );
    const float single_r = get_light_color_at( center ).r;
    CHECK( single_r > 0.0f );


    place_ter_roofed( center + tripoint::east * 2, ter_t_test_red_light );
    rebuild_lightmap( 0 );
    const float double_r = get_light_color_at( center ).r;


    CHECK( double_r > single_r );
}

TEST_CASE( "uncolored_source_produces_no_color", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_test_t_utility_light );

    rebuild_lightmap( 0 );

    const light_color_rgb color = get_light_color_at( src );
    CHECK( color.r == 0.0f );
    CHECK( color.g == 0.0f );
    CHECK( color.b == 0.0f );
}

TEST_CASE( "colored_light_footprint_matches_scalar", "[light_color]" )
{



    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 5;
    place_ter_roofed( src, ter_t_test_red_light );


    const tripoint_bub_ms control = get_player_character().pos_bub() + tripoint::west * 10;
    place_ter_roofed( control, ter_t_floor );

    rebuild_lightmap( 0 );

    const map &here = get_map();
    const level_cache &cache = here.access_cache( 0 );
    const float ambient = cache.lm[control.x()][control.y()].max();




    int tiles_with_both = 0;
    for( int dx = -4; dx <= 4; dx++ ) {
        for( int dy = -4; dy <= 4; dy++ ) {
            const point p( src.x() + dx, src.y() + dy );
            if( p.x < 0 || p.x >= MAPSIZE_X || p.y < 0 || p.y >= MAPSIZE_Y ) {
                continue;
            }
            const float scalar = cache.lm[p.x][p.y].max();
            const bool has_color = cache.light_color_cache[p.x][p.y].is_colored();


            if( scalar > ambient + 1.0f ) {
                CAPTURE( dx, dy, scalar, ambient, cache.light_color_cache[p.x][p.y].r );
                CHECK( has_color );
                tiles_with_both++;
            }
        }
    }

    CHECK( tiles_with_both > 10 );
}

TEST_CASE( "light_color_cache_clears_on_rebuild", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_t_test_red_light );

    rebuild_lightmap( 0 );
    CHECK( get_light_color_at( src ).r > 0.0f );


    place_ter_roofed( src, ter_t_floor );

    rebuild_lightmap( 0 );
    CHECK( get_light_color_at( src ).r == 0.0f );
}

TEST_CASE( "field_colored_light_propagates", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_t_floor );


    map &here = get_map();
    here.add_field( src, field_fd_test_green_glow.id(), 1 );

    rebuild_lightmap( 0 );

    const light_color_rgb color = get_light_color_at( src );
    CHECK( color.g > 0.0f );
    CHECK( color.r == 0.0f );
    CHECK( color.b == 0.0f );


    const light_color_rgb adjacent = get_light_color_at( src + tripoint::east );
    CHECK( adjacent.g > 0.0f );
}

TEST_CASE( "vehicle_cone_light_carries_color", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    map &here = get_map();
    const tripoint_bub_ms veh_pos = get_player_character().pos_bub() + tripoint::east * 5;

    for( int dx = -10; dx <= 10; dx++ ) {
        for( int dy = -10; dy <= 10; dy++ ) {
            place_ter_roofed( veh_pos + tripoint( dx, dy, 0 ), ter_t_floor );
        }
    }

    vehicle *veh = here.add_vehicle( vehicle_prototype_car, veh_pos, 0_degrees, 100,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );


    std::vector<point_rel_ms> hl_mounts;
    for( const vpart_reference &vpr : veh->get_any_parts( VPFLAG_CONE_LIGHT ) ) {
        hl_mounts.push_back( vpr.part().mount );
        veh->remove_part( vpr.part() );
    }
    veh->part_removal_cleanup( here );
    REQUIRE( !hl_mounts.empty() );

    const int part_idx = veh->install_part( here, hl_mounts.front(), vpart_test_red_headlight );
    REQUIRE( part_idx >= 0 );

    vehicle_part &installed = veh->part( part_idx );
    installed.enabled = true;
    REQUIRE( installed.info().has_flag( VPFLAG_CONE_LIGHT ) );

    rebuild_lightmap( 0 );




    const tripoint_bub_ms hl_pos = veh->bub_part_pos( here, installed );
    const level_cache &cache = here.access_cache( 0 );
    float best_r = 0.0f;
    tripoint best_offset;
    for( int dx = -6; dx <= 6; dx++ ) {
        for( int dy = -6; dy <= 6; dy++ ) {
            const tripoint_bub_ms p = hl_pos + tripoint( dx, dy, 0 );
            if( !here.inbounds( p ) ) {
                continue;
            }
            const float r = cache.light_color_cache[p.x()][p.y()].r;
            if( r > best_r ) {
                best_r = r;
                best_offset = tripoint( dx, dy, 0 );
            }
        }
    }
    REQUIRE( best_r > 0.0f );


    const tripoint_bub_ms behind = hl_pos - best_offset;
    REQUIRE( here.inbounds( behind ) );
    const light_color_rgb behind_color = get_light_color_at( behind );
    CAPTURE( best_r, best_offset.x, best_offset.y );
    CAPTURE( behind_color.r, behind_color.g, behind_color.b );

    CHECK( best_r > behind_color.r );
}

TEST_CASE( "fused_color_output_matches_golden_values", "[light_color]" )
{



    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 5;

    for( int dx = -6; dx <= 6; dx++ ) {
        for( int dy = -6; dy <= 6; dy++ ) {
            place_ter_roofed( src + tripoint( dx, dy, 0 ), ter_t_floor );
        }
    }
    place_ter_roofed( src, ter_t_test_red_light );

    rebuild_lightmap( 0 );

    const map &here = get_map();
    const level_cache &cache = here.access_cache( 0 );



    struct golden_entry {
        point d;
        float r;
        float lm;
    };
    // NOLINTBEGIN(cata-use-named-point-constants)
    static const std::vector<golden_entry> golden = {
        { { -3, -3 }, 1.438203f, 3.500000f },
        { { -2, -3 }, 2.298876f, 3.500000f },
        { { -1, -3 }, 2.582018f, 3.500000f },
        { { 0, -3 }, 2.582018f, 3.500000f },
        { { 1, -3 }, 2.582018f, 3.500000f },
        { { 2, -3 }, 2.298876f, 3.500000f },
        { { 3, -3 }, 1.438203f, 3.500000f },
        { { -3, -2 }, 2.298876f, 3.500000f },
        { { -2, -2 }, 4.239466f, 4.723102f },
        { { -1, -2 }, 5.266641f, 4.723102f },
        { { 0, -2 }, 5.821768f, 4.723102f },
        { { 1, -2 }, 5.266641f, 4.723102f },
        { { 2, -2 }, 4.239465f, 4.723102f },
        { { 3, -2 }, 2.298876f, 3.500000f },
        { { -3, -1 }, 2.582018f, 3.500000f },
        { { -2, -1 }, 5.266641f, 4.723102f },
        { { -1, -1 }, 6.974808f, 9.719253f },
        { { 0, -1 }, 8.085064f, 9.719253f },
        { { 1, -1 }, 6.974808f, 9.719253f },
        { { 2, -1 }, 5.266640f, 4.723102f },
        { { 3, -1 }, 2.582018f, 3.500000f },
        { { -3, 0 }, 2.582018f, 3.500000f },
        { { -2, 0 }, 5.821769f, 4.723102f },
        { { -1, 0 }, 8.085064f, 9.719253f },
        { { 0, 0 }, 9.750447f, 10.000000f },
        { { 1, 0 }, 8.085064f, 9.719253f },
        { { 2, 0 }, 5.821768f, 4.723102f },
        { { 3, 0 }, 2.582018f, 3.500000f },
        { { -3, 1 }, 2.582018f, 3.500000f },
        { { -2, 1 }, 5.266641f, 4.723102f },
        { { -1, 1 }, 6.974808f, 9.719253f },
        { { 0, 1 }, 8.085063f, 9.719253f },
        { { 1, 1 }, 6.974808f, 9.719253f },
        { { 2, 1 }, 5.266640f, 4.723102f },
        { { 3, 1 }, 2.582018f, 3.500000f },
        { { -3, 2 }, 2.298876f, 3.500000f },
        { { -2, 2 }, 4.239466f, 4.723102f },
        { { -1, 2 }, 5.266641f, 4.723102f },
        { { 0, 2 }, 5.821768f, 4.723102f },
        { { 1, 2 }, 5.266641f, 4.723102f },
        { { 2, 2 }, 4.239466f, 4.723102f },
        { { 3, 2 }, 2.298876f, 3.500000f },
        { { -3, 3 }, 1.438203f, 3.500000f },
        { { -2, 3 }, 2.298876f, 3.500000f },
        { { -1, 3 }, 2.582018f, 3.500000f },
        { { 0, 3 }, 2.582018f, 3.500000f },
        { { 1, 3 }, 2.582018f, 3.500000f },
        { { 2, 3 }, 2.298876f, 3.500000f },
        { { 3, 3 }, 1.438203f, 3.500000f },
    };
    // NOLINTEND(cata-use-named-point-constants)

    for( const golden_entry &g : golden ) {
        const point p = g.d + point( src.x(), src.y() );
        CAPTURE( g.d );
        CHECK( cache.light_color_cache[p.x][p.y].r == Approx( g.r ).margin( 0.01f ) );
        CHECK( cache.light_color_cache[p.x][p.y].g == Approx( 0.0f ).margin( 0.01f ) );
        CHECK( cache.light_color_cache[p.x][p.y].b == Approx( 0.0f ).margin( 0.01f ) );
        CHECK( cache.lm[p.x][p.y].max() == Approx( g.lm ).margin( 0.01f ) );
    }
}



TEST_CASE( "capture_golden_values_for_point_light", "[.][golden_capture]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 5;

    for( int dx = -6; dx <= 6; dx++ ) {
        for( int dy = -6; dy <= 6; dy++ ) {
            place_ter_roofed( src + tripoint( dx, dy, 0 ), ter_t_floor );
        }
    }
    place_ter_roofed( src, ter_t_test_red_light );

    rebuild_lightmap( 0 );

    const map &here = get_map();
    const level_cache &cache = here.access_cache( 0 );

    // NOLINTNEXTLINE(cata-text-style)
    printf( "\n=== GOLDEN VALUES: 9x9 patch centered on red light ===\n" );
    // NOLINTNEXTLINE(cata-text-style)
    printf( "// { dx, dy, r, g, b, lm_max }\n" );
    for( int dy = -4; dy <= 4; dy++ ) {
        for( int dx = -4; dx <= 4; dx++ ) {
            // NOLINTNEXTLINE(cata-combine-locals-into-point)
            const int x = src.x() + dx;
            const int y = src.y() + dy;
            const auto &c = cache.light_color_cache[x][y];
            const float lm = cache.lm[x][y].max();
            // NOLINTNEXTLINE(cata-text-style)
            printf( "{ %2d, %2d, %10.6ff, %10.6ff, %10.6ff, %10.6ff },\n",
                    dx, dy, c.r, c.g, c.b, lm );
        }
    }
}



TEST_CASE( "capture_golden_values_for_arc_light", "[.][golden_arc_capture]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    map &here = get_map();
    const tripoint_bub_ms veh_pos = get_player_character().pos_bub() + tripoint::east * 5;

    for( int dx = -10; dx <= 10; dx++ ) {
        for( int dy = -10; dy <= 10; dy++ ) {
            place_ter_roofed( veh_pos + tripoint( dx, dy, 0 ), ter_t_floor );
        }
    }

    vehicle *veh = here.add_vehicle( vehicle_prototype_car, veh_pos, 0_degrees, 100,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );

    std::vector<point_rel_ms> hl_mounts;
    for( const vpart_reference &vpr : veh->get_any_parts( VPFLAG_CONE_LIGHT ) ) {
        hl_mounts.push_back( vpr.part().mount );
        veh->remove_part( vpr.part() );
    }
    veh->part_removal_cleanup( here );
    REQUIRE( !hl_mounts.empty() );

    const int part_idx = veh->install_part( here, hl_mounts.front(), vpart_test_red_headlight );
    REQUIRE( part_idx >= 0 );

    vehicle_part &installed = veh->part( part_idx );
    installed.enabled = true;

    rebuild_lightmap( 0 );

    const tripoint_bub_ms hl_pos = veh->bub_part_pos( here, installed );
    const level_cache &cache = here.access_cache( 0 );

    // NOLINTNEXTLINE(cata-text-style)
    printf( "\n=== ARC GOLDEN: hl_pos=(%d,%d), mount=(%d,%d) ===\n",
            hl_pos.x(), hl_pos.y(), hl_mounts.front().x(), hl_mounts.front().y() );
    // NOLINTNEXTLINE(cata-text-style)
    printf( "// { dx, dy, r, lm_max }\n" );
    for( int dy = -5; dy <= 5; dy++ ) {
        for( int dx = -5; dx <= 5; dx++ ) {
            // NOLINTNEXTLINE(cata-combine-locals-into-point)
            const int x = hl_pos.x() + dx;
            const int y = hl_pos.y() + dy;
            if( x < 0 || x >= MAPSIZE_X || y < 0 || y >= MAPSIZE_Y ) {
                continue;
            }
            const float r = cache.light_color_cache[x][y].r;
            const float lm = cache.lm[x][y].max();
            if( r > 0.0f || ( dx >= -2 && dx <= 2 && dy >= -2 && dy <= 2 ) ) {
                // NOLINTNEXTLINE(cata-text-style)
                printf( "{ %2d, %2d, %10.6ff, %10.6ff },\n", dx, dy, r, lm );
            }
        }
    }
}

TEST_CASE( "white_only_map_has_no_colored_light", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_test_t_utility_light );

    rebuild_lightmap( 0 );

    const map &here = get_map();
    const level_cache &cache = here.access_cache( 0 );

    CHECK_FALSE( cache.has_colored_lights );


    bool any_color = false;
    for( int x = 0; x < MAPSIZE_X && !any_color; x++ ) {
        for( int y = 0; y < MAPSIZE_Y && !any_color; y++ ) {
            if( cache.light_color_cache[x][y].is_colored() ) {
                any_color = true;
            }
        }
    }
    CHECK_FALSE( any_color );
}

TEST_CASE( "colored_terrain_sets_has_colored_lights", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_t_test_red_light );

    rebuild_lightmap( 0 );

    const level_cache &cache = get_map().access_cache( 0 );
    CHECK( cache.has_colored_lights );
}

TEST_CASE( "colored_vehicle_cone_sets_has_colored_lights", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    map &here = get_map();
    const tripoint_bub_ms veh_pos = get_player_character().pos_bub() + tripoint::east * 5;

    for( int dx = -10; dx <= 10; dx++ ) {
        for( int dy = -10; dy <= 10; dy++ ) {
            place_ter_roofed( veh_pos + tripoint( dx, dy, 0 ), ter_t_floor );
        }
    }

    vehicle *veh = here.add_vehicle( vehicle_prototype_car, veh_pos, 0_degrees, 100,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );


    std::vector<point_rel_ms> hl_mounts;
    for( const vpart_reference &vpr : veh->get_any_parts( VPFLAG_CONE_LIGHT ) ) {
        hl_mounts.push_back( vpr.part().mount );
        veh->remove_part( vpr.part() );
    }
    veh->part_removal_cleanup( here );
    REQUIRE( !hl_mounts.empty() );

    const int part_idx = veh->install_part( here, hl_mounts.front(), vpart_test_red_headlight );
    REQUIRE( part_idx >= 0 );

    vehicle_part &installed = veh->part( part_idx );
    installed.enabled = true;

    rebuild_lightmap( 0 );

    const level_cache &cache = here.access_cache( 0 );
    CHECK( cache.has_colored_lights );
}

TEST_CASE( "lut_wiring_respects_trigdist_option", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    restore_on_out_of_scope restore_trigdist( trigdist );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 5;

    for( int dx = -6; dx <= 6; dx++ ) {
        for( int dy = -6; dy <= 6; dy++ ) {
            place_ter_roofed( src + tripoint( dx, dy, 0 ), ter_t_floor );
        }
    }
    place_ter_roofed( src, ter_test_t_utility_light );


    const tripoint_bub_ms target = src + tripoint( 3, 3, 0 );
    place_ter_roofed( target, ter_t_floor );

    trigdist = true;
    rebuild_lightmap( 0 );
    const float lm_trig = get_map().access_cache( 0 ).lm[target.x()][target.y()].max();

    trigdist = false;
    rebuild_lightmap( 0 );
    const float lm_cheb = get_map().access_cache( 0 ).lm[target.x()][target.y()].max();



    CAPTURE( lm_trig, lm_cheb );
    CHECK( lm_trig < lm_cheb );
}

TEST_CASE( "fused_arc_color_output_matches_golden_values", "[light_color]" )
{



    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    map &here = get_map();
    const tripoint_bub_ms veh_pos = get_player_character().pos_bub() + tripoint::east * 5;

    for( int dx = -10; dx <= 10; dx++ ) {
        for( int dy = -10; dy <= 10; dy++ ) {
            place_ter_roofed( veh_pos + tripoint( dx, dy, 0 ), ter_t_floor );
        }
    }

    vehicle *veh = here.add_vehicle( vehicle_prototype_car, veh_pos, 0_degrees, 100,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );

    std::vector<point_rel_ms> hl_mounts;
    for( const vpart_reference &vpr : veh->get_any_parts( VPFLAG_CONE_LIGHT ) ) {
        hl_mounts.push_back( vpr.part().mount );
        veh->remove_part( vpr.part() );
    }
    veh->part_removal_cleanup( here );
    REQUIRE( !hl_mounts.empty() );

    const int part_idx = veh->install_part( here, hl_mounts.front(), vpart_test_red_headlight );
    REQUIRE( part_idx >= 0 );

    vehicle_part &installed = veh->part( part_idx );
    installed.enabled = true;

    rebuild_lightmap( 0 );

    const tripoint_bub_ms hl_pos = veh->bub_part_pos( here, installed );
    const level_cache &cache = here.access_cache( 0 );



    struct golden_entry {
        point d;
        float r;
        float lm;
    };

    static const std::vector<golden_entry> golden = {

        { {  2, -1 }, 3104.943359f, 3778.481689f },
        { {  2,  0 }, 4657.414551f, 3778.481689f },
        { {  2,  1 }, 3104.943359f, 3778.481689f },
        { {  3, -1 }, 1957.029907f, 2418.360840f },
        { {  3,  0 }, 2645.568115f, 2418.360840f },
        { {  3,  1 }, 1957.029907f, 2418.360840f },
        { {  4, -1 }, 1562.625610f, 1739.861816f },
        { {  4,  0 }, 1831.332275f, 1739.861816f },
        { {  4,  1 }, 1562.625610f, 1739.861816f },
        { {  5, -1 }, 1381.454834f, 1335.774170f },
        { {  5,  0 }, 1381.454834f, 1335.774170f },
        { {  5,  1 }, 1381.454834f, 1335.774170f },

        { { -2, -1 },    0.000000f,    3.500000f },
        { { -2,  0 },    0.000000f,    3.500000f },
        { { -2,  1 },    0.000000f,    3.500000f },
    };

    for( const golden_entry &g : golden ) {
        const point p = g.d + point( hl_pos.x(), hl_pos.y() );
        CAPTURE( g.d );
        CHECK( cache.light_color_cache[p.x][p.y].r == Approx( g.r ).margin( 0.01f ) );
        CHECK( cache.lm[p.x][p.y].max() == Approx( g.lm ).margin( 0.01f ) );
    }


    const float forward_r = cache.light_color_cache[hl_pos.x() + 4][hl_pos.y()].r;
    const float behind_r = cache.light_color_cache[hl_pos.x() - 2][hl_pos.y()].r;
    CHECK( forward_r > behind_r );
}






static void set_time_and_rebuild( const time_point &when, int zlev = 0 )
{
    calendar::turn = when;
    g->reset_light_level();
    get_player_character().recalc_sight_limits();
    rebuild_lightmap( zlev );
}

TEST_CASE( "dawn_dusk_tint_on_outside_tiles", "[light_color][dawn_dusk]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const time_point dawn = sunrise( calendar::turn_zero ) - 15_minutes;
    set_time_and_rebuild( dawn );

    const level_cache &cache = get_map().access_cache( 0 );


    const tripoint_bub_ms center = get_player_character().pos_bub();
    for( int dx = -3; dx <= 3; dx += 3 ) {
        for( int dy = -3; dy <= 3; dy += 3 ) {
            const point p( center.x() + dx, center.y() + dy );
            CAPTURE( dx, dy );
            const light_color_rgb &c = cache.light_color_cache[p.x][p.y];
            CHECK( c.r > 0.0f );

            CHECK( c.r > c.g );
            CHECK( c.g > c.b );
        }
    }
}

TEST_CASE( "no_dawn_dusk_tint_at_midnight", "[light_color][dawn_dusk]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    set_time_and_rebuild( midnight );

    const level_cache &cache = get_map().access_cache( 0 );
    const tripoint_bub_ms center = get_player_character().pos_bub();
    for( int d = -3; d <= 3; d += 3 ) {
        const light_color_rgb &c = cache.light_color_cache[center.x() + d][center.y()];
        CAPTURE( d );
        CHECK_FALSE( c.is_colored() );
    }
}

TEST_CASE( "no_dawn_dusk_tint_at_noon", "[light_color][dawn_dusk]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const time_point noon = calendar::turn_zero + 12_hours;
    set_time_and_rebuild( noon );

    const level_cache &cache = get_map().access_cache( 0 );
    const tripoint_bub_ms center = get_player_character().pos_bub();
    for( int d = -3; d <= 3; d += 3 ) {
        const light_color_rgb &c = cache.light_color_cache[center.x() + d][center.y()];
        CAPTURE( d );
        CHECK_FALSE( c.is_colored() );
    }
}

TEST_CASE( "dawn_dusk_tint_blocked_by_roof", "[light_color][dawn_dusk]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    map &here = get_map();
    const tripoint_bub_ms center = get_player_character().pos_bub();

    const tripoint_bub_ms room_origin = center + tripoint( 8, 0, 0 );
    for( int dx = 0; dx < 5; dx++ ) {
        for( int dy = 0; dy < 5; dy++ ) {
            const tripoint_bub_ms pos = room_origin + tripoint( dx, dy, 0 );
            if( dx == 0 || dx == 4 || dy == 0 || dy == 4 ) {
                here.ter_set( pos, ter_t_wall.id() );
            } else {
                here.ter_set( pos, ter_t_floor.id() );
            }
            here.ter_set( pos + tripoint::above, ter_t_flat_roof.id() );
        }
    }

    const time_point dawn = sunrise( calendar::turn_zero ) - 15_minutes;
    set_time_and_rebuild( dawn );

    const level_cache &cache = here.access_cache( 0 );

    const tripoint_bub_ms inside = room_origin + tripoint( 2, 2, 0 );
    CHECK_FALSE( cache.light_color_cache[inside.x()][inside.y()].is_colored() );

    CHECK( cache.light_color_cache[center.x()][center.y()].r > 0.0f );
}

TEST_CASE( "dawn_dusk_tint_penetrates_through_opening", "[light_color][dawn_dusk]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    map &here = get_map();
    const tripoint_bub_ms center = get_player_character().pos_bub();







    const tripoint_bub_ms corridor_sw = center + tripoint( 6, -1, 0 );
    constexpr int corridor_len = 9;
    for( int dx = 0; dx < corridor_len; dx++ ) {
        for( int dy = 0; dy < 3; dy++ ) {
            const tripoint_bub_ms pos = corridor_sw + tripoint( dx, dy, 0 );
            if( dy == 0 || dy == 2 || dx == corridor_len - 1 ) {
                here.ter_set( pos, ter_t_wall.id() );
            } else if( dx == 0 ) {
                here.ter_set( pos, ter_t_door_o.id() );
            } else {
                here.ter_set( pos, ter_t_floor.id() );
            }

            here.ter_set( pos + tripoint::above, ter_t_flat_roof.id() );
        }
    }

    const time_point dawn = sunrise( calendar::turn_zero ) - 15_minutes;
    set_time_and_rebuild( dawn );

    const level_cache &cache = here.access_cache( 0 );

    const int cy = corridor_sw.y() + 1;
    const int x_open = corridor_sw.x();
    const int x_near = corridor_sw.x() + 2;
    const int x_deep = corridor_sw.x() + 5;

    const float r_open = cache.light_color_cache[x_open][cy].r;
    const float r_near = cache.light_color_cache[x_near][cy].r;
    const float r_deep = cache.light_color_cache[x_deep][cy].r;

    CAPTURE( r_open, r_near, r_deep );


    CHECK( r_open > 0.0f );

    CHECK( r_near > 0.0f );

    CHECK( r_open > r_deep );
}

TEST_CASE( "no_dawn_dusk_tint_in_alternate_dimension", "[light_color][dawn_dusk]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );


    const time_point dawn = sunrise( calendar::turn_zero ) - 15_minutes;
    calendar::turn = dawn;
    g->reset_light_level();


    const light_color_rgb default_dim = dawn_dusk_color_for_lightmap( dimension_world_default );
    CHECK( default_dim.is_colored() );
    CHECK( default_dim.r > 0.0f );


    const light_color_rgb nether = dawn_dusk_color_for_lightmap( dimension_world_nether );
    CHECK_FALSE( nether.is_colored() );
}
