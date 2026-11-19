#pragma once
#ifndef CATA_SRC_ENUMS_H
#define CATA_SRC_ENUMS_H

#include <type_traits>

template<typename T> struct enum_traits;

template<typename T>
constexpr inline int sgn( const T x )
{
    return x < 0 ? -1 : ( x > 0 ? 1 : 0 );
}

enum class aim_exit : int {
    none = 0,
    okay,
    re_entry,
    last
};

template<>
struct enum_traits<aim_exit> {
    static constexpr aim_exit last = aim_exit::last;
};


enum class aim_entry : int {
    START     = 0,
    VEHICLE   = 1,
    MAP       = 2,
    RESET     = 3,
    last
};

template<>
struct enum_traits<aim_entry> {
    static constexpr aim_entry last = aim_entry::last;
};

using I = std::underlying_type_t<aim_entry>;
inline constexpr aim_entry &operator++( aim_entry &lhs )
{
    lhs = static_cast<aim_entry>( static_cast<I>( lhs ) + 1 );
    return lhs;
}

inline constexpr aim_entry &operator--( aim_entry &lhs )
{
    lhs = static_cast<aim_entry>( static_cast<I>( lhs ) - 1 );
    return lhs;
}

inline constexpr aim_entry operator+( const aim_entry &lhs, const I &rhs )
{
    return static_cast<aim_entry>( static_cast<I>( lhs ) + rhs );
}

inline constexpr aim_entry operator-( const aim_entry &lhs, const I &rhs )
{
    return static_cast<aim_entry>( static_cast<I>( lhs ) - rhs );
}

enum class bionic_ui_sort_mode : int {
    NONE   = 0,
    POWER  = 1,
    NAME   = 2,
    INVLET = 3,
    nsort  = 4,
};

template<>
struct enum_traits<bionic_ui_sort_mode> {
    static constexpr bionic_ui_sort_mode last = bionic_ui_sort_mode::nsort;
};

enum class veh_spawn_status : int {
    DEFAULT_LIGHT_DMG = -1,
    UNDAMAGED = 0,
    DISABLED = 1,
    PRISTINE = 2,
    LAST = 3
};


enum class surroundings_menu_sort_flags : int {
    DEFAULT       = 0,
    NAME          = 1 << 0,
    CATEGORY      = 1 << 1,
    CATEGORY_NAME = NAME | CATEGORY,
    count
};

template<>
struct enum_traits<surroundings_menu_sort_flags> {
    static constexpr surroundings_menu_sort_flags first = surroundings_menu_sort_flags::DEFAULT;
    static constexpr surroundings_menu_sort_flags last = surroundings_menu_sort_flags::count;
    static constexpr bool is_flag_enum = true;
};


enum class trinary : int {
    NONE = 0,
    SOME = 1,
    ALL  = 2
};

enum class holiday : int {
    none = 0,
    new_year,
    easter,
    independence_day,
    halloween,
    thanksgiving,
    christmas,
    num_holiday
};

template<>
struct enum_traits<holiday> {
    static constexpr holiday last = holiday::num_holiday;
};

enum class temperature_flag : int {
    NORMAL = 0,
    HEATER,
    FRIDGE,
    FREEZER,
    ROOT_CELLAR
};


enum class rule_state : int {
    NONE,
    WHITELISTED,
    BLACKLISTED
};

enum class visibility_type : int {
    HIDDEN,
    CLEAR,
    LIT,
    BOOMER,
    DARK,
    BOOMER_DARK
};


enum class ot_match_type : int {



    exact,



    type,


    subtype,



    prefix,



    contains,
    num_ot_match_type
};

template<>
struct enum_traits<ot_match_type> {
    static constexpr ot_match_type last = ot_match_type::num_ot_match_type;
};

using accessor_flags = int;

enum accessor_flags_ {
    Access_None                       = 0,
    Access_Inventory                  = 1 << 0,
    Access_Map_Around                 = 1 << 1,
    Access_Map_Current_Z              = 1 << 2,
    Access_Map_All                    = 1 << 3,
    Access_Vehicle                    = 1 << 4,
    Access_EVERYTHING                 = Access_Inventory | Access_Map_All | Access_Vehicle
};

