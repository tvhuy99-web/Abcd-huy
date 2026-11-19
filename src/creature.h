#pragma once
#ifndef CATA_SRC_CREATURE_H
#define CATA_SRC_CREATURE_H

#include <array>
#include <climits>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "cata_lazy.h"
#include "character_id.h"
#include "compatibility.h"
#include "coordinates.h"
#include "damage.h"
#include "debug.h"
#include "effect_source.h"
#include "enums.h"
#include "global_vars.h"
#include "math_parser_diag_value.h"
#include "pimpl.h"
#include "safe_reference.h"
#include "string_formatter.h"
#include "translation.h"
#include "type_id.h"
#include "units_fwd.h"
#include "viewer.h"
#include "weakpoint.h"

class Character;
class JsonObject;
class JsonOut;
class avatar;
class body_part_set;
class const_talker;
class effect;
class effects_map;
class enchant_cache;
class field;
class field_entry;
class item;
class map;
class monster;
class nc_color;
class npc;
class talker;

namespace enchant_vals
{
enum class mod : int;
}

namespace catacurses
{
class window;
}
struct dealt_projectile_attack;
struct field_immunity_data;
struct pathfinding_settings;
struct projectile;
struct projectile_attack_results;
struct trap;
template <typename T> struct enum_traits;

enum class creature_size : int {


    tiny = 1,

    small,

    medium,

    large,

    huge,

    num_sizes
};

using I = std::underlying_type_t<creature_size>;
constexpr I operator+( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) + static_cast<I>( rhs );
}

constexpr I operator+( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) + rhs;
}

constexpr I operator+( const I lhs, const creature_size rhs )
{
    return lhs + static_cast<I>( rhs );
}

constexpr I operator-( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) - static_cast<I>( rhs );
}

constexpr I operator-( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) - rhs;
}

constexpr I operator-( const I lhs, const creature_size rhs )
{
    return lhs - static_cast<I>( rhs );
}

constexpr I operator*( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) * static_cast<I>( rhs );
}

constexpr I operator*( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) * rhs;
}

constexpr I operator*( const I lhs, const creature_size rhs )
{
    return lhs * static_cast<I>( rhs );
}

constexpr I operator/( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) / static_cast<I>( rhs );
}

constexpr I operator/( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) / rhs;
}

constexpr bool operator<=( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) <= static_cast<I>( rhs );
}

constexpr bool operator<=( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) <= rhs;
}

constexpr bool operator<=( const I lhs, const creature_size rhs )
{
    return lhs <= static_cast<I>( rhs );
}

constexpr bool operator<( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) < static_cast<I>( rhs );
}

constexpr bool operator<( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) < rhs;
}

constexpr bool operator<( const I lhs, const creature_size rhs )
{
    return lhs < static_cast<I>( rhs );
}

constexpr bool operator>=( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) >= static_cast<I>( rhs );
}

constexpr bool operator>=( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) >= rhs;
}

constexpr bool operator>=( const I lhs, const creature_size rhs )
{
    return lhs >= static_cast<I>( rhs );
}

constexpr bool operator>( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) > static_cast<I>( rhs );
}

constexpr bool operator>( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) > rhs;
}

constexpr bool operator>( const I lhs, const creature_size rhs )
{
    return lhs > static_cast<I>( rhs );
}

constexpr bool operator==( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) == rhs;
}

constexpr bool operator==( const I lhs, const creature_size rhs )
{
    return lhs == static_cast<I>( rhs );
}

enum class FacingDirection : int {
    NONE = 0,
    LEFT = 1,
    RIGHT = 2
};

enum class get_body_part_flags : int {
    none = 0,
    only_main = 1 << 0,
    sorted = 1 << 1,
    primary_type = 1 << 2,
    only_minor = 1 << 3
};

template<>
struct enum_traits<get_body_part_flags> {
    static constexpr bool is_flag_enum = true;
};

enum class body_part_filter : int {
    strict = 0,
    equivalent = 1,
    next_best = 2
};

using scheduled_effect = struct scheduled_effect_t {
    efftype_id eff_id;
    time_duration dur;
    bodypart_id bp;
    bool permanent = false;
    int intensity = 0;
    bool force = false;
    bool deferred = false;
};

using terminating_effect = struct terminating_effect_t {
    efftype_id eff_id;
    bodypart_id bp;
};

class Creature : public viewer
{
    public:
        ~Creature() override;

        static const std::map<std::string, creature_size> size_map;


