#pragma once
#ifndef CATA_SRC_CHARACTER_H
#define CATA_SRC_CHARACTER_H

#include <algorithm>
#include <bitset>
#include <climits>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "activity_tracker.h"
#include "body_part_set.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character_attire.h"
#include "character_id.h"
#include "city.h"  // IWYU pragma: keep
#include "compatibility.h"
#include "coordinates.h"
#include "craft_command.h"
#include "creature.h"
#include "damage.h"
#include "enums.h"
#include "flat_set.h"
#include "game_constants.h"
#include "global_vars.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "memory_fast.h"
#include "monster.h"
#include "pimpl.h"
#include "player_activity.h"
#include "pocket_type.h"
#include "point.h"
#include "proficiency.h"
#include "ranged.h"
#include "ret_val.h"
#include "sleep.h"
#include "stomach.h"
#include "string_formatter.h"
#include "subbodypart.h"
#include "temp_crafting_inventory.h"
#include "type_id.h"
#include "units.h"
#include "visitable.h"
#include "weakpoint.h"
#include "weighted_list.h"

class JsonObject;
class JsonOut;
class SkillLevel;
class SkillLevelMap;
class activity_actor;
class addiction;
class basecamp;
class bionic_collection;
class character_martial_arts;
class dispersion_sources;
class effect;
class enchant_cache;
class faction;
class known_magic;
class ma_technique;
class map;
class player_morale;
class profession;
class recipe;
class recipe_subset;
class spell;
class ui_adaptor;
class vehicle;
class vpart_reference;

enum scaling_stat : int;

namespace catacurses
{
class window;
}
namespace Pickup
{
struct pick_info;
}

enum action_id : int;
enum class magic_energy_type : int;
enum class recipe_filter_flags : int;
enum class steed_type : int;
enum npc_attitude : int;
struct attention_plan;
struct bionic;
struct construction;
struct crafting_cost_context;
struct dealt_projectile_attack;

struct islot_comestible;
struct item_comp;
struct vitamin_resource_cost;
struct itype;
struct mutation_branch;
struct mutation_category_trait;
struct mutation_variant;
struct pathfinding_settings;
struct projectile;
struct requirement_data;
struct tool_comp;
struct trait_and_var;
struct trap;
struct w_point;
template <typename E> struct enum_traits;

using bionic_uid = unsigned int;

const int CHARACTER_STAT_MIN = 4;
const int CHARACTER_STAT_MAX = 20;

const int CHARACTER_AGE_MIN = 16;
const int CHARACTER_AGE_MAX = 100;

const int NAME_CHARACTER_LIMIT = 50;

extern int character_max_str;
extern int character_max_dex;
extern int character_max_per;
extern int character_max_int;

constexpr int MAX_CLAIRVOYANCE = 40;

constexpr float KCAL_PER_KG = 3500 * 2.20462;



enum vision_modes {
    DEBUG_NIGHTVISION,
    NV_GOGGLES,
    NIGHTVISION_1,
    NIGHTVISION_2,
    NIGHTVISION_3,
    CEPH_VISION,

    BIRD_EYE,

    URSINE_VISION,
    BOOMERED,
    DARKNESS,
    IR_VISION,
    VISION_CLAIRVOYANCE,
    VISION_CLAIRVOYANCE_PLUS,
    VISION_CLAIRVOYANCE_SUPER,
    NUM_VISION_MODES
};

enum class sleepiness_levels : int {
    TIRED = 191,
    DEAD_TIRED = 383,
    EXHAUSTED = 575,
    MASSIVE_SLEEPINESS = 1000
};
const std::unordered_map<std::string, sleepiness_levels> sleepiness_level_strs = { {
        { "TIRED", sleepiness_levels::TIRED },
        { "DEAD_TIRED", sleepiness_levels::DEAD_TIRED },
        { "EXHAUSTED", sleepiness_levels::EXHAUSTED },
        { "MASSIVE_SLEEPINESS", sleepiness_levels::MASSIVE_SLEEPINESS }
    }
};

constexpr inline bool operator>=( const sleepiness_levels &lhs, const sleepiness_levels &rhs )
{
    return static_cast<int>( lhs ) >= static_cast<int>( rhs );
}

constexpr inline bool operator<( const sleepiness_levels &lhs, const sleepiness_levels &rhs )
{
    return static_cast<int>( lhs ) < static_cast<int>( rhs );
}

template<typename T>
constexpr inline bool operator>=( const T &lhs, const sleepiness_levels &rhs )
{
    return lhs >= static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator>( const T &lhs, const sleepiness_levels &rhs )
{
    return lhs > static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator<=( const T &lhs, const sleepiness_levels &rhs )
{
    return lhs <= static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator<( const T &lhs, const sleepiness_levels &rhs )
{
    return lhs < static_cast<T>( rhs );
}

template<typename T>
constexpr inline int operator/( const sleepiness_levels &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) / rhs;
}

template<typename T>
constexpr inline int operator+( const sleepiness_levels &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) + rhs;
}

template<typename T>
constexpr inline int operator-( const sleepiness_levels &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) - rhs;
}

template<typename T>
constexpr inline int operator-( const T &lhs, const sleepiness_levels &rhs )
{
    return lhs - static_cast<T>( rhs );
}










enum sleep_deprivation_levels {

    SLEEP_DEPRIVATION_HARMLESS = 2 * 24 * 60,

    SLEEP_DEPRIVATION_MINOR = 4 * 24 * 60,

    SLEEP_DEPRIVATION_SERIOUS = 7 * 24 * 60,

    SLEEP_DEPRIVATION_MAJOR = 10 * 24 * 60,

    SLEEP_DEPRIVATION_MASSIVE = 14 * 24 * 60
};

enum class blood_type {
    blood_O,
    blood_A,
    blood_B,
    blood_AB,
    blood_acid,
    num_bt
};

template<>
struct enum_traits<blood_type> {
    static constexpr blood_type last = blood_type::num_bt;
};




enum edible_rating {

    EDIBLE,

    INEDIBLE,

    INEDIBLE_MUTATION,

    ALLERGY,

    ALLERGY_WEAK,

    CANNIBALISM,

    PARASITES,

    ROTTEN,

    NAUSEA,

    TOO_FULL,

    NO_TOOL
};

enum crush_tool_type {
    CRUSH_EMPTY_HANDS,
    CRUSH_HAMMER,
    CRUSH_DRILL_OR_HAMMER_AND_SCREW,
    CRUSH_NO_TOOL
};

struct character_portrait {
    public:
        character_portrait_id id;
};

struct queued_eoc {
    public:
        effect_on_condition_id eoc;
        time_point time;
        global_variables::impl_t context;
};

struct eoc_compare {
    bool operator()( const queued_eoc &lhs, const queued_eoc &rhs ) const {
        return lhs.time > rhs.time;
    }
};

struct queued_eocs {
    using storage_iter = std::list<queued_eoc>::iterator;

    struct eoc_compare : ::eoc_compare {
        bool operator()( const storage_iter &lhs, const storage_iter &rhs ) const {
            return ::eoc_compare::operator()( *lhs, *rhs );
        }
    };
    std::priority_queue<storage_iter, std::vector<storage_iter>, eoc_compare> queue;
    std::list<queued_eoc> list;

    queued_eocs();

    queued_eocs( const queued_eocs &rhs );
    queued_eocs( queued_eocs &&rhs ) noexcept;

    queued_eocs &operator=( const queued_eocs &rhs );
    queued_eocs &operator=( queued_eocs &&rhs ) noexcept;


    bool empty() const;
    const queued_eoc &top() const;
    void push( const queued_eoc &eoc );
    void pop();
};

struct aim_type {
    std::string name;
    std::string action;
    std::string help;
    bool has_threshold;
    int threshold;
    bool operator==( const aim_type &other ) const {
        return has_threshold == other.has_threshold &&
               threshold == other.threshold &&
               name == other.name &&
               action == other.action &&
               help == other.help;
    }
};

struct parallax_cache {
    int parallax_with_zoom;
    int parallax_without_zoom;
};

struct aim_mods_cache {
    float aim_speed_skill_mod;
    float aim_speed_dex_mod;
    float aim_speed_mod;
    int limit;
    double aim_factor_from_volume;
    double aim_factor_from_length;
    parallax_cache parallaxes;
};

struct special_attack {
    std::string text;
    damage_instance damage;
};

struct consumption_event {
    time_point time;
    itype_id type_id;
    uint64_t component_hash;

    consumption_event() = default;
    explicit consumption_event( const item &food ) : time( calendar::turn ) {
        type_id = food.typeId();
        component_hash = food.make_component_hash();
    }
    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &jo );
};


struct stat_mod {
    int strength = 0;
    int dexterity = 0;
    int intelligence = 0;
    int perception = 0;

    int speed = 0;
};

inline social_modifiers operator+( social_modifiers lhs, const social_modifiers &rhs )
{
    lhs += rhs;
    return lhs;
}



enum class character_stat : char {
    STRENGTH,
    DEXTERITY,
    INTELLIGENCE,
    PERCEPTION,
    DUMMY_STAT
};

enum class customize_appearance_choice : int {
    EYES,
    HAIR,
    HAIR_F,
    SKIN
};

enum class book_mastery {
    CANT_DETERMINE,
    CANT_UNDERSTAND,
    LEARNING,
    MASTERED
};

enum class read_condition_result {
    SUCCESS = 0,
    NOT_BOOK = 1 << 0,
    CANT_UNDERSTAND = 1 << 1,
    MASTERED = 1 << 2,
    DRIVING = 1 << 3,
    ILLITERATE = 1 << 4,
    NEED_GLASSES = 1 << 5,
    TOO_DARK = 1 << 6,
    MORALE_LOW = 1 << 7,
    BLIND = 1 << 8
};

template<>
struct enum_traits<read_condition_result> {
    static constexpr bool is_flag_enum = true;
};


template<>
struct ret_val<edible_rating>::default_success : public
    std::integral_constant<edible_rating, EDIBLE> {};


template<>
struct ret_val<edible_rating>::default_failure : public
    std::integral_constant<edible_rating, INEDIBLE> {};


template<>
struct ret_val<crush_tool_type>::default_success : public
    std::integral_constant<crush_tool_type, CRUSH_EMPTY_HANDS> {};


template<>
struct ret_val<crush_tool_type>::default_failure : public
    std::integral_constant<crush_tool_type, CRUSH_NO_TOOL> {};

struct needs_rates {
    float thirst = 0.0f;
    float hunger = 0.0f;
    float sleepiness = 0.0f;
    float recovery = 0.0f;
    float kcal = 0.0f;
};

struct pocket_data_with_parent {
    const item_pocket *pocket_ptr;
    item_location parent;
    int nested_level;
};

struct speed_bonus_effect {
    std::string description;
    int bonus = 0;
};

struct run_cost_effect {
    std::string description;
    float times = 1.0;
    float plus = 0;
};

nutrients default_character_compute_effective_nutrients( const item &comest );



struct intrinsic_quality_source {
    enum class owner_kind : uint8_t { bionic, mutation, body_part, last };



    enum class slot_kind : uint8_t { pseudo, bionic_weapon, trait_item, itemless, last };

    owner_kind owner = owner_kind::last;
    bionic_uid bio_uid = 0;
    trait_id mut;
    bodypart_str_id bp;

    slot_kind slot = slot_kind::last;
    int slot_index = -1;
    int level = 0;
};

class Character : public Creature, public visitable
{
    public:
        Character( const Character & ) = delete;
        Character &operator=( const Character & ) = delete;
        ~Character() override;


        void initialize( bool learn_recipes = true );

        Character *as_character() override {
            return this;
        }
        const Character *as_character() const override {
            return this;
        }
        bool is_npc() const override {
            return false;
        }

        virtual void deserialize( const JsonObject &jsobj ) = 0;


        virtual void serialize( JsonOut &jsout ) const = 0;

        character_id getID() const;


        void setID( character_id i, bool force = false );

        field_type_id bloodType() const override;
        field_type_id gibType() const override;
        bool is_warm() const override;
        bool in_species( const species_id &spec ) const override;

        bool death_drops;

        bool controlling_vehicle = false;

    private:

        int str_max;
        int dex_max;
        int int_max;
        int per_max;




        int trauma_counter = 0;


        void clamp_trauma();

    public:

        int ppen_str;
        int ppen_dex;
        int ppen_int;
        int ppen_per;
        int ppen_spd;

        int get_trauma() const;
        void mod_trauma( int amt );
        void set_trauma( int amt );


        int kill_xp = 0;

        float cached_organic_size;

        character_portrait_id portrait_filename;

        virtual void ensure_portrait_valid();

        const profession *prof;
        std::set<const profession *> hobbies;
        void randomize_hobbies();
        void add_random_hobby( std::vector<profession_id> &choices );



        tripoint_rel_ms grab_point;

        std::optional<city> starting_city;
        std::optional<point_abs_om> world_origin;
        bool random_start_location = true;
        start_location_id start_location;

        bool manual_examine = false;
        int volume = 0;






        int get_str() const;
        int get_dex() const;
        int get_per() const;
        int get_int() const;

        int get_str_base() const;
        int get_dex_base() const;
        int get_per_base() const;
        int get_int_base() const;

        int get_str_bonus() const;
        int get_dex_bonus() const;
        int get_per_bonus() const;
        int get_int_bonus() const;


        int get_primary_stat_value( scaling_stat stat ) const;






