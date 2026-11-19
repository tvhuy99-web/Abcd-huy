#pragma once

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "action.h"
#include "coords_fwd.h"
#include "input_enums.h"
#include "point.h"
#include "translation.h"

enum class kb_menu_status;

class hotkey_queue;
class keybindings_ui;
namespace catacurses
{
class window;
}











class input_context;
struct input_context_handle {
    input_context *cxtx;
};

class input_context_stack_impl
{
    public:
        input_context *back();
        void pop();
        void push( std::shared_ptr<input_context_handle> const &context );

    private:
        std::list<std::weak_ptr<input_context_handle>> stack;
        input_context *reap();

};

class input_context
{
        friend class keybindings_ui;






        std::shared_ptr<input_context_handle> handle{ new input_context_handle{this} };

        std::string a11y_action_result;

    public:
        const std::vector<std::string> &registered_action_ids() const { return registered_actions; }
        const std::string &category_id() const { return category; }

        bool was_accessibility_action() const { return !a11y_action_result.empty(); }
#if defined(__ANDROID__) || defined(TILES)

        static input_context_stack_impl input_context_stack;
#endif
        input_context() : category( "default" ), registered_any_input( false ),
            coordinate_input_received( false ), handling_coordinate_input( false ) {
#if defined(__ANDROID__) || defined(TILES)
            input_context_stack.push( handle );
#endif
#if defined(__ANDROID__)
            allow_text_entry = false;
#endif
            register_action( "toggle_language_to_en" );
        }


        explicit input_context( const std::string &category,
                                const keyboard_mode preferred_keyboard_mode = keyboard_mode::keycode )
            : category( category ), registered_any_input( false ),
              coordinate_input_received( false ), handling_coordinate_input( false ),
              preferred_keyboard_mode( preferred_keyboard_mode ) {
#if defined(__ANDROID__) || defined(TILES)
            input_context_stack.push( handle );
#endif
#if defined(__ANDROID__)
            allow_text_entry = false;
#endif
            register_action( "toggle_language_to_en" );
        }




        class scoped_activation
        {
            public:
                explicit scoped_activation( input_context &ctx ) {
                    ( void )ctx;
#if defined(__ANDROID__) || defined(TILES)
                    input_context_stack.push( ctx.handle );
#endif
                }
                ~scoped_activation() {
#if defined(__ANDROID__) || defined(TILES)
                    input_context_stack.pop();
#endif
                }
                scoped_activation( const scoped_activation & ) = delete;
                scoped_activation &operator=( const scoped_activation & ) = delete;
                scoped_activation( scoped_activation && ) = delete;
                scoped_activation &operator=( scoped_activation && ) = delete;
        };

        input_context( const input_context &other ) {
            reassign( other );
        }

        input_context &operator=( const input_context &other ) {
            if( this == &other ) {
                return *this;
            }
            reassign( other );
            return *this;
        }

        input_context( input_context &&other ) noexcept {
            reassign( std::move( other ) );
        }

        input_context &operator=( input_context &&other ) noexcept {
            reassign( std::move( other ) );
            return *this;
        }

        template<typename Rhs>
        void reassign( Rhs &&other ) {
            a11y_action_result.clear();


            // NOLINTBEGIN(bugprone-use-after-move)

#if defined(__ANDROID__)
            registered_manual_keys = std::forward<Rhs>( other ).registered_manual_keys;
#endif
            registered_actions = std::forward<Rhs>( other ).registered_actions;
            edittext = std::forward<Rhs>( other ).edittext;
            category = std::forward<Rhs>( other ).category;
            coordinate = std::forward<Rhs>( other ).coordinate;
            registered_any_input = std::forward<Rhs>( other ).registered_any_input;
            coordinate_input_received = std::forward<Rhs>( other ).coordinate_input_received;
            handling_coordinate_input = std::forward<Rhs>( other ).handling_coordinate_input;
            iso_mode = std::forward<Rhs>( other ).iso_mode;
            allow_text_entry = std::forward<Rhs>( other ).allow_text_entry;
            next_action = std::forward<Rhs>( other ).next_action;
            timeout = std::forward<Rhs>( other ).timeout;
            preferred_keyboard_mode = std::forward<Rhs>( other ).preferred_keyboard_mode;
            action_name_overrides = std::forward<Rhs>( other ).action_name_overrides;
            // NOLINTEND(bugprone-use-after-move)
        }


#if defined(__ANDROID__)

        struct manual_key {
            manual_key( int _key, const std::string &_text ) : key( _key ), text( _text ) {}
            int key;
            std::string text;
            bool operator==( const manual_key &other ) const {
                return key == other.key && text == other.text;
            }
        };

        void register_manual_key( manual_key mk );
        void register_manual_key( int key, const std::string text = "" );

        std::string get_action_name_for_manual_key( int key ) {
            for( const auto &manual_key : registered_manual_keys ) {
                if( manual_key.key == key ) {
                    return manual_key.text;
                }
            }
            return std::string();
        }
        std::vector<manual_key> &get_registered_manual_keys() {
            return registered_manual_keys;
        }

        std::string &get_category() {
            return category;
        }
        std::vector<std::string> &get_registered_actions() {
            return registered_actions;
        }
        bool is_action_registered( const std::string &action_descriptor ) const {
            return std::find( registered_actions.begin(), registered_actions.end(),
                              action_descriptor ) != registered_actions.end();
        }

