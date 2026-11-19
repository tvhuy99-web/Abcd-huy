#pragma once
#ifndef CATA_SRC_SDLTILES_H
#define CATA_SRC_SDLTILES_H

#include "point.h" // IWYU pragma: keep

namespace catacurses
{
class window;
}

#if defined(TILES)

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "atlas_bake_plan.h"
#include "color_loader.h"
#include "coords_fwd.h"
#include "sdl_wrappers.h"
#include "string_id.h"

#if defined(__APPLE__)

#include <TargetConditionals.h>
#endif

class cata_tiles;

struct weather_type;

using weather_type_id = string_id<weather_type>;

namespace catacurses
{
class window;
}

extern std::shared_ptr<cata_tiles> tilecontext;
extern std::shared_ptr<cata_tiles> closetilecontext;
extern std::shared_ptr<cata_tiles> fartilecontext;
extern std::unique_ptr<cata_tiles> overmap_tilecontext;
extern std::shared_ptr<cata_tiles> portrait_tilecontext;
extern std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> windowsPalette;
extern int fontheight;
extern int fontwidth;




void load_tileset();
void rescale_tileset( int size );
bool save_screenshot( const std::string &file_path );
void toggle_fullscreen_window();

struct window_dimensions {
    point scaled_font_size;
    point window_pos_cell;
    point window_size_cell;
    point window_pos_pixel;
    point window_size_pixel;
};
window_dimensions get_window_dimensions( const catacurses::window &win );


window_dimensions get_window_dimensions( const point &pos, const point &size );

const SDL_Renderer_Ptr &get_sdl_renderer();


std::optional<std::string> gpu_backend_hint( const std::string &option_value, bool env_override );



bool clear_sdl_window();


SDL_Window *get_sdl_window();



void get_display_buffer_dims( int *w, int *h );



SDL_Point window_to_display_buffer_coords( SDL_Point window_pt );




void convert_event_to_display_buffer_coords( SDL_Event *event );

class smooth_lightmap;
namespace cata_shader
{
class variant_pass;
}




cata_shader::variant_pass *get_shared_variant_pass();


smooth_lightmap *get_shared_lightmap();





bool unbind_sprite_shader();



bool display_buffer_scope_is_invalid();




bool display_buffer_scope_recovery_pending();



void display_buffer_scope_signal_recovery_required();


void display_buffer_scope_clear_recovery_required();



struct tile_atlas_config {
    std::string mode;
    uint64_t fingerprint = 0;
};




void apply_tile_atlas_options();
const tile_atlas_config &applied_tile_atlas_config();




void on_tiles_options_changed();





std::optional<atlas_bake_plan> resolve_atlas_bake_plan( const std::string &memory_map_mode );





bool renderer_should_abort_frame();




uint64_t renderer_resource_generation();







class display_buffer_draw_scope
{
    public:




        explicit display_buffer_draw_scope( bool allow_during_recovery = false );
        ~display_buffer_draw_scope();
        display_buffer_draw_scope( const display_buffer_draw_scope & ) = delete;
        display_buffer_draw_scope &operator=( const display_buffer_draw_scope & ) = delete;
        display_buffer_draw_scope( display_buffer_draw_scope && ) = delete;
        display_buffer_draw_scope &operator=( display_buffer_draw_scope && ) = delete;




        void abort_unbind();





        bool should_draw() const;

    private:
        bool outermost_ = false;
        bool active_ = false;
};

#endif


bool window_contains_point_relative( const catacurses::window &win, const point &p );
#endif
