#pragma once
#ifndef CATA_SRC_OUTPUT_H
#define CATA_SRC_OUTPUT_H

#include <algorithm>
#include <clocale>
#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <map>
#include <optional>
#include <stack>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "cata_assert.h"
#include "catacharset.h"
#include "color.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "enums.h"
#include "item.h"
#include "line.h"
#include "point.h"
#include "string_formatter.h"
#include "translations.h"
#include "units_fwd.h"

class input_context;

struct input_event;

namespace catacurses
{
class window;

using chtype = int;
}


#define LINE_XOXO 4194424
#define LINE_OXOX 4194417
#define LINE_XXOO 4194413
#define LINE_OXXO 4194412
#define LINE_OOXX 4194411
#define LINE_XOOX 4194410
#define LINE_XXXO 4194420
#define LINE_XXOX 4194422
#define LINE_XOXX 4194421
#define LINE_OXXX 4194423
#define LINE_XXXX 4194414

#define LINE_XOXO_C 0xa0
#define LINE_OXOX_C 0xa1
#define LINE_XXOO_C 0xa2
#define LINE_OXXO_C 0xa3
#define LINE_OOXX_C 0xa4
#define LINE_XOOX_C 0xa5
#define LINE_XXXO_C 0xa6
#define LINE_XXOX_C 0xa7
#define LINE_XOXX_C 0xa8
#define LINE_OXXX_C 0xa9
#define LINE_XXXX_C 0xaa

#define LINE_XOXO_S "│"
#define LINE_OXOX_S "─"
#define LINE_XXOO_S "└"
#define LINE_OXXO_S "┌"
#define LINE_OOXX_S "┐"
#define LINE_XOOX_S "┘"
#define LINE_XXXO_S "├"
#define LINE_XXOX_S "┴"
#define LINE_XOXX_S "┤"
#define LINE_OXXX_S "┬"
#define LINE_XXXX_S "┼"

#define LINE_XOXO_UNICODE 0x2502
#define LINE_OXOX_UNICODE 0x2500
#define LINE_XXOO_UNICODE 0x2514
#define LINE_OXXO_UNICODE 0x250C
#define LINE_OOXX_UNICODE 0x2510
#define LINE_XOOX_UNICODE 0x2518
#define LINE_XXXO_UNICODE 0x251C
#define LINE_XXOX_UNICODE 0x2534
#define LINE_XOXX_UNICODE 0x2524
#define LINE_OXXX_UNICODE 0x252C
#define LINE_XXXX_UNICODE 0x253C

#if defined(USE_PDCURSES)
#undef LINE_XOXO
#undef LINE_OXOX
#undef LINE_XXOO
#undef LINE_OXXO
#undef LINE_OOXX
#undef LINE_XOOX
#undef LINE_XXXO
#undef LINE_XXOX
#undef LINE_XOXX
#undef LINE_OXXX
#undef LINE_XXXX

#define LINE_XOXO LINE_XOXO_UNICODE
#define LINE_OXOX LINE_OXOX_UNICODE
#define LINE_XXOO LINE_XXOO_UNICODE
#define LINE_OXXO LINE_OXXO_UNICODE
#define LINE_OOXX LINE_OOXX_UNICODE
#define LINE_XOOX LINE_XOOX_UNICODE
#define LINE_XXXO LINE_XXXO_UNICODE
#define LINE_XXOX LINE_XXOX_UNICODE
#define LINE_XOXX LINE_XOXX_UNICODE
#define LINE_OXXX LINE_OXXX_UNICODE
#define LINE_XXXX LINE_XXXX_UNICODE
#endif

std::string string_from_int( catacurses::chtype ch );


#define BORDER_COLOR c_light_gray
#define ACTIVE_HOTKEY_COLOR c_yellow


extern int TERMX;
extern int TERMY;
extern int POSX;
extern int POSY;
extern int TERRAIN_WINDOW_WIDTH;
extern int TERRAIN_WINDOW_HEIGHT;
extern int TERRAIN_WINDOW_TERM_WIDTH;
extern int TERRAIN_WINDOW_TERM_HEIGHT;

extern int FULL_SCREEN_WIDTH;

extern int FULL_SCREEN_HEIGHT;
extern int OVERMAP_WINDOW_WIDTH;
extern int OVERMAP_WINDOW_HEIGHT;
extern int OVERMAP_WINDOW_TERM_WIDTH;
extern int OVERMAP_WINDOW_TERM_HEIGHT;
extern int OVERMAP_LEGEND_WIDTH;

