#pragma once
#ifndef CATA_SRC_SDL_RENDERER_RECOVERY_H
#define CATA_SRC_SDL_RENDERER_RECOVERY_H

#if defined(TILES)

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cata_tiles.h"
#include "point.h"

class Font;
struct font_config;
namespace cataimgui
{
class client;
}
namespace catacurses
{
class window;
}




enum class renderer_recovery_severity : int {
    none = 0,
    targets_reset = 1,
    device_reset = 2,
    device_lost = 3,
};

enum class renderer_recovery_state {
    bootstrapping,
    ready,
    recovering,
    retry_needed,
};




enum class lifecycle_state : uint64_t {
    active = 0,
    paused = 1,
    resumed_pending_rebuild = 2,
};




struct renderer_texture_generations {
    uint64_t instance;
    uint64_t textures;
};




enum class recipe_outcome {
    success,
    failure,
    restart_required,
};
struct recipe_result {
    recipe_outcome outcome;
    renderer_recovery_severity restart_severity = renderer_recovery_severity::none;
};









class recovery_drain_planner
{
    public:



        static constexpr uint64_t make_lifecycle_epoch( uint64_t seq, lifecycle_state s ) {
            return ( seq << 2 ) | static_cast<uint64_t>( s );
        }
        static constexpr uint64_t lifecycle_seq_of( uint64_t epoch ) {
            return epoch >> 2;
        }
        static constexpr lifecycle_state lifecycle_state_of( uint64_t epoch ) {
            return static_cast<lifecycle_state>( epoch & 0x3 );
        }
        static constexpr renderer_recovery_severity severity_max(
            renderer_recovery_severity a, renderer_recovery_severity b ) {
            return static_cast<int>( a ) >= static_cast<int>( b ) ? a : b;
        }





        enum class action_kind {
            load_entry_epoch,
            claim_initial_inbox,
            run_recipe,
            attempt_seq_cas,
            claim_after_success,
            claim_after_restart,
            load_post_resize,
            apply_resize,
            finish_ready,
            finish_retry,
        };
        struct action {
            action_kind kind;
            renderer_recovery_severity severity = renderer_recovery_severity::none;
            uint32_t resize_epoch = 0;
            uint64_t serviced_seq = 0;
        };


        renderer_recovery_state state() const {
            return state_;
        }
        bool abort_latch() const {
            return abort_latch_;
        }
        uint32_t observed_resize_epoch() const {
            return observed_resize_epoch_;
        }
        uint64_t serviced_lifecycle_seq() const {
            return serviced_lifecycle_seq_;
        }
        renderer_recovery_severity current_claimed() const {
            return current_claimed_;
        }


        void finish_bootstrap() {
            state_ = renderer_recovery_state::ready;
        }

        void reset() {
            *this = recovery_drain_planner{};
        }




        action begin();
        action advance_entry_epoch( uint64_t entry_epoch );
        action advance_initial_inbox( renderer_recovery_severity pulled, uint32_t resize_epoch,
                                      bool boundary_recovery_pending );
        action advance_recipe( recipe_result r );
        action advance_seq_cas_done();
        action advance_after_success( renderer_recovery_severity pulled, uint64_t live_epoch );
        action advance_after_restart( uint64_t live_epoch, renderer_recovery_severity pulled );
        action advance_post_resize( uint32_t resize_epoch );
        action advance_resize( bool applied );




        void note_pause_abort();



        void acknowledge_resize( uint32_t serviced_resize_epoch ) {
            observed_resize_epoch_ = serviced_resize_epoch;
        }

    private:



        renderer_recovery_severity merge_retry( renderer_recovery_severity pulled );
        action emit_run_recipe( renderer_recovery_severity sev );
        action emit_apply_resize( uint32_t resize_epoch, bool fast_path );
        action finish_ready_action();
        action finish_retry_action();

        renderer_recovery_state state_ = renderer_recovery_state::bootstrapping;
        renderer_recovery_severity retry_severity_ = renderer_recovery_severity::none;


        renderer_recovery_severity current_claimed_ = renderer_recovery_severity::none;
        bool abort_latch_ = false;
        uint32_t observed_resize_epoch_ = 0;



        uint64_t serviced_lifecycle_seq_ = 0;


        renderer_recovery_severity entry_epoch_implied_ = renderer_recovery_severity::none;


        renderer_recovery_severity restart_severity_pending_ = renderer_recovery_severity::none;


        bool recipe_pause_aborted_ = false;


        bool resize_fast_path_ = false;
};


struct renderer_recovery_test_support;






class renderer_resource_coordinator
{
        friend struct renderer_recovery_test_support;
    public:


