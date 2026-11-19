#pragma once
#ifndef CATA_SRC_CURSESDEF_H
#define CATA_SRC_CURSESDEF_H

#include <memory>
#include <string>
#include <utility>

#include "string_formatter.h"

class nc_color;
struct point;
























namespace catacurses
{



void init_interface();











class window
{
    private:
        std::shared_ptr<void> native_window;

    public:
        window() = default;
        explicit window( std::shared_ptr<void> ptr ) : native_window( std::move( ptr ) ) {
        }
        template<typename T = void>
        T * get() const {
            return static_cast<T *>( native_window.get() );
        }
        explicit operator bool() const {
            return native_window.operator bool();
        }
        bool operator==( const window &rhs ) const {
            return native_window.get() == rhs.native_window.get();
        }
        std::weak_ptr<void> weak_ptr() const {
            return native_window;
        }
};

#if defined(USE_PDCURSES) && !defined(PDC_RGB)
enum base_color : short {
    black = 0x00,
    red = 0x04,
    green = 0x02,
    yellow = 0x06,
    blue = 0x01,
    magenta = 0x05,
    cyan = 0x03,
    white = 0x07,


    dark_gray = 237,
};
#else
enum base_color : short {
    black = 0x00,
    red = 0x01,
    green = 0x02,
    yellow = 0x03,
    blue = 0x04,
    magenta = 0x05,
    cyan = 0x06,
    white = 0x07,


    dark_gray = 237,
};
#endif

using chtype = int;
using attr_t = unsigned short;

extern window stdscr;
#if defined(USE_PDCURSES)
inline constexpr window &newscr = stdscr;
#else
extern window newscr;
#endif

window newwin( int nlines, int ncols, const point &begin );
void wborder( const window &win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr,
              chtype bl, chtype br );
void mvwhline( const window &win, const point &p, chtype ch, int n );
void mvwvline( const window &win, const point &p, chtype ch, int n );
void wnoutrefresh( const window &win );
void wrefresh( const window &win );
void refresh();
void doupdate();
void wredrawln( const window &win, int beg_line, int num_lines );
void mvwprintw( const window &win, const point &p, const std::string &text );
template<typename ...Args>
inline void mvwprintw( const window &win, const point &p, const char *const fmt,
                       Args &&... args )
{
    return mvwprintw( win, p, string_format( fmt, std::forward<Args>( args )... ) );
}

void wprintw( const window &win, const std::string &text );
template<typename ...Args>
inline void wprintw( const window &win, const char *const fmt, Args &&... args )
{
    return wprintw( win, string_format( fmt, std::forward<Args>( args )... ) );
}

void resizeterm();
void werase( const window &win );
void init_pair( short pair, base_color f, base_color b );
void wmove( const window &win, const point &p );
void clear();
void erase();
void endwin();
void mvwaddch( const window &win, const point &p, chtype ch );
void wclear( const window &win );
void curs_set( int visibility );

void wattron( const window &win, const nc_color &attrs );

void wattroff( const window &win, nc_color attrs );
void waddch( const window &win, chtype ch );
int getmaxy( const window &win );
int getmaxx( const window &win );
int getbegx( const window &win );
int getbegy( const window &win );
int getcurx( const window &win );
int getcury( const window &win );
bool supports_256_colors();
}

#endif