nc_color msgtype_to_color( game_message_type type, bool bOldMsg = false );

















































std::string rm_prefix( std::string str, char c1 = '<', char c2 = '>' );





color_tag_parse_result::tag_type update_color_stack(
    std::stack<nc_color> &color_stack, std::string_view seg,
    report_color_error color_error = report_color_error::yes );





std::string remove_color_tags( std::string_view s );










std::vector<std::string> foldstring( const std::string &str, int width, char split = ' ' );














void print_colored_text( const catacurses::window &w, const point &p, nc_color &color,
                         const nc_color &base_color, std::string_view text,
                         report_color_error color_error = report_color_error::yes );












int print_scrollable( const catacurses::window &w, int begin_line, const std::string &text,
                      const nc_color &base_color, const std::string &scroll_msg );















int fold_and_print( const catacurses::window &w, const point &begin, int width,
                    const nc_color &base_color, const std::string &text, char split = ' ' );



template<typename ...Args>
inline int fold_and_print( const catacurses::window &w, const point &begin,
                           const int width, const nc_color &base_color,
                           const char *const mes, Args &&... args )
{
    return fold_and_print( w, begin, width, base_color, string_format( mes,
                           std::forward<Args>( args )... ) );
}

















int fold_and_print_from( const catacurses::window &w, const point &begin, int width,
                         int begin_line, const nc_color &base_color, const std::string &text );



template<typename ...Args>
inline int fold_and_print_from( const catacurses::window &w, const point &begin,
                                const int width, const int begin_line, const nc_color &base_color,
                                const char *const mes, Args &&... args )
{
    return fold_and_print_from( w, begin, width, begin_line, base_color,
                                string_format( mes, std::forward<Args>( args )... ) );
}











void trim_and_print( const catacurses::window &w, const point &begin, int width,
                     const nc_color &base_color, const std::string &text,
                     report_color_error color_error = report_color_error::yes );
