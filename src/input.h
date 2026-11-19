#pragma once
#ifndef CATA_SRC_INPUT_H
#define CATA_SRC_INPUT_H

#include <map>
#include <string>
#include <vector>

#include "input_enums.h"
#include "translation.h"

class cata_path;


constexpr int KEY_ESCAPE     = 27;
constexpr int KEY_MIN        =
    0x101;
constexpr int KEY_BREAK      =
    0x101;
constexpr int KEY_DOWN       = 0x102;
constexpr int KEY_UP         = 0x103;
constexpr int KEY_LEFT       = 0x104;
constexpr int KEY_RIGHT      = 0x105;
constexpr int KEY_HOME       =
    0x106;
constexpr int KEY_BACKSPACE  =
    0x107;
constexpr int KEY_DC         = 0x14A;
constexpr int KEY_F0         = 0x108;
inline constexpr int KEY_F( const int n )
{
    return KEY_F0 + n;
}
constexpr int F_KEY_NUM_BEG  = 0;
constexpr int F_KEY_NUM_END  = 63;
inline constexpr int F_KEY_NUM( const int key )
{
    return key - KEY_F0;
}
inline constexpr bool IS_F_KEY( const int key )
{
    return key >= KEY_F( F_KEY_NUM_BEG ) && key <= KEY_F( F_KEY_NUM_END );
}

inline constexpr bool IS_CTRL_CHAR( const int key )
{

    return key >= 0 && key < ' ';
}

inline constexpr bool IS_NAMED_CTRL_CHAR( const int key )
{
    return key == '\t' || key == '\n' || key == KEY_ESCAPE || key == KEY_BACKSPACE;
}
inline constexpr int KEY_NUM( const int n )
{
    return 0x30 + n;
}
constexpr int KEY_NPAGE      = 0x152;
constexpr int KEY_PPAGE      = 0x153;
constexpr int KEY_ENTER      = 0x157;
constexpr int KEY_BTAB       = 0x161;
constexpr int KEY_END        = 0x168;








namespace keycode
{
enum : int {
    backspace = 0x08,
    tab       = 0x09,
    return_   = 0x0D,
    escape    = 0x1B,
    space     = 0x20,
    f1        = 0x4000003A,
    f2        = 0x4000003B,
    f3        = 0x4000003C,
    f4        = 0x4000003D,
    f5        = 0x4000003E,
    f6        = 0x4000003F,
    f7        = 0x40000040,
    f8        = 0x40000041,
    f9        = 0x40000042,
    f10       = 0x40000043,
    f11       = 0x40000044,
    f12       = 0x40000045,
    home      = 0x4000004A,
    ppage     = 0x4000004B,
    end       = 0x4000004D,
    npage     = 0x4000004E,
    right     = 0x4000004F,
    left      = 0x40000050,
    down      = 0x40000051,
    up        = 0x40000052,
    kp_divide = 0x40000054,
    kp_multiply = 0x40000055,
    kp_minus  = 0x40000056,
    kp_plus   = 0x40000057,
    kp_enter  = 0x40000058,
    kp_1      = 0x40000059,
    kp_2      = 0x4000005A,
    kp_3      = 0x4000005B,
    kp_4      = 0x4000005C,
    kp_5      = 0x4000005D,
    kp_6      = 0x4000005E,
    kp_7      = 0x4000005F,
    kp_8      = 0x40000060,
    kp_9      = 0x40000061,
    kp_0      = 0x40000062,
    kp_period = 0x40000063,
    f13       = 0x40000068,
    f14       = 0x40000069,
    f15       = 0x4000006A,
    f16       = 0x4000006B,
    f17       = 0x4000006C,
    f18       = 0x4000006D,
    f19       = 0x4000006E,
    f20       = 0x4000006F,
    f21       = 0x40000070,
    f22       = 0x40000071,
    f23       = 0x40000072,
    f24       = 0x40000073,
};
}

bool is_mouse_enabled();
bool is_keycode_mode_supported();
std::string get_input_string_from_file( const std::string &fname = "input.txt" );




struct action_attributes {
    action_attributes() : is_user_created( false ) {}
    bool is_user_created;
    translation name;
    std::vector<input_event> input_events;
};