        void request_recovery( renderer_recovery_severity sev );
        void notify_resize();
        void notify_lifecycle( lifecycle_state ev );




        void seed_renderer_policy( const std::string &renderer_name );



        void finish_bootstrap();




        void begin_atlas_upload();
        void end_atlas_upload();


        void drain_pending();
        bool is_render_allowed() const;
        bool should_abort_frame() const;



        bool lifecycle_paused() const;



        bool take_repaint_request();

        renderer_recovery_state state() const {
            return planner_.state();
        }
        renderer_recovery_severity pending() const {
            return pending_severity_.load();
        }
        uint64_t resource_generation() const {
            return renderer_resource_generation_;
        }
        uint64_t instance_generation() const {
            return renderer_instance_generation_;
        }
        uint64_t textures_generation() const {
            return gpu_textures_generation_;
        }


        renderer_texture_generations texture_generations() const {
            return { renderer_instance_generation_, gpu_textures_generation_ };
        }





        atlas_upload_interrupt mode2_upload_poll();

    private:



        bool external_inbox_blocks_render() const;




        bool recovery_blank_blocked() const;


        void attempt_serviced_seq_cas( uint64_t serviced_seq );
        recipe_result run_recipe_for( renderer_recovery_severity sev );



        void assert_recipe_postcondition( uint64_t resource_gen_before,
                                          renderer_texture_generations gens_before ) const;



        bool apply_resize_only( uint32_t serviced_resize_epoch );
        void run_post_success_full_invalidation();
        bool install_renderer_with_fallback();
        bool safe_pre_teardown_unbind( recipe_result &out );
        bool check_pause_abort();


        void release_live_atlases() const;



        atlas_upload_interrupt replay_live_atlases();



        recipe_result map_replay_interrupt( atlas_upload_interrupt reason );


        atlas_upload_interrupt replay_poll();



        recipe_result setup_target_phase();
        recipe_result recipe_targets_reset();
        recipe_result recipe_device_reset();
        recipe_result recipe_device_lost();
        void record_display_buffer_dims();



        recovery_drain_planner planner_;


        int atlas_upload_depth_ = 0;
        uint64_t renderer_resource_generation_ = 0;
        bool repaint_requested_ = false;
        uint64_t renderer_instance_generation_ = 0;
        uint64_t gpu_textures_generation_ = 0;


        int display_buffer_w_ = 0;
        int display_buffer_h_ = 0;



        bool ui_relayout_pending_ = false;


        uint64_t ui_relayout_count_ = 0;



        std::string renderer_name_;
        bool software_renderer_ = false;

        std::atomic<renderer_recovery_severity> pending_severity_{ renderer_recovery_severity::none };
        std::atomic<uint32_t> pending_resize_epoch_{ 0 };
        std::atomic<uint64_t> lifecycle_epoch_{ 0 };




        atlas_replay_quarantine replay_quarantine_;




        enum class test_phase_action {
            none,
            fail_retry,
            queue_device_reset,
        };
        test_phase_action test_phase_action_ = test_phase_action::none;
        int test_phase_countdown_ = 0;
        bool test_phase_fault_fired_ = false;
        int test_replay_pause_countdown_ = 0;



        int test_mode2_interrupt_countdown_ = 0;
        atlas_upload_interrupt test_mode2_interrupt_ = atlas_upload_interrupt::none;
};



extern renderer_resource_coordinator renderer_coordinator;







class atlas_upload_scope
{
    public:
        explicit atlas_upload_scope( bool active = true )
            : active_( active ) {
            if( active_ ) {
                renderer_coordinator.begin_atlas_upload();
            }
        }
        ~atlas_upload_scope() {
            if( active_ ) {
                renderer_coordinator.end_atlas_upload();
            }
        }
        atlas_upload_scope( const atlas_upload_scope & ) = delete;
        atlas_upload_scope &operator=( const atlas_upload_scope & ) = delete;
    private:
        bool active_;
};





struct renderer_recovery_test_support {



    static bool setup_software_renderer();



    static void teardown_software_renderer();



    static void reset_coordinator();






    static std::shared_ptr<const tileset> install_synthetic_bundle(
        const std::string &tileset_id, const std::string &memory_map_mode,
        uint64_t renderer_instance_generation, uint64_t gpu_textures_generation );





    static std::shared_ptr<const tileset> install_synthetic_bundle(
        const std::string &tileset_id, const std::string &memory_map_mode,
        uint64_t renderer_instance_generation, uint64_t gpu_textures_generation,
        const atlas_bake_plan &plan, bool with_highlight = false,
        const std::vector<std::string> &tile_ids = {} );


