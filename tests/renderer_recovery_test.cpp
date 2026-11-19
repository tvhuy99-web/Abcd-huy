#if defined(TILES)

#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "cata_imgui.h"
#include "cata_scope_helpers.h"
#include "cata_tiles.h"
#include "cursesport.h"
#include "font_loader.h"
#include "imgui/imgui.h"
#include "options_helpers.h"
#include "output.h"
#include "point.h"
#include "sdl_renderer_recovery.h"
#include "sdltiles.h"

class Font;

namespace
{
using ak = recovery_drain_planner::action_kind;
using sev = renderer_recovery_severity;

uint64_t epoch( uint64_t seq, lifecycle_state s )
{
    return recovery_drain_planner::make_lifecycle_epoch( seq, s );
}
}





TEST_CASE( "drain_planner_epoch_pack_unpack_roundtrips", "[tiles][renderer_recovery]" )
{
    using P = recovery_drain_planner;
    const uint64_t e = P::make_lifecycle_epoch( 12345, lifecycle_state::resumed_pending_rebuild );
    CHECK( P::lifecycle_seq_of( e ) == 12345 );
    CHECK( P::lifecycle_state_of( e ) == lifecycle_state::resumed_pending_rebuild );

    CHECK( P::severity_max( sev::targets_reset, sev::device_lost ) == sev::device_lost );
    CHECK( P::severity_max( sev::device_lost, sev::targets_reset ) == sev::device_lost );
    CHECK( P::severity_max( sev::device_reset, sev::none ) == sev::device_reset );
}

TEST_CASE( "drain_planner_paused_entry_finishes_without_claiming", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    REQUIRE( p.begin().kind == ak::load_entry_epoch );


    const recovery_drain_planner::action a =
        p.advance_entry_epoch( epoch( 3, lifecycle_state::paused ) );
    CHECK( a.kind == ak::finish_ready );
    CHECK( p.state() == renderer_recovery_state::ready );
    CHECK_FALSE( p.abort_latch() );
}

TEST_CASE( "drain_planner_foreground_epoch_alone_claims_device_reset",
           "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    REQUIRE( p.begin().kind == ak::load_entry_epoch );

    recovery_drain_planner::action a =
        p.advance_entry_epoch( epoch( 1, lifecycle_state::resumed_pending_rebuild ) );
    REQUIRE( a.kind == ak::claim_initial_inbox );
    a = p.advance_initial_inbox( sev::none, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::device_reset );

    a = p.advance_recipe( { recipe_outcome::success } );
    REQUIRE( a.kind == ak::attempt_seq_cas );
    CHECK( a.serviced_seq == 1 );
    REQUIRE( p.advance_seq_cas_done().kind == ak::claim_after_success );

    a = p.advance_after_success( sev::none, epoch( 1, lifecycle_state::active ) );
    REQUIRE( a.kind == ak::load_post_resize );
    CHECK( p.advance_post_resize( 0 ).kind == ak::finish_ready );
    CHECK( p.state() == renderer_recovery_state::ready );
}

TEST_CASE( "drain_planner_coalesces_severity_queued_during_recipe",
           "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::targets_reset, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::targets_reset );

    a = p.advance_recipe( { recipe_outcome::success } );
    REQUIRE( a.kind == ak::attempt_seq_cas );
    REQUIRE( p.advance_seq_cas_done().kind == ak::claim_after_success );


    a = p.advance_after_success( sev::device_reset, epoch( 0, lifecycle_state::active ) );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::device_reset );

    a = p.advance_recipe( { recipe_outcome::success } );
    REQUIRE( a.kind == ak::attempt_seq_cas );
    REQUIRE( p.advance_seq_cas_done().kind == ak::claim_after_success );
    a = p.advance_after_success( sev::none, epoch( 0, lifecycle_state::active ) );
    REQUIRE( a.kind == ak::load_post_resize );
    CHECK( p.advance_post_resize( 0 ).kind == ak::finish_ready );
}

TEST_CASE( "drain_planner_seq_cas_preserves_newer_foreground", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a =
        p.advance_entry_epoch( epoch( 5, lifecycle_state::resumed_pending_rebuild ) );
    a = p.advance_initial_inbox( sev::none, 0, false );
    REQUIRE( a.severity == sev::device_reset );

    a = p.advance_recipe( { recipe_outcome::success } );
    REQUIRE( a.kind == ak::attempt_seq_cas );
    CHECK( a.serviced_seq == 5 );
    REQUIRE( p.advance_seq_cas_done().kind == ak::claim_after_success );


    a = p.advance_after_success( sev::none, epoch( 6, lifecycle_state::resumed_pending_rebuild ) );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::device_reset );

    CHECK( p.serviced_lifecycle_seq() == 6 );
}

