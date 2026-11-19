#pragma once
#ifndef CATA_SRC_UI_H
#define CATA_SRC_UI_H

#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "cata_assert.h"
#include "cata_imgui.h"
#include "color.h"
#include "cuboid_rectangle.h"
#include "cursesdef.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#undef IMGUI_DEFINE_MATH_OPERATORS
#include "input_context.h"
#include "memory_fast.h"
#include "pimpl.h"
#include "point.h"
#include "string_formatter.h"

class scrollbar;
class translation;
namespace a11y
{
class screen_scope;
struct semantic_node;
}





const int UILIST_ERROR = -1024;
const int UILIST_WAIT_INPUT = -1025;
const int UILIST_UNBOUND = -1026;
const int UILIST_CANCEL = -1027;
const int UILIST_TIMEOUT = -1028;
const int UILIST_ADDITIONAL = -1029;
const int MENU_AUTOASSIGN = -1;

class string_input_popup_imgui;
class uilist_impl;

catacurses::window new_centered_win( int nlines, int ncols );





struct mvwzstr {
    int left = 0;
    nc_color color = c_unset;
    std::string txt;
    int sym = 0;
};




struct uilist_entry {
        int retval;
        bool enabled;
        bool force_color = false;


        std::optional<input_event> hotkey;
        std::string txt;
        std::string desc;
        std::string ctxt;
        std::optional<bool> a11y_checked;
        std::string a11y_value;
        std::string a11y_disabled_reason;
        nc_color hotkey_color;
        nc_color text_color;
        mvwzstr extratxt;








        explicit uilist_entry( const std::string &txt );





        uilist_entry( const std::string &txt, const std::string &desc );




        uilist_entry( const std::string &txt, int key );




        uilist_entry( const std::string &txt, const std::optional<input_event> &key );






        uilist_entry( int retval, bool enabled, int key, const std::string &txt );






        uilist_entry( int retval, bool enabled, const std::optional<input_event> &key,
                      const std::string &txt );








        uilist_entry( int retval, bool enabled, int key, const std::string &txt, const std::string &desc );








        uilist_entry( int retval, bool enabled, const std::optional<input_event> &key,
                      const std::string &txt, const std::string &desc );









        uilist_entry( int retval, bool enabled, int key, const std::string &txt, const std::string &desc,
                      const std::string &column );









        uilist_entry( int retval, bool enabled, const std::optional<input_event> &key,
                      const std::string &txt, const std::string &desc,
                      const std::string &column );








        uilist_entry( int retval, bool enabled, int key, const std::string &txt,
                      const nc_color &keycolor, const nc_color &txtcolor );
        template<typename Enum, typename... Args,
                 typename = std::enable_if_t<std::is_enum_v<Enum>>>
                                             explicit uilist_entry( Enum e, Args && ... args ) :
                                                 uilist_entry( static_cast<int>( e ), std::forward<Args>( args )... )
        {}

    public:
        const std::string &_txt_nocolor();
        const std::string &_ctxt_nocolor();
    private:
        std::string _txt_nocolor_cached;
        std::string _ctxt_nocolor_cached;
};




























class uilist;









class uilist_callback
{
    public:




        virtual void select( uilist * ) {}

        virtual void confirm( uilist * ) {}
        virtual bool key( const input_context &, const input_event &, int,
                          uilist * ) {
            return false;
        }


        virtual void describe_accessibility_action( const std::string &, int,
                a11y::semantic_node & ) const {}
        virtual void refresh( uilist * ) {}
        virtual float desired_extra_space_left( ) {
            return 0.0;
        }
        virtual float desired_extra_space_right( ) {
            return 0.0;
        }
        virtual ~uilist_callback() = default;
};





class uilist // NOLINT(cata-xy)
{
        friend class uilist_impl;
    public:
        class size_scalar
        {
            public:
                struct auto_assign {
                };

                size_scalar &operator=( auto_assign );
                size_scalar &operator=( int val );
                size_scalar &operator=( const std::function<int()> &fun );

                friend class uilist;

            private:
                std::function<int()> fun;
        };

        class pos_scalar
        {
            public:
                struct auto_assign {
                };

                pos_scalar &operator=( auto_assign );
                pos_scalar &operator=( int val );


                pos_scalar &operator=( const std::function<int( int )> &fun );

                friend class uilist;

            private:
                std::function<int( int )> fun;
        };

        uilist();