constexpr int JOY_0  = 0;
constexpr int JOY_1  = 1;
constexpr int JOY_2  = 2;
constexpr int JOY_3  = 3;
constexpr int JOY_4  = 4;
constexpr int JOY_5  = 5;
constexpr int JOY_6  = 6;
constexpr int JOY_7  = 7;
constexpr int JOY_8  = 8;
constexpr int JOY_9  = 9;
constexpr int JOY_10 = 10;
constexpr int JOY_11 = 11;
constexpr int JOY_12 = 12;
constexpr int JOY_13 = 13;
constexpr int JOY_14 = 14;
constexpr int JOY_15 = 15;
constexpr int JOY_16 = 16;
constexpr int JOY_17 = 17;
constexpr int JOY_18 = 18;
constexpr int JOY_19 = 19;
constexpr int JOY_20 = 20;
constexpr int JOY_21 = 21;
constexpr int JOY_22 = 22;
constexpr int JOY_23 = 23;
constexpr int JOY_24 = 24;
constexpr int JOY_25 = 25;
constexpr int JOY_26 = 26;
constexpr int JOY_27 = 27;
constexpr int JOY_28 = 28;
constexpr int JOY_29 = 29;
constexpr int JOY_30 = 30;

constexpr int JOY_LS_LEFT      = 256 + 1;
constexpr int JOY_LS_RIGHT     = 256 + 2;
constexpr int JOY_LS_UP        = 256 + 3;
constexpr int JOY_LS_DOWN      = 256 + 4;
constexpr int JOY_LS_UP_RIGHT   = 256 + 5;
constexpr int JOY_LS_DOWN_RIGHT = 256 + 6;
constexpr int JOY_LS_UP_LEFT    = 256 + 7;
constexpr int JOY_LS_DOWN_LEFT  = 256 + 8;

constexpr int JOY_RS_LEFT      = 256 + 9;
constexpr int JOY_RS_RIGHT     = 256 + 10;
constexpr int JOY_RS_UP        = 256 + 11;
constexpr int JOY_RS_DOWN      = 256 + 12;
constexpr int JOY_RS_UP_RIGHT   = 256 + 13;
constexpr int JOY_RS_DOWN_RIGHT = 256 + 14;
constexpr int JOY_RS_UP_LEFT    = 256 + 15;
constexpr int JOY_RS_DOWN_LEFT  = 256 + 16;



constexpr int JOY_A  = 256 + 32;
constexpr int JOY_B  = 256 + 33;
constexpr int JOY_X  = 256 + 34;
constexpr int JOY_Y  = 256 + 35;


constexpr int JOY_LB = 256 + 36;
constexpr int JOY_RB = 256 + 37;


constexpr int JOY_LT = 256 + 38;
constexpr int JOY_RT = 256 + 39;


constexpr int JOY_LS = 256 + 40;
constexpr int JOY_RS = 256 + 41;


constexpr int JOY_UP    = 256 + 42;
constexpr int JOY_DOWN  = 256 + 43;
constexpr int JOY_LEFT  = 256 + 44;
constexpr int JOY_RIGHT = 256 + 45;


constexpr int JOY_START = 256 + 46;
constexpr int JOY_BACK  = 256 + 47;


constexpr int JOY_ALT_A  = 256 + 48;
constexpr int JOY_ALT_B  = 256 + 49;
constexpr int JOY_ALT_X  = 256 + 50;
constexpr int JOY_ALT_Y  = 256 + 51;
constexpr int JOY_ALT_RB = 256 + 52;
constexpr int JOY_ALT_LB = 256 + 53;
constexpr int JOY_ALT_RT = 256 + 55;
constexpr int JOY_ALT_LS = 256 + 56;
constexpr int JOY_ALT_RS = 256 + 57;
constexpr int JOY_ALT_UP    = 256 + 58;
constexpr int JOY_ALT_DOWN  = 256 + 59;
constexpr int JOY_ALT_LEFT  = 256 + 60;
constexpr int JOY_ALT_RIGHT = 256 + 61;
constexpr int JOY_ALT_START = 256 + 62;
constexpr int JOY_ALT_BACK  = 256 + 63;