        int arms_power_use;
        int legs_power_use;
        float arms_stam_mult;
        float legs_stam_mult;

        int get_arms_power_use() const;
        int get_legs_power_use() const;
        float get_arms_stam_mult() const;
        float get_legs_stam_mult() const;

    private:

        std::vector<speed_bonus_effect> speed_bonus_effects;


        int dodges_left;

        int free_dodges_left;

    public:
        std::vector<speed_bonus_effect> get_speed_bonus_effects() const;
        int get_speed() const override;
        int get_enchantment_speed_bonus() const;

        int get_arm_str() const;

        int get_eff_per() const override;

        int count_threshold_substitute_traits() const;


        int ranged_dex_mod() const;
        int ranged_per_mod() const;


        void set_str_bonus( int nstr );
        void set_dex_bonus( int ndex );
        void set_per_bonus( int nper );
        void set_int_bonus( int nint );
        void mod_str_bonus( int nstr );
        void mod_dex_bonus( int ndex );
        void mod_per_bonus( int nper );
        void mod_int_bonus( int nint );

        void set_str_base( int nstr );
        void set_dex_base( int ndex );
        void set_per_base( int nper );
        void set_int_base( int nint );


        using Creature::mod_speed_bonus;
        void mod_speed_bonus( int nspeed, const std::string &desc );


        int get_lifestyle() const;
        int get_daily_health() const;
        int get_health_tally() const;


        void mod_livestyle( int nhealthy );
        void mod_daily_health( int nhealthy_mod, int cap );
        void mod_health_tally( int mod );


        void set_lifestyle( int nhealthy );
        void set_daily_health( int nhealthy_mod );


        int get_stored_kcal() const;
        int get_healthy_kcal() const;

        float get_kcal_percent() const;
        int kcal_speed_penalty() const;
        int get_hunger() const;
        int get_starvation() const;
        virtual int get_thirst() const;
        virtual int get_instant_thirst() const;

        time_duration get_daily_sleep() const;
        void mod_daily_sleep( time_duration mod );
        void reset_daily_sleep();
        time_duration get_continuous_sleep() const;
        void mod_continuous_sleep( time_duration mod );
        void reset_continuous_sleep();

        int get_sleepiness() const;
        int get_sleep_deprivation() const;


        void mod_stored_kcal( int nkcal, bool ignore_weariness = false );
        void mod_hunger( int nhunger );
        void mod_thirst( int nthirst );
        void mod_sleepiness( int nsleepiness );
        void mod_sleep_deprivation( int nsleep_deprivation );


        void set_stored_kcal( int kcal );
        void set_hunger( int nhunger );
        void set_thirst( int nthirst );
        void set_sleepiness( int nsleepiness );
        void set_sleepiness( sleepiness_levels nsleepiness );
        void set_sleep_deprivation( int nsleep_deprivation );

    protected:


        void mod_stored_calories( int ncal, bool ignore_weariness = false );
        void set_stored_calories( int cal );

    public:

        void gravity_check() override;
        void gravity_check( map *here ) override;
        void stagger();

        void mod_stat( const std::string &stat, float modifier ) override;

        int get_standard_stamina_cost( const item *thrown_item = nullptr ) const;


        int get_fat_to_hp() const;


        creature_size get_size() const override;

        void recalculate_size();



        std::string disp_name( bool possessive = false, bool capitalize_first = false ) const override;



        std::string disp_profession() const;
        virtual std::string name_and_maybe_activity() const;

        std::string skin_name() const override;


