#pragma once
#ifndef CATA_SRC_CATA_SHADER_H
#define CATA_SRC_CATA_SHADER_H

#if defined(TILES)

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "point.h"
#include "sdl_wrappers.h"
#include "smooth_lighting.h"

struct renderer_recovery_test_support;

namespace cata_shader
{




void request_reprobe();
bool reprobe_requested();
void clear_reprobe();



enum class probe_state {
    available,
    unavailable,

    unsafe,
};




void test_arm_probe_unsafe( int count );
int test_probe_unsafe_remaining();
void test_arm_flush_failure();
int test_probe_runs();
void test_reset_seams();






enum class variant_kind : int {
    NORMAL = 0,
    SHADOW,
    NIGHT,
    OVEREXPOSED,
    MEMORY,
    count
};




bool variant_takes_tint( variant_kind v );




enum class memory_preset : int {
    DARKEN = 0,
    SEPIA_LIGHT,
    SEPIA_DARK,
    BLUE_DARK,
    count
};



std::optional<memory_preset> memory_preset_from_option_value(
    const std::string &mode );



class shader
{
    public:









        static shader load_fragment( SDL_GPUDevice *device,
                                     const std::string &basename,
                                     unsigned int num_samplers,
                                     unsigned int num_uniform_buffers );

        shader() = default;
        ~shader();

        shader( const shader & ) = delete;
        shader &operator=( const shader & ) = delete;
        shader( shader &&other ) noexcept;
        shader &operator=( shader &&other ) noexcept;

        bool is_valid() const {
            return ptr_ != nullptr && device_ != nullptr;
        }
        SDL_GPUShader *get() const {
            return ptr_;
        }




        void abandon() {
            ptr_ = nullptr;
            device_ = nullptr;
        }

    private:
        shader( SDL_GPUDevice *device, SDL_GPUShader *ptr )
            : device_( device ), ptr_( ptr ) {}

        SDL_GPUDevice *device_ = nullptr;
        SDL_GPUShader *ptr_ = nullptr;
};




class render_state
{
    public:





        static render_state create( SDL_Renderer *renderer,
                                    const shader &fragment_shader );


        static render_state create( SDL_Renderer *renderer, const shader &fragment_shader,
                                    const SDL_GPUTextureSamplerBinding *bindings, int num_bindings );

        render_state() = default;
        ~render_state();

        render_state( const render_state & ) = delete;
        render_state &operator=( const render_state & ) = delete;
        render_state( render_state &&other ) noexcept;
        render_state &operator=( render_state &&other ) noexcept;

        bool is_valid() const {
            return ptr_ != nullptr;
        }
        SDL_GPURenderState *get() const {
            return ptr_;
        }



        void abandon() {
            ptr_ = nullptr;
        }

    private:
        explicit render_state( SDL_GPURenderState *ptr ) : ptr_( ptr ) {}

        SDL_GPURenderState *ptr_ = nullptr;
};



struct memory_look {
    std::optional<memory_preset> preset;

    std::array<float, 3> custom_dark = {};
    std::array<float, 3> custom_light = {};
    float custom_gamma = 1.0f;
};


constexpr int32_t custom_memory_look = static_cast<int32_t>( memory_preset::count );
static_assert( custom_memory_look == smooth_lighting::custom_look,
               "lit shaders and their reference agree on the custom look id" );

struct lit_frame {
    SDL_Texture *lightmap = nullptr;


    SDL_Texture *prefiltered = nullptr;
    smooth_lighting::prefilter_layout layout;
    smooth_lighting::lookup lookup = smooth_lighting::lookup::hardware;
    memory_look memory;

    bool blend_memory = false;
    bool per_tile = false;
    bool iso = false;
    bool night_vision = false;
};


enum class lit_begin_outcome : uint8_t {

    active,

    classic,

    failed,

    abort_frame,
};

struct lit_begin_result {
    lit_begin_outcome outcome = lit_begin_outcome::classic;
    std::optional<smooth_lighting::lit_failure> failure;
};

smooth_lighting::lit_frame_action action_for( lit_begin_outcome o );


struct lit_prepare_result {
    lit_begin_outcome outcome = lit_begin_outcome::classic;
    std::optional<smooth_lighting::lit_failure> failure;
    smooth_lighting::lit_capability capability;
};



enum class prefilter_outcome : uint8_t {
    ok,
    failed,
    unsafe,
};



SDL_Texture *create_prefilter_target( SDL_Renderer *renderer, const point &size );



namespace lit_probe
{
struct rgb {
    int r = 0;
    int g = 0;
    int b = 0;
};


constexpr int channel_tolerance = 4;

struct probe_case {
    std::string name;


    std::vector<smooth_lighting::lightmap_texel> texels;
    int columns = 0;
    int rows_per_level = 0;
    int levels = 3;

    std::array<uint8_t, 4> source = { 128, 128, 128, 255 };
    lit_frame frame;
    bool night = false;


    point size;
    std::array<smooth_lighting::lit_coords, 4> corners;
};

smooth_lighting::lightmap_view view_of( const probe_case &c );


smooth_lighting::prefilter_layout layout_of( const probe_case &c );
std::vector<probe_case> cases();

std::vector<rgb> expected( const probe_case &c );
bool matches( const std::vector<rgb> &expected, const std::vector<rgb> &readback );

std::vector<rgb> readback_if_ignored( const probe_case &c );

std::vector<rgb> readback_if_full_light( const probe_case &c );
}


























class variant_pass
{
    public:
        explicit variant_pass( SDL_Renderer *renderer );
        ~variant_pass();

