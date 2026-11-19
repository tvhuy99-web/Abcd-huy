#include "cursesport.h"
#ifndef TUI

#include <cstdint>
#include <memory>

#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "game_ui.h"
#include "output.h"
#include "wcwidth.h"























catacurses::window catacurses::stdscr;
std::array<cata_cursesport::pairs, 100> cata_cursesport::colorpairs;
unsigned cata_cursesport::curses_render_epoch = 0;

void cata_cursesport::bump_curses_render_epoch()
{
    ++curses_render_epoch;
}

static bool wmove_internal( const catacurses::window &win_, const point &p )
{
    if( !win_ ) {
        return false;
    }
    cata_cursesport::WINDOW &win = *win_.get<cata_cursesport::WINDOW>();
    if( p.x >= win.width ) {
        return false;
    }
    if( p.y >= win.height ) {
        return false;
    }
    if( p.y < 0 ) {
        return false;
    }
    if( p.x < 0 ) {
        return false;
    }
    win.cursor = p;
    return true;
}





catacurses::window catacurses::newwin( int nlines, int ncols, const point &begin )
{
    if( begin.y < 0 || begin.x < 0 ) {
        return window();
    }


    if( ncols == 0 ) {
        ncols = TERMX - begin.x;
    }
    if( nlines == 0 ) {
        nlines = TERMY - begin.y;
    }

    cata_cursesport::WINDOW *newwindow = new cata_cursesport::WINDOW();
    newwindow->pos = begin;
    newwindow->width = ncols;
    newwindow->height = nlines;
    newwindow->inuse = true;
    newwindow->draw = false;
    newwindow->BG = black;
    newwindow->FG = static_cast<base_color>( 8 );
    newwindow->cursor = point::zero;
    newwindow->line.resize( nlines );

    for( int j = 0; j < nlines; j++ ) {
        newwindow->line[j].chars.resize( ncols );
        newwindow->line[j].touched = true;
    }
    return catacurses::window( std::shared_ptr<void>( newwindow, []( void *const w ) {
        delete static_cast<cata_cursesport::WINDOW *>( w );
    } ) );
}

static int newline( cata_cursesport::WINDOW *win )
{
    if( win->cursor.y < win->height - 1 ) {
        win->cursor.y++;
        win->cursor.x = 0;
        return 1;
    }
    return 0;
}



static void addedchar( cata_cursesport::WINDOW *win )
{
    win->cursor.x++;
    win->line[win->cursor.y].touched = true;
    if( win->cursor.x >= win->width ) {
        newline( win );
    }
}


void catacurses::wborder( const window &win_, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl,
                          chtype tr, chtype bl, chtype br )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    if( win == nullptr ) {

        return;
    }
    point old = win->cursor;

    const chtype border_ls = ls ? ls : LINE_XOXO;
    const chtype border_rs = rs ? rs : LINE_XOXO;
    const chtype border_ts = ts ? ts : LINE_OXOX;
    const chtype border_bs = bs ? bs : LINE_OXOX;
    const chtype border_tl = tl ? tl : LINE_OXXO;
    const chtype border_tr = tr ? tr : LINE_OOXX;
    const chtype border_bl = bl ? bl : LINE_XXOO;
    const chtype border_br = br ? br : LINE_XOOX;

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwvline( win_, point( 0,               1 ), border_ls, win->height - 2 );
    mvwvline( win_, point( win->width - 1,  1 ), border_rs, win->height - 2 );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwhline( win_, point( 1,               0 ), border_ts, win->width - 2 );
    mvwhline( win_, point( 1, win->height - 1 ), border_bs, win->width - 2 );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwaddch( win_, point( 0,                            0 ), border_tl );
    mvwaddch( win_, point( win->width - 1,               0 ), border_tr );
    mvwaddch( win_, point( 0,              win->height - 1 ), border_bl );
    mvwaddch( win_, point( win->width - 1, win->height - 1 ), border_br );


    wmove( win_, old );
    wattroff( win_, c_white );
}