        virtual std::string get_name() const = 0;
        virtual std::string disp_name( bool possessive = false,
                                       bool capitalize_first = false ) const = 0;
        virtual std::string skin_name() const = 0;

        virtual std::vector<std::string> get_grammatical_genders() const;

        virtual bool is_avatar() const {
            return false;
        }
        virtual bool is_npc() const {
            return false;
        }
        virtual bool is_monster() const {
            return false;
        }
        virtual Character *as_character() {
            return nullptr;
        }
        virtual const Character *as_character() const {
            return nullptr;
        }
        virtual avatar *as_avatar() {
            return nullptr;
        }
        virtual const avatar *as_avatar() const {
            return nullptr;
        }
        virtual npc *as_npc() {
            return nullptr;
        }
        virtual const npc *as_npc() const {
            return nullptr;
        }
        virtual monster *as_monster() {
            return nullptr;
        }
        virtual const monster *as_monster() const {
            return nullptr;
        }
        virtual mfaction_id get_monster_faction() const = 0;


        virtual void witness_thievery( item *it ) = 0;

        FacingDirection facing = FacingDirection::RIGHT;

        virtual bool is_fake() const;

        virtual void set_fake( bool fake_value );
        tripoint_bub_ms pos_bub() const;
        tripoint_bub_ms pos_bub( const map &here ) const;
        inline int posx( const map &here ) const {
            return pos_bub( here ).x();
        }
        inline int posy( const map &here ) const {
            return pos_bub( here ).y();
        }
        inline int posz() const {
            return pos_abs().z();
        }
        virtual void gravity_check();
        virtual void gravity_check( map *here );
        void setpos( map &here, const tripoint_bub_ms &p, bool check_gravity = true );
        void setpos( const tripoint_abs_ms &p, bool check_gravity = true );


        bool will_be_cramped_in_vehicle_tile( map &here, const tripoint_abs_ms &loc ) const;

        void move_to( const tripoint_abs_ms &loc );


        virtual void normalize();
    protected:

        void process_turn_no_moves();
    public:

        virtual void process_turn();

        virtual void reset_bonuses();

        virtual void reset_stats() = 0;

        virtual void reset();

        virtual void bleed( map &here ) const;

        void leave_bloody_footprint( const tripoint_abs_ms &old_pos ) const;
        void check_bloody_feet( const tripoint_abs_ms &old_pos );

        virtual void die( map *here, Creature *killer ) = 0;


        virtual float hit_roll() const = 0;
        virtual float dodge_roll() const = 0;
        virtual float stability_roll() const = 0;
        virtual bool can_attack_high() const = 0;








        enum class Attitude : int {
            HOSTILE,
            NEUTRAL,
            FRIENDLY,
            ANY
        };




        static std::string attitude_raw_string( Attitude att );




        static const std::pair<translation, nc_color> &get_attitude_ui_data( Attitude att );




        virtual Attitude attitude_to( const Creature &other ) const = 0;






        virtual bool avoid_trap( const tripoint_bub_ms &pos, const trap &tr ) const = 0;










        bool sees( const map &here, const Creature &critter ) const override;
        bool sees( const map &here, const tripoint_bub_ms &t, bool is_avatar = false,
                   int range_mod = 0 ) const override;






        virtual int sight_range( float light_level ) const = 0;


        virtual float power_rating() const = 0;

        virtual float speed_rating() const = 0;












        Creature *auto_find_hostile_target( int range, int &boo_hoo, int area = 0 );





        double ranged_target_size() const;


        virtual bool block_hit( Creature *source, bodypart_id &bp_hit,
                                damage_instance &dam ) = 0;



        virtual const weakpoint *absorb_hit( const weakpoint_attack &attack, const bodypart_id &bp,
                                             damage_instance &dam, const weakpoint &wp = weakpoint() ) = 0;


        void knock_back_from( const tripoint_bub_ms &p );
        double calculate_by_enchantment( double modify, enchant_vals::mod value,
                                         bool round_output = false ) const;
        void adjust_taken_damage_by_enchantments( damage_unit &du ) const;
        void adjust_taken_damage_by_enchantments_post_absorbed( damage_unit &du ) const;
        virtual void knock_back_to( const tripoint_bub_ms &to ) = 0;



        float get_crit_factor( const bodypart_id &bp ) const;

        int size_melee_penalty() const;


        virtual int deal_melee_attack( Creature *source, int hitroll );






        bool is_adjacent( const Creature *target, bool allow_z_levels ) const;



        virtual damage_instance modify_damage_dealt_with_enchantments( const damage_instance &dam ) const;