TEST_CASE( "drain_planner_restart_required_reruns_escalated", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::device_reset, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    a = p.advance_recipe( { recipe_outcome::restart_required, sev::device_lost } );
    REQUIRE( a.kind == ak::claim_after_restart );

    SECTION( "an active live epoch reruns at the escalated severity" ) {
        a = p.advance_after_restart( epoch( 0, lifecycle_state::active ), sev::none );
        REQUIRE( a.kind == ak::run_recipe );
        CHECK( a.severity == sev::device_lost );
        CHECK( p.serviced_lifecycle_seq() == 0 );
    }

    SECTION( "a newer foreground at restart is adopted as the serviced sequence" ) {
        a = p.advance_after_restart( epoch( 7, lifecycle_state::resumed_pending_rebuild ), sev::none );
        REQUIRE( a.kind == ak::run_recipe );
        CHECK( a.severity == sev::device_lost );
        CHECK( p.serviced_lifecycle_seq() == 7 );
    }
}

TEST_CASE( "drain_planner_failure_persists_retry_for_next_drain", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::targets_reset, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    a = p.advance_recipe( { recipe_outcome::failure } );
    CHECK( a.kind == ak::finish_retry );
    CHECK( p.state() == renderer_recovery_state::retry_needed );


    p.begin();
    a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::none, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::targets_reset );
}

TEST_CASE( "drain_planner_resize_fast_path", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();

    SECTION( "an applied entry resize finishes ready" ) {
        p.begin();
        recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
        a = p.advance_initial_inbox( sev::none, 1, false );
        REQUIRE( a.kind == ak::apply_resize );
        CHECK( a.resize_epoch == 1 );
        a = p.advance_resize( true );
        CHECK( a.kind == ak::finish_ready );
        CHECK( p.state() == renderer_recovery_state::ready );
    }

    SECTION( "a failed entry resize needs retry and persists no recipe severity" ) {
        p.begin();
        recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
        a = p.advance_initial_inbox( sev::none, 1, false );
        REQUIRE( a.kind == ak::apply_resize );
        a = p.advance_resize( false );
        CHECK( a.kind == ak::finish_retry );
        CHECK( p.state() == renderer_recovery_state::retry_needed );



        p.begin();
        a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
        a = p.advance_initial_inbox( sev::none, 0, false );
        CHECK( a.kind == ak::load_post_resize );
    }
}

TEST_CASE( "drain_planner_defers_resize_until_after_recovery", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );

    a = p.advance_initial_inbox( sev::device_reset, 1, false );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::device_reset );

    a = p.advance_recipe( { recipe_outcome::success } );
    REQUIRE( a.kind == ak::attempt_seq_cas );
    REQUIRE( p.advance_seq_cas_done().kind == ak::claim_after_success );
    a = p.advance_after_success( sev::none, epoch( 0, lifecycle_state::active ) );
    REQUIRE( a.kind == ak::load_post_resize );

    a = p.advance_post_resize( 1 );
    REQUIRE( a.kind == ak::apply_resize );
    CHECK( a.resize_epoch == 1 );
    CHECK( p.advance_resize( true ).kind == ak::finish_ready );
}

TEST_CASE( "drain_planner_pause_during_recipe_aborts_to_retry", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::device_reset, 0, false );
    REQUIRE( a.kind == ak::run_recipe );

    p.note_pause_abort();

    a = p.advance_recipe( { recipe_outcome::success } );
    CHECK( a.kind == ak::finish_retry );
    CHECK( p.state() == renderer_recovery_state::retry_needed );


    p.begin();
    a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::none, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::device_reset );
}





TEST_CASE( "renderer_coordinator_inbox_keeps_max_severity", "[tiles][renderer_recovery]" )
{
    renderer_resource_coordinator coord;
    coord.finish_bootstrap();
    REQUIRE( coord.pending() == renderer_recovery_severity::none );

    coord.request_recovery( renderer_recovery_severity::targets_reset );
    CHECK( coord.pending() == renderer_recovery_severity::targets_reset );

    coord.request_recovery( renderer_recovery_severity::device_lost );
    CHECK( coord.pending() == renderer_recovery_severity::device_lost );


    coord.request_recovery( renderer_recovery_severity::device_reset );
    CHECK( coord.pending() == renderer_recovery_severity::device_lost );
}

TEST_CASE( "renderer_coordinator_embargoes_render_on_inbox_change", "[tiles][renderer_recovery]" )
{
    renderer_resource_coordinator coord;
    coord.finish_bootstrap();
    REQUIRE( coord.is_render_allowed() );
    REQUIRE_FALSE( coord.should_abort_frame() );

    SECTION( "queued severity embargoes rendering" ) {
        coord.request_recovery( renderer_recovery_severity::targets_reset );
        CHECK_FALSE( coord.is_render_allowed() );
        CHECK( coord.should_abort_frame() );
    }

    SECTION( "pending resize embargoes rendering" ) {
        coord.notify_resize();
        CHECK_FALSE( coord.is_render_allowed() );
        CHECK( coord.should_abort_frame() );
    }

    SECTION( "paused lifecycle embargoes rendering" ) {
        coord.notify_lifecycle( lifecycle_state::paused );
        CHECK_FALSE( coord.is_render_allowed() );
        CHECK( coord.should_abort_frame() );
    }
}

