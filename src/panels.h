#pragma once
#ifndef CATA_SRC_PANELS_H
#define CATA_SRC_PANELS_H

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "coords_fwd.h"
#include "translation.h"
#include "type_id.h"

class JsonArray;
class JsonOut;
class avatar;
class widget;
struct point;

enum face_type : int {
    face_human = 0,
    face_bird,
    face_bear,
    face_cat,
    num_face_types
};

namespace catacurses
{
class window;
}

namespace overmap_ui
{
void draw_overmap_chunk( const catacurses::window &w_minimap, const avatar &you,
                         const tripoint_abs_omt &global_omt, const point &start, int width,
                         int height );
void draw_overmap_chunk_imgui( const avatar &you, const tripoint_abs_omt &global_omt,
                               int width, int height );
}

bool default_render();




struct draw_args {
    public:
        const avatar &_ava;
        const catacurses::window &_win;

        draw_args( const avatar &a, const catacurses::window &w, const widget_id &wgt ) :
            _ava( a ), _win( w ), _wgt( wgt ) {}

        widget *get_widget() const {
            return _wgt.is_null() ? nullptr : const_cast<widget *>( &*_wgt );
        }
    private:
        widget_id _wgt;
};





class window_panel
{
    public:
        window_panel( const std::function<void( const draw_args & )> &draw_func,
                      const std::string &id, const translation &nm, int ht, int wd, bool default_toggle_,
                      const std::function<bool()> &render_func = default_render, bool force_draw = false );

        window_panel( const std::string &id, const translation &nm, int ht, int wd, bool default_toggle_,
                      const std::function<bool()> &render_func = default_render, bool force_draw = false );



        std::function<int( const draw_args & )> draw;
        std::function<bool()> render;

        int get_height() const;
        int get_width() const;
        const std::string &get_id() const;
        std::string get_name() const;
        void set_widget( const widget_id &w );
        const widget_id &get_widget() const;
        void set_draw_func( const std::function<int( const draw_args & )> &draw_func );
        bool toggle;
        bool always_draw;

    private:
        int height;
        int width;
        widget_id wgt;
        std::string id;
        translation name;
};



class panel_layout
{
    public:
        panel_layout( const translation &_name,
                      const std::vector<window_panel> &_panels );

        std::string name() const;
        const std::vector<window_panel> &panels() const;
        std::vector<window_panel> &panels();
    private:
        translation _name;
        std::vector<window_panel> _panels;
};




class panel_manager
{
    public:
        panel_manager();
        ~panel_manager() = default;
        panel_manager( panel_manager && ) = default;
        panel_manager( const panel_manager & ) = default;
        panel_manager &operator=( panel_manager && ) = default;
        panel_manager &operator=( const panel_manager & ) = default;

        static panel_manager &get_manager() {
            static panel_manager single_instance;
            return single_instance;
        }

        panel_layout &get_current_layout();
        widget *get_current_sidebar();
        widget *get_sidebar( const std::string &name );
        std::string get_current_layout_id() const;
        int get_width_right() const;
        int get_width_left() const;

        void show_adm();

        void init();

    private:
        bool save();
        bool load();
        void serialize( JsonOut &json );
        void deserialize( const JsonArray &ja );

        void update_offsets( int x );


        int width_right = 0; // NOLINT(cata-serialize)
        int width_left = 0; // NOLINT(cata-serialize)
        std::string current_layout_id;
        std::map<std::string, panel_layout> layouts;

        friend widget;
};

#endif