        virtual void deal_melee_hit( Creature *source, int hit_spread, bool critical_hit,
                                     damage_instance dam, dealt_damage_instance &dealt_dam,
                                     const weakpoint_attack &attack = weakpoint_attack(),
                                     const bodypart_id *bp = nullptr );



        virtual void deal_projectile_attack( map *here, Creature *source, dealt_projectile_attack &attack,
                                             const double &missed_by = 0,
                                             bool print_messages = true, const weakpoint_attack &wp_attack = weakpoint_attack() );













        virtual dealt_damage_instance deal_damage( Creature *source, bodypart_id bp,
                const damage_instance &dam, const weakpoint_attack &attack = weakpoint_attack(),
                const weakpoint &wp = weakpoint() );


        virtual void deal_damage_handle_type( const effect_source &source, const damage_unit &du,
                                              bodypart_id bp, int &damage, int &pain );



        virtual void apply_damage( Creature *source, bodypart_id bp, int amount,
                                   bool bypass_med = false ) = 0;

        virtual void heal_bp( bodypart_id bp, int dam );






        void longpull( const std::string &name, const tripoint_bub_ms &p );




        bool dodge_check( float hit_roll, bool force_try = false, float training_level = 0.0f );
        bool dodge_check( monster *z, float training_level = 0.0f );
        bool dodge_check( monster *z, bodypart_id bp, const damage_instance &dam_inst,
                          float training_level = 0.0f );


        bool stumble_invis( const Creature &player, bool stumblemsg = true );

        bool attack_air( const tripoint_bub_ms &p );





        virtual void on_dodge( Creature *source, float difficulty, float training_level = 0.0f ) = 0;



        virtual void on_try_dodge() = 0;




        virtual void on_hit( map *here, Creature *source, bodypart_id bp_hit,
                             float difficulty = INT_MIN, dealt_projectile_attack const *proj = nullptr ) = 0;


        virtual bool is_ranged_attacker() const;

        virtual bool digging() const;
        virtual bool is_on_ground() const = 0;
        bool cant_do_underwater( bool msg = true ) const;
        virtual bool is_underwater() const;
        bool is_likely_underwater( const map &here )
        const;
        virtual bool is_warm() const;
        virtual bool in_species( const species_id & ) const;

        virtual bool has_weapon() const = 0;
        virtual bool is_hallucination() const = 0;
        bool hallucination_die( map *here, Creature *killer );


        virtual bool is_electrical() const = 0;


        virtual bool is_fae() const = 0;


        virtual bool is_nether() const = 0;


        virtual bool has_mind() const = 0;


        virtual bool is_dead_state() const = 0;


        virtual bool is_elec_immune() const = 0;
        virtual bool is_immune_effect( const efftype_id &type ) const = 0;
        virtual bool is_immune_damage( const damage_type_id &type ) const = 0;



        bool is_dangerous_fields( const field &fld ) const;

        bool is_dangerous_field( const field_entry &entry ) const;



        virtual bool is_immune_field( const field_type_id & ) const {
            return false;
        }


        bool is_immune_fields( const std::vector<field_type_id> &fields ) const;


        virtual bool check_immunity_data( const field_immunity_data & ) const {
            return false;
        }


        virtual float fall_damage_mod() const = 0;

        virtual int impact( int force, const tripoint_bub_ms &pos ) = 0;











        void check_dead_state( map *here );


        virtual bool move_effects( bool attacking ) = 0;



        void add_effect( const effect &eff, bool force = false, bool deferred = false ) {
            add_effect( effect_source::empty(), eff, force, deferred );
        }
        void add_effect( const efftype_id &eff_id, const time_duration &dur, bodypart_id bp,
                         bool permanent = false, int intensity = 0, bool force = false, bool deferred = false ) {
            add_effect( effect_source::empty(), eff_id, dur, bp, permanent, intensity, force, deferred );
        }
        void add_effect( const efftype_id &eff_id, const time_duration &dur, bool permanent = false,
                         int intensity = 0, bool force = false, bool deferred = false ) {
            add_effect( effect_source::empty(), eff_id, dur, permanent, intensity, force, deferred );
        }

        void add_effect( const effect_source &source, const effect &eff, bool force = false,
                         bool deferred = false );


        void add_effect( const effect_source &source, const efftype_id &eff_id, const time_duration &dur,
                         bodypart_id bp, bool permanent = false, int intensity = 0, bool force = false,
                         bool deferred = false );
        void add_effect( const effect_source &source, const efftype_id &eff_id, const time_duration &dur,
                         bool permanent = false, int intensity = 0, bool force = false, bool deferred = false );