TEST_CASE( "renderer_coordinator_mode2_upload_poll_maps_inbox", "[tiles][renderer_recovery]" )
{




    renderer_resource_coordinator coord;
    coord.finish_bootstrap();
    REQUIRE( coord.mode2_upload_poll() == atlas_upload_interrupt::none );

    SECTION( "paused lifecycle pauses the upload" ) {
        coord.notify_lifecycle( lifecycle_state::paused );
        CHECK( coord.mode2_upload_poll() == atlas_upload_interrupt::paused );
    }

    SECTION( "resumed foreground invalidates textures" ) {
        coord.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
        CHECK( coord.mode2_upload_poll() == atlas_upload_interrupt::texture_resources_invalidated );
    }

    SECTION( "queued device reset invalidates textures" ) {
        coord.request_recovery( renderer_recovery_severity::device_reset );
        CHECK( coord.mode2_upload_poll() == atlas_upload_interrupt::texture_resources_invalidated );
    }

    SECTION( "queued targets reset is left to the drain loop" ) {
        coord.request_recovery( renderer_recovery_severity::targets_reset );
        CHECK( coord.mode2_upload_poll() == atlas_upload_interrupt::none );
    }

    SECTION( "queued device loss invalidates the renderer" ) {
        coord.request_recovery( renderer_recovery_severity::device_lost );
        CHECK( coord.mode2_upload_poll() == atlas_upload_interrupt::renderer_invalidated );
    }

    SECTION( "paused outranks a queued device loss" ) {
        coord.request_recovery( renderer_recovery_severity::device_lost );
        coord.notify_lifecycle( lifecycle_state::paused );
        CHECK( coord.mode2_upload_poll() == atlas_upload_interrupt::paused );
    }
}

TEST_CASE( "renderer_coordinator_paused_drain_preserves_severity", "[tiles][renderer_recovery]" )
{
    renderer_resource_coordinator coord;
    coord.finish_bootstrap();

    coord.request_recovery( renderer_recovery_severity::device_reset );
    coord.notify_lifecycle( lifecycle_state::paused );



    coord.drain_pending();
    CHECK( coord.pending() == renderer_recovery_severity::device_reset );
    CHECK_FALSE( coord.is_render_allowed() );
    CHECK( coord.should_abort_frame() );
}

TEST_CASE( "renderer_coordinator_lifecycle_survives_background_ordering",
           "[tiles][renderer_recovery]" )
{
    renderer_resource_coordinator coord;
    coord.finish_bootstrap();
    REQUIRE( coord.is_render_allowed() );




    coord.notify_lifecycle( lifecycle_state::paused );
    coord.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    coord.notify_lifecycle( lifecycle_state::paused );

    CHECK_FALSE( coord.is_render_allowed() );


    coord.drain_pending();
    CHECK_FALSE( coord.is_render_allowed() );
    CHECK( coord.should_abort_frame() );
}





TEST_CASE( "renderer_coordinator_targets_reset_bumps_only_resource_generation",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    const uint64_t inst0 = renderer_coordinator.instance_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();

    renderer_coordinator.request_recovery( renderer_recovery_severity::targets_reset );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.resource_generation() == res0 + 1 );
    CHECK( renderer_coordinator.instance_generation() == inst0 );
    CHECK( renderer_coordinator.textures_generation() == tex0 );
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_device_reset_bumps_texture_generation",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    const uint64_t inst0 = renderer_coordinator.instance_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.resource_generation() == res0 + 1 );
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );


    CHECK( renderer_coordinator.instance_generation() == inst0 );
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_device_lost_bumps_all_generations",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    const uint64_t inst0 = renderer_coordinator.instance_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();

    std::shared_ptr<const tileset> bundle =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_lost_ts", "color_pixel_sepia_light", inst0, tex0 );
    REQUIRE( bundle );

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_lost );
    renderer_coordinator.drain_pending();


    CHECK( renderer_coordinator.resource_generation() == res0 + 1 );
    CHECK( renderer_coordinator.instance_generation() == inst0 + 1 );
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );


    CHECK( bundle->get_renderer_instance_generation_at_upload()
           == renderer_coordinator.instance_generation() );
    CHECK( bundle->get_gpu_textures_generation_at_upload()
           == renderer_coordinator.textures_generation() );
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_foreground_epoch_alone_forces_rebuild",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst0 = renderer_coordinator.instance_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();



    renderer_coordinator.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
    CHECK( renderer_coordinator.instance_generation() == inst0 );
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_device_reset_replays_cached_bundle",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();

    std::shared_ptr<const tileset> bundle =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_replay_ts", "color_pixel_sepia_light", inst, tex );
    REQUIRE( bundle );
    REQUIRE( bundle->get_gpu_textures_generation_at_upload() == tex );

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.textures_generation() == tex + 1 );


    CHECK( bundle->get_gpu_textures_generation_at_upload()
           == renderer_coordinator.textures_generation() );
}

