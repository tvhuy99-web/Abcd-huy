#pragma once
#ifndef CATA_SRC_WIDGET_H
#define CATA_SRC_WIDGET_H

#include <climits>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "color.h"
#include "dialogue_helpers.h"
#include "output.h"
#include "translation.h"
#include "type_id.h"

class avatar;

namespace catacurses
{
class window;
}
struct const_dialogue;
template <typename E> struct enum_traits;



enum class widget_var : int {
    focus,
    move,
    move_remainder,
    move_cost,
    pain,
    sound,
    speed,
    stamina,
    sleepiness,
    health,
    daily_health,
    mana,
    max_mana,
    power_percentage,
    log_power_balance,
    morale_level,
    weariness_level,
    weary_transition_level,
    stat_str,
    stat_dex,
    stat_int,
    stat_per,
    bp_hp,
    bp_encumb,
    bp_warmth,
    bp_wetness,
    mood,
    cardio_fit,
    cardio_acc,
    carry_weight,
    custom,

    activity_text,
    body_graph,
    body_graph_temp,
    body_graph_encumb,
    body_graph_status,
    body_graph_wet,
    bp_armor_outer_text,
    carry_weight_text,
    carry_weight_value,
    compass_text,
    compass_legend_text,
    date_text,
    env_temp_text,
    faction_territory,
    mood_text,
    move_count_mode_text,
    overmap_loc_text,
    overmap_text,
    oxygen,
    pain_text,
    place_text,
    power_text,
    power_balance_text,
    safe_mode_text,
    safe_mode_classic_text,
    style_text,
    sundial_text,
    sundial_time_text,
    time_text,
    veh_azimuth_text,
    veh_battery_text,
    veh_cruise_text,
    veh_fuel_text,
    weariness_text,
    weary_malus_text,
    snow_depth_text,
    weather_text,
    wielding_text,
    wielding_simple_text,
    wielding_mode_text,
    wielding_ammo_text,
    wind_text,
    last
};



template<>
struct enum_traits<widget_var> {
    static constexpr widget_var last = widget_var::last;
};



enum class cardinal_direction : int {
    NORTH = 0,
    NORTHEAST = 1,
    EAST = 2,
    SOUTHEAST = 3,
    SOUTH = 4,
    SOUTHWEST = 5,
    WEST = 6,
    NORTHWEST = 7,
    LOCAL = 8,
    num_cardinal_directions
};



template<>
struct enum_traits<cardinal_direction> {
    static constexpr cardinal_direction last = cardinal_direction::num_cardinal_directions;
};


enum class bodypart_status : int {
    BITTEN,
    INFECTED,
    BROKEN,
    SPLINTED,
    BANDAGED,
    DISINFECTED,
    BLEEDING,
    num_bodypart_status
};

template<>
struct enum_traits<bodypart_status> {
    static constexpr bodypart_status last = bodypart_status::num_bodypart_status;
};


enum class widget_alignment : int {
    LEFT,
    CENTER,
    RIGHT,
    num_widget_alignments
};

template<>
struct enum_traits<widget_alignment> {
    static constexpr widget_alignment last = widget_alignment::num_widget_alignments;
};


class JsonObject;
class widget;

class window_panel;
template<typename T>
class generic_factory;

struct widget_clause {
    private:
        friend class widget;
        std::string id;
        std::string sym;
        translation text;
        nc_color color = c_unset;
        int value = INT_MIN;
        std::vector<widget_id> widgets;


        bool has_condition = false;

        bool should_parse_tags = false;
        std::function<bool( const_dialogue const & )> condition;
        bool meets_condition( const std::string &opt_var = "" ) const;
        bool meets_condition( const std::set<bodypart_id> &bps ) const;

        static const widget_clause *get_clause_for_id( const std::string &clause_id, const widget_id &wgt,
                int thresh_val = INT_MIN, bool skip_condition = false );

    public:
        void load( const JsonObject &jo );


















        static int get_val_for_id( const std::string &clause_id,
                                   const widget_id &wgt, bool skip_condition = false );
        static const translation &get_text_for_id( const std::string &clause_id,
                const widget_id &wgt, bool skip_condition = false );
        static const std::string &get_sym_for_id( const std::string &clause_id,
                const widget_id &wgt, bool skip_condition = false );
        static nc_color get_color_for_id( const std::string &clause_id,
                                          const widget_id &wgt, bool skip_condition = false );
};


