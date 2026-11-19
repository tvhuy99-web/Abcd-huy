#pragma once
#ifndef CATA_SRC_CURSESPORT_H
#define CATA_SRC_CURSESPORT_H

#if defined(IMTUI) || !(defined(TILES) || defined(WIN32))
#   define TUI
#endif

#ifndef TUI
#include <array>
#include <string>
#include <vector>
#include <utility>

#include "point.h"

namespace catacurses
{
class window;

enum base_color : short;
}






namespace cata_cursesport
{
using base_color = catacurses::base_color;


struct pairs {
    base_color FG;
    base_color BG;
};


struct cursecell {
    std::string ch;
    base_color FG = static_cast<base_color>( 0 );
    base_color BG = static_cast<base_color>( 0 );

    explicit cursecell( std::string ch ) : ch( std::move( ch ) ) { }
    cursecell() : cursecell( std::string( 1, ' ' ) ) { }

    bool operator==( const cursecell &b ) const {
        return FG == b.FG && BG == b.BG && ch == b.ch;
    }
};

struct curseline {
    bool touched;
    std::vector<cursecell> chars;
};


struct WINDOW {

    point pos;
    int width;
    int height;

    base_color FG;

    base_color BG;

    bool inuse;

    bool draw;




    unsigned last_render_epoch = 0;
    point cursor;
    std::vector<curseline> line;
};

extern std::array<pairs, 100> colorpairs;


extern unsigned curses_render_epoch;
void bump_curses_render_epoch();
void curses_drawwindow( const catacurses::window &win );

}



void clear_window_area( const catacurses::window &win );
int projected_window_width();
int projected_window_height();
bool handle_resize( int w, int h );
void resize_term( int cell_w, int cell_h );

void request_terminal_relayout();
int get_scaling_factor();
#endif
#endif