    static std::shared_ptr<const tileset> install_atlas_bundle( const std::string &tileset_id,
            const std::string &image_path, const point &sprite_size, int tilecount );

    static std::shared_ptr<const tileset> upload_test_bundle( std::shared_ptr<tileset> ts,
            const atlas_replay_descriptor &desc, const std::string &memory_map_mode,
            uint64_t renderer_instance_generation, uint64_t gpu_textures_generation,
            const atlas_bake_plan &plan );

    static std::shared_ptr<const tileset> install_synthetic_bundle_with_highlight(
        const std::string &tileset_id, const std::string &memory_map_mode,
        uint64_t renderer_instance_generation, uint64_t gpu_textures_generation );




    static std::shared_ptr<const tileset> fetch_cached_bundle(
        const std::string &tileset_id, const std::string &memory_map_mode,
        uint64_t current_renderer_instance_gen, uint64_t current_gpu_textures_gen );




    static bool cache_lookup_is_fresh(
        const std::string &tileset_id, const std::string &memory_map_mode,
        uint64_t current_renderer_instance_gen, uint64_t current_gpu_textures_gen );




    static void arm_phase_fail_retry( int phase_countdown );
    static void arm_phase_queue_device_reset( int phase_countdown );
    static void arm_replay_pause( int poll_countdown );


    static void arm_mode2_interrupt( int poll_countdown, atlas_upload_interrupt interrupt );
    static bool replay_quarantine_empty();



    static atlas_replay_quarantine::gate populate_mode2_quarantine(
        atlas_replay_quarantine &quarantine );

    static bool phase_fault_fired();

    static uint64_t ui_relayout_count();



    static void set_scaling_and_resize_window( int scaling, int window_w, int window_h );

    static void current_display_buffer_dims( int &w, int &h );



    static void current_window_metrics( int &window_w, int &window_h, int &font_w,
                                        int &font_h, int &scaling, int &min_term_w,
                                        int &min_term_h );

    static void current_drawable_dims( int &w, int &h );


    static void override_drawable_pixels( int w, int h );


    static void set_needupdate( bool armed );
    static bool needupdate_armed();



    static void pause_during_draw_scope( bool &bound_during, bool &null_after );



    static void arm_probe_unsafe( int count = 1 );
    static int probe_unsafe_remaining();


    static void arm_flush_failure();


    static void mark_shader_fault();


    static void simulate_draw_bind_failure();

    static int variant_probe_count();


    static bool run_present_gate();




    static void override_shader_variants_available( std::optional<bool> available );






    static bool install_test_font( const std::string &typeface, int w, int h, int size,
                                   bool blending, Uint32 font_pixel_format = SDL_PIXELFORMAT_UNKNOWN );


    static bool draw_test_window( const catacurses::window &w );



    static bool draw_test_minimap_window( const catacurses::window &w, bool fail_unbind,
                                          int &paints );

    static Font *test_font();




    static bool rebuild_terminal_fonts( const point &cell, int point_size,
                                        const std::vector<font_config> &typefaces,
                                        const std::vector<font_config> &gui_typefaces );

    static point terminal_cell();
    static const Font *terminal_font();
    static const Font *terminal_gui_font();


    static void with_imgui_client( const std::vector<font_config> &gui_typefaces,
                                   const std::vector<font_config> &mono_typefaces,
                                   const std::function<void( cataimgui::client & )> &body );


    static std::unique_ptr<cata_tiles> make_test_tiles( const std::shared_ptr<const tileset> &ts );

    static void draw_test_map( cata_tiles &tiles, const tripoint_bub_ms &center, int w, int h );

    static void draw_test_overmap( cata_tiles &tiles, const tripoint_abs_omt &center );
    static void set_has_animated_tiles( cata_tiles &tiles, bool animated );


    static void log_draw_light( cata_tiles &tiles, std::vector<drawn_sprite_record> *log );

    static std::shared_ptr<const tileset> install_tiles_bundle( const std::string &tileset_id,
            const std::string &memory_map_mode, uint64_t renderer_instance_generation,
            uint64_t gpu_textures_generation, const atlas_bake_plan &plan, bool with_highlight,
            const std::vector<std::pair<std::string, tile_type>> &tiles );
};


class software_render_fixture
{
    public:
        software_render_fixture()
            : available_( renderer_recovery_test_support::setup_software_renderer() ) {}
        ~software_render_fixture() {
            if( available_ ) {
                renderer_recovery_test_support::teardown_software_renderer();
            }
        }
        software_render_fixture( const software_render_fixture & ) = delete;
        software_render_fixture &operator=( const software_render_fixture & ) = delete;
        bool available() const {
            return available_;
        }
    private:
        bool available_ = false;
};

#endif

#endif