TEST_CASE( "renderer_coordinator_cache_returns_bundle_on_fresh_generations",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();

    std::shared_ptr<const tileset> installed =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_hit_ts", "color_pixel_sepia_light", inst, tex );
    REQUIRE( installed );



    std::shared_ptr<const tileset> fetched =
        renderer_recovery_test_support::fetch_cached_bundle(
            "synthetic_hit_ts", "color_pixel_sepia_light", inst, tex );
    CHECK( fetched == installed );
}

TEST_CASE( "renderer_coordinator_cache_freshness_predicate", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();
    const std::string id = "synthetic_fresh_ts";
    const std::string mode = "color_pixel_sepia_light";

    std::shared_ptr<const tileset> bundle;
    {


        override_option scaling( "SCALING_MODE", "nearest" );
        bundle = renderer_recovery_test_support::install_synthetic_bundle( id, mode, inst, tex );
        REQUIRE( bundle );


        CHECK( renderer_recovery_test_support::cache_lookup_is_fresh( id, mode, inst, tex ) );

        CHECK_FALSE( renderer_recovery_test_support::cache_lookup_is_fresh( id, mode, inst, tex + 1 ) );

        CHECK_FALSE( renderer_recovery_test_support::cache_lookup_is_fresh( id, mode, inst + 1, tex ) );
    }

    {
        override_option scaling( "SCALING_MODE", "linear" );
        CHECK_FALSE( renderer_recovery_test_support::cache_lookup_is_fresh( id, mode, inst, tex ) );
    }
}





TEST_CASE( "renderer_coordinator_second_reset_mid_drain_coalesces", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();

    renderer_coordinator.request_recovery( renderer_recovery_severity::targets_reset );


    renderer_recovery_test_support::arm_phase_queue_device_reset( 2 );
    renderer_coordinator.drain_pending();



    CHECK( renderer_coordinator.resource_generation() == res0 + 2 );
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
    CHECK( renderer_coordinator.is_render_allowed() );
}





TEST_CASE( "renderer_coordinator_resize_failure_persists_retry", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    renderer_coordinator.notify_resize();
    REQUIRE_FALSE( renderer_coordinator.is_render_allowed() );



    renderer_recovery_test_support::arm_phase_fail_retry( 1 );
    renderer_coordinator.drain_pending();
    CHECK_FALSE( renderer_coordinator.is_render_allowed() );


    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_resize_retry_keeps_ui_relayout", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    int win_w = 0;
    int win_h = 0;
    int font_w = 0;
    int font_h = 0;
    int scaling = 0;
    int min_term_w = 0;
    int min_term_h = 0;
    renderer_recovery_test_support::current_window_metrics( win_w, win_h, font_w, font_h, scaling,
            min_term_w, min_term_h );
    REQUIRE( font_w > 0 );
    REQUIRE( font_h > 0 );


    int window_w = std::max( win_w, ( min_term_w + 2 ) * font_w );
    const int window_h = std::max( win_h, ( min_term_h + 2 ) * font_h );
    int gates_swept = 0;
    for( int phase = 1; phase <= 32; ++phase ) {
        window_w += 2 * font_w;
        CAPTURE( phase, window_w, window_h );
        const uint64_t relayouts_before = renderer_recovery_test_support::ui_relayout_count();
        renderer_recovery_test_support::set_scaling_and_resize_window( 1, window_w, window_h );
        renderer_recovery_test_support::arm_phase_fail_retry( phase );
        renderer_coordinator.drain_pending();
        const bool fired = renderer_recovery_test_support::phase_fault_fired();
        if( fired ) {

            CHECK_FALSE( renderer_coordinator.is_render_allowed() );
            CHECK( renderer_recovery_test_support::ui_relayout_count() == relayouts_before );

            renderer_coordinator.drain_pending();
            ++gates_swept;
        }
        REQUIRE( renderer_coordinator.is_render_allowed() );
        CHECK( renderer_recovery_test_support::ui_relayout_count() == relayouts_before + 1 );
        if( !fired ) {
            break;
        }
    }

    CHECK( gates_swept >= 2 );
}