        variant_pass( const variant_pass & ) = delete;
        variant_pass &operator=( const variant_pass & ) = delete;
        variant_pass( variant_pass && ) = delete;
        variant_pass &operator=( variant_pass && ) = delete;



        bool available() const {
            return probed_ok_ && !session_disabled_;
        }


        bool tint_available() const {
            return available();
        }




        probe_state ensure_probed();



        bool boundary_lost() const {
            return boundary_lost_;
        }


        uint32_t resource_generation() const {
            return resource_generation_;
        }



        bool shader_fault() const {
            return shader_fault_;
        }
        std::optional<memory_preset> active_memory_preset() const {
            return active_memory_preset_;
        }


        void note_draw_bind_failure( bool log_error = true );



        // session_disabled) -- renderer valid, fall through to the pre-baked



        enum class begin_result {
            bound,
            use_atlas,
            abort_frame,
        };



        begin_result try_begin( variant_kind v, bool tinted = false );
        bool end();




        bool flush();

        void select_memory_preset( std::optional<memory_preset> preset );



        lit_prepare_result prepare_lit();










        lit_begin_result begin_lit( const lit_frame &frame );

        prefilter_outcome prefilter_lit( const lit_frame &frame );

        void end_lit();



        bool drop_lit();


        bool drop_lit_states();
        bool lit_active() const {
            return lit_active_;
        }


        bool lit_takes( variant_kind v ) const;


        void set_lit_suspended( bool suspended ) {
            lit_suspended_ = suspended;
        }




        void release_gpu_resources();




        void force_abandon_gpu_resources();




        void rebind_renderer( SDL_Renderer *renderer );

    private:
        friend struct ::renderer_recovery_test_support;

        void probe();
        void reset();
        void mark_probe_unsafe();
        void mark_flush_failed();



        void clear_state_arrays( bool abandon_handles );
        SDL_GPURenderState *state_for( variant_kind v, bool tinted ) const;
        void release_lit( bool abandon_handles );

        SDL_Renderer *renderer_ = nullptr;
        std::array<shader, static_cast<size_t>( variant_kind::count )> shaders_;
        std::array<render_state, static_cast<size_t>( variant_kind::count )> states_;
        std::array<shader, static_cast<size_t>( memory_preset::count )> memory_shaders_;
        std::array<render_state, static_cast<size_t>( memory_preset::count )>
        memory_states_;
        shader tint_shader_;
        render_state tint_state_;
        std::optional<memory_preset> active_memory_preset_;
        shader lit_shader_;
        shader nv_lit_shader_;
        render_state lit_state_;
        render_state nv_lit_state_;

        shader prefilter_shader_;
        render_state prefilter_state_;
        SDL_GPUDevice *lit_device_ = nullptr;
        SDL_GPUSampler *lit_sampler_ = nullptr;
        SDL_GPUSampler *lit_linear_sampler_ = nullptr;

        SDL_Texture *lit_texture_ = nullptr;
        SDL_GPUSampler *lit_bound_sampler_ = nullptr;

        struct lit_params {

            std::array<int32_t, 4> size = {};

            std::array<int32_t, 4> mode = { -1, 0, 0, 0 };

            std::array<int32_t, 4> flags = {};

            std::array<float, 4> tone = {};

            std::array<float, 4> custom_dark = {};
            std::array<float, 4> custom_light = {};

            std::array<float, 4> look = {};

            std::array<int32_t, 4> prefilter = {};


            std::array<int32_t, 4> prefilter_grid = {};
            bool operator==( const lit_params &o ) const {
                return size == o.size && mode == o.mode && flags == o.flags && tone == o.tone &&
                       custom_dark == o.custom_dark && custom_light == o.custom_light && look == o.look &&
                       prefilter == o.prefilter && prefilter_grid == o.prefilter_grid;
            }
        };
        static_assert( sizeof( lit_params ) == 9 * 16, "lit_params is a std140 block of nine vec4" );
        static_assert( std::is_trivially_copyable_v<lit_params> );

        static lit_params make_lit_params( const lit_frame &frame );


        smooth_lighting::probe_group_result probe_lit_group( bool filtered,
                smooth_lighting::lookup lookup );
        smooth_lighting::lit_capability probe_lit();

        prefilter_outcome run_prefilter( SDL_Texture *lightmap, SDL_Texture *target,
                                         const lit_params &params, const point &size );
        lit_params lit_params_;
        bool lit_active_ = false;
        bool lit_suspended_ = false;

        bool lit_night_vision_ = false;

        std::optional<smooth_lighting::lit_capability> lit_capability_;
        uint32_t resource_generation_ = 0;
        SDL_GPURenderState *bound_state_ = nullptr;



        bool unbind_required_ = false;





        bool abandoned_pending_rebind_ = false;
        bool probe_attempted_ = false;
        bool probed_ok_ = false;
        bool session_disabled_ = false;
        bool boundary_lost_ = false;
        bool shader_fault_ = false;
};

}

#endif

#endif