        void schedule_effect( const effect &eff, bool force = false,
                              bool deferred = false );
        void schedule_effect( const efftype_id &eff_id, const time_duration &dur,
                              bodypart_id bp, bool permanent = false, int intensity = 0, bool force = false,
                              bool deferred = false );
        void schedule_effect( const efftype_id &eff_id, const time_duration &dur,
                              bool permanent = false, int intensity = 0, bool force = false, bool deferred = false );

        bool add_env_effect( const efftype_id &eff_id, const bodypart_id &vector, int strength,
                             const time_duration &dur, const bodypart_id &bp, bool permanent = false, int intensity = 1,
                             bool force = false );
        bool add_env_effect( const efftype_id &eff_id, const bodypart_id &vector, int strength,
                             const time_duration &dur, bool permanent = false, int intensity = 1, bool force = false );



        bool remove_effect( const efftype_id &eff_id, const bodypart_id &bp );
        bool remove_effect( const efftype_id &eff_id );

        void schedule_effect_removal( const efftype_id &eff_id, const bodypart_id &bp );
        void schedule_effect_removal( const efftype_id &eff_id );

        void clear_effects();


        bool has_effect( const efftype_id &eff_id, const bodypart_id &bp ) const;
        bool has_effect( const efftype_id &eff_id ) const;

        bool has_effect_with_flag( const flag_id &flag, const bodypart_id &bp ) const;
        bool has_effect_with_flag( const flag_id &flag ) const;
        std::vector<std::reference_wrapper<const effect>> get_effects_with_flag(
                    const flag_id &flag ) const;
        std::vector<std::reference_wrapper<const effect>> get_effects_from_bp(
                    const bodypart_id &bp ) const;
        std::vector<std::reference_wrapper<const effect>> get_effects() const;


        const effect &get_effect( const efftype_id &eff_id,
                                  const bodypart_id &bp = bodypart_str_id::NULL_ID() ) const;
        effect &get_effect( const efftype_id &eff_id, const bodypart_id &bp = bodypart_str_id::NULL_ID() );

        time_duration get_effect_dur( const efftype_id &eff_id,
                                      const bodypart_id &bp = bodypart_str_id::NULL_ID() ) const;

        int get_effect_int( const efftype_id &eff_id,
                            const bodypart_id &bp = bodypart_str_id::NULL_ID() ) const;

        bool resists_effect( const effect &e ) const;


        void set_value( const std::string &key, diag_value value );
        template <typename... Args>
        void set_value( const std::string &key, Args... args ) {
            set_value( key, diag_value{ std::forward<Args>( args )... } );
        }
        void remove_value( const std::string &key );
        diag_value const &get_value( const std::string &key ) const;
        diag_value const *maybe_get_value( const std::string &key ) const;
        void clear_values();

        virtual units::mass get_weight() const = 0;


        virtual void process_effects();


        virtual bool has_trait( const trait_id &flag ) const;


        virtual int mod_pain( int npain, bodypart_id bp = bodypart_id() );
        virtual void mod_pain_noresist( int npain );
        virtual void set_pain( int npain );
        virtual int get_pain() const;
        virtual int get_perceived_pain() const;

        int get_moves() const;
        void mod_moves( int nmoves );
        void set_moves( int nmoves );

        virtual bool in_sleep_state() const;





        virtual Creature *get_killer() const;





        virtual int get_num_blocks() const;
        virtual int get_num_dodges() const;
        virtual int get_num_free_dodges() const;
        virtual int get_num_blocks_bonus() const;
        virtual int get_num_dodges_bonus() const;
        virtual int get_num_dodges_base() const;
        virtual int get_num_blocks_base() const;

        virtual int get_env_resist( bodypart_id bp ) const;

        virtual int get_armor_res( const damage_type_id &dt, bodypart_id bp ) const;
        virtual int get_armor_res_base( const damage_type_id &dt, bodypart_id bp ) const;
        virtual int get_armor_res_bonus( const damage_type_id &dt ) const;
        virtual int get_spell_resist() const;

        virtual int get_armor_type( const damage_type_id &dt, bodypart_id bp ) const = 0;

        virtual float get_dodge() const;
        virtual float get_melee() const = 0;
        virtual float get_hit() const;