std::string trim_by_length( const std::string &text, int width );
template<typename ...Args>
inline void trim_and_print( const catacurses::window &w, const point &begin,
                            const int width, const nc_color &base_color,
                            const char *const mes, Args &&... args )
{
    return trim_and_print( w, begin, width, base_color,
                           string_format( mes, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void trim_and_print( const catacurses::window &w, const point &begin,
                            const int width, const nc_color &base_color,
                            const report_color_error color_error,
                            const char *const mes, Args &&... args )
{
    return trim_and_print( w, begin, width, base_color,
                           string_format( mes, std::forward<Args>( args )... ),
                           color_error );
}
void center_print( const catacurses::window &w, int y, const nc_color &FG,
                   const std::string &text );
int right_print( const catacurses::window &w, int line, int right_indent,
                 const nc_color &FG, const std::string &text );
void insert_table( const catacurses::window &w, int pad, int line, int columns,
                   const nc_color &FG, const std::string &divider, bool r_align,
                   const std::vector<std::string> &data );
std::string satiety_bar( int calpereffv );
std::string healthy_bar( int healthy );
void scrollable_text( const std::function<catacurses::window()> &init_window,
                      const std::string &title, const std::string &text );
void add_folded_name_and_value( std::vector<std::string> &current_vector, const std::string &name,
                                int value, int field_width );
void add_folded_name_and_value( std::vector<std::string> &current_vector, const std::string &name,
                                const std::string &value, int field_width );
std::string trimmed_name_and_value( const std::string &name, int value,
                                    int field_width );
std::string trimmed_name_and_value( const std::string &name, const std::string &value,
                                    int field_width );

void wputch( const catacurses::window &w, const nc_color &FG, int ch );

void mvwputch( const catacurses::window &w, const point &p, const nc_color &FG, int ch );
void mvwputch( const catacurses::window &w, const point &p, const nc_color &FG,
               const std::string &ch );

void mvwputch_inv( const catacurses::window &w, const point &p, const nc_color &FG, int ch );
void mvwputch_inv( const catacurses::window &w, const point &p, const nc_color &FG,
                   const std::string &ch );

void mvwputch_hi( const catacurses::window &w, const point &p, const nc_color &FG, int ch );
void mvwputch_hi( const catacurses::window &w, const point &p, const nc_color &FG,
                  const std::string &ch );

void mvwhline( const catacurses::window &win, const point &p, const nc_color &color, int ch,
               int n );
void mvwvline( const catacurses::window &win, const point &p, const nc_color &color, int ch,
               int n );

void mvwrectf( const catacurses::window &win, const point &p, int ch, int w, int h );
void mvwrectf( const catacurses::window &win, const point &p, const nc_color &color, int ch, int w,
               int h );

void mvwprintz( const catacurses::window &w, const point &p, const nc_color &FG,
                const std::string &text );
template<typename ...Args>
inline void mvwprintz( const catacurses::window &w, const point &p, const nc_color &FG,
                       const char *const mes, Args &&... args )
{
    mvwprintz( w, p, FG, string_format( mes, std::forward<Args>( args )... ) );
}

void wprintz( const catacurses::window &w, const nc_color &FG, const std::string &text );
template<typename ...Args>
inline void wprintz( const catacurses::window &w, const nc_color &FG, std::string_view mes,
                     Args &&... args )
{
    wprintz( w, FG, string_format( mes, std::forward<Args>( args )... ) );
}

std::string wrap60( const std::string &text );

void draw_custom_border(
    const catacurses::window &w, catacurses::chtype ls = 1, catacurses::chtype rs = 1,
    catacurses::chtype ts = 1, catacurses::chtype bs = 1, catacurses::chtype tl = 1,
    catacurses::chtype tr = 1, catacurses::chtype bl = 1, catacurses::chtype br = 1,
    const nc_color &FG = BORDER_COLOR, const point &pos = point::zero, int height = 0, int width = 0 );
void draw_border( const catacurses::window &w, nc_color border_color = BORDER_COLOR,
                  const std::string &title = "", nc_color title_color = c_light_red );
void draw_border_below_tabs( const catacurses::window &w, nc_color border_color = BORDER_COLOR );

class border_helper
{
    public:
        class border_info
        {
            public:
                void set( const point &pos, const point &size );


                border_info( const border_info & ) = delete;
                border_info( border_info && ) = default;
                border_info &operator=( const border_info & ) = delete;
                border_info &operator=( border_info && ) = default;

                friend class border_helper;
            private:
                explicit border_info( border_helper &helper );

                point pos;
                point size;
                std::reference_wrapper<border_helper> helper;
        };

        border_info &add_border();
        void draw_border( const catacurses::window &win, nc_color border_color = BORDER_COLOR );

        friend class border_info;
    private:
        struct border_connection {
            bool top = false;
            bool right = false;
            bool bottom = false;
            bool left = false;

            int as_curses_line() const;
        };
        std::optional<std::map<point, border_connection>> border_connection_map;

        std::forward_list<border_info> border_info_list;
};

std::string word_rewrap( const std::string &ins, int width, uint32_t split = ' ' );
std::vector<size_t> get_tag_positions( std::string_view s );
std::vector<std::string> split_by_color( std::string_view s );

bool query_yn( const std::string &text );
template<typename ...Args>
inline bool query_yn( const char *const msg, Args &&... args )
{
    return query_yn( string_format( msg, std::forward<Args>( args )... ) );
}

enum class query_ynq_result {
    quit, no, yes
};
query_ynq_result query_ynq( const std::string &text );
template<typename ...Args>
inline query_ynq_result query_ynq( const char *const msg, Args &&... args )
{
    return query_ynq( string_format( msg, std::forward<Args>( args )... ) );
}


bool query_int( int &result, bool show_default, const std::string &text );
template<typename ...Args>
inline bool query_int( int &result, bool show_default, const char *const msg, Args &&... args )
{
    return query_int( result, show_default, string_format( msg, std::forward<Args>( args )... ) );
}

std::vector<std::string> get_hotkeys( std::string_view s );

























enum PopupFlags {
    PF_NONE        = 0,
    PF_GET_KEY     = 1 << 0,
    PF_ON_TOP      = 1 << 2,
    PF_FULLSCREEN  = 1 << 3,
};

inline auto  format_as( PopupFlags pf )
{
    return static_cast<std::underlying_type_t<PopupFlags>>( pf );
}

PopupFlags popup_flag_from_string( const std::string &str );

template<typename ...Args>
inline int popup_getkey( const char *const mes, Args &&... args )
{
    return popup( string_format( mes, std::forward<Args>( args )... ), PF_GET_KEY );
}
template<typename ...Args>
inline void popup_top( const char *const mes, Args &&... args )
{
    popup( string_format( mes, std::forward<Args>( args )... ), PF_ON_TOP );
}
template<typename ...Args>
inline void popup( const char *mes, Args &&... args )
{
    popup( string_format( mes, std::forward<Args>( args )... ), PF_NONE );
}
int popup( const std::string &text, PopupFlags flags = PF_NONE );


int choose_key_from_user( const std::string &message, const std::string &allowed_keys,
                          int clear_key = -1 );
template<typename ...Args>
inline void full_screen_popup( const char *mes, Args &&... args )
{
    popup( string_format( mes, std::forward<Args>( args )... ), PF_FULLSCREEN );
}


std::string format_item_info( const std::vector<iteminfo> &vItemDisplay,
                              const std::vector<iteminfo> &vItemCompare );
void display_item_info( const std::vector<iteminfo> &vItemDisplay,
                        const std::vector<iteminfo> &vItemCompare );


struct item_info_data {
    private:
        std::string sItemName;
        std::string sTypeName;
        std::vector<iteminfo> vItemDisplay;
        std::vector<iteminfo> vItemCompare;
        int selected = 0;

    public:

        item_info_data() = default;

        item_info_data( const std::string &sItemName, const std::string &sTypeName,
                        const std::vector<iteminfo> &vItemDisplay, const std::vector<iteminfo> &vItemCompare )
            : sItemName( sItemName ), sTypeName( sTypeName ),
              vItemDisplay( vItemDisplay ), vItemCompare( vItemCompare ) {}

        item_info_data( const std::string &sItemName, const std::string &sTypeName,
                        const std::vector<iteminfo> &vItemDisplay, const std::vector<iteminfo> &vItemCompare,
                        int &ptr_selected )
            : sItemName( sItemName ), sTypeName( sTypeName ),
              vItemDisplay( vItemDisplay ), vItemCompare( vItemCompare ),
              ptr_selected( &ptr_selected ) {}

        const std::string &get_item_name() const {
            return sItemName;
        }
        const std::string &get_type_name() const {
            return sTypeName;
        }
        const std::vector<iteminfo> &get_item_display() const {
            return vItemDisplay;
        }
        const std::vector<iteminfo> &get_item_compare() const {
            return vItemCompare;
        }

        int *ptr_selected = &selected;
        bool without_getch = false;
        bool without_border = false;
        bool handle_scrolling = false;
        bool arrow_scrolling = false;
        bool any_input = true;
        bool scrollbar_left = true;
        bool use_full_win = false;
        unsigned int padding = 1;
};

input_event draw_item_info( const catacurses::window &win, item_info_data &data );

input_event draw_item_info( int iLeft, int iWidth, int iTop, int iHeight, item_info_data &data );

input_event draw_item_info( const std::function<catacurses::window()> &init_window,
                            item_info_data &data );

enum class item_filter_type : int {
    FIRST = 1,
    FILTER = 1,
    LOW_PRIORITY = 2,
    HIGH_PRIORITY = 3
};








void draw_item_filter_rules( const catacurses::window &win, int starty, int height,
                             item_filter_type type );




std::string item_filter_rule_string( item_filter_type type );

char rand_char();


std::string trim( std::string_view s );

std::string rtrim( std::string_view s );

std::string trim_trailing_punctuations( std::string_view s );

std::string remove_punctuations( std::string_view s );

std::string to_upper_case( std::string_view s );

std::string to_lower_case( std::string_view s );



void replace_city_tag( std::string &input, const std::string &name );

void replace_substring( std::string &input, const std::string &substring,
                        const std::string &replacement, bool all );

std::string string_replace( std::string text, const std::string &before, const std::string &after );
std::string replace_colors( std::string text );
std::string uppercase_first_letter( std::string_view str );
std::string lowercase_first_letter( std::string_view str );
size_t shortcut_print( const catacurses::window &w, const point &p, nc_color text_color,
                       nc_color shortcut_color, const std::string &fmt );
size_t shortcut_print( const catacurses::window &w, nc_color text_color, nc_color shortcut_color,
                       const std::string &fmt );
std::string shortcut_text( nc_color shortcut_color, const std::string &fmt );








std::pair<std::string, nc_color> get_bar( float cur, float max, int width = 5,
        bool extra_resolution = true,
        const std::vector<nc_color> &colors = { c_green, c_light_green, c_yellow, c_light_red, c_red } );

std::pair<std::string, nc_color> get_hp_bar( int cur_hp, int max_hp, bool is_mon = false );

std::pair<std::string, nc_color> get_stamina_bar( int cur_stam, int max_stam );

std::pair<std::string, nc_color> get_light_level( float light );

std::pair<std::string, nc_color> rad_badge_color( int rad );

enum class enumeration_conjunction : int {
    none,
    and_,
    or_,
    arrow
};






template<typename Container>
std::string enumerate_as_string( const Container &values,
                                 enumeration_conjunction conj = enumeration_conjunction::and_ )
{
    const std::string final_separator = [&]() {
        switch( conj ) {
            case enumeration_conjunction::none:
                return _( ", " );
            case enumeration_conjunction::and_:
                return values.size() > 2 ? _( ", and " ) : _( " and " );
            case enumeration_conjunction::or_:
                return values.size() > 2 ? _( ", or " ) : _( " or " );
            case enumeration_conjunction::arrow:
                return _( " > " );
        }
        debugmsg( "Unexpected conjunction" );
        return _( ", " );
    }
    ();
    const std::string separator = [&conj]() {
        switch( conj ) {
            case enumeration_conjunction::arrow:
                return _( " > " );
            default:
                return _( ", " );
        }
    }
    ();
    std::string res;
    for( auto iter = values.begin(); iter != values.end(); ++iter ) {
        if( iter != values.begin() ) {
            if( std::next( iter ) == values.end() ) {
                res += final_separator;
            } else {
                res += separator;
            }
        }
        res += *iter;
    }
    return res;
}









template<typename FIter, typename F>

// NOLINTNEXTLINE(performance-unnecessary-value-param)
std::string enumerate_as_string( FIter first, FIter last, F &&string_for,
                                 enumeration_conjunction conj = enumeration_conjunction::and_ )
{
    std::vector<std::string> values;
    values.reserve( static_cast<size_t>( std::distance( first, last ) ) );
    for( FIter iter = first; iter != last; ++iter ) {
        const std::string str( string_for( *iter ) );
        if( !str.empty() ) {
            values.push_back( str );
        }
    }
    return enumerate_as_string( values, conj );
}

template<typename Container, typename F>
std::string enumerate_as_string( const Container &cont, F &&string_for,
                                 enumeration_conjunction conj = enumeration_conjunction::and_ )
{
    return enumerate_as_string( cont.begin(), cont.end(), std::forward<F>( string_for ), conj );
}







std::string formatted_hotkey( const std::string &hotkey, nc_color text_color );












template<typename BarIterator>
std::string get_labeled_bar( double val, int width, const std::string &label,
                             BarIterator begin, BarIterator end );














template<typename BarIterator>
std::string get_labeled_bar( double val, int width, const std::string &label,
                             BarIterator begin, BarIterator end,
                             const std::function<std::string( BarIterator, int )> &printer );








std::string get_labeled_bar( double val, int width, const std::string &label, char c );

void draw_tab( const catacurses::window &w, int iOffsetX, std::string_view sText,
               bool bSelected );
inclusive_rectangle<point> draw_subtab( const catacurses::window &w, int iOffsetX,
                                        const std::string &sText, bool bSelected,
                                        bool bDecorate = true, bool bDisabled = false );








std::map<size_t, inclusive_rectangle<point>> draw_tabs( const catacurses::window &,
        const std::vector<std::string> &tab_texts, size_t current_tab, size_t offset = 0 );

std::map<std::string, inclusive_rectangle<point>> draw_tabs( const catacurses::window &,
        const std::vector<std::string> &tab_texts, const std::string &current_tab );











template<typename TabList, typename CurrentTab, typename = std::enable_if_t<
             std::is_same_v<CurrentTab,
                            std::remove_const_t<typename TabList::value_type::first_type>>>>
             std::map<CurrentTab, inclusive_rectangle<point>> draw_tabs( const catacurses::window &w,
                     const TabList &tab_list, const CurrentTab &current_tab )
{
    std::vector<std::string> tab_text;
    std::transform( tab_list.begin(), tab_list.end(), std::back_inserter( tab_text ),
    []( const typename TabList::value_type & pair ) {
        return pair.second;
    } );
    auto current_tab_it = std::find_if( tab_list.begin(), tab_list.end(),
    [&current_tab]( const typename TabList::value_type & pair ) {
        return pair.first == current_tab;
    } );
    cata_assert( current_tab_it != tab_list.end() );
    std::map<size_t, inclusive_rectangle<point>> tab_map =
                draw_tabs( w, tab_text, std::distance( tab_list.begin(), current_tab_it ) );

    std::map<CurrentTab, inclusive_rectangle<point>> ret_map;
    size_t i = 0;
    for( const typename TabList::value_type &pair : tab_list ) {
        if( tab_map.count( i ) > 0 ) {
            ret_map.emplace( pair.first, tab_map.at( i ) );
        }
        i++;
    }
    return ret_map;
}



template<typename TabList, typename TabKeys, typename CurrentTab, typename = std::enable_if_t<
             std::is_same_v<CurrentTab,
                            std::remove_const_t<typename TabList::value_type::first_type>>>>
             std::map<CurrentTab, inclusive_rectangle<point>> draw_tabs( const catacurses::window &w,
                     const TabList &tab_list, const TabKeys &keys, const CurrentTab &current_tab )
{
    std::vector<typename TabList::value_type> ordered_tab_list;
    for( const auto &key : keys ) {
        auto it = tab_list.find( key );
        cata_assert( it != tab_list.end() );
        ordered_tab_list.push_back( *it );
    }
    return draw_tabs( w, ordered_tab_list, current_tab );
}

















std::pair<std::vector<std::string>, size_t> fit_tabs_to_width( size_t max_width,
        int current_tab,
        const std::vector<std::string> &original_tab_list );

struct best_fit {
    int start;
    int length;
};












best_fit find_best_fit_in_size( const std::vector<int> &size_of_items_to_fit, const int &selected,
                                const int &allowed_size, const int &spacer_size = 0, const int &continuation_marker_size = 0 );


void draw_scrollbar( const catacurses::window &window, int iCurrentLine,
                     int iContentHeight, int iNumLines, const point &offset = point::zero,
                     nc_color bar_color = c_white, bool bDoNotScrollToEnd = false );
void calcStartPos( int &iStartPos, int iCurrentLine, int iContentHeight,
                   int iNumEntries );

class scrollbar
{
    public:
        scrollbar();

        scrollbar &offset_x( int offx );
        scrollbar &offset_y( int offy );

        scrollbar &content_size( int csize );

        scrollbar &viewport_pos( int vpos );

        scrollbar &viewport_size( int vsize );

        scrollbar &border_color( nc_color border_c );

        scrollbar &arrow_color( nc_color arrow_c );

        scrollbar &slot_color( nc_color slot_c );

        scrollbar &bar_color( nc_color bar_c );

        scrollbar &scroll_to_last( bool scr2last );

        scrollbar &set_draggable( input_context &ctxt );

        void apply( const catacurses::window &window, bool draw_unneeded = false );

        bool handle_dragging( const std::string &action, const std::optional<point> &coord,
                              int &position );
    private:
        int offset_x_v, offset_y_v;
        int content_size_v, viewport_pos_v, viewport_size_v;
        nc_color border_color_v, arrow_color_v, slot_color_v, bar_color_v;
        bool scroll_to_last_v;
        bool dragging = false;
        inclusive_rectangle<point> scrollbar_area;
};
















enum scrolling_key_scheme : int {
    no_scheme,
    angle_bracket_scroll,
    arrow_scroll,
    page_page
};
class scrolling_text_view
{
    public:
        explicit scrolling_text_view( catacurses::window &w ) : w_( w ) {}

        bool handle_navigation( const std::string &action, input_context &ctxt );
        void set_up_navigation( input_context &ctxt,
                                scrolling_key_scheme scheme = scrolling_key_scheme::no_scheme, bool enable_paging = false );
        void set_text( const std::string &text, bool scroll_to_top = true );
        void scroll_up();
        void scroll_down();
        void page_up();
        void page_down();
        void draw( const nc_color &base_color );
    private:
        int text_width();
        int num_lines();
        int max_offset();

        catacurses::window &w_;
        std::vector<std::string> text_;
        int offset_ = 0;
        std::string scroll_up_action;
        std::string scroll_down_action;
        bool paging_enabled = false;
        scrollbar text_view_scrollbar;
};

class scrollingcombattext
{
    public:
        static constexpr int iMaxSteps = 8;

        scrollingcombattext() = default;

        class cSCT
        {
            private:
                point pos;
                direction oDir;
                direction oUp;
                direction oUpRight;
                direction oRight;
                direction oDownRight;
                direction oDown;
                direction oDownLeft;
                direction oLeft;
                direction oUpLeft;
                point dir;
                int iStep;
                int iStepOffset;
                std::string sText;
                game_message_type gmt;
                std::string sText2;
                game_message_type gmt2;
                std::string sType;
                bool iso_mode;

            public:
                cSCT( const point &pos, direction p_oDir,
                      const std::string &p_sText, game_message_type p_gmt,
                      const std::string &p_sText2 = "", game_message_type p_gmt2 = m_neutral,
                      const std::string &p_sType = "" );

                int getStep() const {
                    return iStep;
                }
                int getStepOffset() const {
                    return iStepOffset;
                }
                int advanceStep() {
                    return ++iStep;
                }
                int advanceStepOffset() {
                    return ++iStepOffset;
                }
                int getPosX() const;
                int getPosY() const;
                direction getDirection() const {
                    return oDir;
                }
                int getInitPosX() const {
                    return pos.x;
                }
                int getInitPosY() const {
                    return pos.y;
                }
                std::string getType() const {
                    return sType;
                }
                std::string getText( const std::string &type = "full" ) const;
                game_message_type getMsgType( const std::string &type = "first" ) const;
        };

        std::vector<cSCT> vSCT;

        void add( const point &pos, direction p_oDir,
                  const std::string &p_sText, game_message_type p_gmt,
                  const std::string &p_sText2 = "", game_message_type p_gmt2 = m_neutral,
                  const std::string &p_sType = "" );
        void advanceAllSteps();
        void removeCreatureHP();
};

extern scrollingcombattext SCT;

std::string wildcard_trim_rule( const std::string &pattern_in );
bool wildcard_match( const std::string &text_in, const std::string &pattern_in );
int ci_find_substr( std::string_view str1, std::string_view str2,
                    const std::locale &loc = std::locale() );

std::string format_volume( const units::volume &volume );
std::string format_volume( const units::volume &volume, int width, bool *out_truncated,
                           double *out_value );

inline std::string format_money( int cents )
{
    return string_format( _( "$%.2f" ), cents / 100.0 );
}






int get_terminal_width();






int get_terminal_height();










bool is_draw_tiles_mode();

int get_window_width();
int get_window_height();








void refresh_display();






void drain_renderer_recovery();







void pump_until_renderer_foreground();

enum class atlas_upload_interrupt;
class atlas_replay_quarantine;








bool service_mode2_upload_interrupt( atlas_upload_interrupt interrupt,
                                     atlas_replay_quarantine &quarantine,
                                     uint64_t instance_before );





void refresh_mouse_config();








template<typename F>
std::string colorize_symbols( const std::string_view str, F color_of )
{
    std::string res;
    nc_color prev_color = c_unset;

    const auto closing_tag = [ &res, prev_color ]() {
        if( prev_color != c_unset ) {
            res += "</color>";
        }
    };

    for( const char &elem : str ) {
        const nc_color new_color = color_of( elem );

        if( prev_color != new_color ) {
            closing_tag();
            res += "<color_" + get_all_colors().get_name( new_color ) + ">";
            prev_color = new_color;
        }

        res += elem;
    }

    closing_tag();

    return res;
}


class tab_list
{
    private:
        size_t _index = 0;
        const std::vector<std::string> *_list;
    public:
        explicit tab_list( const std::vector<std::string> &_list ) : _list( &_list ) {
        }

        void last() {
            if( !_list->empty() ) {
                _index = _list->size() - 1;
            }
        }

        void next() {
            _index = ( _index == _list->size() - 1 ? 0 : _index + 1 );
        }

        void prev() {
            _index = ( _index == 0 ? _list->size() - 1 : _index - 1 );
        }

        int cur_index() const {
            return static_cast<int>( _index );
        }

        std::string cur() const {
            if( _list->empty() ) {
                return std::string();
            }
            return ( *_list )[_index];
        }

        void set_index( const size_t new_index ) {
            if( new_index < _list->size() ) {
                _index = new_index;
            }
        }
};

#endif
