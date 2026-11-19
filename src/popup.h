#pragma once
#ifndef CATA_SRC_POPUP_H
#define CATA_SRC_POPUP_H

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "color.h"
#include "input_enums.h"
#include "point.h"
#include "string_formatter.h"

class query_popup_impl;



















class query_popup
{
        friend class query_popup_impl;
    public:






















        struct result {
            result();
            result( bool wait_input, const std::string &action, const input_event &evt );

            bool wait_input;
            std::string action;
            input_event evt;
        };






        query_popup();








        query_popup &context( const std::string &cat );



        template <typename ...Args>
        query_popup &message( const std::string &fmt, Args &&... args ) {
            assert_format<Args...>( fmt );
            invalidate_ui();
            text = string_format( fmt, std::forward<Args>( args )... );
            return *this;
        }
        template <typename ...Args>
        query_popup &message( const char *const fmt, Args &&... args ) {
            assert_format<Args...>( fmt );
            invalidate_ui();
            text = string_format( fmt, std::forward<Args>( args )... );
            return *this;
        }



        template <typename ...Args>
        query_popup &wait_message( const nc_color &bar_color, const std::string &fmt, Args &&... args ) {
            assert_format<Args...>( fmt );
            invalidate_ui();
            text = wait_text( string_format( fmt, std::forward<Args>( args )... ), bar_color );
            return *this;
        }
        template <typename ...Args>
        query_popup &wait_message( const nc_color &bar_color, const char *const fmt, Args &&... args ) {
            assert_format<Args...>( fmt );
            invalidate_ui();
            text = wait_text( string_format( fmt, std::forward<Args>( args )... ), bar_color );
            return *this;
        }
        template <typename ...Args>
        query_popup &wait_message( const std::string &fmt, Args &&... args ) {
            assert_format<Args...>( fmt );
            invalidate_ui();
            text = wait_text( string_format( fmt, std::forward<Args>( args )... ) );
            return *this;
        }
        template <typename ...Args>
        query_popup &wait_message( const char *const fmt, Args &&... args ) {
            assert_format<Args...>( fmt );
            invalidate_ui();
            text = wait_text( string_format( fmt, std::forward<Args>( args )... ) );
            return *this;
        }





        query_popup &option( const std::string &opt );







        query_popup &option( const std::string &opt,
                             const std::function<bool( const input_event & )> &filter );



        query_popup &allow_anykey( bool allow );





        query_popup &allow_cancel( bool allow );



        query_popup &on_top( bool top );




        query_popup &full_screen( bool full );



        query_popup &cursor( size_t pos );



        query_popup &default_color( const nc_color &d_color );




        query_popup &preferred_keyboard_mode( keyboard_mode mode );







        result query_once();



        result query();
    protected:




        std::shared_ptr<query_popup_impl> create_or_get_impl();

    private:
        struct query_option {
            query_option( const std::string &action,
                          const std::function<bool( const input_event & )> &filter );

            std::string action;
            std::function<bool( const input_event & )> filter;
        };

        std::string category;
        std::string text;
        std::vector<query_option> options;
        size_t cur;
        nc_color default_text_color;
        bool anykey;
        bool cancel;
        bool ontop;
        bool fullscr;
        keyboard_mode pref_kbd_mode;

        struct button {
            button( const std::string &text, const point & );
            bool contains( const point &p ) const;

            std::string text;
            point pos;
            int width;
        };

        std::weak_ptr<query_popup_impl> p_impl;


        mutable std::vector<std::string> folded_msg;
        mutable std::vector<button> buttons;

        void invalidate_ui() const;

        template <typename ...Args>
        static void assert_format( const std::string_view ) {
            static_assert( sizeof...( Args ) > 0,
                           "Format string should take at least one argument.  "
                           "If your message is not a format string, "
                           "use `message( \"%s\", text )` instead." );
        }

        static std::string wait_text( const std::string &text, const nc_color &bar_color );
        static std::string wait_text( const std::string &text );
};
























class static_popup : public query_popup
{
    public:
        static_popup();

    private:
        std::shared_ptr<query_popup_impl> ui;
};

#endif
