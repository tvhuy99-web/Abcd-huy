#pragma once

#include <set>
#include <string>
#include <vector>

#include "point.h"





enum action_id : int;



enum class MouseInput : int {

    LeftButtonPressed = 1,
    LeftButtonReleased,

    RightButtonPressed,
    RightButtonReleased,

    ScrollWheelUp,
    ScrollWheelDown,

    Move,

    X1ButtonPressed,
    X1ButtonReleased,

    X2ButtonPressed,
    X2ButtonReleased

};

enum class input_event_t : int  {
    error,
    timeout,

    keyboard_char,

    keyboard_code,
    gamepad,
    mouse
};

enum class keymod_t {
    ctrl,
    alt,
    shift,
};









struct input_event {
    input_event_t type;

    std::set<keymod_t> modifiers;


    std::vector<int> sequence;



    point mouse_pos;



    std::string text;
    std::string edit;
    bool edit_refresh;

#if defined(__ANDROID__)

    int shortcut_last_used_action_counter = 0;
#endif

    input_event() : edit_refresh( false ) {
        type = input_event_t::error;
    }
    input_event( int s, input_event_t t )
        : type( t ), edit_refresh( false ) {
        sequence.push_back( s );
    }
















    input_event( const MouseInput s, input_event_t t )
        : type( t ), edit_refresh( false ) {
        sequence.push_back( static_cast<int>( s ) );
    }

    input_event( const std::set<keymod_t> &mod, int s, input_event_t t );

    int get_first_input() const;

    void add_input( const int input ) {
        sequence.push_back( input );
    }
















    void add_input( const MouseInput mouse_input ) {
        sequence.push_back( static_cast<int>( mouse_input ) );
    }

    bool operator==( const input_event &other ) const {
        return type == other.type && modifiers == other.modifiers && sequence == other.sequence;
    }

    bool operator!=( const input_event &other ) const;

    std::string long_description() const;
    std::string short_description() const;





    static bool compare_type_mod_code( const input_event &lhs, const input_event &rhs );
};

enum class keyboard_mode {


    keychar,





    keycode,
};