void catacurses::mvwhline( const window &win, const point &p, chtype ch, int n )
{
    const chtype hline_char = ch ? ch : LINE_OXOX;
    for( int i = 0; i < n; i++ ) {
        mvwaddch( win, p + point( i, 0 ), hline_char );
    }
}

void catacurses::mvwvline( const window &win, const point &p, chtype ch, int n )
{
    const chtype vline_char = ch ? ch : LINE_XOXO;
    for( int j = 0; j < n; j++ ) {
        mvwaddch( win, p + point( 0, j ), vline_char );
    }
}

void catacurses::wnoutrefresh( const window &win_ )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();



    if( win != nullptr && ( win->draw
                            || win->last_render_epoch != cata_cursesport::curses_render_epoch ) ) {
        cata_cursesport::curses_drawwindow( win_ );
    }
}


void catacurses::wrefresh( const window &win )
{
    wnoutrefresh( win );
    doupdate();
}


void catacurses::refresh()
{
    wrefresh( stdscr );
}

void catacurses::doupdate()
{
    refresh_display();
}

void catacurses::wredrawln( const window &, int, int )
{






}



static int fill( const char *&fmt, int &len, std::string &target )
{
    const char *const start = fmt;
    int dlen = 0;
    const char *tmpptr = fmt;
    int tmplen = len;
    while( tmplen > 0 ) {
        const uint32_t ch = UTF8_getch( &tmpptr, &tmplen );

        const int cw = ch == UNKNOWN_UNICODE ? 1 : mk_wcwidth( ch );
        if( cw > 0 && dlen > 0 ) {

            break;
        }
        if( cw == -1 && start == fmt ) {



            target.assign( " ", 1 );
            len = tmplen;
            fmt = tmpptr;
            return 1;
        } else if( cw == -1 ) {



            break;
        }
        fmt = tmpptr;
        dlen += cw;
    }
    target.assign( start, fmt - start );
    len -= target.length();
    return dlen;
}




static cata_cursesport::cursecell *cur_cell( cata_cursesport::WINDOW *win )
{
    if( win->cursor.y >= win->height || win->cursor.x >= win->width ) {
        return nullptr;
    }
    return &win->line[win->cursor.y].chars[win->cursor.x];
}


static void printstring( cata_cursesport::WINDOW *win, const std::string &text )
{
    using cata_cursesport::cursecell;
    win->draw = true;
    int len = text.length();
    if( len == 0 ) {
        return;
    }
    const char *fmt = text.c_str();


    if( win->cursor.x >= win->width ) {
        if( newline( win ) == 0 ) {
            return;
        }
    }
    if( win->cursor.y >= win->height || win->cursor.x >= win->width ) {
        return;
    }
    if( win->cursor.x > 0 && win->line[win->cursor.y].chars[win->cursor.x].ch.empty() ) {

        win->line[win->cursor.y].chars[win->cursor.x - 1].ch.assign( " " );
    }
    while( len > 0 ) {
        if( *fmt == '\n' ) {
            if( newline( win ) == 0 ) {
                return;
            }
            fmt++;
            len--;
            continue;
        }
        cursecell *curcell = cur_cell( win );
        if( curcell == nullptr ) {
            return;
        }
        const int dlen = fill( fmt, len, curcell->ch );
        if( dlen >= 1 ) {
            curcell->FG = win->FG;
            curcell->BG = win->BG;
            addedchar( win );
        }
        if( dlen == 1 ) {


            cursecell *seccell = cur_cell( win );
            if( seccell && seccell->ch.empty() ) {
                seccell->ch.assign( ' ', 1 );
            }
        } else if( dlen == 2 ) {

            cursecell *seccell = cur_cell( win );
            if( seccell == nullptr ) {



                curcell->ch.assign( ' ', 1 );
                return;
            }
            seccell->FG = win->FG;
            seccell->BG = win->BG;
            seccell->ch.erase();
            addedchar( win );


            if( win->cursor.x == 1 ) {


                seccell->ch = curcell->ch;
                curcell->ch.assign( 1, ' ' );

                addedchar( win );
                cursecell *thicell = cur_cell( win );
                if( thicell != nullptr ) {
                    thicell->ch.erase();
                }
            }
        }
        if( win->cursor.y >= win->height ) {
            return;
        }
    }
}