        uilist( const std::string &msg, const std::vector<uilist_entry> &opts );
        uilist( const std::string &msg, const std::vector<std::string> &opts );
        uilist( const std::string &msg, std::initializer_list<const char *const> opts );

        ~uilist();


        void color_error( bool report );

        void init();


        void calc_data();


        void register_uilist_inputs( input_context &ctxt ) const;


        void setup();

        void reposition();
        struct scroll_amount {
            enum { relative_wrapped, relative_clamped, absolute } type;
            int qty;
            static scroll_amount wrapped( int qty ) {
                return {scroll_amount::relative_wrapped, qty};
            };
            static scroll_amount clamped( int qty ) {
                return {scroll_amount::relative_clamped, qty};
            };
            static scroll_amount abs( int idx ) {
                return {scroll_amount::absolute, idx};
            };
        };
        bool scrollby( uilist::scroll_amount scrollby );


        bool query_setup();

        shared_ptr_fast<uilist_impl> query( bool loop = true, int timeout = 50,
                                            bool allow_unfiltered_hotkeys = false );

        void query_once( input_context &ctxt, int timeout = 50, bool allow_unfiltered_hotkeys = false );

        std::unique_ptr<a11y::screen_scope> create_reader_scope( input_context &ctxt );
        void update_reader( a11y::screen_scope &reader, const input_context &ctxt );

        void filterlist();







        void addentry( const std::string &txt );



        void addentry( const uilist_entry &entry );






        void addentry( int retval, bool enabled, int key, const std::string &txt );






        void addentry( int retval, bool enabled, const std::optional<input_event> &key,
                       const std::string &txt );







        template<typename K, typename ...Args>
        void addentry( const int retval, const bool enabled, K &&key, const char *const format,
                       Args &&... args ) {
            return addentry( retval, enabled, std::forward<K>( key ),
                             string_format( format, std::forward<Args>( args )... ) );
        }





        void addentry_desc( const std::string &txt, const std::string &desc );








        void addentry_desc( int retval, bool enabled, int key, const std::string &txt,
                            const std::string &desc );








        void addentry_desc( int retval, bool enabled, const std::optional<input_event> &key,
                            const std::string &txt,
                            const std::string &desc );









        void addentry_col( int retval, bool enabled, int key, const std::string &txt,
                           const std::string &column,
                           const std::string &desc = "" );









        void addentry_col( int retval, bool enabled, const std::optional<input_event> &key,
                           const std::string &txt, const std::string &column,
                           const std::string &desc = std::string() );
        void settext( const std::string &str );

        void add_category( const std::string &key, const std::string &name );
        void set_category( const std::string &key );
        void set_category_filter( const std::function<bool( const uilist_entry &, const std::string & )>
                                  &fun );

        void reset();

#if defined(TILES)
        void set_hide( bool val );





        void hide_if_hidden();
#endif

        shared_ptr_fast<uilist_impl> create_or_get_ui();
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator int() const;

    private:
        scroll_amount scroll_amount_from_action( const std::string &action );


        void inputfilter();

    public:


        std::string title;
        std::string text;

        std::string footer_text;
        std::vector<uilist_entry> entries;

        std::string input_category;
        std::vector<std::pair<std::string, translation>> additional_actions;

        nc_color border_color;
        nc_color text_color;
        nc_color title_color;
        nc_color hilight_color;
        nc_color hotkey_color;
        nc_color disabled_color;

        uilist_callback *callback;

        std::optional<cataimgui::bounds> desired_bounds;
        bool desc_enabled = false;

        std::string filter;
        bool filtering = false;
        bool filtering_nocase = false;


        bool allow_disabled = false;

        bool allow_anykey = false;

        bool allow_cancel = true;

        bool allow_confirm = true;


        bool allow_additional = false;
        bool hilight_disabled = false;

        bool size_to_all_categories = false;

        bool force_desired_bounds = false;

    private:
        report_color_error _color_error = report_color_error::yes;
        input_context create_main_input_context() const;

    public:


        catacurses::window window;

        int vshift = 0;
        int fselected = 0;

    private:
        ImVec2 calculated_menu_size;
        cataimgui::bounds calculated_bounds;
        float calculated_hotkey_width;
        float calculated_label_width;
        float calculated_secondary_width;
        float extra_space_left;
        float extra_space_right;
        std::vector<int> fentries;
        std::map<input_event, int, std::function<bool( const input_event &, const input_event & )>>
        keymap { input_event::compare_type_mod_code };

        weak_ptr_fast<uilist_impl> ui;

        std::unique_ptr<string_input_popup_imgui> filter_popup;