constexpr int JOY_L_RADIAL_N  = 256 + 64;
constexpr int JOY_L_RADIAL_NE = 256 + 65;
constexpr int JOY_L_RADIAL_E  = 256 + 66;
constexpr int JOY_L_RADIAL_SE = 256 + 67;
constexpr int JOY_L_RADIAL_S  = 256 + 68;
constexpr int JOY_L_RADIAL_SW = 256 + 69;
constexpr int JOY_L_RADIAL_W  = 256 + 70;
constexpr int JOY_L_RADIAL_NW = 256 + 71;


constexpr int JOY_R_RADIAL_N  = 256 + 72;
constexpr int JOY_R_RADIAL_NE = 256 + 73;
constexpr int JOY_R_RADIAL_E  = 256 + 74;
constexpr int JOY_R_RADIAL_SE = 256 + 75;
constexpr int JOY_R_RADIAL_S  = 256 + 76;
constexpr int JOY_R_RADIAL_SW = 256 + 77;
constexpr int JOY_R_RADIAL_W  = 256 + 78;
constexpr int JOY_R_RADIAL_NW = 256 + 79;










class input_manager
{
        friend class keybindings_ui;
    public:










        const std::vector<input_event> &get_input_for_action( const std::string &action_descriptor,
                const std::string &context = "default", bool *overwrites_default = nullptr );




        int get_first_char_for_action( const std::string &action_descriptor,
                                       const std::string &context = "default" );




        void init();




        void save();





        int get_previously_pressed_key() const;




        int get_keycode( input_event_t inp_type, const std::string &name ) const;










        std::string get_keyname( int ch, input_event_t inp_type, bool portable = false ) const;






        input_event get_input_event( keyboard_mode preferred_keyboard_mode = keyboard_mode::keycode );



        void pump_events();





        void wait_for_any_key();






        void set_timeout( int delay );
        void reset_timeout() {
            set_timeout( -1 );
        }
        int get_timeout() const {
            return input_timeout;
        }

        static keyboard_mode actual_keyboard_mode( keyboard_mode preferred_keyboard_mode );


        void check_keybind( const std::string &category, const std::string &keybind,
                            const std::string &context ) const;
    private:
        friend class input_context;

        using t_input_event_list = std::vector<input_event>;
        using t_actions = std::map<std::string, action_attributes>;
        using t_action_contexts = std::map<std::string, t_actions>;
        t_action_contexts action_contexts;




        t_action_contexts basic_action_contexts;

        using t_key_to_name_map = std::map<int, std::string>;
        t_key_to_name_map keyboard_char_keycode_to_keyname;
        t_key_to_name_map keyboard_code_keycode_to_keyname;
        t_key_to_name_map gamepad_keycode_to_keyname;
        t_key_to_name_map mouse_keycode_to_keyname;
        using t_name_to_key_map = std::map<std::string, int>;
        t_name_to_key_map keyboard_char_keyname_to_keycode;
        t_name_to_key_map keyboard_code_keyname_to_keycode;
        t_name_to_key_map gamepad_keyname_to_keycode;
        t_name_to_key_map mouse_keyname_to_keycode;


        int previously_pressed_key;



        void init_keycode_mapping();
        void add_keyboard_char_keycode_pair( int ch, const std::string &name );
        void add_keyboard_code_keycode_pair( int ch, const std::string &name );
        void add_gamepad_keycode_pair( int ch, const std::string &name );
        void add_mouse_keycode_pair( MouseInput mouse_input, const std::string &name );




        void load( const cata_path &file_name, bool is_user_preferences );

        int input_timeout;

        t_input_event_list &get_or_create_event_list( const std::string &action_descriptor,
                const std::string &context );



        bool remove_input_for_action( const std::string &action_descriptor, const std::string &context );
        void add_input_for_action( const std::string &action_descriptor, const std::string &context,
                                   const input_event &event );
















        const action_attributes &get_action_attributes(
            const std::string &action_id,
            const std::string &context = "default",
            bool *overwrites_default = nullptr,
            bool use_basic_action_contexts = false );












        translation get_default_action_name( const std::string &action_id ) const;
};


extern input_manager inp_mngr;






bool gamepad_available();

#endif