        virtual int get_speed() const;
        virtual int get_eff_per() const;
        virtual creature_size get_size() const = 0;
        virtual int get_hp( const bodypart_id &bp ) const;
        virtual int get_hp() const;
        virtual int get_hp_max( const bodypart_id &bp ) const;
        virtual int get_hp_max() const;
        virtual int hp_percentage() const = 0;
        virtual bool made_of( const material_id &m ) const = 0;
        virtual bool made_of_any( const std::set<material_id> &ms ) const = 0;

        static const std::set<material_id> cmat_flesh;
        static const std::set<material_id> cmat_fleshnveg;
        static const std::set<material_id> cmat_flammable;
        static const std::set<material_id> cmat_flameres;
        virtual field_type_id bloodType() const = 0;
        virtual field_type_id gibType() const = 0;

        virtual bool has_flag( const mon_flag_id & ) const {
            return false;
        }
        virtual bool has_flag( const flag_id & ) const {
            return false;
        }
        virtual bool uncanny_dodge() {
            return false;
        }
        virtual bool check_avoid_friendly_fire() const {
            return false;
        }
        void set_reachable_zone( int zone ) {
            reachable_zone = zone;
        }
        int get_reachable_zone() const {
            return reachable_zone;
        }

    private:




        int reachable_zone = 0;


        tripoint_abs_ms location;
        lazy<safe_reference_anchor> anchor;
    protected:

        void set_pos_bub_only( const map &here, const tripoint_bub_ms &p );

    public:

        void set_pos_abs_only( const tripoint_abs_ms &loc );
        safe_reference<Creature> get_safe_reference();
    protected:

        virtual void on_move( const tripoint_abs_ms &old_pos );

        anatomy_id creature_anatomy = anatomy_id( "default_anatomy" );

        std::map<bodypart_str_id, bodypart> body;
    public:
        anatomy_id get_anatomy() const;
        void set_anatomy( const anatomy_id &anat );

        bodypart_id get_random_body_part( bool main = false ) const;



        std::vector<bodypart_id> get_all_body_parts(
            get_body_part_flags = get_body_part_flags::none ) const;
        std::vector<bodypart_id> get_all_body_parts_of_type(
            bp_type part_type,
            get_body_part_flags flags = get_body_part_flags::none ) const;
        bodypart_id get_random_body_part_of_type( bp_type part_type ) const;
        bodypart_id get_root_body_part() const;

        std::vector<bodypart_id> get_all_body_parts_with_flag( const json_character_flag &flag ) const;

        body_part_set get_drenching_body_parts( bool upper = true, bool mid = true,
                                                bool lower = true ) const;


        std::vector<bodypart_id> get_ground_contact_bodyparts( bool arms_legs = false ) const;

        std::string string_for_ground_contact_bodyparts( const std::vector<bodypart_id> &bps ) const;


        int get_num_body_parts_of_type( bp_type part_type ) const;


        int get_num_broken_body_parts_of_type( bp_type part_type ) const;

        const std::map<bodypart_str_id, bodypart> &get_body() const;
        void set_body();

        bool has_part( const bodypart_id &id, body_part_filter filter = body_part_filter::strict ) const;


        bodypart *get_part( const bodypart_id &id );
        const bodypart *get_part( const bodypart_id &id ) const;


        bodypart_id get_part_id( const bodypart_id &id,
                                 body_part_filter filter = body_part_filter::next_best, bool suppress_debugmsg = false ) const;

        int get_part_hp_cur( const bodypart_id &id ) const;
        int get_part_hp_max( const bodypart_id &id ) const;

        int get_part_healed_total( const bodypart_id &id ) const;
        int get_part_damage_disinfected( const bodypart_id &id ) const;
        int get_part_damage_bandaged( const bodypart_id &id ) const;
        int get_part_drench_capacity( const bodypart_id &id ) const;
        int get_part_wetness( const bodypart_id &id ) const;
        units::temperature get_part_temp_cur( const bodypart_id &id ) const;
        units::temperature get_part_temp_conv( const bodypart_id &id ) const;
        int get_part_frostbite_timer( const bodypart_id &id )const;

        std::array<int, NUM_WATER_TOLERANCE> get_part_mut_drench( const bodypart_id &id ) const;

        float get_part_wetness_percentage( const bodypart_id &id ) const;

        bool compare_encumbrance_data( const bodypart_id &id_a, const bodypart_id &id_b ) const;
        int get_part_encumbrance( const bodypart_id &id ) const;
        int get_part_layer_penalty( const bodypart_id &id ) const;