void catacurses::wprintw( const window &win, const std::string &text )
{
    if( !win ) {

        return;
    }

    printstring( win.get<cata_cursesport::WINDOW>(), text );
}


void catacurses::mvwprintw( const window &win, const point &p, const std::string &text )
{
    if( !wmove_internal( win, p ) ) {
        return;
    }
    printstring( win.get<cata_cursesport::WINDOW>(), text );
}


void catacurses::resizeterm()
{
    game_ui::init_ui();
}


void catacurses::werase( const window &win_ )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    if( win == nullptr ) {

        return;
    }

    for( int j = 0; j < win->height; j++ ) {
        win->line[j].chars.assign( win->width, cata_cursesport::cursecell() );
        win->line[j].touched = true;
    }
    win->draw = true;
    wmove( win_, point::zero );
}


void catacurses::erase()
{
    werase( stdscr );
}


void catacurses::init_pair( const short pair, const base_color f, const base_color b )
{
    cata_cursesport::colorpairs[pair].FG = f;
    cata_cursesport::colorpairs[pair].BG = b;
}


void catacurses::wmove( const window &win_, const point &p )
{
    if( !wmove_internal( win_, p ) ) {
        return;
    }
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    win->cursor = p;
}


void catacurses::clear()
{
    wclear( stdscr );
}


void catacurses::mvwaddch( const window &win, const point &p, const chtype ch )
{
    if( !wmove_internal( win, p ) ) {
        return;
    }
    waddch( win, ch );
}


void catacurses::wclear( const window &win_ )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    werase( win_ );
    if( win == nullptr ) {

        return;
    }

    for( int i = 0; i < win->pos.y && i < stdscr.get<cata_cursesport::WINDOW>()->height; i++ ) {
        stdscr.get<cata_cursesport::WINDOW>()->line[i].touched = true;
    }
}


int catacurses::getmaxx( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->width : 0;
}


int catacurses::getmaxy( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->height : 0;
}


int catacurses::getbegx( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->pos.x : 0;
}


int catacurses::getbegy( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->pos.y : 0;
}


int catacurses::getcurx( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->cursor.x : 0;
}


int catacurses::getcury( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->cursor.y : 0;
}

void catacurses::curs_set( int )
{
}

void catacurses::wattron( const window &win_, const nc_color &attrs )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    if( win == nullptr ) {

        return;
    }

    int pairNumber = attrs.to_color_pair_index();
    win->FG = cata_cursesport::colorpairs[pairNumber].FG;
    win->BG = cata_cursesport::colorpairs[pairNumber].BG;
    if( attrs.is_bold() ) {
        win->FG = static_cast<base_color>( win->FG + 8 );
    }
    if( attrs.is_blink() ) {
        win->BG = static_cast<base_color>( win->BG + 8 );
    }
}

void catacurses::wattroff( const window &win_, nc_color )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    if( win == nullptr ) {

        return;
    }

    win->FG = static_cast<base_color>( 8 );
    win->BG = black;
}

void catacurses::waddch( const window &win, const chtype ch )
{
    printstring( win.get<cata_cursesport::WINDOW>(), string_from_int( ch ) );
}

static constexpr int A_BLINK = 0x00000800;
static constexpr int A_BOLD = 0x00002000;
static constexpr int A_COLOR = 0x03fe0000;

nc_color nc_color::from_color_pair_index( const int index )
{
    return nc_color( index << 17 & A_COLOR );
}

int nc_color::to_color_pair_index() const
{
    return ( attribute_value & A_COLOR ) >> 17;
}

nc_color nc_color::bold() const
{
    return nc_color( attribute_value | A_BOLD );
}

bool nc_color::is_bold() const
{
    return attribute_value & A_BOLD;
}

nc_color nc_color::blink() const
{
    return nc_color( attribute_value | A_BLINK );
}

bool nc_color::is_blink() const
{
    return attribute_value & A_BLINK;
}

#endif
