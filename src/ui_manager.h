#pragma once
#ifndef CATA_SRC_UI_MANAGER_H
#define CATA_SRC_UI_MANAGER_H

#include <stddef.h>
#include <functional>
#include <memory>

#include "cuboid_rectangle.h"
#include "point.h"

namespace cataimgui
{
class client;
}

extern std::unique_ptr<cataimgui::client> imclient;

namespace catacurses
{
class window;
}





















































class ui_adaptor
{
    public:
        bool is_imgui;
        bool is_on_top;
        using redraw_callback_t = std::function<void( ui_adaptor & )>;
        using screen_resize_callback_t = std::function<void( ui_adaptor & )>;

        struct disable_uis_below {
        };

        struct debug_message_ui {
        };






        ui_adaptor();




        explicit ui_adaptor( disable_uis_below );







        explicit ui_adaptor( debug_message_ui );
        ui_adaptor( const ui_adaptor &rhs ) = delete;
        ui_adaptor( ui_adaptor &&rhs ) = delete;
        ~ui_adaptor();

        ui_adaptor &operator=( const ui_adaptor &rhs ) = delete;
        ui_adaptor &operator=( ui_adaptor &&rhs ) = delete;









        void position_from_window( const catacurses::window &win );






        void position( const point &topleft, const point &size );




        void position_absolute( const point &topleft, const point &size );























        void on_redraw( const redraw_callback_t &fun );

        void on_screen_resize( const screen_resize_callback_t &fun );














        void set_cursor( const catacurses::window &win, const point &pos );






        void record_cursor( const catacurses::window &win );





        void record_term_cursor();




        void default_cursor();






        void disable_cursor();








        void mark_resize() const;







        void invalidate_ui() const;





        void reset();

        void shutdown();


        static void invalidate( const rectangle<point> &rect, bool reenable_uis_below );
        static bool has_imgui();
        static void redraw();
        static void redraw_invalidated();
        static void screen_resized();

        static void mark_all_for_resize();
        static size_t ui_stack_size();
    private:
        static void invalidation_consistency_and_optimization();


        rectangle<point> dimensions;
        redraw_callback_t redraw_cb;
        screen_resize_callback_t screen_resized_cb;





        enum class cursor {
            last, custom, disabled
        };
        cursor cursor_type = cursor::last;
        point cursor_pos;

        bool disabling_uis_below;
        bool is_debug_message_ui;
        bool is_shutting_down = false;

        mutable bool invalidated;
        mutable bool deferred_resize;
};





class background_pane
{
    public:
        background_pane();
    private:
        ui_adaptor ui;
};


namespace ui_manager
{





void invalidate( const rectangle<point> &rect, bool reenable_uis_below );







void redraw();



void redraw_invalidated();




void screen_resized();
void invalidate_all_ui_adaptors();
void reset();
}

#endif