        virtual void set_part_hp_cur( const bodypart_id &id, int set );
        void set_part_hp_max( const bodypart_id &id, int set );
        void set_part_healed_total( const bodypart_id &id, int set );
        void set_part_damage_disinfected( const bodypart_id &id, int set );
        void set_part_damage_bandaged( const bodypart_id &id, int set );

        void set_part_encumbrance_data( const bodypart_id &id, const encumbrance_data &set );

        void set_part_wetness( const bodypart_id &id, int set );
        void set_part_temp_cur( const bodypart_id &id, units::temperature set );
        void set_part_temp_conv( const bodypart_id &id, units::temperature set );
        void set_part_frostbite_timer( const bodypart_id &id, int set );

        void set_part_mut_drench( const bodypart_id &id, std::pair<water_tolerance, int> set );

        virtual void mod_part_hp_cur( const bodypart_id &id, int mod );
        void mod_part_hp_max( const bodypart_id &id, int mod );
        void mod_part_healed_total( const bodypart_id &id, int mod );
        void mod_part_damage_disinfected( const bodypart_id &id, int mod );
        void mod_part_damage_bandaged( const bodypart_id &id, int mod );
        void mod_part_wetness( const bodypart_id &id, int mod );
        void mod_part_temp_cur( const bodypart_id &id, units::temperature_delta mod );
        void mod_part_temp_conv( const bodypart_id &id, units::temperature_delta mod );
        void mod_part_frostbite_timer( const bodypart_id &id, int mod );

        void set_all_parts_temp_cur( units::temperature set );
        void set_all_parts_temp_conv( units::temperature set );
        void set_all_parts_wetness( int set );
        void set_all_parts_hp_cur( int set );
        void set_all_parts_hp_to_max();

        bool has_atleast_one_wet_part() const;
        bool is_part_at_max_hp( const bodypart_id &id ) const;

        virtual int get_speed_base() const;
        virtual int get_speed_bonus() const;
        virtual int get_block_bonus() const;
        virtual int get_bash_bonus() const;
        virtual int get_cut_bonus() const;

        virtual float get_dodge_base() const = 0;
        virtual float get_hit_base() const = 0;
        virtual float get_dodge_bonus() const;
        virtual float get_hit_bonus() const;

        virtual float get_bash_mult() const;
        virtual float get_cut_mult() const;

        virtual bool get_melee_quiet() const;
        virtual bool has_grab_break_tec() const = 0;
        virtual int get_throw_resist() const;

        pimpl<enchant_cache> enchantment_cache;



        virtual void mod_stat( const std::string &stat, float modifier );

        virtual void set_num_blocks_bonus( int nblocks );
        virtual void mod_num_blocks_bonus( int nblocks );
        virtual void mod_num_dodges_bonus( int ndodges );

        virtual void set_armor_res_bonus( int narm, const damage_type_id &dt );

        virtual void set_speed_base( int nspeed );
        virtual void set_speed_bonus( int nspeed );
        virtual void set_block_bonus( int nblock );
        virtual void set_bash_bonus( int nbash );
        virtual void set_cut_bonus( int ncut );

        virtual void mod_speed_bonus( int nspeed );
        virtual void mod_block_bonus( int nblock );
        virtual void mod_bash_bonus( int nbash );
        virtual void mod_cut_bonus( int ncut );
        virtual void mod_size_bonus( int nsize );

        virtual void set_dodge_bonus( float ndodge );
        virtual void set_hit_bonus( float nhit );

        virtual void mod_dodge_bonus( float ndodge );
        virtual void mod_hit_bonus( float  nhit );

        virtual void set_bash_mult( float nbashmult );
        virtual void set_cut_mult( float ncutmult );

        virtual void set_melee_quiet( bool nquiet );
        virtual void set_throw_resist( int nthrowres );

        virtual units::mass weight_capacity() const;


        virtual const pathfinding_settings &get_pathfinding_settings() const = 0;

        virtual std::function<bool( const tripoint_bub_ms & )> get_path_avoid() const = 0;

        bool underwater;
        void draw( const catacurses::window &w, const point_bub_ms &origin, bool inverted ) const;
        void draw( const catacurses::window &w, const tripoint_bub_ms &origin, bool inverted ) const;











        virtual int print_info( const catacurses::window &w, int vStart, int vLines, int column ) const = 0;