struct widget_custom_var {
    dbl_or_var::part_t value;
    dbl_or_var::part_t min;
    dbl_or_var::part_t max;
    std::pair<dbl_or_var::part_t, dbl_or_var::part_t> norm;

    void deserialize( const JsonObject &jo );
    void set_widget_var_range( const avatar &ava, widget &wgt ) const;
    int get_var_value( const avatar &ava ) const;
};



class widget
{
    private:
        friend class generic_factory<widget>;
        friend struct mod_tracker;

        widget_id id;
        std::vector<std::pair<widget_id, mod_id>> src;
        bool was_loaded = false;
        const widget_clause *get_clause( const std::string &clause_id = "" ) const;
        std::vector<const widget_clause *> get_clauses() const;

    public:
        widget() = default;
        explicit widget( const widget_id &id ) : id( id ) {}




        std::string _style;

        translation _label;

        std::string _description;

        int _label_width = 0;

        std::string _separator;

        int _padding;

        widget_var _var = widget_var::last;

        widget_custom_var _custom_var;

        int _var_min = INT_MIN;

        int _var_max = INT_MAX;

        bool explicit_separator;

        bool explicit_padding;

        bool _pad_labels;


        std::pair<int, int> _var_norm = std::make_pair( INT_MIN, INT_MAX );

        std::set<bodypart_id> _bps;

        int _width = 0;

        int _height = 0;

        int _height_max = 0;

        std::string _symbols;

        std::string _fill;

        translation _string;

        std::vector<nc_color> _colors;

        std::vector<int> _breaks;

        std::vector<widget_id> _widgets;

        std::string _arrange;

        std::string _body_graph;

        cardinal_direction _direction;

        std::set<flag_id> _flags;

        std::vector<widget_clause> _clauses;

        widget_clause _default_clause;

        widget_alignment _text_align;

        widget_alignment _label_align;


        static void load_widget( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, std::string_view src );

        static void finalize_all();
        void finalize();

        static int finalize_label_width_recursive( const widget_id &id );

        static void finalize_inherited_fields_recursive( const widget_id &id,
                const std::string &label_separator, int col_padding );

        static void reset();

        static const std::vector<widget> &get_all();

        const widget_id &getId() const;




        std::string layout( const avatar &ava, unsigned int max_width = 0, int label_width = 0,
                            bool skip_pad = false );

        std::string show( const avatar &ava, unsigned int max_width );

        window_panel get_window_panel( int width, int req_height = 1 );

        std::string color_text_function_string( const avatar &ava, unsigned int max_width );

        bool uses_text_function() const;


        int get_var_value( const avatar &ava ) const;

        bool has_flag( const flag_id &flag ) const;
        bool has_flag( const std::string &flag ) const;

        const bodypart_id &only_bp() const;


        void set_default_var_range( const avatar &ava );


        std::string color_value_string( int value, int width_max = 0 );

        std::string value_string( int value, int width_max = 0 );

        nc_color value_color( int value );


        std::string number( int value, bool from_condition ) const;

        std::string number_cond( enumeration_conjunction join_type = enumeration_conjunction::none ) const;

        std::string text( bool from_condition, int width = 0 );

        std::string text_cond( bool no_join = false, int width = 0 );

        std::string sym( bool from_condition );

        std::string sym_cond( bool no_join = true,
                              enumeration_conjunction join_type = enumeration_conjunction::none ) const;

        std::string sym_text( bool from_condition, int width = 0 );

        std::string sym_text_cond( bool no_join = true, int width = 0 );

        std::vector<string_id<widget>> widgets( bool from_condition );

        std::vector<string_id<widget>> widgets_cond();

        std::string graph( int value ) const;
        // Takes a string generated by widget::layout and draws the text to the window w.



        static int custom_draw_multiline( const std::string &widget_string, const catacurses::window &w,
                                          int margin, int width, int row_num );
};





std::string format_widget_multiline( const std::vector<std::string> &keys, int max_height,
                                     int width, int &height, bool join = false );

#endif