        int max_entry_len = 0;
        int max_column_len = 0;

        int vmax = 0;

        bool started = false;
        bool clicked = false;
        bool need_to_scroll = false;


        std::vector<std::pair<std::string, std::string>> categories;
        std::function<bool( const uilist_entry &, const std::string & )> category_filter;
        size_t current_category = 0;
        size_t switch_to_category = 0;

    public:


        std::string ret_act;
        input_event ret_evt;
        int ret = 0;
        int selected = 0;
        int previewing = 0;

        void set_selected( int index );
};


class uilist_impl : public cataimgui::window
{
#if defined(TILES)
        friend class uilist;
#endif
        uilist &parent;
    public:
        explicit uilist_impl( uilist &parent ) : cataimgui::window( "UILIST",
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoNavInputs ),
            parent( parent ) {
        }

        uilist_impl( uilist &parent, const std::string &title ) : cataimgui::window( title,
                    ImGuiWindowFlags_None | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoNavInputs ),
            parent( parent ) {
        }

        cataimgui::bounds get_bounds() override {
            if( !parent.started ) {
                parent.setup();
            }

            return parent.desired_bounds.value_or( parent.calculated_bounds );
        }
        void draw_controls() override;
};




class uimenu
{
    public:
        explicit uimenu( int columns ) {
            cata_assert( columns >= 1 );
            col_count = columns;
        }





        void addentry( int retval, bool enabled, const std::vector<std::string> &col_content );
        void set_selected( int index );
        void set_title( const std::string &title );
        int query();

    private:



        void finalize_addentries();
        struct col {
            col( int i_retval, bool i_enabled, const std::vector<std::string> &i_col_content ) :
                retval( i_retval ), enabled( i_enabled ), col_content( i_col_content )
            {};
            int retval;
            bool enabled;
            const std::vector<std::string> col_content;
        };
        std::vector<col> cols;
        uilist menu;
        int col_count;

        int suggest_width = 74;
};





class pointmenu_cb : public uilist_callback
{
    private:
        struct impl_t;

        pimpl<impl_t> impl;
    public:
        explicit pointmenu_cb( const std::vector< tripoint_bub_ms > &pts );
        ~pointmenu_cb() override;
        void select( uilist *menu ) override;
};

void temp_hide_advanced_inv();





template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp_wrap( V val, int delta, S size )
{
    if( size == 0 ) {
        return 0;
    }

    if( val <= 0 && delta < 0 ) {
        return size - 1;
    }


    // NOLINTNEXTLINE(cata-no-long)
    if( val >= static_cast<V>( size ) - 1 && delta > 0 ) {
        return 0;
    }
    return std::clamp<V>( val + delta, 0, size - 1 );
}





template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp_wrap( V val, bool inc, S size )
{
    return inc_clamp_wrap( val, static_cast<int>( inc ? 1 : -1 ), size );
}






template<typename T, typename I>
inline std::enable_if_t<std::is_enum_v<T>, T>
inc_clamp_wrap( T val, I inc, T size )
{
    return static_cast<T>( inc_clamp_wrap( static_cast<int>( val ), inc, static_cast<int>( size ) ) );
}





template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp( V val, int delta, S min, S max )
{

    if constexpr( std::is_unsigned_v<V> ) {
        // NOLINTNEXTLINE(cata-no-long)
        if( delta < 0 && val <= static_cast<V>( -delta ) ) {
            return min;
        }
    }
    return std::clamp<V>( val + delta, min, max );
}





template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp( V val, bool inc, S max )
{
    // NOLINTNEXTLINE(cata-no-long)
    return inc_clamp( val, inc ? 1 : -1, static_cast<S>( 0 ), max );
}





template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp( V val, int delta, S max )
{
    // NOLINTNEXTLINE(cata-no-long)
    return inc_clamp( val, delta, static_cast<S>( 0 ), max );
}





template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp( V val, bool inc, S min, S max )
{
    return inc_clamp( val, inc ? 1 : -1, min, max );
}






template<typename T, typename I>
inline std::enable_if_t<std::is_enum_v<T>, T>
inc_clamp( T val, I inc, T size )
{
    return static_cast<T>( inc_clamp( static_cast<int>( val ), inc, static_cast<int>( size ) ) );
}






template<typename V, typename S>
bool navigate_ui_list( const std::string &action, V &val, int page_delta, S size, bool wrap );













std::pair<int, int> subindex_around_cursor( int num_entries, int available_space, int cursor_pos,
        bool focused = true );

#endif