enum class special_game_type : int {
    NONE = 0,
    TUTORIAL,
    NUM_SPECIAL_GAME_TYPES
};

enum art_effect_passive : int {
    AEP_NULL = 0,

    AEP_STR_UP,
    AEP_DEX_UP,
    AEP_PER_UP,
    AEP_INT_UP,
    AEP_ALL_UP,
    AEP_SPEED_UP,
    AEP_PBLUE,
    AEP_SNAKES,
    AEP_INVISIBLE,
    AEP_CLAIRVOYANCE,
    AEP_SUPER_CLAIRVOYANCE,
    AEP_STEALTH,
    AEP_EXTINGUISH,
    AEP_GLOW,
    AEP_PSYSHIELD,
    AEP_RESIST_ELECTRICITY,
    AEP_CARRY_MORE,
    AEP_SAP_LIFE,
    AEP_FUN,

    AEP_SPLIT,

    AEP_HUNGER,
    AEP_THIRST,
    AEP_SMOKE,
    AEP_EVIL,
    AEP_SCHIZO,
    AEP_RADIOACTIVE,
    AEP_MUTAGENIC,
    AEP_ATTENTION,
    AEP_STR_DOWN,
    AEP_DEX_DOWN,
    AEP_PER_DOWN,
    AEP_INT_DOWN,
    AEP_ALL_DOWN,
    AEP_SPEED_DOWN,
    AEP_FORCE_TELEPORT,
    AEP_MOVEMENT_NOISE,
    AEP_BAD_WEATHER,
    AEP_SICK,
    AEP_CLAIRVOYANCE_PLUS,

    NUM_AEPS
};

template<>
struct enum_traits<art_effect_passive> {
    static constexpr art_effect_passive last = art_effect_passive::NUM_AEPS;
};

enum artifact_natural_property {
    ARTPROP_NULL,
    ARTPROP_WRIGGLING,
    ARTPROP_GLOWING,
    ARTPROP_HUMMING,
    ARTPROP_MOVING,
    ARTPROP_WHISPERING,
    ARTPROP_BREATHING,
    ARTPROP_DEAD,
    ARTPROP_ITCHY,
    ARTPROP_GLITTERING,
    ARTPROP_ELECTRIC,
    ARTPROP_SLIMY,
    ARTPROP_ENGRAVED,
    ARTPROP_CRACKLING,
    ARTPROP_WARM,
    ARTPROP_RATTLING,
    ARTPROP_SCALED,
    ARTPROP_FRACTAL,
    ARTPROP_MAX
};

enum class phase_id : int {
    PNULL,
    SOLID,
    LIQUID,
    GAS,
    PLASMA,
    num_phases
};

template<>
struct enum_traits<phase_id> {
    static constexpr phase_id last = phase_id::num_phases;
};




enum class object_type : int {
    NONE,
    ITEM,
    ACTOR,
    PLAYER,
    NPC,
    MONSTER,
    VEHICLE,
    TRAP,
    FIELD,
    TERRAIN,
    FURNITURE,
    FURNITURE_ON_VEHICLE,
    NUM_OBJECT_TYPES,
};

enum class liquid_source_type : int {
    INFINITE_MAP = 1,
    MAP_ITEM = 2,
    VEHICLE = 3,
    MONSTER = 4
};

enum class liquid_target_type : int {
    CONTAINER = 1,
    VEHICLE = 2,
    MAP = 3,
    MONSTER = 4
};










enum class layer_level : int {

    PERSONAL = 0,

    SKINTIGHT,

    NORMAL,

    WAIST,

    OUTER,

    BELTED,

    AURA,

    NUM_LAYER_LEVELS
};

template<>
struct enum_traits<layer_level> {
    static constexpr layer_level last = layer_level::NUM_LAYER_LEVELS;
};

inline layer_level &operator++( layer_level &l )
{
    l = static_cast<layer_level>( static_cast<int>( l ) + 1 );
    return l;
}


enum class distraction_type : int {
    noise,
    pain,
    attacked,
    hostile_spotted_far,
    hostile_spotted_near,
    talked_to,
    asthma,
    motion_alarm,
    weather_change,
    portal_storm_popup,
    eoc,
    dangerous_field,
    hunger,
    thirst,
    temperature,
    mutation,
    oxygen,
    withdrawal,
    craft_step_complete,
    last,
};