TEST_CASE( "renderer_coordinator_deferred_scaled_resize_rebuilds_buffer",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }



    renderer_recovery_test_support::set_scaling_and_resize_window( 2, 802, 602 );
    REQUIRE_FALSE( renderer_coordinator.is_render_allowed() );

    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.is_render_allowed() );

    int win_w = 0;
    int win_h = 0;
    int font_w = 0;
    int font_h = 0;
    int scaling = 0;
    int min_term_w = 0;
    int min_term_h = 0;
    renderer_recovery_test_support::current_window_metrics( win_w, win_h, font_w, font_h, scaling,
            min_term_w, min_term_h );
    int buf_w = 0;
    int buf_h = 0;
    renderer_recovery_test_support::current_display_buffer_dims( buf_w, buf_h );
    CAPTURE( win_w, win_h, font_w, font_h, scaling, min_term_w, min_term_h, buf_w, buf_h );
    REQUIRE( scaling == 2 );
    REQUIRE( font_w > 0 );
    REQUIRE( font_h > 0 );


    const int term_w = std::max( win_w / font_w / scaling, min_term_w );
    const int term_h = std::max( win_h / font_h / scaling, min_term_h );
    CHECK( buf_w == term_w * font_w );
    CHECK( buf_h == term_h * font_h );


    const bool height_scaled = win_h / font_h / scaling > min_term_h;
    REQUIRE( height_scaled );
    CHECK( buf_h != win_h / scaling );
}

TEST_CASE( "renderer_coordinator_resize_without_buffer_change_keeps_generation",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    int buf_w0 = 0;
    int buf_h0 = 0;
    renderer_recovery_test_support::current_display_buffer_dims( buf_w0, buf_h0 );
    int draw_w0 = 0;
    int draw_h0 = 0;
    renderer_recovery_test_support::current_drawable_dims( draw_w0, draw_h0 );




    renderer_recovery_test_support::set_needupdate( false );
    renderer_recovery_test_support::override_drawable_pixels( draw_w0 * 2, draw_h0 * 2 );
    REQUIRE_FALSE( renderer_coordinator.is_render_allowed() );

    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.is_render_allowed() );
    CHECK( renderer_coordinator.resource_generation() == res0 );


    int buf_w1 = 0;
    int buf_h1 = 0;
    renderer_recovery_test_support::current_display_buffer_dims( buf_w1, buf_h1 );
    CHECK( buf_w1 == buf_w0 );
    CHECK( buf_h1 == buf_h0 );


    int draw_w1 = 0;
    int draw_h1 = 0;
    renderer_recovery_test_support::current_drawable_dims( draw_w1, draw_h1 );
    CHECK( draw_w1 == draw_w0 * 2 );
    CHECK( draw_h1 == draw_h0 * 2 );


    CHECK( renderer_recovery_test_support::needupdate_armed() );
}

TEST_CASE( "renderer_coordinator_buffer_changing_resize_bumps_resource_generation",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();



    renderer_recovery_test_support::set_scaling_and_resize_window( 2, 802, 602 );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.is_render_allowed() );
    CHECK( renderer_coordinator.resource_generation() == res0 + 1 );
    CHECK( renderer_coordinator.textures_generation() == tex0 );
}

TEST_CASE( "renderer_coordinator_background_then_foreground_rebuilds",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t tex0 = renderer_coordinator.textures_generation();


    renderer_coordinator.notify_lifecycle( lifecycle_state::paused );
    renderer_coordinator.drain_pending();
    CHECK_FALSE( renderer_coordinator.is_render_allowed() );
    CHECK( renderer_coordinator.textures_generation() == tex0 );



    renderer_coordinator.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.is_render_allowed() );
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
}

TEST_CASE( "imgui_frame_display_size_prefers_explicit_display_buffer_dims",
           "[tiles][renderer_recovery]" )
{



    CHECK( cataimgui::imgui_frame_display_size( 320, 240, 1920, 1080 ) == point{ 320, 240 } );
    CHECK( cataimgui::imgui_frame_display_size( 0, 0, 1920, 1080 ) == point{ 1920, 1080 } );


    CHECK( cataimgui::imgui_frame_display_size( 320, 0, 1920, 1080 ) == point{ 1920, 1080 } );
}

TEST_CASE( "renderer_coordinator_pause_during_draw_scope_restores_target",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t tex0 = renderer_coordinator.textures_generation();




    bool bound_during = false;
    bool null_after = false;
    renderer_recovery_test_support::pause_during_draw_scope( bound_during, null_after );
    CHECK( bound_during );
    CHECK( null_after );


    renderer_coordinator.drain_pending();
    CHECK_FALSE( renderer_coordinator.is_render_allowed() );



    renderer_coordinator.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.is_render_allowed() );
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
}

