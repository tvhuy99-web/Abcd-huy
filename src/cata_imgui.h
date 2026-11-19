#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

#include "color.h"
#include "font_loader.h"

class nc_color;
struct input_event;
using ImGuiInputTextFlags = int;

#if defined(IMTUI) || !(defined(WIN32) || defined(TILES))
#   define TUI
#endif

#ifndef TUI
#include "sdl_geometry.h"
#include "sdl_wrappers.h"
#include "color_loader.h"
#endif
#include "text.h"

struct point;
struct ImVec2;
struct ImVec4;
class Font;
class input_context;

namespace cataimgui
{
struct bounds {
    float x;
    float y;
    float w;
    float h;
};

enum class mbox_btn {
    BT_OK = 0,
    BT_OKCancel = 1,
    BT_YesNoCancel = 2,
    BT_YesNo = 3
};

enum class dialog_result {
    None = 0,
    OKClicked,
    CancelClicked,
    YesClicked,
    NoClicked
};

enum class scroll : int {
    none = 0,
    begin,
    end,
    line_up,
    line_down,
    page_up,
    page_down
};


constexpr int min_screen_res_x = 640;
constexpr int min_screen_res_y = 384;

#ifndef TUI

class font_reload
{
    public:

        void request( const std::vector<font_config> &gui_typefaces,
                      const std::vector<font_config> &mono_typefaces );
        bool pending() const {
            return pending_;
        }

        bool apply( ImGuiIO &io, bool cjk );
    private:
        bool pending_ = false;
        std::vector<font_config> gui_typefaces_;
        std::vector<font_config> mono_typefaces_;
};
#endif

class client
{
        std::vector<int> cata_input_trail;
#ifndef TUI



        bool platform_backend_active_ = false;
        bool renderer_backend_active_ = false;
        font_reload font_reload_;
        bool fonts_reloaded_this_frame_ = false;
#endif
    public:
#ifdef TUI
        client();
#else
        client( const SDL_Renderer_Ptr &sdl_renderer, const SDL_Window_Ptr &sdl_window,
                const GeometryRenderer_Ptr &sdl_geometry );
        void load_fonts( const std::unique_ptr<Font> &gui_font, const std::unique_ptr<Font> &mono_font,
                         const std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> &windowsPalette,
                         const std::vector<font_config> &gui_typeface, const std::vector<font_config> &mono_typeface );
#endif
        ~client();




        void new_frame( int display_buffer_w = 0, int display_buffer_h = 0 );
        void end_frame();



        void abort_frame();
        void process_input( void *input, int display_buffer_w = 0, int display_buffer_h = 0,
                            int scaling_factor = 1 );
        void process_cata_input( const input_event &event );
#ifdef TUI
        void upload_color_pair( int p, int f, int b );
        void set_alloced_pair_count( short count );
#else
        const SDL_Renderer_Ptr &sdl_renderer;
        const SDL_Window_Ptr &sdl_window;
        const GeometryRenderer_Ptr &sdl_geometry;




        void init_platform_backend();
        void init_renderer_backend();
        void shutdown_renderer_backend();
        void shutdown_platform_backend();



        void destroy_backend_device_objects() const;

        void reload_fonts( const std::vector<font_config> &gui_typefaces,
                           const std::vector<font_config> &mono_typefaces ) {
            font_reload_.request( gui_typefaces, mono_typefaces );
        }

        bool fonts_reloaded_this_frame() const {
            return fonts_reloaded_this_frame_;
        }
#endif
        bool auto_size_frame_active();
        bool any_window_shown();


        static bool want_capture_mouse();

        static bool want_capture_keyboard();

        static bool want_text_input();

        static void clear_text_focus();
};

#ifndef TUI



point imgui_frame_display_size( int display_buffer_w, int display_buffer_h,
                                int renderer_output_w, int renderer_output_h );

void add_cata_fonts( ImGuiIO &io, const std::vector<font_config> &gui_typefaces,
                     const std::vector<font_config> &mono_typefaces, bool cjk );
#endif

void point_to_imvec2( point *src, ImVec2 *dest );
void imvec2_to_point( ImVec2 *src, point *dest );

ImVec4 imvec4_from_color( const nc_color &color );

ImU32 ImU32_from_color( const nc_color &color );

void set_scroll( scroll &s );

void draw_colored_text( const std::string &original_text, const nc_color &color,
                        float wrap_width = 0.0F, bool *is_selected = nullptr,
                        bool *is_focused = nullptr, bool *is_hovered = nullptr );
#ifndef TUI
bool clear_pending();
#endif
void draw_colored_text( const std::string &original_text, nc_color &color,
                        float wrap_width = 0.0F, bool *is_selected = nullptr,
                        bool *is_focused = nullptr, bool *is_hovered = nullptr );
void draw_colored_text( const std::string &original_text,
                        float wrap_width = 0.0F, bool *is_selected = nullptr,
                        bool *is_focused = nullptr, bool *is_hovered = nullptr );


size_t get_string_width( std::string_view str );

size_t get_string_height( std::string_view str, float wrap_width );

class window
{
        std::unique_ptr<class window_impl> p_impl;
        std::unique_ptr<class filter_box_impl> filter_impl;
        bounds cached_bounds;
    protected:
        explicit window( int window_flags = 0 );
    public:
        explicit window( const std::string &id_, int window_flags = 0 );
        virtual ~window();
        bool action_button( const std::string &action, const std::string &text );
        bool has_button_action();
        std::string get_button_action();
        void draw_header( std::string const &text );
        bool get_is_open() const;
        virtual void draw();
        virtual void on_resized() {}
        bool is_bounds_changed();
        size_t get_text_width( std::string_view text );
        size_t get_text_height( const char *text );
        size_t str_width_to_pixels( size_t len );
        size_t str_height_to_pixels( size_t len );
        std::string get_filter();
        void clear_filter();
        void defocus_filter();
        void mark_resized();

    protected:
        bool force_to_back = false;
        bool hide_ui = false;
        bool is_open;
        std::string id;
        int window_flags;
        std::string button_action;
        virtual bounds get_bounds();
        virtual void draw_controls() = 0;
        void draw_filter( const input_context &ctxt, bool filtering_active );
        void hide_if_hidden() const;
};

#ifdef TUI
void init_pair( int p, int f, int b );
void load_colors();
#endif


bool InputFloat( const char *label, float *v, float step = 0.0f, float step_fast = 0.0f,
                 const char *format = "%.3f", ImGuiInputTextFlags flags = 0 );

void PushGuiFont();
void PushMonoFont();
void PushGuiFont1_5x();
void PopGuiFont1_5x();

bool BeginRightAlign( const char *str_id );
void EndRightAlign();



bool BeginTabItem( const char *label, bool is_selected, bool *p_open = nullptr,
                   ImGuiTabItemFlags flags = 0 );




void init_colors();











void TextKeybinding( const input_context &ctxt,
                     const char *action, const char *description, bool active,
                     int max_limit = 0, const nc_color &default_color = c_light_gray );





void TextListSeparator( const nc_color &color = c_light_gray );

}