template<>
struct enum_traits<distraction_type> {
    static constexpr distraction_type last = distraction_type::last;
};

enum game_message_type : int {
    m_good,
    m_bad,
    m_mixed,

    m_warning,
    m_info,

    m_neutral,



    m_debug,

    m_headshot,
    m_critical,
    m_grazing,
    num_game_message_type
};

template<>
struct enum_traits<game_message_type> {
    static constexpr game_message_type last = game_message_type::num_game_message_type;
};

enum game_message_flags {

    gmf_none = 0,

    gmf_bypass_cooldown = 1,
};



struct game_message_params {
    // NOLINTNEXTLINE(google-explicit-constructor)
    game_message_params( const game_message_type message_type ) : type( message_type ),
        flags( gmf_none ) {}
    game_message_params( const game_message_type message_type,
                         const game_message_flags message_flags ) : type( message_type ), flags( message_flags ) {}


    game_message_type type;

    game_message_flags flags;
};

struct social_modifiers {
    int lie = 0;
    int persuade = 0;
    int intimidate = 0;

    social_modifiers &operator+=( const social_modifiers &other ) {
        this->lie += other.lie;
        this->persuade += other.persuade;
        this->intimidate += other.intimidate;
        return *this;
    }
    bool empty() const {
        return this->lie != 0 || this->persuade != 0 || this->intimidate != 0;
    }
};

enum MULTITILE_TYPE {
    center,
    corner,
    edge,
    t_connection,
    end_piece,
    unconnected,
    open_,
    broken,
    num_multitile_types
};

enum class reachability_cache_quadrant : int {
    NE, SE, NW, SW
};

template<>
struct enum_traits<reachability_cache_quadrant> {
    static constexpr reachability_cache_quadrant last = reachability_cache_quadrant::SW;
    static constexpr int size = static_cast<int>( last ) + 1;

    inline static reachability_cache_quadrant quadrant( bool S, bool W ) {
        return static_cast<reachability_cache_quadrant>(
                   ( static_cast<int>( W ) << 1 ) | static_cast<int>( S )
               );
    }
};

enum class monotonically : int {
    constant,
    increasing,
    decreasing,
    unknown,
};

constexpr bool is_increasing( monotonically m )
{
    return m == monotonically::constant || m == monotonically::increasing;
}

constexpr bool is_decreasing( monotonically m )
{
    return m == monotonically::constant || m == monotonically::decreasing;
}

enum class character_type : int {
    CUSTOM,
    RANDOM,
    TEMPLATE,
    NOW,
    FULL_RANDOM,
};

enum class aggregate_type : int {
    FIRST,
    LAST,
    MIN,
    MAX,
    SUM,
    AVERAGE,
    num_aggregate_types
};

template<>
struct enum_traits<aggregate_type> {
    static constexpr aggregate_type last = aggregate_type::num_aggregate_types;
};

enum class link_state : int {
    no_link = 0,
    needs_reeling,
    ups,
    solarpack,
    vehicle_port,
    vehicle_battery,
    bio_cable,
    vehicle_tow,
    automatic,
    last
};
template<>
struct enum_traits<link_state> {
    static constexpr link_state last = link_state::last;
};

enum mut_count_type {
    POSITIVE,
    NEGATIVE,
    ALL
};

enum class bp_type {

    head,

    torso,

    sensor,

    mouth,

    arm,

    hand,

    leg,

    foot,

    wing,

    tail,

    other,
    num_types
};

template<>
struct enum_traits<bp_type> {
    static constexpr bp_type last = bp_type::num_types;
};

enum class surroundings_menu_tab_enum : int {
    items = 0,
    monsters,
    terfurn,
    num_tabs
};

template<>
struct enum_traits<surroundings_menu_tab_enum> {
    static constexpr surroundings_menu_tab_enum last = surroundings_menu_tab_enum::num_tabs;
};


enum class rem_cap_return {
    SUCCESS,
    NO_SPACE,
    NO_SPACE_IN_PARENT,
    BUCKET_FAIL,
    ANOTHER_LIQUID_INSIDE,
    LAST
};

#endif