TEST_CASE( "renderer_coordinator_recovery_then_resize_in_one_drain",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t tex0 = renderer_coordinator.textures_generation();


    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
    renderer_recovery_test_support::set_scaling_and_resize_window( 2, 802, 602 );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.is_render_allowed() );

    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );


    int win_w = 0;
    int win_h = 0;
    int font_w = 0;
    int font_h = 0;
    int scaling = 0;
    int min_term_w = 0;
    int min_term_h = 0;
    renderer_recovery_test_support::current_window_metrics( win_w, win_h, font_w, font_h, scaling,
            min_term_w, min_term_h );
    int buf_w = 0;
    int buf_h = 0;
    renderer_recovery_test_support::current_display_buffer_dims( buf_w, buf_h );
    CHECK( buf_w == std::max( win_w / font_w / scaling, min_term_w ) * font_w );
    CHECK( buf_h == std::max( win_h / font_h / scaling, min_term_h ) * font_h );
}

TEST_CASE( "renderer_coordinator_replay_pause_quarantines_then_retries",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();
    std::shared_ptr<const tileset> bundle =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_pause_ts", "color_pixel_sepia_light", inst, tex );
    REQUIRE( bundle );
    REQUIRE( renderer_recovery_test_support::replay_quarantine_empty() );

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );




    renderer_recovery_test_support::arm_replay_pause( 4 );
    renderer_coordinator.drain_pending();

    CHECK_FALSE( renderer_recovery_test_support::replay_quarantine_empty() );
    CHECK_FALSE( renderer_coordinator.is_render_allowed() );



    renderer_coordinator.drain_pending();
    CHECK_FALSE( renderer_recovery_test_support::replay_quarantine_empty() );


    renderer_coordinator.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    renderer_coordinator.drain_pending();
    CHECK( renderer_recovery_test_support::replay_quarantine_empty() );
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_mode2_interrupt_disposes_quarantine",
           "[tiles][renderer_recovery]" )
{





    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }

    SECTION( "successful reset drains the candidate on the surviving renderer" ) {
        atlas_replay_quarantine quarantine;
        const atlas_replay_quarantine::gate gate =
            renderer_recovery_test_support::populate_mode2_quarantine( quarantine );
        REQUIRE_FALSE( quarantine.empty() );
        REQUIRE( gate );
        REQUIRE_FALSE( gate->load() );
        const uint64_t inst = renderer_coordinator.instance_generation();

        renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
        const bool recovered = service_mode2_upload_interrupt(
                                   atlas_upload_interrupt::texture_resources_invalidated, quarantine, inst );

        CHECK( recovered );
        CHECK( quarantine.empty() );
        CHECK_FALSE( gate->load() );
    }

    SECTION( "a failed reset drain abandons the candidate and stops" ) {
        atlas_replay_quarantine quarantine;
        const atlas_replay_quarantine::gate gate =
            renderer_recovery_test_support::populate_mode2_quarantine( quarantine );
        REQUIRE_FALSE( quarantine.empty() );
        REQUIRE( gate );
        const uint64_t inst = renderer_coordinator.instance_generation();




        renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
        renderer_recovery_test_support::arm_phase_fail_retry( 1 );
        const bool recovered = service_mode2_upload_interrupt(
                                   atlas_upload_interrupt::texture_resources_invalidated, quarantine, inst );

        CHECK_FALSE( recovered );
        CHECK( quarantine.empty() );
        CHECK( gate->load() );
    }

    SECTION( "device loss abandons the candidate before the renderer is recreated" ) {
        atlas_replay_quarantine quarantine;
        const atlas_replay_quarantine::gate gate =
            renderer_recovery_test_support::populate_mode2_quarantine( quarantine );
        REQUIRE_FALSE( quarantine.empty() );
        REQUIRE( gate );
        const uint64_t inst = renderer_coordinator.instance_generation();

        renderer_coordinator.request_recovery( renderer_recovery_severity::device_lost );
        const bool recovered = service_mode2_upload_interrupt(
                                   atlas_upload_interrupt::renderer_invalidated, quarantine, inst );

        CHECK( recovered );
        CHECK( quarantine.empty() );


        CHECK( gate->load() );
    }

    SECTION( "shader boundary loss abandons the candidate and recovers through device loss" ) {
        atlas_replay_quarantine quarantine;
        const atlas_replay_quarantine::gate gate =
            renderer_recovery_test_support::populate_mode2_quarantine( quarantine );
        REQUIRE_FALSE( quarantine.empty() );
        REQUIRE( gate );
        const uint64_t inst = renderer_coordinator.instance_generation();


        const bool recovered = service_mode2_upload_interrupt(
                                   atlas_upload_interrupt::shader_boundary_lost, quarantine, inst );

        CHECK( recovered );
        CHECK( quarantine.empty() );
        CHECK( gate->load() );
        CHECK( renderer_coordinator.instance_generation() == inst + 1 );
    }
}