        virtual void add_msg_if_player( const std::string & ) const {}
        virtual void add_msg_if_player( const game_message_params & ,
                                        const std::string & ) const {}
        void add_msg_if_player( const translation & ) const;
        void add_msg_if_player( const game_message_params & , const translation & ) const;
        template<typename ...Args>
        void add_msg_if_player( const char *const msg, Args &&... args ) const {
            return add_msg_if_player( string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_player( const std::string &msg, Args &&... args ) const {
            return add_msg_if_player( string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_player( const translation &msg, Args &&... args ) const {
            return add_msg_if_player( string_format( msg.translated(), std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_player( const game_message_params &params, const char *const msg,
                                Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_player( params, string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_player( const game_message_params &params, const std::string &msg,
                                Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_player( params, string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_player( const game_message_params &params, const translation &msg,
                                Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_player( params, string_format( msg.translated(),
                                      std::forward<Args>( args )... ) );
        }

        virtual void add_msg_if_npc( const std::string & ) const {}
        virtual void add_msg_if_npc( const game_message_params & ,
                                     const std::string & ) const {}
        void add_msg_if_npc( const translation & ) const;
        void add_msg_if_npc( const game_message_params & , const translation & ) const;
        template<typename ...Args>
        void add_msg_if_npc( const char *const msg, Args &&... args ) const {
            return add_msg_if_npc( string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_npc( const std::string &msg, Args &&... args ) const {
            return add_msg_if_npc( string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_npc( const translation &msg, Args &&... args ) const {
            return add_msg_if_npc( string_format( msg.translated(), std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_npc( const game_message_params &params, const char *const msg,
                             Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_npc( params, string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_npc( const game_message_params &params, const std::string &msg,
                             Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_npc( params, string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_npc( const game_message_params &params, const translation &msg,
                             Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_npc( params, string_format( msg.translated(), std::forward<Args>( args )... ) );
        }

        virtual void add_msg_player_or_npc( const std::string & ,
                                            const std::string & ) const {}
        virtual void add_msg_player_or_npc( const game_message_params & ,
                                            const std::string & ,
                                            const std::string & ) const {}
        void add_msg_player_or_npc( const translation & ,
                                    const translation & ) const;
        void add_msg_player_or_npc( const game_message_params & ,
                                    const translation & ,
                                    const translation & ) const;
        template<typename ...Args>
        void add_msg_player_or_npc( const char *const player_msg, const char *const npc_msg,
                                    const Args &... args ) const {
            return add_msg_player_or_npc( string_format( player_msg, args... ),
                                          string_format( npc_msg, args... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_npc( const std::string &player_msg, const std::string &npc_msg,
                                    const Args &... args ) const {
            return add_msg_player_or_npc( string_format( player_msg, args... ),
                                          string_format( npc_msg, args... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_npc( const translation &player_msg, const translation &npc_msg,
                                    const Args &... args ) const {
            return add_msg_player_or_npc( string_format( player_msg.translated(), args... ),
                                          string_format( npc_msg.translated(), args... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_npc( const game_message_params &params, const char *const player_msg,
                                    const char *const npc_msg, const Args &... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_npc( params, string_format( player_msg, args... ),
                                          string_format( npc_msg, args... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_npc( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_msg, const Args &... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_npc( params, string_format( player_msg, args... ),
                                          string_format( npc_msg, args... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_npc( const game_message_params &params, const translation &player_msg,
                                    const translation &npc_msg, const Args &... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_npc( params, string_format( player_msg.translated(), args... ),
                                          string_format( npc_msg.translated(), args... ) );
        }

        virtual void add_msg_player_or_say( const std::string & ,
                                            const std::string & ) const {}
        virtual void add_msg_player_or_say( const game_message_params & ,
                                            const std::string & ,
                                            const std::string & ) const {}
        void add_msg_player_or_say( const translation & ,
                                    const translation & ) const;
        void add_msg_player_or_say( const game_message_params & ,
                                    const translation & ,
                                    const translation & ) const;
        template<typename ...Args>
        void add_msg_player_or_say( const char *const player_msg, const char *const npc_speech,
                                    const Args &... args ) const {
            return add_msg_player_or_say( string_format( player_msg, args... ),
                                          string_format( npc_speech, args... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_say( const std::string &player_msg, const std::string &npc_speech,
                                    const Args &... args ) const {
            return add_msg_player_or_say( string_format( player_msg, args... ),
                                          string_format( npc_speech, args... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_say( const translation &player_msg, const translation &npc_speech,
                                    const Args &... args ) const {
            return add_msg_player_or_say( string_format( player_msg.translated(), args... ),
                                          string_format( npc_speech.translated(), args... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_say( const game_message_params &params, const char *const player_msg,
                                    const char *const npc_speech, const Args &... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_say( params, string_format( player_msg, args... ),
                                          string_format( npc_speech, args... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_say( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_speech, const Args &... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_say( params, string_format( player_msg, args... ),
                                          string_format( npc_speech, args... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_say( const game_message_params &params, const translation &player_msg,
                                    const translation &npc_speech, const Args &... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_say( params, string_format( player_msg.translated(), args... ),
                                          string_format( npc_speech.translated(), args... ) );
        }

        virtual std::vector<std::string> extended_description() const = 0;


        virtual nc_color symbol_color() const = 0;

        virtual nc_color basic_symbol_color() const = 0;

        virtual const std::string &symbol() const = 0;
        virtual bool is_symbol_highlighted() const;

        global_variables::impl_t &get_values();
        void clear_killer();

        void set_summon_time( const time_duration &length );
        time_point get_summon_time();

        void decrement_summon_timer();
        void set_summoner( Creature *summoner );
        void set_summoner( character_id summoner );
        Creature *get_summoner() const;

        void migrate_effects();
    protected:



        int moves;
        Creature *killer;
        void set_killer( Creature *killer );
        std::optional<time_point> lifespan_end = std::nullopt;
        std::optional<character_id> summoner = std::nullopt;




        virtual void process_one_effect( effect &e, bool is_new ) = 0;

        pimpl<effects_map> effects;
        std::queue<scheduled_effect, std::list<scheduled_effect>> scheduled_effects;
        std::queue<terminating_effect, std::list<terminating_effect>> terminating_effects;

        std::vector<damage_over_time_data> damage_over_time_map;


        global_variables::impl_t values;




        int num_blocks = 0;
        int num_dodges = 0;

        int num_free_dodges = 0;
        int num_blocks_bonus = 0;
        int num_dodges_bonus = 0;

        std::map<damage_type_id, float> armor_bonus;





        int speed_base = 0;

        int speed_bonus = 0;
        float dodge_bonus = 0.0f;
        int block_bonus = 0;
        float hit_bonus = 0.0f;
        int bash_bonus = 0;
        int cut_bonus = 0;
        int size_bonus = 0;

        float bash_mult = 0.0f;
        float cut_mult = 0.0f;
        bool melee_quiet = false;

        int throw_resist = 0;

        time_point last_updated;

        bool fake = false;
        Creature();
        Creature( const Creature & );
        Creature( Creature && ) noexcept( map_is_noexcept );
        Creature &operator=( const Creature & );
        Creature &operator=( Creature && ) noexcept;

        virtual void on_stat_change( const std::string &, int ) {}
        virtual void on_effect_int_change( const efftype_id &, int, const bodypart_id & ) {}
        virtual void on_damage_of_type( const effect_source &, int, const damage_type_id &,
                                        const bodypart_id & ) {}

    public:


        int archery_aim_counter = 0;


        short times_combatted_player = 0;


        bodypart_id get_max_hitsize_bodypart() const;

        bodypart_id select_body_part( int min_hit, int max_hit, bool can_attack_high, int hit_roll ) const;
        bodypart_id select_blocking_part( bool arm, bool leg, bool nonstandard ) const;
        bodypart_id random_body_part( bool main_parts_only = false ) const;
        std::vector<bodypart_id> get_all_eligable_parts( int min_hit, int max_hit,
                bool can_attack_high ) const;

        void add_damage_over_time( const damage_over_time_data &DoT );
        void process_damage_over_time();

        static void load_hit_range( const JsonObject & );

        static std::vector <int> dispersion_for_even_chance_of_good_hit;






        std::string replace_with_npc_name( std::string input ) const;




        tripoint_abs_ms pos_abs() const;



        tripoint_abs_sm pos_abs_sm() const;



        tripoint_abs_omt pos_abs_omt() const;
    protected:


























        void store( JsonOut &jsout ) const;

        void load( const JsonObject &jsin );

    private:
        int pain;

        double accuracy_projectile_attack( const int &speed, const double &missed_by ) const;

        projectile_attack_results select_body_part_projectile_attack( const projectile &proj,
                double goodhit, bool magic, double missed_by,
                const weakpoint_attack &attack ) const;

        void messaging_projectile_attack( const Creature *source,
                                          const projectile_attack_results &hit_selection, int total_damage ) const;
        void print_proj_avoid_msg( Creature *source, viewer &player_view ) const;
};
std::unique_ptr<talker> get_talker_for( Creature &me );
std::unique_ptr<const_talker> get_const_talker_for( const Creature &me );
std::unique_ptr<talker> get_talker_for( Creature *me );
#endif
