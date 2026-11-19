#pragma once
#ifndef CATA_SRC_STRING_EDITOR_WINDOW_H
#define CATA_SRC_STRING_EDITOR_WINDOW_H

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "catacharset.h"
#include "cursesdef.h"
#include "point.h"

class folded_text;
class input_context;
class ui_adaptor;
struct ime_preview_range;









class string_editor_window
{
    private:

        catacurses::window _win;

        std::function<catacurses::window()> _create_window;

        point _max;

        utf8_wrapper _utext;

        std::unique_ptr<folded_text> _folded;


        int _position = -1;

        point _cursor_display;

        int _cursor_desired_x = -1;

        std::unique_ptr<ime_preview_range> _ime_preview_range;

        std::unique_ptr<input_context> ctxt;

    public:
        string_editor_window( const std::function<catacurses::window()> &create_window,
                              const std::string &text );
        ~string_editor_window();



        std::pair<bool, std::string> query_string();

    private:

        void print_editor( ui_adaptor &ui );

        void create_context();


        void cursor_leftright( int diff );
        void cursor_updown( int diff );


        point get_line_and_position( int position, bool zero_x );
};
#endif