TEST_CASE( "renderer_coordinator_retries_from_any_phase", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }




    for( const renderer_recovery_severity sev : {
             renderer_recovery_severity::device_reset,
             renderer_recovery_severity::device_lost
         } ) {
        CAPTURE( static_cast<int>( sev ) );
        const uint64_t inst = renderer_coordinator.instance_generation();
        const uint64_t tex = renderer_coordinator.textures_generation();
        std::shared_ptr<const tileset> bundle =
            renderer_recovery_test_support::install_synthetic_bundle(
                "synthetic_sweep_ts", "color_pixel_sepia_light", inst, tex );
        REQUIRE( bundle );

        int gates_swept = 0;
        for( int phase = 1; phase <= 32; ++phase ) {
            CAPTURE( phase );
            renderer_coordinator.request_recovery( sev );
            renderer_recovery_test_support::arm_phase_fail_retry( phase );
            renderer_coordinator.drain_pending();
            if( !renderer_recovery_test_support::phase_fault_fired() ) {

                CHECK( renderer_coordinator.is_render_allowed() );
                break;
            }


            renderer_coordinator.drain_pending();
            CHECK( renderer_coordinator.is_render_allowed() );
            ++gates_swept;
        }
        CHECK( gates_swept >= 3 );
    }
}

TEST_CASE( "replay_reaches_every_live_bundle_published_under_one_key",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();


    const std::shared_ptr<const tileset> older =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_shared_key_ts", "color_pixel_sepia_light", inst, tex );
    const std::shared_ptr<const tileset> newer =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_shared_key_ts", "color_pixel_sepia_light", inst, tex );
    REQUIRE( older );
    REQUIRE( newer );
    REQUIRE( older != newer );

    WHEN( "device reset replays the cache" ) {
        renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
        renderer_coordinator.drain_pending();
        const uint64_t tex_now = renderer_coordinator.textures_generation();
        REQUIRE( tex_now == tex + 1 );
        THEN( "both have new texture generation" ) {
            CHECK( older->get_gpu_textures_generation_at_upload() == tex_now );
            CHECK( newer->get_gpu_textures_generation_at_upload() == tex_now );
        }
        THEN( "but still the lookup returns the newer object" ) {
            CHECK( renderer_recovery_test_support::fetch_cached_bundle(
                       "synthetic_shared_key_ts", "color_pixel_sepia_light",
                       renderer_coordinator.instance_generation(), tex_now ) == newer );
        }
    }
    WHEN( "device loss replays the cache on a new renderer" ) {
        renderer_coordinator.request_recovery( renderer_recovery_severity::device_lost );
        renderer_coordinator.drain_pending();
        const uint64_t inst_now = renderer_coordinator.instance_generation();
        REQUIRE( inst_now == inst + 1 );
        THEN( "both have new instance generation" ) {
            CHECK( older->get_renderer_instance_generation_at_upload() == inst_now );
            CHECK( newer->get_renderer_instance_generation_at_upload() == inst_now );
        }
    }
}

TEST_CASE( "lookup_skips_a_bundle_superseded_by_a_forced_reload",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();
    const std::shared_ptr<const tileset> older =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_superseded_ts", "color_pixel_sepia_light", inst, tex );
    std::shared_ptr<const tileset> newer =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_superseded_ts", "color_pixel_sepia_light", inst, tex );
    REQUIRE( older );
    REQUIRE( newer );


    newer.reset();

    CHECK_FALSE( renderer_recovery_test_support::cache_lookup_is_fresh(
                     "synthetic_superseded_ts", "color_pixel_sepia_light", inst, tex ) );

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
    renderer_coordinator.drain_pending();

    CHECK( older->get_gpu_textures_generation_at_upload()
           == renderer_coordinator.textures_generation() );
    CHECK_FALSE( renderer_recovery_test_support::cache_lookup_is_fresh(
                     "synthetic_superseded_ts", "color_pixel_sepia_light",
                     renderer_coordinator.instance_generation(),
                     renderer_coordinator.textures_generation() ) );
}

TEST_CASE( "renderer_coordinator_blanking_drain_asks_for_repaint", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    GIVEN( "nothing pending" ) {
        renderer_coordinator.drain_pending();
        THEN( "no repaint requested" ) {
            CHECK_FALSE( renderer_coordinator.take_repaint_request() );
        }
    }
    GIVEN( "a resize that rebuilds the display buffer" ) {
        renderer_recovery_test_support::set_scaling_and_resize_window( 2, 802, 602 );
        renderer_coordinator.drain_pending();
        REQUIRE( renderer_coordinator.is_render_allowed() );
        THEN( "exactly one repaint is requested" ) {
            CHECK( renderer_coordinator.take_repaint_request() );
            CHECK_FALSE( renderer_coordinator.take_repaint_request() );
        }
    }
    GIVEN( "a blanking drain followed by another resize before any redraw" ) {
        renderer_recovery_test_support::set_scaling_and_resize_window( 2, 802, 602 );
        renderer_coordinator.drain_pending();
        renderer_coordinator.notify_resize();
        REQUIRE_FALSE( renderer_coordinator.is_render_allowed() );
        THEN( "the repaint waits until drawing is allowed again" ) {
            CHECK_FALSE( renderer_coordinator.take_repaint_request() );
            renderer_coordinator.drain_pending();
            REQUIRE( renderer_coordinator.is_render_allowed() );
            CHECK( renderer_coordinator.take_repaint_request() );
        }
    }
    GIVEN( "render targets reset" ) {
        renderer_coordinator.request_recovery( renderer_recovery_severity::targets_reset );
        renderer_coordinator.drain_pending();
        REQUIRE( renderer_coordinator.is_render_allowed() );
        THEN( "exactly one repaint is requested" ) {
            CHECK( renderer_coordinator.take_repaint_request() );
            CHECK_FALSE( renderer_coordinator.take_repaint_request() );
        }
    }
}