        using Creature::add_msg_if_player;
        void add_msg_if_player( const std::string &msg ) const override;
        void add_msg_if_player( const game_message_params &params, const std::string &msg ) const override;
        using Creature::add_msg_player_or_npc;
        void add_msg_player_or_npc( const std::string &player_msg,
                                    const std::string &npc_str ) const override;
        void add_msg_player_or_npc( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        using Creature::add_msg_player_or_say;
        void add_msg_player_or_say( const std::string &player_msg,
                                    const std::string &npc_speech ) const override;
        void add_msg_player_or_say( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_speech ) const override;


        virtual faction *get_faction() const {
            return nullptr;
        }

        faction_id get_faction_id() const;
        void set_fac_id( const std::string &my_fac_id );
        virtual bool is_ally( const Character &p ) const = 0;
        virtual bool is_obeying( const Character &p ) const = 0;


        const profession *get_profession() const;
        void add_profession_items();

        std::set<const profession *> get_hobbies() const;


        bool has_mission_item( int mission_id ) const;


        int effective_dispersion( int dispersion, bool zoom = false ) const;

        dispersion_sources total_gun_dispersion( const item &gun, double recoil, int spread ) const;

        int get_character_parallax( bool zoom = false ) const;


        std::vector<aim_type> get_aim_types( const item &gun ) const;
        int point_shooting_limit( const item &gun ) const;
        double fastest_aiming_method_speed( const item &gun, double recoil,
                                            const Target_attributes &target_attributes,
                                            const parallax_cache &parallaxes ) const;
        int most_accurate_aiming_method_limit( const item &gun ) const;
        double aim_factor_from_volume( const item &gun ) const;
        double aim_factor_from_length( const item &gun ) const;
        aim_mods_cache gen_aim_mods_cache( const item &gun )const;



        float get_modifier( const character_modifier_id &mod,
                            const skill_id &skill = skill_id::NULL_ID() ) const;





        bool has_gun_for_ammo( const ammotype &at ) const;
        bool has_magazine_for_ammo( const ammotype &at ) const;




        double aim_per_move( const item &gun, double recoil,
                             const Target_attributes &target_attributes,
                             const aim_mods_cache &aim_cache ) const;

        int get_dodges_left() const;
        void set_dodges_left( int dodges );
        void mod_dodges_left( int mod );



        void mod_free_dodges( int added );

        int get_free_dodges_left() const;
        void set_free_dodges_left( int dodges );
        void mod_free_dodges_left( int mod );
        void consume_dodge_attempts();
        ret_val<void> can_try_dodge( bool ignore_dodges_left = false ) const;

        float get_stamina_dodge_modifier() const;


        void on_dodge( Creature *source, float difficulty, float training_level = 0.0f ) override;

        void on_try_dodge() override;


        float get_dodge_base() const override;

        float dodge_roll() const override;

        float get_dodge() const override;

        int get_spell_resist() const override;

        bool uncanny_dodge() override;
        bool check_avoid_friendly_fire() const override;
        float get_hit_base() const override;


        void tally_organic_size();
        float get_cached_organic_size() const;

        void recalc_limb_energy_usage();


        int sight_range( float light_level ) const override;

        int  unimpaired_range() const;

        bool overmap_los( const tripoint_abs_omt &omt, int sight_points ) const;

        int  overmap_sight_range( float light_level ) const;

        int overmap_modified_sight_range( float light_level ) const;

        int  clairvoyance() const;

        bool sight_impaired() const;

        bool has_alarm_clock() const;

        bool has_watch() const;

        void action_taken();

        bool is_on_ground() const override;

        int  swim_speed() const;

        float get_melee() const override;








        void add_miss_reason( const std::string &reason, unsigned int weight );

        void clear_miss_reasons();



        std::string get_miss_reason();




        void regen( int rate_multiplier );




        ret_val<void> can_takeoff( const item &it, const std::list<item> *res = nullptr );


        std::pair<int, int> gunmod_installation_odds( const item_location &gun, const item &mod ) const;

        void recalc_speed_bonus();
        void set_underwater( bool );
        bool is_hallucination() const override;


        bool is_electrical() const override;

        bool is_fae() const override;

        bool is_nether() const override;

        bool has_mind() const override;

        static int thirst_speed_penalty( int thirst );

        stat_mod get_weight_penalty() const;

        stat_mod get_pain_penalty() const;
        stat_mod read_pain_penalty() const;

        int get_lift_str() const;



        bool takeoff( item_location loc, std::list<item> *res = nullptr );
        bool takeoff( int pos );


        virtual void update_health();

        void update_body();

        void update_body( const time_point &from, const time_point &to );

        void update_stomach( const time_point &from, const time_point &to );

        void update_cached_mutations();

        void apply_wound( bodypart_id bp, wound_type_id wd );

        void update_wounds( time_duration time_passed );

        bool needs_food() const;

        void update_needs( int rate_multiplier );
        needs_rates calc_needs_rates() const;
        void calc_sleep_recovery_rate( needs_rates &rates ) const;

        void check_needs_extremes();

        void get_sick( bool is_flu = false );

        bool is_hibernating() const;

        void update_bodytemp();
        void update_frostbite( const bodypart_id &bp, int FBwindPower,
                               const std::map<bodypart_id, int> &warmth_per_bp );

        void temp_equalizer( const bodypart_id &bp1, const bodypart_id &bp2 );


        int focus_equilibrium_sleepiness_cap( int equilibrium ) const;

        int calc_focus_equilibrium( bool ignore_pain = false ) const;

        int calc_focus_change() const;

        void update_mental_focus();


        void reset_bonuses() override;

        void reset_stats() override;

        void reset() override;


        int encumb( const bodypart_id &bp ) const;
        int avg_encumb_of_limb_type( bp_type part_type ) const;


        units::mass get_weight() const override;

        units::mass bodyweight_with_bionic() const;


        void print_encumbrance( ui_adaptor &ui, const catacurses::window &win, int line = -1,
                                const item *selected_clothing = nullptr ) const;

        bool is_wearing_power_armor( bool *hasHelmet = nullptr ) const;

        bool is_wearing_active_power_armor() const;

        bool is_wearing_active_optcloak() const;


        std::pair<int, int> climate_control_strength() const;


        std::map<bodypart_id, int> get_wind_resistance() const;


        bool is_blind() const;

        bool is_invisible() const;

        int visibility( bool check_color = false, int stillness = 0 ) const;


        float active_light() const;






        bool sees_with_specials( const Creature &critter ) const;


        body_part_set exclusive_flag_coverage( const flag_id &flag ) const;



        bool move_effects( bool attacking ) override;

        void wait_effects( bool attacking = false );


        bool try_remove_grab( bool attacking = false );
        void try_remove_downed();
        void try_remove_bear_trap();
        void try_remove_lightsnare();
        void try_remove_heavysnare();
        void try_remove_crushed();
        void try_remove_webs();
        void try_remove_impeding_effect();

        bool can_escape_trap( int difficulty, bool manip ) const;


        bool movement_mode_is( const move_mode_id &mode ) const;
        move_mode_id current_movement_mode() const;

        bool is_running() const;
        bool is_walking() const;
        bool is_crouching() const;
        bool is_prone() const;

        int footstep_sound() const;

        int clatter_sound() const;
        void make_footstep_noise() const;
        void make_clatter_sound() const;

        bool can_switch_to( const move_mode_id &mode ) const;
        int move_mode_switch_cost( const move_mode_id &old_mode, const move_mode_id &new_mode ) const;
        steed_type get_steed_type() const;
        virtual void set_movement_mode( const move_mode_id &mode ) = 0;


















        int get_instability_per_category( const mutation_category_id &categ ) const;


        void expose_to_disease( const diseasetype_id &dis_type );



        void process_turn() override;


        void recalc_hp();


        void update_body_wetness( const w_point &weather );










        void recalc_sight_limits();





        float get_vision_threshold( float light_level ) const;



        void flag_encumbrance();




        void check_item_encumbrance_flag();


        bool natural_attack_restricted_on( const bodypart_id &bp ) const;
        bool natural_attack_restricted_on( const sub_bodypart_id &bp ) const;

        int blocks_left;

        double recoil = MAX_RECOIL;


        bool is_quiet() const;



        bool block_hit( Creature *source, bodypart_id &bp_hit, damage_instance &dam ) override;

        item_location best_shield();

        bool handle_melee_wear( item_location shield, float wear_multiplier = 1.0f );

        std::tuple<matec_id, attack_vector_id, sub_bodypart_str_id> pick_technique(
            Creature const &t, const item_location &weap,
            bool crit, bool dodge_counter, bool block_counter, const std::vector<matec_id> &blacklist = {} )
        const;

        std::optional<std::tuple<matec_id, attack_vector_id, sub_bodypart_str_id>>
                evaluate_technique( const matec_id &tec_id, Creature const &t, const item_location &weap,
                                    bool crit = false, bool dodge_counter = false, bool block_counter = false ) const;
        void perform_technique( const ma_technique &technique, Creature &t, damage_instance &di,
                                int &move_cost, item_location &cur_weapon );









        bool melee_attack( Creature &t, bool allow_special, const matec_id &force_technique,
                           bool allow_unarmed = true, int forced_movecost = -1 );
        bool melee_attack_abstract( Creature &t, bool allow_special, const matec_id &force_technique,
                                    bool allow_unarmed = true, int forced_movecost = -1 );
        void reduce_moves_from_attack( int forced_movecost, int move_cost );

        bool can_reach_attack( const Creature &target ) const;
        void reach_attack( const tripoint_bub_ms &p, int forced_movecost = -1 );





        bool melee_attack( Creature &t, bool allow_special );

        std::string melee_special_effects( Creature &t, damage_instance &d, item &weap );

        void perform_special_attacks( Creature &t, dealt_damage_instance &dealt_dam );
        bool reach_attacking = false;


        std::vector<special_attack> mutation_attacks( Creature &t ) const;

        float bonus_damage( bool random ) const;

        float get_melee_hit_base() const;

        float hit_roll() const override;

        double crit_chance( float roll_hit, float target_dodge, const item &weap ) const;

        bool scored_crit( float target_dodge, const item &weap ) const;

        int attack_speed( const item &weap ) const;

        int get_base_melee_stamina_cost( const item *weap = nullptr ) const;

        int get_total_melee_stamina_cost( const item *weap = nullptr ) const;

        float get_hit_weapon( const item &weap ) const;

        bool can_attack_high() const override;


        double evaluate_weapon( const item &maybe_weapon, bool pretend_have_ammo = false ) const;
        double gun_value( const item &weap, int ammo = 10 ) const;
        double melee_value( const item &weap ) const;
        double unarmed_value() const;




        dispersion_sources get_weapon_dispersion( const item &obj ) const;



        void roll_all_damage( bool crit, damage_instance &di, bool average, const item &weap,
                              const attack_vector_id &attack_vector, const sub_bodypart_str_id &contact,
                              const Creature *target, const bodypart_id &bp ) const;
        void roll_damage( const damage_type_id &dt, bool crit, damage_instance &di, bool average,
                          const item &weap, const attack_vector_id &attack_vector, const sub_bodypart_str_id &contact,
                          float crit_mod ) const;


        bool is_dead_state() const override;

    private:
        double evaluate_weapon_internal( const item &maybe_weapon, bool can_use_gun,
                                         bool use_silent, int pretend_ammo = 0 ) const;
        mutable std::optional<bool> cached_dead_state;
    public:
        void set_part_hp_cur( const bodypart_id &id, int set ) override;
        void mod_part_hp_cur( const bodypart_id &id, int set ) override;

        void calc_all_parts_hp( float hp_mod = 0.0, float hp_adjust = 0.0, int str_max = 0,
                                int dex_max = 0, int per_max = 0, int int_max = 0, int healthy_mod = 0,
                                int fat_to_max_hp = 0 );


        bool is_stealthy() const;

        bool can_melee() const;

        float stability_roll() const override;

        bool can_autolearn( const matype_id &ma_id ) const;
    private:

        bool valid_aoe_technique( Creature const &t, const ma_technique &technique ) const;
        bool valid_aoe_technique( Creature const &t, const ma_technique &technique,
                                  std::vector<Creature *> &targets ) const;
    public:


        bool practice( const skill_id &id, int amount, int cap = 99, bool suppress_warning = false,
                       bool allow_multilevel = false );

        void handle_skill_warning( const skill_id &id, bool force_warning = false );





        ret_val<void> can_wield( const item &it ) const;

        bool unwield();


        std::string weapname( bool color_faults = false ) const;

        std::string weapname_simple( bool color_faults = false ) const;

        std::string weapname_mode() const;

        std::string weapname_ammo() const;



        void on_hit( map *here, Creature *source, bodypart_id bp_hit,
                     float difficulty = INT_MIN, dealt_projectile_attack const *proj = nullptr ) override;

        void did_hit( Creature &target );


        void apply_damage( Creature *source, bodypart_id hurt, int dam,
                           bool bypass_med = false ) override;


        bool is_within_wound_limit_for_bp( bodypart_id bp, wound_type_id wound_id ) const;
        void apply_random_wound( bodypart_id bp, const damage_instance &d );

        dealt_damage_instance deal_damage( Creature *source, bodypart_id bp,
                                           const damage_instance &d,
                                           const weakpoint_attack &attack = weakpoint_attack(),
                                           const weakpoint &wp = weakpoint() ) override;

        int reduce_healing_effect( const efftype_id &eff_id, int remove_med, const bodypart_id &hurt );

        void cough( bool harmful = false, int loudness = 4 );






        void passive_absorb_hit( const bodypart_id &bp, damage_unit &du ) const;

        const weakpoint *absorb_hit( const weakpoint_attack &attack, const bodypart_id &bp,
                                     damage_instance &dam, const weakpoint &wp = weakpoint() ) override;
    protected:
        float generic_weakpoint_skill( skill_id skill_1, skill_id skill_2,
                                       limb_score_id limb_score_1, limb_score_id limb_score_2 ) const;
    public:

        float melee_weakpoint_skill( const item &weapon ) const;
        float ranged_weakpoint_skill( const item &weapon ) const;
        float throw_weakpoint_skill() const;

        void armor_use_power_when_hit( damage_unit &du, item &armor ) const;





        bool armor_absorb( damage_unit &du, item &armor, const bodypart_id &bp, int roll ) const;





        bool armor_absorb( damage_unit &du, item &armor, const bodypart_id &bp, const sub_bodypart_id &sbp,
                           int roll ) const;





        bool ablative_armor_absorb( damage_unit &du, item &armor, const sub_bodypart_id &bp, int roll );

        void describe_damage( damage_unit &du, item &armor ) const;




        float bionic_armor_bonus( const bodypart_id &bp, const damage_type_id &dt ) const;

        int mabuff_armor_bonus( const damage_type_id &type ) const;



        trait_id random_good_trait();

        trait_id random_bad_trait();

        trait_id get_random_trait( const std::function<bool( const mutation_branch & )> &func );
        void randomize_cosmetic_trait( const std::string &mutation_type );



        void starting_inv_damage_worn( int days );





        bool has_conflicting_trait( const trait_id &flag ) const;

        std::unordered_set<trait_id> get_conflicting_traits( const trait_id &flag ) const;

        bool has_lower_trait( const trait_id &flag ) const;

        std::unordered_set<trait_id> get_lower_traits( const trait_id &flag ) const;

        bool has_replacement_trait( const trait_id &flag ) const;

        std::unordered_set<trait_id> get_replacement_traits( const trait_id &flag ) const;

        bool has_addition_trait( const trait_id &flag ) const;

        std::unordered_set<trait_id> get_addition_traits( const trait_id &flag ) const;

        bool has_same_type_trait( const trait_id &flag ) const;

        std::unordered_set<trait_id> get_same_type_traits( const trait_id &flag ) const;



        bool purifiable( const trait_id &flag ) const;

        bool handle_bio_mut_conflict( const bionic_id &bio, const trait_id &mut );

        std::string get_category_dream( const mutation_category_id &cat, int strength ) const;

        bool has_trait( const trait_id &b ) const override;

        bool has_trait_variant( const trait_and_var & ) const;

        bool has_permanent_trait( const trait_id &b ) const;

        bool has_base_trait( const trait_id &b ) const;

        bool has_trait_flag( const json_character_flag &b ) const;

        bool has_bionic_with_flag( const json_character_flag &flag ) const;

        bool has_bodypart_with_flag( const json_character_flag &flag ) const;

        int count_trait_flag( const json_character_flag &b ) const;

        int count_bionic_with_flag( const json_character_flag &flag ) const;

        int count_bodypart_with_flag( const json_character_flag &flag ) const;

        using Creature::has_flag;

        bool has_flag( const json_character_flag &flag ) const override;

        int count_flag( const json_character_flag &flag ) const;

        bool empathizes_with_species( const species_id &species ) const;

        bool empathizes_with_monster( const mtype_id &monster ) const;

        bool has_unfulfilled_pyromania() const;

        void fulfill_pyromania( int bonus = 15, int max_bonus = 15,
                                const time_duration &duration = 8_hours, const time_duration &decay_start = 6_hours );

        void fulfill_pyromania_msg( const std::string &message, int bonus = 15, int max_bonus = 15,
                                    const time_duration &duration = 8_hours, const time_duration &decay_start = 6_hours );

        void fulfill_pyromania_msg_std( const std::string &target_name = "",
                                        int bonus = 15, int max_bonus = 15,
                                        const time_duration &duration = 8_hours, const time_duration &decay_start = 6_hours );

        bool fulfill_pyromania_sees( const map &here, const Creature &target,
                                     int bonus = 15, int max_bonus = 15,
                                     const time_duration &duration = 8_hours, const time_duration &decay_start = 6_hours );

        bool fulfill_pyromania_sees( const map &here, const tripoint_bub_ms &target,
                                     const std::string &target_str = "", bool std_msg = true, int bonus = 15, int max_bonus = 15,
                                     const time_duration &duration = 8_hours, const time_duration &decay_start = 6_hours );


        bool schizo_symptoms( int chance = 1 ) const;

    private:

        mutable std::map<const json_character_flag, bool> trait_flag_cache;

    public:

        trait_id trait_by_invlet( int ch ) const;

        std::vector<trait_id> get_in_category( const mutation_category_id &categ,
                                               mut_count_type count_type ) const;

        void toggle_trait( const trait_id &, const std::string & = "" );

        void set_mutations( const std::vector<trait_id> &traits );
        void set_mutation( const trait_id &, const mutation_variant * = nullptr );

        void set_mut_variant( const trait_id &, const mutation_variant * );
    protected:


        void set_mutation_unsafe( const trait_id &, const mutation_variant * = nullptr );
    public:

        void do_mutation_updates();
        void unset_mutation( const trait_id & );

        void switch_mutations( const trait_id &switched, const trait_id &target, bool start_powered,
                               bool safe = false );

        bool can_power_mutation( const trait_id &mut ) const;

        virtual void power_bionics() {}




        bool can_interface_armor() const;

        virtual void power_mutations() {}


        void mutation_reflex_trigger( const trait_id &mut );


        void activate_mutation( const trait_id &mut );
        void activate_cached_mutation( const trait_id &mut );
        void deactivate_mutation( const trait_id &mut );

        bool can_mount( const monster &critter ) const;
        void mount_creature( monster &z );
        bool cant_do_mounted( bool msg = true ) const;
        bool is_mounted() const;
        bool check_mount_will_move( const tripoint_bub_ms &dest_loc );
        bool check_mount_is_spooked();
        void dismount();
        void forced_dismount();

        bool is_deaf() const;
        bool is_mute() const;


        float get_limb_score( const limb_score_id &score,
                              const bp_type &bp = bp_type::num_types,
                              int override_encumb = -1, int override_wounds = -1 ) const;
        float manipulator_score( const std::map<bodypart_str_id, bodypart> &body,
                                 bp_type type, int override_encumb, int override_wounds ) const;

        bool has_min_manipulators() const;

        bool has_two_arms_lifting() const;

        std::set<matec_id> get_limb_techs() const;
        int get_working_arm_count() const;



        bool enough_working_legs() const;

        int get_working_leg_count() const;

        bool is_limb_broken( const bodypart_id &limb ) const;

        bool can_run() const;

        void hurtall( int dam, Creature *source, bool disturb = true );

        int hitall( int dam, int vary, Creature *source );

        void on_hurt( Creature *source, bool disturb = true );

        void heal_bp( bodypart_id bp, int dam ) override;

        void heal( const bodypart_id &healed, int dam );

        void healall( int dam );




        std::vector<skill_id> skills_offered_to( const Character *you ) const;



        std::vector<proficiency_id> proficiencies_offered_to( const Character *guy ) const;



        std::vector<matype_id> styles_offered_to( const Character *you ) const;




        std::vector<spell_id> spells_offered_to( const Character *you ) const;

        int calc_spell_training_cost( bool knows, int difficulty, int level ) const;
















        bodypart_id body_window( const std::string &menu_header,
                                 bool show_all, bool precise,
                                 int normal_bonus, int head_bonus, int torso_bonus,
                                 int bleed, float bite, float infect, float bandage_power, float disinfectant_power ) const;

        static const std::vector<material_id> fleshy;
        bool made_of( const material_id &m ) const override;
        bool made_of_any( const std::set<material_id> &ms ) const override;

    protected:

        void on_move( const tripoint_abs_ms &old_pos ) override;
        void do_skill_rust();


        void mut_cbm_encumb( std::map<bodypart_id, encumbrance_data> &vals ) const;

        void apply_mut_encumbrance( std::map<bodypart_id, encumbrance_data> &vals ) const;


        void calc_bmi_encumb( std::map<bodypart_id, encumbrance_data> &vals ) const;
        void calc_bmi_encumb() const;

    public:

        void calc_encumbrance();

        void calc_discomfort();

        void apply_murder_penalties( Creature *victim );

        void calc_encumbrance( const item &new_item );

        void recalculate_bodyparts();

        void recalculate_enchantment_cache();



        bool has_mabuff( const mabuff_id &buff_id ) const;

        bool has_grab_break_tec() const override;


        float mabuff_tohit_bonus() const;

        float mabuff_critical_hit_chance_bonus() const;

        float mabuff_dodge_bonus() const;

        int mabuff_block_effectiveness_bonus() const;

        int mabuff_block_bonus() const;

        int mabuff_speed_bonus() const;

        int mabuff_arpen_bonus( const damage_type_id &type ) const;

        float mabuff_damage_mult( const damage_type_id &type ) const;

        int mabuff_damage_bonus( const damage_type_id &type ) const;

        int mabuff_attack_cost_penalty() const;

        float mabuff_attack_cost_mult() const;

        bool has_mabuff_flag( const json_character_flag &flag ) const;

        int count_mabuff_flag( const json_character_flag &flag ) const;


        void mutation_effect( const trait_id &mut, bool worn_destroyed_override );

        void mutation_loss_effect( const trait_id &mut );

        bool has_active_mutation( const trait_id &b ) const;

        time_duration get_cost_timer( const trait_id &mut_id ) const;
        void set_cost_timer( const trait_id &mut, time_duration set );
        void mod_cost_timer( const trait_id &mut, time_duration mod );


        void mutate( const int &true_random_chance, bool use_vitamins );
        void mutate( );

        bool mutation_ok( const trait_id &mutation, bool allow_good, bool allow_bad, bool allow_neutral,
                          const vitamin_id &mut_vit ) const;
        bool mutation_ok( const trait_id &mutation, bool allow_good, bool allow_bad,
                          bool allow_neutral ) const;

        int get_total_in_category( const mutation_category_id &categ, mut_count_type count_type ) const;

        int get_total_in_category_char_has( const mutation_category_id &categ,
                                            mut_count_type count_type ) const;

        bool roll_bad_mutation( const mutation_category_id &categ ) const;

        bool mutation_selector( const std::vector<trait_id> &prospective_traits,
                                const mutation_category_id &cat, const bool &use_vitamins );

        void mutate_category( const mutation_category_id &mut_cat, bool use_vitamins,
                              bool true_random = false );
        void mutate_category( const mutation_category_id &mut_cat );

        bool mutate_towards( std::vector<trait_id> muts, const mutation_category_id &mut_cat,
                             int num_tries = INT_MAX, bool use_vitamins = true, bool removed_base_trait = false );

        bool mutate_towards( const trait_id &mut, const mutation_category_id &mut_cat,
                             const mutation_variant *chosen_var = nullptr, bool use_vitamins = true,
                             bool removed_base_trait = false );
        bool mutate_towards( const trait_id &mut, const mutation_variant *chosen_var = nullptr );

        void remove_mutation( const trait_id &mut, bool silent = false );

        bool has_child_flag( const trait_id &flag ) const;

        void remove_child_flag( const trait_id &flag );

        void try_crossing_threshold( const mutation_category_id &mutation_category );

        void cross_threshold( const mutation_category_id &mutation_category );

        bool can_cross_threshold( const mutation_category_id &mutation_category );

        int mutation_height( const trait_id &mut ) const;

        void calc_mutation_levels();

        weighted_int_list<mutation_category_id> get_vitamin_weighted_categories() const;

        void drench_mut_calc();




        bool is_category_allowed( const std::vector<mutation_category_id> &category ) const;
        bool is_category_allowed( const mutation_category_id &category ) const;


        bool can_use_heal_item( const item &med ) const;

        bool can_install_cbm_on_bp( const std::vector<bodypart_id> &bps ) const;



        resistances mutation_armor( bodypart_id bp ) const;
        float mutation_armor( bodypart_id bp, const damage_type_id &dt ) const;
        float mutation_armor( bodypart_id bp, const damage_unit &du ) const;


        void give_all_mutations( const mutation_category_trait &category, bool include_postthresh = false );

        void unset_all_mutations();



        bool activate_bionic( bionic &bio, bool eff_only = false, bool *close_bionics_ui = nullptr );
        std::vector<bionic_id> get_bionics() const;
        std::vector<const item *> get_pseudo_items() const;


        std::vector<item> crafting_pseudo_items() const;
        void invalidate_pseudo_items();

        void update_last_bionic_uid() const;

        bionic_uid generate_bionic_uid() const;

        bool has_bionic( const bionic_id &b ) const;

        bool has_active_bionic( const bionic_id &b ) const;

        bool has_any_bionic() const;

        std::vector<bionic_id> get_bionic_fueled_with_muscle() const;

        std::vector<bionic_id> get_fueled_bionics() const;

        bionic_id get_remote_fueled_bionic() const;


        std::vector<item *> get_bionic_fuels( const bionic_id &bio );

        std::vector<item *> get_cable_ups();

        std::vector<item *> get_cable_solar();

        std::vector<vehicle *> get_cable_vehicle() const;


        std::vector<tripoint_abs_omt> omt_path;

        std::unordered_set<tripoint_abs_omt> map_revealed_omts;
        bool is_using_bionic_weapon() const;
        bionic_uid get_weapon_bionic_uid() const;


        void process_bionic( bionic &bio );


        ret_val<void> can_deactivate_bionic( bionic &bio, bool eff_only = false ) const;


        bool deactivate_bionic( bionic &bio, bool eff_only = false );

        void force_bionic_deactivation( bionic &bio );

        int num_bionics() const;

        bionic &bionic_at_index( int i );

        void clear_bionics();
        int get_used_bionics_slots( const bodypart_id &bp ) const;
        int get_total_bionics_slots( const bodypart_id &bp ) const;
        int get_free_bionics_slots( const bodypart_id &bp ) const;


        bool has_enough_anesth( const itype &cbm, Character &patient ) const;
        bool has_enough_anesth( const itype &cbm ) const;
        void consume_anesth_requirement( const itype &cbm, Character &patient );

        bool has_installation_requirement( const bionic_id &bid ) const;
        void consume_installation_requirement( const bionic_id &bid );

        void introduce_into_anesthesia( const time_duration &duration, Character &installer,
                                        bool needs_anesthesia );

        std::optional<bionic *> find_bionic_by_type( const bionic_id &b ) const;

        std::optional<bionic *> find_bionic_by_uid( bionic_uid bio_uid ) const;

        void remove_bionic( const bionic &bio );

        bionic_uid add_bionic( const bionic_id &b, bionic_uid parent_uid = 0, bool suppress_debug = false );

        float env_surgery_bonus( int radius ) const;

        float bionics_adjusted_skill( bool autodoc, int skill_level = -1 ) const;

        int bionics_pl_skill( bool autodoc, int skill_level = -1 ) const;

        bool can_install_bionics( const itype &type, Character &installer, bool autodoc = false,
                                  int skill_level = -1 ) const;

        ret_val<void> is_installable( const item *it, bool by_autodoc ) const;
        std::map<bodypart_id, int> bionic_installation_issues( const bionic_id &bioid ) const;

        bool install_bionics( const itype &type, Character &installer, bool autodoc = false,
                              int skill_level = -1 );

        void perform_install( const bionic_id &bid, bionic_uid upbio_uid, int difficulty, int success,
                              int pl_skill, const std::string &installer_name,
                              const std::vector<trait_id> &trait_to_rem, const tripoint_bub_ms &patient_pos );
        void bionics_install_failure( const bionic_id &bid, const std::string &installer, int difficulty,
                                      int success, float adjusted_skill, const tripoint_bub_ms &patient_pos );







        bool wield_contents( item &container, item *internal_item = nullptr, bool penalties = true,
                             int base_cost = INVENTORY_HANDLING_PENALTY );

        void use( int inventory_position );

        void use( item_location loc, int pre_obtain_moves = -1, std::string const &method = {} );

        void use_wielded();

        std::optional<std::list<item>::iterator>
        wear( int pos, bool interactive = true );





        std::optional<std::list<item>::iterator>
        wear( item_location item_wear, bool interactive = true );




        trinary consume( item_location loc, bool force = false );




        trinary consume( item &target, bool force = false );









        void store( item &container, item &put, bool penalties = true,
                    int base_cost = INVENTORY_HANDLING_PENALTY,
                    pocket_type pk_type = pocket_type::CONTAINER,
                    bool check_best_pkt = false );
        void store( item_pocket *pocket, item &put, bool penalties = true,
                    int base_cost = INVENTORY_HANDLING_PENALTY );

        bool can_uninstall_bionic( const bionic &bio, Character &installer, bool autodoc = false,
                                   int skill_level = -1 ) const;

        bool uninstall_bionic( const bionic &bio, Character &installer, bool autodoc = false,
                               int skill_level = -1 );

        void perform_uninstall( const bionic &bio, int difficulty, int success, int pl_skill );

        void bionics_uninstall_failure( int difficulty, int success, float adjusted_skill );


        void roll_critical_bionics_failure( const bodypart_id &bp );


        bool uninstall_bionic( const bionic &bio, monster &installer, Character &patient,
                               float adjusted_skill );

        void bionics_uninstall_failure( monster &installer, Character &patient, int difficulty, int success,
                                        float adjusted_skill );
        void on_worn_item_transform( const item &old_it, const item &new_it );





        bool gunmod_remove( item &gun, item &mod );


        void gunmod_add( item &gun, item &mod );


        void toolmod_add( item_location tool, item_location mod );






        void mend_item( item_location &&obj, bool interactive = true );












        bool list_ammo( const item_location &base, std::vector<item::reload_option> &ammo_list,
                        bool empty = true, bool per_well_targets = false ) const;






        virtual item::reload_option select_ammo( const item_location &base, bool prompt = false,
                bool empty = true ) = 0;

        void process_items( map *here );
        void leak_items();

        void search_surroundings();

        void heat_emission( const bionic &bio, units::energy fuel_energy );

        float get_effective_efficiency( const bionic &bio, float fuel_efficiency ) const;

        units::energy get_power_level() const;
        units::energy get_max_power_level() const;
        void mod_power_level( const units::energy &npower );
        void mod_max_power_level_modifier( const units::energy &npower_max );
        void set_power_level( const units::energy &npower );
        void set_max_power_level( const units::energy &capacity );
        void set_max_power_level_modifier( const units::energy &capacity );
        void update_bionic_power_capacity();
        bool is_max_power() const;
        bool has_power() const;
        bool has_max_power() const;
        bool enough_power_for( const bionic_id &bid ) const;

        void disp_info( bool customize_character = false );

        void disp_morale();

        bool disp_medical();

        bool pick_wound_fix( const bodypart_id &bp_id );
        void conduct_blood_analysis();


        struct has_mission_item_filter {
            int mission_id;
            bool operator()( const item &it ) const {
                return it.mission_id == mission_id || it.has_any_with( [&]( const item & it ) {
                    return it.mission_id == mission_id;
                }, pocket_type::E_FILE_STORAGE );
            }
        };


        static int worn_position_to_index( int position ) {
            return -2 - position;
        }


        bool is_worn( const item &thing ) const {
            return worn.is_worn( thing );
        }

        bool is_worn_module( const item &thing ) const {
            return worn.is_worn_module( thing );
        }

        bool has_worn_module_with_flag( const flag_id &f );





        virtual bool invoke_item( item *, const tripoint_bub_ms &pt, int pre_obtain_moves = -1 );

        virtual bool invoke_item( item *, const std::string &, const tripoint_bub_ms &pt,
                                  int pre_obtain_moves = -1 );

        virtual bool invoke_item( item * );
        virtual bool invoke_item( item *, const std::string & );







        virtual bool dispose_item( item_location &&obj, const std::string &prompt = std::string() );





        bool consume_charges( item &used, int qty );









        item reduce_charges( item *it, int quantity );










        int item_handling_cost( const item &it, bool penalties = true,
                                int base_cost = INVENTORY_HANDLING_PENALTY, int charges_in_it = -1, bool bulk_cost = false ) const;









        int item_store_cost( const item &it, const item &container, bool penalties = true,
                             int base_cost = INVENTORY_HANDLING_PENALTY ) const;









        int item_retrieve_cost( const item &it, const item &container, bool penalties = true,
                                int base_cost = INVENTORY_HANDLING_PENALTY ) const;


        int item_wear_cost( const item &it ) const;





        std::optional<std::list<item>::iterator>
        wear_item( const item &to_wear, bool interactive = true, bool do_calc_encumbrance = true,
                   bool do_sort_items = true, bool quiet = false );


        int  amount_worn( const itype_id &id ) const;


        int count_softwares( const itype_id &id ) const;


        bool has_software( const itype_id &software_id, int min_charges = 0,
                           const itype_id &device_id = itype_id::NULL_ID() ) const;


        std::vector<item_location> nearby( const std::function<bool( const item_location & )> &func,
                                           int radius = 1 ) const;






        std::list<item> remove_worn_items_with( const std::function<bool( const item & )> &filter );

        void clear_worn();



        std::vector<item_location> all_items_loc();

        std::vector<item_location> top_items_loc();



        item_location invlet_to_item( int invlet ) const;


        item &i_at( int position );
        const item &i_at( int position ) const;







        int get_item_position( const item *it ) const;






        item_location used_weapon() const;
        item_location used_weapon();







        item_location i_add( item it, bool should_stack = true, const item *avoid = nullptr,
                             const item *original_inventory_item = nullptr, bool allow_drop = true,
                             bool allow_wield = true, bool ignore_pkt_settings = false );
        item_location i_add( item it, int &copies_remaining, bool should_stack = true,
                             const item *avoid = nullptr,
                             const item *original_inventory_item = nullptr, bool allow_drop = true,
                             bool allow_wield = true, bool ignore_pkt_settings = false );

        item_location try_add( item it, const item *avoid = nullptr,
                               const item *original_inventory_item = nullptr, bool allow_wield = true,
                               bool ignore_pkt_settings = false );
        item_location try_add( item it, int &copies_remaining, const item *avoid = nullptr,
                               const item *original_inventory_item = nullptr, bool allow_wield = true,
                               bool ignore_pkt_settings = false );

        bool can_add( const item &it, const item *avoid = nullptr,
                      bool allow_wield = true,
                      bool ignore_pkt_settings = false );

        ret_val<item_location> i_add_or_fill( item &it, bool should_stack = true,
                                              const item *avoid = nullptr,
                                              const item *original_inventory_item = nullptr, bool allow_drop = true,
                                              bool allow_wield = true, bool ignore_pkt_settings = false );










        bool pour_into( item_location &container, item &liquid, bool ignore_settings, bool silent );
        bool pour_into( const vpart_reference &vp, item &liquid ) const;









        item i_rem( const item *it );
        void i_rem_keep_contents( const item *it );




        bool i_add_or_drop( item &it, int qty = 1, const item *avoid = nullptr,
                            const item *original_inventory_item = nullptr );




        bool i_drop_at( item &it, int qty = 1 );
















        void handle_contents_changed( const std::vector<item_location> &containers );




        bool has_active_item( const itype_id &id ) const;
        item remove_weapon();
        void remove_mission_items( int mission_id );




        std::vector<item_location> get_ammo( const ammotype &at ) const;










        std::vector<item_location> find_ammo( const item &obj, bool empty = true, int radius = 1,
                                              bool now = true ) const;




        std::vector<item_location> find_reloadables() const;



        int ammo_count_for( const item_location &gun ) const;








        bool can_reload( const item &it, const item *ammo = nullptr ) const;


        hint_rating rate_action_reload( const item &it ) const;

        hint_rating rate_action_unload( const item &it ) const;
        hint_rating rate_action_insert( const item_location &loc ) const;





        bool add_or_drop_with_msg( item &it, bool unloading = false, const item *avoid = nullptr,
                                   const item *original_inventory_item = nullptr );




        bool unload( item_location &loc, bool bypass_activity = false,
                     const item_location &new_container = item_location::nowhere );






        int item_reload_cost( const item &it, const item &ammo, int qty ) const;

        projectile thrown_item_projectile( const item &thrown ) const;
        int thrown_item_adjusted_damage( const item &thrown ) const;

        int thrown_item_total_damage_raw( const item &thrown ) const;

        int throw_range( const item & ) const;

        int throwing_dispersion( const item &to_throw, Creature *critter = nullptr,
                                 bool is_blind_throw = false ) const;

        int throw_dispersion_per_dodge( bool add_encumbrance = true ) const;


        bool unarmed_attack() const;



        units::mass best_nearby_lifting_assist() const;


        units::mass best_nearby_lifting_assist( const tripoint_bub_ms &world_pos ) const;


        std::vector<item *> inv_dump();
        std::vector<const item *> inv_dump() const;



        units::volume get_total_volume() const;

        units::volume get_base_volume() const;


        units::mass weight_carried() const;

        units::volume volume_carried() const;

        units::length max_single_item_length() const;
        units::volume max_single_item_volume() const;




        struct item_tweaks {
            item_tweaks() : without_items( nullptr ) {}
            explicit item_tweaks( const std::map<const item *, int> &w ) :
                without_items( &w )
            {}
            const std::map<const item *, int> *const without_items;
        };

        units::mass weight_carried_with_tweaks( const item_tweaks &tweaks ) const;
        units::mass weight_carried_with_tweaks( const std::vector<std::pair<item_location, int>>
                                                &locations ) const;
        units::mass weight_capacity() const override;


        units::mass max_pickup_capacity() const;


        units::volume volume_capacity( const std::function<bool( const item_pocket & )> &include_pocket =
                                           item_pocket::ok_default_containers ) const;

        units::volume volume_capacity_recursive( const std::function<bool( const item_pocket & )>
                &include_pocket,
                const std::function<bool( const item_pocket & )> &check_pocket_tree ) const;







        units::volume free_space( const std::function<bool( const item_pocket & )> &include_pocket = [](
        const item_pocket &pocket ) {
            return !pocket.is_restricted()
                   && item_pocket::ok_for_solids( pocket );
        },
        const std::function<bool( const item_pocket & )> &check_pocket_tree =
            item_pocket::ok_default_containers )
        const;
        units::mass free_weight_capacity() const;



        virtual bool has_identified( const itype_id &item_id ) const = 0;
        book_mastery get_book_mastery( const item &book ) const;
        virtual void identify( const item &item ) = 0;

        bool fun_to_read( const item &book ) const;
        int book_fun_for( const item &book, const Character &p ) const;

        std::map<itype_id, int> book_chapters;

        bool can_pickVolume( const item &it, bool safe = false, const item *avoid = nullptr,
                             bool ignore_pkt_settings = true ) const;
        bool can_pickVolume_partial( const item &it, bool safe = false, const item *avoid = nullptr,
                                     bool ignore_pkt_settings = true, bool ignore_non_container_pocket = false ) const;
        bool can_pickWeight( const item &it, bool safe = true ) const;
        bool can_pickWeight_partial( const item &it, bool safe = true ) const;









        std::pair<item_location, item_pocket *> best_pocket( const item &it, const item *avoid = nullptr,
                bool ignore_settings = false );






        using pocket_with_constraints = std::pair<const item_pocket *, pocket_constraint>;
        std::vector<pocket_with_constraints> get_all_pockets_with_constraints(
            const std::function<bool( const item_pocket & )> &include_pocket,
            const std::function<bool( const item_pocket & )> &check_pocket_tree
        );








        std::vector<pocket_data_with_parent> get_all_pocket_with_parent(
            const std::function<bool( const item_pocket * )> &filter = return_true<const item_pocket *>,
            const std::function<bool( const pocket_data_with_parent &a, const pocket_data_with_parent &b )>
            *sort_func = nullptr );







        bool can_use( const item &it, const item &context = item() ) const;





        ret_val<void> can_wear( const item &it, bool with_equip_change = false ) const;




        bool is_armed() const;





        bool has_wield_conflicts( const item &it ) const;








        bool wield( item_location loc, bool remove_old = true );









        bool wield( item &it, std::optional<int> obtain_cost = std::nullopt, bool combat = true );




        ret_val<void> can_unwield( const item &it ) const;




        ret_val<void> can_drop( const item &it ) const;


        void clear_inventory_search_cache();

        void drop_invalid_inventory();

        void invalidate_inventory_validity_cache();

        void invalidate_weight_carried_cache();

        std::list<item *> get_dependent_worn_items( const item &it );

        void drop( item_location loc, const tripoint_bub_ms &where );
        virtual void drop( const drop_locations &what, const tripoint_bub_ms &target, bool stash = false );



        void pick_up( const drop_locations &what );
        void pick_up( const drop_locations &what, Pickup::pick_info &info );

        bool is_wielding( const item &target ) const;

        bool covered_with_flag( const flag_id &flag, const body_part_set &parts ) const;
        bool is_waterproof( const body_part_set &parts ) const;



        bool is_wearing( const itype_id &it ) const;

        bool is_wearing_on_bp( const itype_id &it, const bodypart_id &bp ) const;

        bool worn_with_flag( const flag_id &flag, const bodypart_id &bp ) const;
        bool worn_with_flag( const flag_id &flag ) const;

        item item_worn_with_flag( const flag_id &flag, const bodypart_id &bp ) const;
        item item_worn_with_flag( const flag_id &flag ) const;

        item *item_worn_with_id( const itype_id &id );








        std::vector<std::pair<std::string, std::string>> get_overlay_ids() const;






        std::vector<std::pair<std::string, std::string>> get_overlay_ids_when_override_look() const;




        void zero_all_skills();
        void set_all_skills( int lvl );
        float get_skill_level( const skill_id &ident ) const;
        float get_skill_level( const skill_id &ident, const item &context ) const;
        int get_knowledge_level( const skill_id &ident ) const;
        float get_knowledge_plus_progress( const skill_id &ident ) const;
        int get_knowledge_level( const skill_id &ident, const item &context ) const;
        float get_average_skill_level( const skill_id &ident ) const;

        float get_greater_skill_or_knowledge_level( const skill_id &ident ) const;


        SkillLevelMap get_all_skills() const;
        SkillLevel &get_skill_level_object( const skill_id &ident );
        const SkillLevel &get_skill_level_object( const skill_id &ident ) const;

        void set_skill_level( const skill_id &ident, int level );
        void mod_skill_level( const skill_id &ident, int delta );
        void set_knowledge_level( const skill_id &ident, int level );
        void mod_knowledge_level( const skill_id &ident, int delta );

        bool meets_skill_requirements( const std::map<skill_id, int> &req,
                                       const item &context = item() ) const;

        bool meets_skill_requirements( const construction &con ) const;

        bool meets_stat_requirements( const item &it ) const;

        bool meets_requirements( const item &it, const item &context = item() ) const;

        std::string enumerate_unmet_requirements( const item &it, const item &context = item() ) const;



        int read_speed() const;

        int lie_skill() const;

        int persuade_skill() const;

        int intimidation() const;

        void set_skills_from_hobbies( bool no_override = false );

        void set_bionics_from_hobbies();


        bool has_proficiency( const proficiency_id &prof ) const;
        float get_proficiency_practice( const proficiency_id &prof ) const;
        time_duration get_proficiency_practiced_time( const proficiency_id &prof ) const;
        bool has_prof_prereqs( const proficiency_id &prof ) const;
        void add_proficiency( const proficiency_id &prof, bool ignore_requirements = false,
                              bool recursive = false );
        void lose_proficiency( const proficiency_id &prof, bool ignore_requirements = false );
        bool practice_proficiency( const proficiency_id &prof, const time_duration &amount,
                                   const std::optional<time_duration> &max = std::nullopt );
        time_duration proficiency_training_needed( const proficiency_id &prof ) const;
        void set_proficiency_practiced_time( const proficiency_id &prof, int turns );
        std::vector<display_proficiency> display_proficiencies() const;
        std::vector<proficiency_id> known_proficiencies() const;
        std::vector<proficiency_id> learning_proficiencies() const;
        int get_proficiency_bonus( const std::string &category, proficiency_bonus_type prof_bonus ) const;
        void add_default_background();
        void set_proficiencies_from_hobbies();
        void set_recipes_from_hobbies();


        void set_proficiency_practice( const proficiency_id &id, const time_duration &amount );




        time_point get_time_died() const {
            return time_died;
        }

        void set_time_died( const time_point &time ) {
            if( time_died != calendar::before_time_starts ) {
                time_died = time;
            }
        }

        pimpl<known_magic> magic;



        std::vector<spell> spells_known_of_class( const trait_id &spell_class ) const;




        bool cast_spell( spell &sp, bool fake_spell, const std::optional<tripoint_bub_ms> &target );


        bool avoid_trap( const tripoint_bub_ms &pos, const trap &tr ) const override;


        bool add_faction_warning( const faction_id &id ) const;
        int current_warnings_fac( const faction_id &id );
        bool beyond_final_warning( const faction_id &id );









        const Character *get_book_reader( const item &book, std::vector<std::string> &reasons ) const;









        time_duration time_to_read( const item &book, const Character &reader,
                                    const Character *learner = nullptr ) const;






        read_condition_result check_read_condition( const item &book ) const;





        void normalize() override;
        void die( map *here, Creature *nkiller ) override;
        virtual void prevent_death();

        std::string get_name() const override;

        std::vector<std::string> get_grammatical_genders() const override;




        template<typename ...Args>
        bool query_yn( const char *const msg, Args &&... args ) const {
            return query_yn( string_format( msg, std::forward<Args>( args ) ... ) );
        }
        virtual bool query_yn( const std::string &msg ) const = 0;

        std::pair<bodypart_id, int> best_part_to_smash() const;
        virtual std::map<damage_type_id, int> smash_ability() const;


        bool check_immunity_data( const field_immunity_data &ft ) const override;
        bool is_immune_field( const field_type_id &fid ) const override;

        bool is_elec_immune() const override;

        bool is_immune_effect( const efftype_id & ) const override;

        bool is_immune_damage( const damage_type_id & ) const override;

        bool is_rad_immune() const;

        bool is_knockdown_immune() const;

        bool is_melee_bash_damage_cap_bonus() const;












        bool immune_to( const bodypart_id &bp, damage_unit dam ) const;


        int get_pain() const override;

        int mod_pain( int npain, bodypart_id bp = bodypart_id() ) override;

        void set_pain( int npain ) override;

        int get_perceived_pain() const override;

        float fall_damage_mod() const override;

        int impact( int force, const tripoint_bub_ms &pos ) override;

        bool can_fly();

        void knock_back_to( const tripoint_bub_ms &to ) override;


        int hp_percentage() const override;


        bool has_nv_goggles();

        int get_lift_assist() const;





        float rest_quality() const;



        float healing_rate( float at_rest_quality ) const;



        float healing_rate_medicine( float at_rest_quality, const bodypart_id &bp ) const;




        social_modifiers get_mutation_bionic_social_mods() const;


        nc_color symbol_color() const override;
        const std::string &symbol() const override;

        std::vector<std::string> extended_description() const override;

        std::string mutation_name( const trait_id &mut ) const;
        std::string mutation_desc( const trait_id &mut ) const;

        void empty_skills();

        void pick_name( bool bUseDefault = false );

        std::vector<trait_id> get_base_traits() const;

        std::vector<trait_id> get_mutations(
            bool include_hidden = true,
            bool ignore_enchantment = false,
            const std::function<bool( const mutation_branch & )> &filter = nullptr ) const;

        std::vector<trait_id> get_functioning_mutations(
            bool include_hidden = true,
            bool ignore_enchantment = false,
            const std::function<bool( const mutation_branch & )> &filter = nullptr ) const;

        std::vector<trait_and_var> get_mutations_variants( bool include_hidden = true,
                bool ignore_enchantment = false ) const;
        const std::bitset<NUM_VISION_MODES> &get_vision_modes() const {
            return vision_mode_cache;
        }

        void clear_mutations();

        void toggle_trait_deps( const trait_id &tr, const std::string &variant = "" );




        void add_traits();



        bool crossed_threshold() const;


        mutation_category_id get_threshold_category() const;

        void environmental_revert_effect();







        int climbing_cost( const tripoint_bub_ms &from, const tripoint_bub_ms &to ) const;


        bodypart_id most_staunchable_bp();
        bodypart_id most_staunchable_bp( int &max );

        void pause();


        bool can_lift( item &obj ) const;
        bool can_lift( vehicle &veh, map &here ) const;

        std::string name;

        std::optional<std::string> play_name;

        std::optional<std::string> play_name_suffix;

        std::string custom_profession;

        bool male = false;

        std::vector<effect_on_condition_id> death_eocs;
        outfit worn;
        bool nv_cached = false;

        bool in_vehicle = false;


        bool hauling = false;

        bool autohaul = false;

        bool suppress_autohaul = false;
        std::string hauling_filter;

        std::vector<item_location> haul_list;

        mutable tripoint_rel_ms view_offset;

        player_activity stashed_outbounds_activity;
        player_activity stashed_outbounds_backlog;
        player_activity activity;
        std::list<player_activity> backlog;
        std::optional<tripoint_abs_ms> destination_point;
        itype_id last_item;
    private:
        item weapon;
    public:
        item_location get_wielded_item() const;
        item_location get_wielded_item();

        void set_wielded_item( const item &to_wield );

        int scent = 0;
        pimpl<bionic_collection> my_bionics;
        pimpl<character_martial_arts> martial_arts_data;
        std::vector<matype_id> known_styles( bool teachable_only ) const;
        bool has_martialart( const matype_id &m ) const;

        stomach_contents stomach;
        stomach_contents guts;
        std::list<consumption_event> consumption_history;

        int get_oxygen_max() const;

        bool can_recover_oxygen() const;
        int oxygen = 0;
        int slow_rad = 0;
        blood_type my_blood_type;
        bool blood_rh_factor = false;

        void randomize_blood();

        int avg_nat_bpm;
        void randomize_heartrate();
        void randomize( bool random_scenario, bool play_now = false );
        void randomize_cosmetics();
        int get_focus() const {
            return std::max( 1, focus_pool / 1000 );
        }
        int get_effective_focus() const;
        void mod_focus( int amount ) {
            focus_pool += amount * 1000;
            focus_pool = std::max( focus_pool, 0 );
        }

        void set_focus( int amount ) {
            focus_pool = amount * 1000;
        }
    protected:
        int focus_pool = 0;
        std::set<mtype_id> known_monsters;
    public:
        int cash = 0;
        weak_ptr_fast<Creature> last_target;
        std::optional<tripoint_abs_ms> last_target_pos;
        std::optional<tripoint_abs_ms> last_magic_target_pos;

        item_location ammo_location;

        std::set<tripoint_abs_omt> camps;

        std::vector <addiction> addictions;
        std::vector<effect_on_condition_id> inactive_effect_on_condition_vector;
        queued_eocs queued_effect_on_conditions;


        void queue_effects( const std::vector<effect_on_condition_id> &effects );

        void queue_effect( const std::string &name, const time_duration &delay,
                           const time_duration &duration );

        int count_queued_effects( const std::string &effect ) const;


        void add_addiction( const addiction_id &type, int strength );

        void rem_addiction( const addiction_id &type );

        bool has_addiction( const addiction_id &type ) const;

        int addiction_level( const addiction_id &type ) const;


        bool knows_creature_type( const Creature *c ) const;

        bool knows_creature_type( const mtype_id &c ) const;

        void set_knows_creature_type( const Creature *c );

        void set_knows_creature_type( const mtype_id &c );

        const std::set<mtype_id> &get_known_monsters() const {
            return known_monsters;
        }

        shared_ptr_fast<monster> mounted_creature;

        int mounted_creature_id = 0;

        int activity_vehicle_part_index = -1;


        void toggle_hauling();
        void start_hauling( const std::vector<item_location> &items_to_haul );
        void stop_hauling();
        bool is_hauling() const;

        void start_autohaul();
        void stop_autohaul();
        bool is_autohauling() const;

        bool trim_haul_list( const std::vector<item_location> &valid_items );









        void cache_visit_items_with( const itype_id &type,
                                     const std::function<void( item_location & )> &do_func );
        void cache_visit_items_with( const flag_id &type_flag,
                                     const std::function<void( item_location & )> &do_func );
        void cache_visit_items_with( const std::string &key, bool( item::*filter_func )() const,
                                     const std::function<void( item_location & )> &do_func );
        void cache_visit_items_with( const std::string &key, const itype_id &type,
                                     const flag_id &type_flag, bool( item::*filter_func )() const,
                                     const std::function<void( item_location & )> &do_func );
        void cache_visit_items_with( const itype_id &type,
                                     const std::function<void( const item_location & )> &do_func ) const;
        void cache_visit_items_with( const flag_id &type_flag,
                                     const std::function<void( const item_location & )> &do_func ) const;
        void cache_visit_items_with( const std::string &key, bool( item::*filter_func )() const,
                                     const std::function<void( const item_location & )> &do_func ) const;
        void cache_visit_items_with( const std::string &key, const itype_id &type,
                                     const flag_id &type_flag, bool( item::*filter_func )() const,
                                     const std::function<void( const item_location & )> &do_func ) const;











        bool cache_has_item_with( const itype_id &type,
                                  const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        bool cache_has_item_with( const flag_id &type_flag,
                                  const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        bool cache_has_item_with( const std::string &key, bool( item::*filter_func )() const,
                                  const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        bool cache_has_item_with( const std::string &key, const itype_id &type, const flag_id &type_flag,
                                  bool( item::*filter_func )() const,
                                  const std::function<bool( const item & )> &check_func = return_true<item> ) const;



        bool has_item_with_flag( const flag_id &flag, bool need_charges = false ) const;



        bool cache_has_item_with_flag( const flag_id &type_flag, bool need_charges = false ) const;










        std::vector<item_location> cache_get_items_with( const itype_id &type,
                const std::function<bool( item_location & )> &do_and_check_func = return_true<item_location> );
        std::vector<item_location> cache_get_items_with( const flag_id &type_flag,
                const std::function<bool( item_location & )> &do_and_check_func = return_true<item_location> );
        std::vector<item_location> cache_get_items_with( const std::string &key,
                bool( item::*filter_func )() const,
                const std::function<bool( item_location & )> &do_and_check_func = return_true<item_location> );
        std::vector<item_location> cache_get_items_with( const std::string &key, const itype_id &type,
                const flag_id &type_flag, bool( item::*filter_func )() const,
                const std::function<bool( item_location & )> &do_and_check_func = return_true<item_location> );
        std::vector<item_location> cache_get_items_with( const itype_id &type,
                const std::function<bool( const item_location & )> &check_func = return_true<item_location> ) const;
        std::vector<item_location> cache_get_items_with( const flag_id &type_flag,
                const std::function<bool( const item_location & )> &check_func = return_true<item_location> ) const;
        std::vector<item_location> cache_get_items_with( const std::string &key,
                bool( item::*filter_func )() const,
                const std::function<bool( const item_location & )> &check_func = return_true<item_location> ) const;
        std::vector<item_location> cache_get_items_with( const std::string &key, const itype_id &type,
                const flag_id &type_flag, bool( item::*filter_func )() const,
                const std::function<bool( const item_location & )> &check_func = return_true<item_location> ) const;



        void add_to_inv_search_caches( const item_location &it ) const;

        bool has_charges( const itype_id &it, int quantity,
                          const std::function<bool( const item & )> &filter = return_true<item> ) const override;



        std::list<item> use_amount( const itype_id &it, int quantity,
                                    const std::function<bool( const item & )> &filter = return_true<item>, bool select_ind = false );

        bool use_charges_if_avail( const itype_id &it, int quantity );

        std::pair<float, item> get_best_weapon_by_damage_type( damage_type_id dmg_type ) const;






        units::energy available_ups() const;







        units::energy consume_ups( units::energy qty, int radius = -1 );








        std::list<item> use_charges( const itype_id &what, int qty,
                                     const std::function<bool( const item & )> &filter = return_true<item> );








        std::list<item> use_charges( const itype_id &what, int qty, int radius,
                                     const std::function<bool( const item & )> &filter = return_true<item>,
                                     bool in_tools = false );

        item find_firestarter_with_charges( int quantity ) const;
        bool has_fire( int quantity ) const;
        void use_fire( int quantity );
        void assign_stashed_activity();
        bool check_outbounds_activity( const player_activity &act, bool check_only = false );



        void assign_activity( const activity_id &type, int moves = calendar::INDEFINITELY_LONG,
                              int index = -1, int pos = INT_MIN,
                              const std::string &name = "" );

        void assign_activity( const player_activity &act );

        void assign_activity( const activity_actor &actor );

        bool has_activity( const activity_id &type ) const;

        bool has_activity( const std::vector<activity_id> &types ) const;

        void process_activity();

        bool has_sub_bodypart( const sub_bodypart_id &sbp ) const;
        void resume_backlog_activity();
        void cancel_activity();
        void cancel_stashed_activity();
        player_activity get_stashed_activity() const;
        void set_stashed_activity( const player_activity &act,
                                   const player_activity &act_back = player_activity() );
        bool has_stashed_activity() const;
        bool can_stash( const item &it, bool ignore_pkt_settings = false );
        bool can_stash( const item &it, int &copies_remaining, bool ignore_pkt_settings = false );
        bool can_stash_partial( const item &it, bool ignore_pkt_settings = false );
        bool can_stash_partial( const item &it, int &copies_remaining, bool ignore_pkt_settings = false );
        void initialize_stomach_contents();


        float metabolic_rate_base() const;

        float metabolic_rate() const;

        int get_max_healthy() const;

        float get_bmi() const;
        float get_bmi_fat() const;
        float get_bmi_lean() const;
        bool has_calorie_deficit() const;

        int get_bmr() const;

        virtual void add_spent_calories( int ) {}

        virtual void add_gained_calories( int ) {}

        virtual void log_activity_level( float ) {}

        void reset_chargen_attributes();

        int base_age() const;
        void set_base_age( int age );
        void mod_base_age( int mod );

        int age( time_point when = calendar::turn ) const;
        std::string age_string( time_point when = calendar::turn ) const;

        int ugliness() const;

        int base_height() const;
        void set_base_height( int height );
        void mod_base_height( int mod );
        std::string height_string() const;

        int height() const;

        void randomize_height();

        units::mass bodyweight() const;

        units::mass bodyweight_lean() const;

        units::mass bodyweight_fat() const;

        float fat_ratio() const;

        units::mass bionics_weight() const;


        void set_activity_level( float new_level );


        void decrease_activity_level( float new_level );

        void reset_activity_level();

        std::string activity_level_str() const;


        int get_env_resist( bodypart_id bp ) const override;

        int get_armor_type( const damage_type_id &dt, bodypart_id bp ) const override;
        std::map<bodypart_id, int> get_all_armor_type( const damage_type_id &dt ) const;

        int get_stim() const;
        void set_stim( int new_stim );
        void mod_stim( int mod );

        int get_rad() const;
        void set_rad( int new_rad );
        void mod_rad( int mod );

        float get_heartrate_index() const;
        void set_heartrate_effect_mod( int  mod );
        void modify_heartrate_effect_mod( int mod );
        int get_heartrate_effect_mod() const;
        void update_heartrate_index();

        float get_bloodvol_index() const;
        void update_bloodvol_index();

        float get_circulation_resistance() const;
        void set_bp_effect_mod( int mod );
        void modify_bp_effect_mod( int mod );
        int get_bp_effect_mod() const;
        void update_circulation_resistance();

        float get_respiration_rate() const;
        void set_respiration_effect_mod( int mod );
        int get_respiration_effect_mod() const;
        void modify_respiration_effect_mod( int mod );
        void update_respiration_rate();

        void update_circulation();

        int get_stamina() const;
        int get_stamina_max() const;
        void set_stamina( int new_stamina );

        void burn_energy_arms( int mod );
        void burn_energy_legs( int mod );
        void burn_energy_all( int mod );

        float get_bionic_limb_percentage() const;
        void mod_stamina( int mod );
        void burn_move_stamina( int moves );

        void update_stamina( int turns );

        int get_cardiofit() const;

        int get_cardio_acc() const;
        void set_cardio_acc( int ncardio_acc );
        void reset_cardio_acc();
        int get_cardio_acc_base() const;
        virtual void update_cardio_acc() = 0;


        bool handle_gun_damage( item &it );


        bool handle_gun_overheat( item &it );


        double recoil_vehicle() const;


        double recoil_total() const;


        int gun_engagement_moves( const item &gun, int target = 0, int start = MAX_RECOIL,
                                  const Target_attributes &attributes = Target_attributes() ) const;








        int fire_gun( const tripoint_bub_ms &target, int shots = 1 );








        int fire_gun( map &here, const tripoint_bub_ms &target, int shots, item &gun,
                      item_location ammo = item_location() );

        dealt_projectile_attack throw_item( const tripoint_bub_ms &target, const item &to_throw,
                                            const std::optional<tripoint_bub_ms> &blind_throw_from_pos = std::nullopt );

    protected:
        void on_damage_of_type( const effect_source &source, int adjusted_damage,
                                const damage_type_id &dt, const bodypart_id &bp ) override;
    public:

        void on_item_wear( const item &it );

        void on_item_takeoff( const item &it );

        void on_worn_item_washed( const item &it );

        void on_item_acquire( const item_location &it );

        void on_effect_int_change( const efftype_id &eid, int intensity,
                                   const bodypart_id &bp = bodypart_str_id::NULL_ID() ) override;

        void on_mutation_gain( const trait_id &mid );

        void on_mutation_loss( const trait_id &mid );

        void on_stat_change( const std::string &stat, int value ) override;

        tripoint_bub_ms adjacent_tile() const;

        bool has_opposite_trait( const trait_id &flag ) const;

        std::unordered_set<trait_id> get_opposite_traits( const trait_id &flag ) const;

        virtual void wake_up();

        int get_shout_volume() const;

        void shout( std::string msg = "", bool order = false );

        void signal_nemesis();

        void vomit();

        void customize_appearance( customize_appearance_choice choice );

        std::map<mutation_category_id, int> mutation_category_level;

        float adjust_for_focus( float amount ) const;
        void update_type_of_scent( bool init = false );
        void update_type_of_scent( const trait_id &mut, bool gain = true );
        void set_type_of_scent( const scenttype_id &id );
        scenttype_id get_type_of_scent() const;

        void restore_scent();

        void mod_painkiller( int npkill );

        void set_painkiller( int npkill );

        int get_painkiller() const;
        void react_to_felt_pain( int intensity );


        void clear_moncams();
        void remove_moncam( const mtype_id &moncam_id );
        void add_moncam( const std::pair<mtype_id, int> &moncam );
        void set_moncams( std::map<mtype_id, int> nmoncams );
        std::map<mtype_id, int> const &get_moncams() const;
        using cached_moncam = std::pair<monster const *, tripoint_abs_ms>;
        using moncam_cache_t = cata::flat_set<cached_moncam>;
        moncam_cache_t get_active_moncams() const;

        void spores();
        void blossoms();


        void rooted_message() const;
        void rooted();


        void fall_asleep();
        void fall_asleep( const time_duration &duration );

        std::string is_snuggling() const;

        ret_val<crush_tool_type> can_crush_frozen_liquid( item_location const &loc ) const;


        bool crush_frozen_liquid( item_location loc );

        float power_rating() const override;
        float speed_rating() const override;


        void place_corpse( map *here );

        void place_corpse( const tripoint_abs_omt &om_target );


        std::vector<run_cost_effect> run_cost_effects( float &movecost ) const;

        int run_cost( int base_cost, bool diag = false ) const;

        const pathfinding_settings &get_pathfinding_settings() const override;
        std::function<bool( const tripoint_bub_ms & )> get_path_avoid() const override;



        std::vector<Creature *> get_hostile_creatures( int range ) const;








        std::vector<Creature *> get_visible_creatures( int range ) const;




        std::vector<Creature *> get_targetable_creatures( int range, bool melee ) const;






        std::vector<vehicle *> get_visible_vehicles( int range ) const;

        int get_mutation_visibility_cap( const Character *observed ) const;

        std::string visible_mutations( int visibility_cap ) const;

        player_activity get_destination_activity() const;
        const player_activity &peek_destination_activity() const;
        void set_destination_activity( const player_activity &new_destination_activity );
        void clear_destination_activity();

        bool can_use_pockets() const;
        bool can_use_hood() const;
        bool can_use_collar() const;

        std::map<bodypart_id, int> bonus_item_warmth() const;

        bool can_use_floor_warmth() const;




        static units::temperature_delta floor_bedding_warmth( const tripoint_bub_ms &pos );

        static units::temperature_delta floor_item_warmth( const tripoint_bub_ms &pos );

        units::temperature_delta floor_warmth( const tripoint_bub_ms &pos ) const;


        units::temperature_delta bodytemp_modifier_traits( bool overheated ) const;

        units::temperature temp_corrected_by_climate_control( units::temperature temperature,
                int heat_strength,
                int chill_strength ) const;

        bool in_sleep_state() const override;

        void update_vitamins( const vitamin_id &vit );









        int vitamin_get( const vitamin_id &vit ) const;

        int get_daily_vitamin( const vitamin_id &vit, bool actual = true ) const;








        void vitamin_set( const vitamin_id &vit, int qty );






        int vitamin_mod( const vitamin_id &vit, int qty );
        void vitamins_mod( const std::map<vitamin_id, int> & );

        time_duration vitamin_rate( const vitamin_id &vit ) const;

        std::map<vitamin_id, int> effect_vitamin_mod( const std::map<vitamin_id, int> & );

        void clear_vitamins();


        void reset_daily_vitamin( const vitamin_id &vit );


        int nutrition_for( const item &comest ) const;

        ret_val<edible_rating> can_eat( const item &food ) const;




        ret_val<edible_rating> will_eat( const item &food, bool interactive = false ) const;



        int get_acquirable_energy( const item &it ) const;


        void modify_stimulation( const islot_comestible &comest );


        void modify_sleepiness( const islot_comestible &comest );
        void modify_radiation( const islot_comestible &comest );

        void modify_addiction( const islot_comestible &comest );

        void modify_health( const islot_comestible &comest );

        double compute_effective_food_volume_ratio( const item &food ) const;

        std::pair<units::volume, units::volume> masticated_volume( const item &food ) const;

        int compute_calories_per_effective_volume( const item &food,
                const nutrients *nutrient = nullptr ) const;

        bool consume_effects( item &food );

        bool can_consume_as_is( const item &it ) const;

        bool can_estimate_rot() const;





        item_location get_consumable_from( const item_location &it ) const;





        std::pair<nutrients, nutrients> compute_nutrient_range(
            const item &, const recipe_id &,
            std::map<recipe_id, std::pair<nutrients, nutrients>> &rec_cache,
            const cata::flat_set<flag_id> &extra_flags = {} ) const;

        std::pair<nutrients, nutrients> compute_nutrient_range(
            const itype_id &,
            std::map<recipe_id, std::pair<nutrients, nutrients>> &rec_cache,
            const cata::flat_set<flag_id> &extra_flags = {} ) const;

        morale_type allergy_type( const item &food ) const;
        nutrients compute_effective_nutrients( const item & ) const;

        bool wearing_something_on( const bodypart_id &bp ) const;

        bool wearing_fitting_on( const bodypart_id &bp ) const;

        double armwear_factor() const;


        int shoe_type_count( const itype_id &it ) const;

        bool is_wearing_shoes( const side &check_side = side::BOTH ) const;



        bool is_barefoot() const;


        bool is_worn_item_visible( std::list<item>::const_iterator ) const;


        std::list<item_location> get_visible_worn_items() const;


        bool change_side( item &it, bool interactive = true );
        bool change_side( item_location &loc, bool interactive = true );

        bool get_check_encumbrance() const {
            return check_encumbrance;
        }
        void set_check_encumbrance( bool new_check ) {
            check_encumbrance = new_check;
        }

        void update_morale();

        void apply_persistent_morale();

        double get_modifier_for_ALL_morale() const;

        void hoarder_morale_penalty();

        void modify_morale( item &food, int nutr = 0 );

        int get_morale_level( bool raw = false ) const;
        void add_morale( const morale_type &type, int bonus, int max_bonus = 0,
                         const time_duration &duration = 1_hours,
                         const time_duration &decay_start = 30_minutes, bool capped = false,
                         const itype *item_type = nullptr );
        int has_morale( const morale_type &type ) const;
        void rem_morale( const morale_type &type, const itype *item_type = nullptr );
        void clear_morale();
        bool has_morale_to_read() const;
        bool has_morale_to_craft() const;
        const temp_crafting_inventory &crafting_inventory( bool clear_path ) const;







        const temp_crafting_inventory &crafting_inventory( const tripoint_bub_ms &src_pos =
                    tripoint_bub_ms::zero,
                int radius = PICKUP_RANGE, bool clear_path = true ) const;
        const temp_crafting_inventory &crafting_inventory( map *here,
                const tripoint_bub_ms &src_pos = tripoint_bub_ms::zero,
                int radius = PICKUP_RANGE, bool clear_path = true ) const;
        void invalidate_crafting_inventory();



        book_proficiency_bonuses book_bonuses_nearby( int radius = PICKUP_RANGE ) const;






        float fine_detail_vision_mod( const tripoint_bub_ms &p = tripoint_bub_ms::invalid ) const;


        void make_craft_with_command( const recipe_id &id_to_make, int batch_size, bool is_long,
                                      const std::optional<tripoint_bub_ms> &loc );
        pimpl<craft_command> last_craft;

        recipe_id lastrecipe;
        int last_batch;



        bool has_recipe( const recipe *r ) const;
        bool knows_recipe( const recipe *rec ) const;
        void learn_recipe( const recipe *rec, bool override_never_learn = false );
        void forget_recipe( const recipe *rec );
        void forget_all_recipes();
        int exceeds_recipe_requirements( const recipe &rec ) const;
        bool has_recipe_requirements( const recipe &rec ) const;
        bool can_decomp_learn( const recipe &rec ) const;

        bool studied_all_recipes( const itype &book ) const;


        const recipe_subset &get_learned_recipes() const;
        recipe_subset get_available_nested( const recipe_subset & ) const;

        recipe_subset get_recipes_from_books( const temp_crafting_inventory &crafting_inv ) const;

        recipe_subset get_recipes_from_ebooks( const temp_crafting_inventory &crafting_inv ) const;
    protected:





        recipe_subset get_available_recipes( const temp_crafting_inventory &crafting_inv,
                                             const std::vector<Character *> *helpers = nullptr ) const;
    public:




        recipe_subset &get_group_available_recipes( temp_crafting_inventory *inventory_override = nullptr )
        const;






        std::set<itype_id> get_books_for_recipe( const temp_crafting_inventory &crafting_inv,
                const recipe *r ) const;


        float morale_crafting_speed_multiplier( const recipe &rec ) const;
        float lighting_craft_speed_multiplier( const recipe &rec,
                                               const tripoint_bub_ms &p = tripoint_bub_ms::invalid ) const;
        float crafting_speed_multiplier( const recipe &rec ) const;
        float workbench_crafting_speed_multiplier( const item &craft,
                const std::optional<tripoint_bub_ms> &loc )const;
        float limb_score_crafting_speed_multiplier( const recipe &rec ) const;
        float pain_crafting_speed_multiplier( const recipe &rec ) const;
        float mut_crafting_speed_multiplier( const recipe &rec ) const;





        float crafting_speed_multiplier( const item &craft, const std::optional<tripoint_bub_ms> &loc,
                                         bool use_cached_workbench_multiplier = false, float cached_workbench_multiplier = 0.0f
                                       ) const;
        int available_assistant_count( const recipe &rec ) const;



        int64_t expected_time_to_craft( const recipe &rec, int batch_size = 1 ) const;
        std::vector<const item *> get_eligible_containers_for_crafting() const;
        bool check_eligible_containers_for_crafting( const recipe &rec, int batch_size = 1 ) const;
        bool can_make( const recipe *r, int batch_size = 1 ) const;





        bool can_start_craft( const recipe *rec, recipe_filter_flags, int batch_size = 1 ) const;
        bool making_would_work( const recipe_id &id_to_make, int batch_size ) const;






        void craft( const std::optional<tripoint_bub_ms> &loc = std::nullopt,
                    const recipe_id &goto_recipe = recipe_id(), const std::string &filterstring = "" );
        void recraft( const std::optional<tripoint_bub_ms> &loc = std::nullopt );
        void long_craft( const std::optional<tripoint_bub_ms> &loc = std::nullopt,
                         const recipe_id &goto_recipe = recipe_id() );
        void make_craft( const recipe_id &id, int batch_size,
                         const std::optional<tripoint_bub_ms> &loc = std::nullopt );
        void make_all_craft( const recipe_id &id, int batch_size,
                             const std::optional<tripoint_bub_ms> &loc );

        void start_craft( craft_command &command, const std::optional<tripoint_bub_ms> &loc,
                          std::vector<attention_plan> plans = {} );

        struct craft_roll_data {
            float center;
            float stddev;
            float final_difficulty;
        };






        float crafting_success_roll( const recipe &making ) const;
        float crafting_failure_roll( const recipe &making ) const;
        float get_recipe_weighted_skill_average( const recipe &making ) const;
        float recipe_success_chance( const recipe &making ) const;
        float item_destruction_chance( const recipe &making ) const;
        craft_roll_data recipe_success_roll_data( const recipe &making ) const;
        craft_roll_data recipe_failure_roll_data( const recipe &making ) const;
        void complete_craft( item &craft, const std::optional<tripoint_bub_ms> &loc );








        bool can_continue_craft( item &craft );
        bool can_continue_craft( item &craft, const requirement_data &continue_reqs );

        std::vector<Character *> get_crafting_helpers() const;




        std::vector<Character *> get_crafting_group() const;
        int get_num_crafting_helpers( int max ) const;







        bool craft_skill_gain( const item &craft, const int &num_practice_ticks );






        bool craft_proficiency_gain( const item &craft, const time_duration &time );





        ret_val<void> can_disassemble( const item &obj, const temp_crafting_inventory &inv ) const;
        item_location create_in_progress_disassembly( item_location target );

        bool disassemble();
        bool disassemble( item_location target, bool interactive = true, bool disassemble_all = false );
        void disassemble_all( bool one_pass );





        void complete_disassemble( item_location target );
        void complete_disassemble( item_location &target, const recipe &dis );

        const requirement_data *select_requirements(
            const std::vector<const requirement_data *> &, int batch, const temp_crafting_inventory &,
            const std::function<bool( const item & )> &filter ) const;
        comp_selection<item_comp>
        select_item_component( const std::vector<item_comp> &components,
                               int batch, temp_crafting_inventory &map_inv, bool can_cancel = false,
                               const std::function<bool( const item & )> &filter = return_true<item>, bool player_inv = true,
                               bool npc_query = false, const recipe *rec = nullptr );
        std::list<item> consume_items( const comp_selection<item_comp> &is, int batch,
                                       const std::function<bool( const item & )> &filter = return_true<item>, bool select_ind = false,
                                       bool disable_preference = false );
        std::list<item> consume_items( map &m, const comp_selection<item_comp> &is, int batch,
                                       const std::function<bool( const item & )> &filter = return_true<item>,
                                       const std::vector<tripoint_bub_ms> &reachable_pts = {}, bool select_ind = false,
                                       bool disable_preference = false );

        std::list<item> consume_items( const std::vector<item_comp> &components, int batch = 1,
                                       const std::function<bool( const item & )> &filter = return_true<item>,
                                       const std::function<bool( const itype_id & )> &select_ind = return_false<itype_id>,
                                       bool can_cancel = false, bool disable_preference = false );
        bool consume_software_container( const itype_id &software_id );
        comp_selection<tool_comp>
        select_tool_component( const std::vector<tool_comp> &tools, int batch,
                               temp_crafting_inventory &map_inv,
                               bool can_cancel = false, bool player_inv = true, bool npc_query = false,
        const std::function<int( int )> &charges_required_modifier = []( int i ) {
            return i;
        } );

        bool craft_consume_tools( item &craft, int multiplier, bool start_craft );




        bool craft_consume_step_tools( item &craft, const crafting_cost_context *cost_ctx = nullptr );


        bool craft_consume_character_resources( item &craft, int target_progress, bool consume = true );


        int craft_character_resource_available( magic_energy_type resource ) const;



        int craft_vitamin_available( const vitamin_resource_cost &resource ) const;


        bool craft_consume_passive_step_tools( item &craft, time_point now, const item_location &loc );







        bool consume_step_tool_targets( item &craft, const std::vector<int> &targets,
                                        const tripoint_bub_ms &origin, int radius,
                                        bool pin_to_map );







        bool verify_step_tools( item &craft, int step_idx,
                                const tripoint_bub_ms &origin, int radius, bool pin_to_map );
        void consume_tools( const comp_selection<tool_comp> &tool, int batch );
        void consume_tools( map &m, const comp_selection<tool_comp> &tool, int batch,
                            const tripoint_bub_ms &origin = tripoint_bub_ms::zero, int radius = PICKUP_RANGE,
                            basecamp *bcp = nullptr );
        void consume_tools( map &m, const comp_selection<tool_comp> &tool, int batch,
                            const std::vector<tripoint_bub_ms> &reachable_pts = {},   basecamp *bcp = nullptr );
        void consume_tools( const std::vector<tool_comp> &tools, int batch = 1 );


        void check_and_recover_morale();







        std::pair<int, int> fun_for( const item &comest, bool ignore_already_ate = false ) const;


        void suffer();

        bool irradiate( float rads, bool bypass = false );

        void mend( int rate_multiplier );

    private:

        float leak_level = 0.0f;

        bool leak_level_dirty = true;

        mutable std::map<const json_character_flag, bool> bio_flag_cache;


        std::list<item> temporary_load_items;
    public:
        void add_temporary_load_items();
        void stash_temporary_load_item( const item &it );
        float get_leak_level() const;

        void calculate_leak_level();

        void invalidate_leak_level_cache();


        void sound_hallu();


        void echo_pulse();


        bool is_driving() const;


        void drench( int saturation, const body_part_set &flags, bool ignore_waterproof );

        void apply_wetness_morale( units::temperature temperature );
        int heartrate_bpm() const;
        std::vector<std::string> short_description_parts() const;
        std::string short_description() const;

        bool can_hear( const tripoint_bub_ms &source, int volume ) const;

        float hearing_ability() const;

        using trap_map = std::map<tripoint_abs_ms, std::string>;


        bool knows_trap( const tripoint_bub_ms &pos ) const;
        void add_known_trap( const tripoint_bub_ms &pos, const trap &t );


        bool sees( const map &here, const tripoint_bub_ms &t, bool is_avatar = false,
                   int range_mod = 0 ) const override;

        bool sees( const map &here, const Creature &critter ) const override;
        Attitude attitude_to( const Creature &other ) const override;
        virtual npc_attitude get_attitude() const;


        int get_lowest_hp() const;

        int get_highest_hp() const;
        bool has_weapon() const override;
        void shift_destination( const point_rel_ms &shift );

        void set_destination( const std::vector<tripoint_bub_ms> &route,
                              const player_activity &new_destination_activity = player_activity() );
        void clear_destination();

        void abort_automove();
        bool has_distant_destination() const;



        bool is_auto_moving() const;

        bool has_destination() const;

        bool has_destination_activity() const;

        void start_destination_activity();
        std::vector<tripoint_bub_ms> &get_auto_move_route();
        action_id get_next_auto_move_direction();
        bool defer_move( const tripoint_bub_ms &next );
        time_duration get_consume_time( const item &it ) const;


        std::pair<int, int> weariness_transition_progress() const;
        int weariness_level() const;
        int weariness_transition_level() const;
        int weary_threshold() const;
        int weariness() const;
        float activity_level() const;

        float instantaneous_activity_level() const;

        int activity_level_index() const;
        float exertion_adjusted_move_multiplier( float level = -1.0f ) const;
        float maximum_exertion_level() const;
        std::string activity_level_str( float level ) const;
        std::string debug_weary_info() const;

        void add_pain_msg( int, const bodypart_id & ) const {}

        double vomit_mod();

        bool can_sleep();

        void process_effects() override;

        void hardcoded_effects( effect &it );




        time_duration estimate_effect_dur( const skill_id &relevant_skill, const efftype_id &effect,
                                           const time_duration &error_magnitude,
                                           const time_duration &minimum_error, int threshold, const Creature &target ) const;




        item &best_item_with_quality( const quality_id &qid );
        const item &best_item_with_quality( const quality_id &qid ) const;


        bool has_quality( const quality_id &qual, int level = 1, int qty = 1 ) const override;


        std::vector<intrinsic_quality_source> intrinsic_quality_sources(
            const quality_id &qual, int level ) const;

        bool has_intrinsic_quality( const quality_id &qual, int level = 1, int qty = 1 ) const;


        bool has_unreserved_quality( const quality_id &qual, int level = 1, int qty = 1 ) const;
        item &best_unreserved_item_with_quality( const quality_id &qid );
        int max_quality( const quality_id &qual ) const override;
        int max_quality( const quality_id &qual, int radius ) const;
        VisitResponse visit_carried( const std::function<VisitResponse( const item_location & )> &func )
        const;
        VisitResponse visit_items( const std::function<VisitResponse( const item_location & )> &func ) const
        override;
        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX ) override;
        int charges_of(
            const itype_id &what, int limit = INT_MAX,
            const std::function<bool( const item & )> &filter = return_true<item>,
            const std::function<void( int )> &visitor = nullptr,
            bool in_tools = false ) const override;
        int amount_of(
            const itype_id &what, bool pseudo = true, int limit = INT_MAX,
            const std::function<bool( const item & )> &filter = return_true<item> ) const override;

        std::pair<int, int> kcal_range(
            const itype_id &id,
            const std::function<bool( const item & )> &filter, Character &player_character ) const;










        const comfort_data &get_comfort_data_for( const tripoint_bub_ms &p ) const;







        const comfort_data::response &get_comfort_at( const tripoint_bub_ms &p );
        comfort_data::response comfort_cache;

    protected:
        Character();
        Character( Character && ) noexcept( map_is_noexcept );
        Character &operator=( Character && ) noexcept( list_is_noexcept );
        void swap_character( Character &other );
    public:
        struct trait_data {





            int corrupted = 0;

            bool powered = false;

            char key = ' ';





            time_duration charge = 0_turns;

            bool show_sprite = true;


            const mutation_variant *variant = nullptr;

            void serialize( JsonOut &json ) const;
            void deserialize( const JsonObject &data );

            trait_data() = default;
            explicit trait_data( const mutation_variant *chosen ) : variant( chosen ) {}
        };


        int str_bonus = 0;
        int dex_bonus = 0;
        int per_bonus = 0;
        int int_bonus = 0;

        int str_bonus_hardcoded = 0;
        int dex_bonus_hardcoded = 0;
        int per_bonus_hardcoded = 0;
        int int_bonus_hardcoded = 0;

        int enchantment_speed_bonus = 0;


        int lifestyle = 0;
        int daily_health = 0;
        int health_tally = 0;


        int base_bmr() const;


        int init_age = 25;

        int init_height = 175;

        creature_size size_class = creature_size::medium;






        static int min_height( creature_size size_category = creature_size::medium );
        static int default_height( creature_size size_category = creature_size::medium );
        static int max_height( creature_size size_category = creature_size::medium );



        activity_tracker activity_history;
        std::vector<item_location> may_activity_occupancy_after_end_items_loc;

        int old_weary_level = 0;

        trap_map known_traps;
        mutable std::map<std::string, double> cached_info;
        bool bio_soporific_powered_at_last_sleep_check = false;

        time_point last_sleep_check = calendar::turn_zero;

        mutable std::map<faction_id, std::pair<int, time_point>> warning_record;






        std::unordered_map<trait_id, trait_data> my_mutations;



        std::unordered_set<trait_id> my_traits;

        std::unordered_set<trait_id> my_intrinsic_mutations;



        std::unordered_map<trait_id, trait_data> cached_mutations;



        std::unordered_map<trait_id, trait_data> my_mutations_dirty;



        std::set<trait_id> mutations_to_remove;
        std::set<trait_id> mutations_to_add;




        mutable std::optional<units::mass> cached_weight_carried = std::nullopt;

        void store( JsonOut &json ) const;
        void load( const JsonObject &data );


        pimpl<SkillLevelMap> _skills;

        pimpl<proficiency_set> _proficiencies;


        std::bitset<NUM_VISION_MODES> vision_mode_cache;
        int sight_max = 0;



        time_point time_died = calendar::before_time_starts;





        mutable pimpl<pathfinding_settings> path_settings;





        int faction_api_version = 2;

        faction_id fac_id;
        faction *my_fac = nullptr;

        move_mode_id move_mode;

        std::map<vitamin_id, int> vitamin_levels;

        std::map<vitamin_id, std::pair<int, int>> daily_vitamins;

        int vitamin_RDA( const vitamin_id &vitamin, int amount ) const;

        bool has_wounds( const bodypart_str_id &part ) const;

        pimpl<player_morale> morale;

        void process_one_effect( effect &it, bool is_new ) override;






        std::map<bodypart_id, float> bodypart_exposure();
    private:






        bool is_visible_in_range( const Creature &critter, int range ) const;

        struct bionic_fuels;







        bionic_fuels bionic_fuel_check( bionic &bio, bool start );





        void burn_fuel( bionic &bio );

        player_activity destination_activity;

        character_id id;

        units::energy power_level;
        units::energy max_power_level_cached;
        units::energy max_power_level_modifier;


        std::map<mtype_id, int> moncams;



        int stored_calories;
        int healthy_calories;

        int hunger;
        int thirst;
        int stamina;

        int cardio_acc;
        int base_cardio_acc;



        float heart_rate_index = 1.0f;
        float blood_vol_index = 1.0f;
        float respiration_rate = 1.0f;

        float circulation;

        float circulation_resistance = 1.0f;

        time_duration daily_sleep = 0_turns;
        time_duration continuous_sleep = 0_turns;

        int sleepiness;
        int sleep_deprivation;
        bool check_encumbrance = true;
        bool cache_inventory_is_valid = false;
        mutable bool using_lifting_assist = false;

        int stim;
        int pkill;

        int bp_effect_mod = 0;
        int heart_rate_effect_mod = 0;
        int resp_rate_effect_mod = 0;

        int radiation;

        std::vector<tripoint_bub_ms> auto_move_route;

        std::optional<tripoint_bub_ms> next_expected_position;
        scenttype_id type_of_scent;

        struct weighted_int_list<std::string> melee_miss_reasons;

        struct crafting_cache_type {
            bool valid = false;
            time_point time;
            int moves;
            tripoint_bub_ms position;
            int radius;

            uint64_t reservation_generation = 0;
            pimpl<temp_crafting_inventory> crafting_inventory;
        };
        mutable crafting_cache_type crafting_cache;

        time_point melee_warning_turn = calendar::turn_zero;

        mutable bool pseudo_items_valid = false;
        mutable std::vector<const item *> pseudo_items;


        struct inv_search_cache {
            itype_id type;
            flag_id type_flag;
            bool ( item::*filter_func )() const;
            std::list<item_location> items;
        };
        mutable std::unordered_map<std::string, inv_search_cache> inv_search_caches;
    protected:

        bionic_uid weapon_bionic_uid = 0;
        mutable bionic_uid next_bionic_uid = 0;  // NOLINT(cata-serialize)


        mutable pimpl<recipe_subset> learned_recipes;


        mutable decltype( _skills ) valid_autolearn_skills;


        std::unordered_map<point_abs_omt, time_duration> overmap_time;

    public:
        mutable time_point next_climate_control_check;
        mutable bool last_climate_control_ret;


        pimpl<enchant_cache> old_mutation_cache;
        pimpl<enchant_cache> new_mutation_cache;

    private:

        mutable time_point cached_recipe_turn;
        pimpl<recipe_subset> cached_recipe_subset;
};

Character &get_player_character();

template<>
struct enum_traits<character_stat> {
    static constexpr character_stat last = character_stat::DUMMY_STAT;
};

namespace io
{
std::string enum_to_full_string( character_stat data );
}


std::string get_stat_name( character_stat Stat );
#endif