        bool operator==( const input_context &other ) const {
            return category == other.category &&
                   registered_actions == other.registered_actions &&
                   registered_manual_keys == other.registered_manual_keys &&
                   allow_text_entry == other.allow_text_entry &&
                   registered_any_input == other.registered_any_input &&
                   coordinate == other.coordinate &&
                   coordinate_input_received == other.coordinate_input_received &&
                   handling_coordinate_input == other.handling_coordinate_input &&
                   next_action == other.next_action &&
                   iso_mode == other.iso_mode &&
                   action_name_overrides == other.action_name_overrides &&
                   timeout == other.timeout;
        }
        bool operator!=( const input_context &other ) const {
            return !( *this == other );
        }
#endif



















        void register_action( const std::string &action_descriptor );










        void register_action( const std::string &action_descriptor, const translation &name );













        std::string get_available_single_char_hotkeys(
            std::string requested_keys =
                "abcdefghijkpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-=:;'\",./<>?!@#$%^&*()_+[]\\{}|`~" );

        using input_event_filter = std::function<bool( const input_event & )>;


        static bool disallow_lower_case_or_non_modified_letters( const input_event &evt );
        static bool allow_all_keys( const input_event &evt );

















        std::string get_desc( const std::string &action_descriptor,
                              unsigned int max_limit = 0,
                              const input_event_filter &evt_filter = allow_all_keys ) const;








        std::string get_button_text( const std::string &action_descriptor ) const;










        std::string get_button_text( const std::string &action_descriptor,
                                     const std::string &action_text ) const;



















        std::string get_desc( const std::string &action_descriptor,
                              const std::string &text,
                              const input_event_filter &evt_filter,
                              const translation &inline_fmt,
                              const translation &separate_fmt ) const;
        std::string get_desc( const std::string &action_descriptor,
                              const std::string &text,
                              const input_event_filter &evt_filter = allow_all_keys ) const;




        std::string describe_key_and_name( const std::string &action_descriptor,
                                           const input_event_filter &evt_filter = allow_all_keys ) const;














        const std::string &handle_input();
        const std::string &handle_input( int timeout );








        std::optional<tripoint_rel_ms> get_direction_rel_ms( const std::string &action ) const;
        std::optional<tripoint_rel_omt> get_direction_rel_omt( const std::string &action ) const;




        std::optional<tripoint_bub_ms> get_coordinates( const catacurses::window &capture_win_,
                const point &offset = point::zero, bool center_cursor = false ) const;
        std::optional<tripoint_rel_omt> get_coordinates_rel_omt( const catacurses::window &capture_win_,
                const point &offset = point::zero, bool center_cursor = false ) const;


        void register_directions();
        void register_updown();
        void register_leftright();
        void register_cardinal();
        void register_navigate_ui_list();







        action_id display_menu( bool permit_execute_action = false );
    private:





        bool action_reset( const std::string &action_id );





        bool action_add( const std::string &name, const std::string &action_id, bool is_local,
                         kb_menu_status status );





        bool action_remove( const std::string &name, const std::string &action_id, bool is_local,
                            bool is_empty );
    public:






        input_event get_raw_input();




        std::optional<point> get_coordinates_text( const catacurses::window &capture_win ) const;




        std::string get_action_name( const std::string &action_id ) const;







        std::string press_x( const std::string &action_id ) const;
        std::string press_x( const std::string &action_id, const std::string &key_bound,
                             const std::string &key_unbound ) const;
        std::string press_x( const std::string &action_id, const std::string &key_bound_pre,
                             const std::string &key_bound_suf, const std::string &key_unbound ) const;
















        std::vector<input_event> keys_bound_to( const std::string &action_descriptor,
                                                int maximum_modifier_count = -1,
                                                bool restrict_to_printable = true,
                                                bool restrict_to_keyboard = true ) const;




        void set_edittext( const std::string &s );
        std::string get_edittext();

        void set_iso( bool mode = true );








        void set_timeout( int val );
        void reset_timeout();

        input_event first_unassigned_hotkey( const hotkey_queue &queue ) const;
        input_event next_unassigned_hotkey( const hotkey_queue &queue, const input_event &prev ) const;
    private:
#if defined(__ANDROID__)
        std::vector<manual_key> registered_manual_keys;
#endif
        std::vector<std::string> registered_actions;
        std::string edittext;
    public:
        const std::string &input_to_action( const input_event &inp ) const;
        bool is_event_type_enabled( input_event_t type ) const;
        bool is_registered_action( const std::string &action_name ) const;
    private:
        std::string category;
        point coordinate;

        bool registered_any_input;
        bool coordinate_input_received;
        bool handling_coordinate_input;
        bool iso_mode = false;

    public:


        bool allow_text_entry;
    private:
        input_event next_action;
        int timeout = -1;
        keyboard_mode preferred_keyboard_mode = keyboard_mode::keycode;







        std::map<std::string, translation> action_name_overrides;




        bool action_uses_input( const std::string &action_id, const input_event &event ) const;




        std::string get_conflicts( const input_event &event, const std::string &ignore_action ) const;







        void clear_conflicting_keybindings( const input_event &event, const std::string &ignore_action );









        bool resolve_conflicts( const std::vector<input_event> &events, const std::string &ignore_action );







        std::vector<std::string> filter_strings_by_phrase( const std::vector<std::string> &strings,
                std::string_view phrase ) const;
};

class hotkey_queue
{
    public:


        input_event first( const input_context &ctxt ) const;

        input_event next( const input_event &prev ) const;







        static const hotkey_queue &alphabets();







        static const hotkey_queue &alpha_digits();

    private:
        std::vector<int> codes_keychar;
        std::vector<int> codes_keycode;
        std::vector<std::set<keymod_t>> modifiers_keycode;
};