TEST_CASE( "imgui_font_reload_reaches_the_renderer", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    restore_on_out_of_scope restore_height( fontheight );
    fontheight = 16;
    const std::vector<font_config> gui = { font_config( "data/font/Roboto-Medium.ttf" ) };
    const std::vector<font_config> mono = { font_config( "data/font/Terminus.ttf" ) };
    renderer_recovery_test_support::with_imgui_client( gui, mono, [&]( cataimgui::client & client ) {
        restore_on_out_of_scope restore_inner_height( fontheight );
        ImGuiIO &io = ImGui::GetIO();
        const auto frame_with_text = [&client]() {
            client.new_frame( 640, 384 );
            ImGui::Begin( "font_reload" );
            ImGui::TextUnformatted( "MMMMMMMMMM" );
            ImGui::End();
            client.end_frame();
        };
        frame_with_text();
        REQUIRE_FALSE( client.fonts_reloaded_this_frame() );
        const ImTextureData *old_tex = io.Fonts->TexData;
        fontheight = 24;
        client.reload_fonts( gui, mono );
        frame_with_text();
        CHECK( client.fonts_reloaded_this_frame() );
        CHECK( io.Fonts->Fonts[0]->LegacySize == 24.0f );

        const ImTextureData *tex = io.Fonts->TexData;
        REQUIRE( tex != nullptr );
        CHECK( tex != old_tex );
        CHECK( tex->Status == ImTextureStatus_OK );
        CHECK( tex->GetTexID() != ImTextureID_Invalid );
    } );
}

TEST_CASE( "terminal_font_rebuild_changes_cell_or_keeps_state", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const std::vector<font_config> faces = { font_config( "data/font/Terminus.ttf" ) };

    const std::vector<font_config> no_faces;
    REQUIRE( renderer_recovery_test_support::rebuild_terminal_fonts( point( 8, 16 ), 16, faces,
             faces ) );
    const point before = renderer_recovery_test_support::terminal_cell();
    const Font *font_before = renderer_recovery_test_support::terminal_font();
    const Font *gui_before = renderer_recovery_test_support::terminal_gui_font();
    GIVEN( "faces that load" ) {
        THEN( "the cell becomes the new size" ) {
            REQUIRE( renderer_recovery_test_support::rebuild_terminal_fonts( point( 12, 24 ), 24, faces,
                     faces ) );
            CHECK( renderer_recovery_test_support::terminal_cell() == point( 12, 24 ) );
        }
    }
    GIVEN( "no terminal face that loads" ) {
        THEN( "both font roots and the cell stay as they were" ) {
            CHECK_FALSE( renderer_recovery_test_support::rebuild_terminal_fonts( point( 12, 24 ), 24,
                         no_faces, faces ) );
            CHECK( renderer_recovery_test_support::terminal_cell() == before );
            CHECK( renderer_recovery_test_support::terminal_font() == font_before );
            CHECK( renderer_recovery_test_support::terminal_gui_font() == gui_before );
        }
    }
    GIVEN( "terminal font builds, gui font fails" ) {
        THEN( "neither live font changes" ) {
            CHECK_FALSE( renderer_recovery_test_support::rebuild_terminal_fonts( point( 12, 24 ), 24,
                         faces, no_faces ) );
            CHECK( renderer_recovery_test_support::terminal_cell() == before );
            CHECK( renderer_recovery_test_support::terminal_font() == font_before );
            CHECK( renderer_recovery_test_support::terminal_gui_font() == gui_before );
        }
    }
}

TEST_CASE( "renderer_coordinator_font_change_redraws_in_full", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }

    const point cell = renderer_recovery_test_support::terminal_cell();
    const std::vector<font_config> faces = { font_config( "data/font/Terminus.ttf" ) };
    REQUIRE( renderer_recovery_test_support::rebuild_terminal_fonts( cell, cell.y, faces, faces ) );
    const unsigned epoch_before = cata_cursesport::curses_render_epoch;
    renderer_coordinator.notify_resize();
    renderer_coordinator.drain_pending();
    REQUIRE( renderer_coordinator.is_render_allowed() );
    CHECK( cata_cursesport::curses_render_epoch == epoch_before + 1 );
}

#endif
