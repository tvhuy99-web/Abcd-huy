#pragma once
#ifndef CATA_SRC_MAGIC_H
#define CATA_SRC_MAGIC_H

#include <functional>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "body_part_set.h"
#include "bodypart.h"
#include "color.h"
#include "coordinates.h"
#include "damage.h"
#include "dialogue_helpers.h"
#include "enum_bitset.h"
#include "event_subscriber.h"
#include "magic_type.h"
#include "point.h"
#include "sounds.h"
#include "translation.h"
#include "type_id.h"
#include "uilist.h"

class Character;
class Creature;
class JsonObject;
class JsonOut;
class spell;
class time_duration;
class vehicle;
struct const_dialogue;
struct dealt_projectile_attack;
struct requirement_data;

namespace spell_effect
{
struct override_parameters;
}
template <typename E> struct enum_traits;

enum class spell_flag : int {
    PERMANENT,
    PERMANENT_ALL_LEVELS,
    PERCENTAGE_DAMAGE,
    SPLIT_DAMAGE,
    IGNORE_WALLS,
    NO_PROJECTILE,
    SWAP_POS,
    HOSTILE_SUMMON,
    HOSTILE_50,
    POLYMORPH_GROUP,
    FRIENDLY_POLY,
    SILENT,
    NO_EXPLOSION_SFX,
    LOUD,
    VERBAL,
    SOMATIC,
    NO_HANDS,
    UNSAFE_TELEPORT,
    TARGET_TELEPORT,
    NO_LEGS,
    CONCENTRATE,
    TOUCH_REQUIRED,
    RANDOM_AOE,
    RANDOM_DAMAGE,
    RANDOM_DURATION,
    RANDOM_TARGET,
    RANDOM_CRITTER,
    MUTATE_TRAIT,
    WONDER,
    EXTRA_EFFECTS_FIRST,
    PAIN_NORESIST,
    NO_FAIL,
    HIDDEN_SPELL,
    SPAWN_GROUP,
    IGNITE_FLAMMABLE,
    MUST_HAVE_CLASS_TO_LEARN,
    SPAWN_WITH_DEATH_DROPS,
    NO_CORPSE_QUIET,
    NON_MAGICAL,
    PSIONIC,
    RECHARM,
    CHARM_PET,
    EVOCATION_SPELL,
    CHANNELING_SPELL,
    CONJURATION_SPELL,
    ENHANCEMENT_SPELL,
    ENERVATION_SPELL,
    CONVEYANCE_SPELL,
    RESTORATION_SPELL,
    TRANSFORMATION_SPELL,
    LAST
};

enum class spell_target : int {
    ally,
    hostile,
    self,
    ground,
    none,
    item,
    field,
    vehicle,
    num_spell_targets
};

enum class spell_shape : int {

    blast,

    line,

    cone,
    num_shapes
};

template<>
struct enum_traits<spell_shape> {
    static constexpr spell_shape last = spell_shape::num_shapes;
};

template<>
struct enum_traits<spell_target> {
    static constexpr spell_target last = spell_target::num_spell_targets;
};

template<>
struct enum_traits<spell_flag> {
    static constexpr spell_flag last = spell_flag::LAST;
};

struct fake_spell {
    spell_id id;

    static const std::optional<int> max_level_default;


    std::optional<int> max_level;

    static const int level_default;

    int level = level_default;

    static const bool self_default;

    bool self = self_default;

    static const int trigger_once_in_default;

    int trigger_once_in = trigger_once_in_default;

    translation trigger_message; // NOLINT(cata-serialize)

    translation npc_trigger_message; // NOLINT(cata-serialize)

    fake_spell() = default;
    explicit fake_spell( const spell_id &sp_id, bool hit_self = false,
                         const std::optional<int> &max_level = std::nullopt ) : id( sp_id ),
        max_level( max_level ), self( hit_self ) {}

    bool operator==( const fake_spell &rhs ) const {
        return id == rhs.id &&
               max_level == rhs.max_level &&
               level == rhs.level &&
               self == rhs.self &&
               trigger_once_in == rhs.trigger_once_in &&
               trigger_message == rhs.trigger_message &&
               npc_trigger_message == rhs.npc_trigger_message;
    }


    spell get_spell( const Creature &caster, int min_level_override = 0 ) const;
    spell get_spell() const;

    bool is_valid() const;
    void load( const JsonObject &jo );
    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &data );
};

class spell_events : public event_subscriber
{
    public:
        using event_subscriber::notify;
        void notify( const cata::event & ) override;
};

// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class spell_type
{
    public:

        spell_type() = default;

        bool was_loaded = false;

        spell_id id;
        // NOLINTNEXTLINE(cata-serialize)
        std::vector<std::pair<spell_id, mod_id>> src;
        mod_id src_mod;

        translation name;

        translation description;

        translation message;

        translation sound_description;
        skill_id skill;

        std::optional<magic_type_id> magic_type;

        requirement_id spell_components;

        sounds::sound_t sound_type = sounds::sound_t::LAST;
        bool sound_ambient = false;
        std::string sound_id;
        std::string sound_variant;

        std::string effect_name;
        bool teachable;
        // NOLINTNEXTLINE(cata-serialize)
        std::function<void( const spell &, Creature &, const tripoint_bub_ms & )> effect;
        std::function<std::set<tripoint_bub_ms>( const spell_effect::override_parameters &params,
                const tripoint_bub_ms &source, const tripoint_bub_ms &target )>
        spell_area_function; // NOLINT(cata-serialize)

        spell_shape spell_area = spell_shape::line;

        std::string effect_str;

        std::vector<fake_spell> additional_spells;

        int channelling_turns = 0;
        std::string channel_spell;
        std::string channel_end_spell;
        std::string channel_interrupt_spell;
        bool channel_uses_energy = true;


        std::optional<field_type_id> field = std::nullopt;

        dbl_or_var field_chance;

        dbl_or_var min_field_intensity;

        dbl_or_var field_intensity_increment;

        dbl_or_var max_field_intensity;

        dbl_or_var field_intensity_variance;





        dbl_or_var min_accuracy;
        dbl_or_var accuracy_increment;
        dbl_or_var max_accuracy;


        dbl_or_var min_damage;

        dbl_or_var damage_increment;

        dbl_or_var max_damage;
        std::pair<int, int> damage_at_max_level()
        const;
        std::pair<float, float> calculate_damage_increment()
        const;


        dbl_or_var min_range;

        dbl_or_var range_increment;

        dbl_or_var max_range;



        dbl_or_var min_aoe;

        dbl_or_var aoe_increment;

        dbl_or_var max_aoe;




        dbl_or_var min_dot;

        dbl_or_var dot_increment;

        dbl_or_var max_dot;




        dbl_or_var min_duration;


        dbl_or_var duration_increment;

        dbl_or_var max_duration;





        dbl_or_var min_pierce;

        dbl_or_var pierce_increment;

        dbl_or_var max_pierce;

        dbl_or_var min_bash_scaling;
        dbl_or_var bash_scaling_increment;
        dbl_or_var max_bash_scaling;


        dbl_or_var base_energy_cost;

        dbl_or_var energy_increment;

        dbl_or_var final_energy_cost;



        trait_id spell_class;


        dbl_or_var difficulty;


        dbl_or_var multiple_projectiles;


        dbl_or_var max_level;


        dbl_or_var base_casting_time;

        dbl_or_var casting_time_increment;

        dbl_or_var final_casting_time;


        std::map<std::string, int> learn_spells;


        magic_energy_type get_energy_source() const;


        vitamin_id vitamin_energy_source() const;


        nc_color energy_color() const;


        std::string energy_string() const;

        damage_type_id dmg_type = damage_type_id::NULL_ID();


        enum_bitset<spell_target> valid_targets;


        std::function<bool( const_dialogue const & )> caster_condition; // NOLINT(cata-serialize)
        bool has_caster_condition = false; // NOLINT(cata-serialize)

        translation caster_condition_fail_message_; // NOLINT(cata-serialize)


        std::function<bool( const_dialogue const & )> target_condition; // NOLINT(cata-serialize)
        bool has_target_condition = false; // NOLINT(cata-serialize)

        translation target_condition_fail_message_; // NOLINT(cata-serialize)

        std::set<mtype_id> targeted_monster_ids;

        std::set<species_id> targeted_species_ids;

        std::set<species_id> ignored_species_ids;


        body_part_set affected_bps;

        std::set<std::string> flags;


        enum_bitset<spell_flag> spell_tags; // NOLINT(cata-serialize)

        static void load_spell( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, std::string_view src );



        static const std::vector<spell_type> &get_all();
        static void check_consistency();
        static void finalize_all();
        static void reset_all();
        bool is_valid() const;


        std::optional<jmath_func_id> overall_get_level_formula_id() const;
        std::optional<jmath_func_id> overall_exp_for_level_formula_id() const;


        int exp_for_level( int level ) const;

        int get_level( int experience ) const;

        std::optional<int> get_max_book_level() const;

        int get_difficulty( const Creature &caster ) const;
    private:

        static const skill_id skill_default;
        static const requirement_id spell_components_default;
        static const translation message_default;
        static const translation sound_description_default;
        static const sounds::sound_t sound_type_default;
        static const bool sound_ambient_default;
        static const std::string sound_id_default;
        static const std::string sound_variant_default;
        static const std::string effect_str_default;
        static const std::optional<field_type_id> field_default;
        static const int field_chance_default;
        static const int min_field_intensity_default;
        static const int max_field_intensity_default;
        static const float field_intensity_increment_default;
        static const float field_intensity_variance_default;
        static const int min_accuracy_default;
        static const float accuracy_increment_default;
        static const int max_accuracy_default;
        static const int min_damage_default;
        static const float damage_increment_default;
        static const int max_damage_default;
        static const int min_range_default;
        static const float range_increment_default;
        static const int max_range_default;
        static const int min_aoe_default;
        static const float aoe_increment_default;
        static const int max_aoe_default;
        static const int min_dot_default;
        static const float dot_increment_default;
        static const int max_dot_default;
        static const int min_duration_default;
        static const int duration_increment_default;
        static const int max_duration_default;
        static const int min_pierce_default;
        static const float pierce_increment_default;
        static const int max_pierce_default;
        static const float min_bash_scaling_default;
        static const float max_bash_scaling_default;
        static const float bash_scaling_increment_default;
        static const int base_energy_cost_default;
        static const float energy_increment_default;
        static const trait_id spell_class_default;
        static const magic_energy_type energy_source_default;
        static const damage_type_id dmg_type_default;
        static const int difficulty_default;
        static const int multiple_projectiles_default;
        static const int max_level_default;
        static const int base_casting_time_default;
        static const float casting_time_increment_default;

        std::optional<magic_energy_type> energy_source;
        std::optional<vitamin_id> vitamin_energy_source_; // NOLINT(cata-serialize)
        std::optional<nc_color> energy_color_; // NOLINT(cata-serialize)
        std::optional<jmath_func_id> get_level_formula_id;
        std::optional<jmath_func_id> exp_for_level_formula_id;
        std::optional<int> max_book_level;
};

class spell
{
    private:
        friend class spell_events;

        spell_id type;


        int experience = 0;

        const damage_type_id &dmg_type() const;


        int temp_level_adjustment = 0; // NOLINT(cata-serialize)
        float temp_damage_multiplyer = 1; // NOLINT(cata-serialize)
        float temp_cast_time_multiplyer = 1; // NOLINT(cata-serialize)
        float temp_spell_cost_multiplyer = 1; // NOLINT(cata-serialize)
        float temp_aoe_multiplyer = 1; // NOLINT(cata-serialize)
        float temp_range_multiplyer = 1; // NOLINT(cata-serialize)
        float temp_duration_multiplyer = 1; // NOLINT(cata-serialize)
        int temp_difficulty_adjustment = 0; // NOLINT(cata-serialize)
        float temp_somatic_difficulty_multiplyer = 1; // NOLINT(cata-serialize)
        float temp_sound_multiplyer = 1; // NOLINT(cata-serialize)
        float temp_concentration_difficulty_multiplyer = 1; // NOLINT(cata-serialize)



        translation alt_message;

        double min_leveled_dot( const Creature &caster ) const;

        int min_leveled_aoe( const Creature &caster ) const;

        int min_leveled_duration( const Creature &caster ) const;
        int min_leveled_accuracy( const Creature &caster ) const;

    public:

        int min_leveled_damage( const Creature &caster ) const;
        float get_eoc_damage_multiplier() const;

        spell() = default;
        explicit spell( spell_id sp, int xp = 0, int level_adjustment = 0 );


        void set_message( const translation &msg );

        double bash_scaling( const Creature &caster ) const;

        int exp_for_level( int level ) const;

        int exp_to_next_level() const;

        std::string exp_progress() const;

        int xp() const;

        void gain_exp( const Character &guy, int nxp );
        void set_exp( int nxp );

        int casting_exp( const Character &guy ) const;

        float exp_modifier( const Character &guy ) const;

        void gain_level( const Character &guy );

        void gain_levels( const Character &guy, int gains );
        void set_level( const Character &guy, int nlevel );

        bool is_max_level( const Creature &caster ) const;

        int get_max_level( const Creature &caster ) const;
        int get_temp_level_adjustment() const;
        void set_temp_level_adjustment( int adjustment );
        void set_temp_adjustment( const std::string &target_property, float adjustment );
        void clear_temp_adjustments();

        spell_shape shape() const;

        int field_intensity( const Creature &caster ) const;

        int damage( const Creature &caster ) const;
        int accuracy( Creature &caster ) const;
        double damage_dot( const Creature &caster ) const;
        damage_over_time_data damage_over_time( const std::vector<bodypart_id> &bps,
                                                const Creature &caster ) const;
        dealt_damage_instance get_dealt_damage_instance( Creature &caster ) const;
        dealt_projectile_attack get_projectile_attack( const tripoint_bub_ms &target,
                Creature &hit_critter, Creature &caster ) const;
        damage_instance get_damage_instance( Creature &caster ) const;

        float dps( const Character &caster, const Creature &target ) const;

        std::optional<tripoint_bub_ms> select_target( Creature *source );

        int aoe( const Creature &caster ) const;
        std::set<tripoint_bub_ms> effect_area( const spell_effect::override_parameters &params,
                                               const tripoint_bub_ms &source, const tripoint_bub_ms &target ) const;
        std::set<tripoint_bub_ms> effect_area( const tripoint_bub_ms &source, const tripoint_bub_ms &target,
                                               const Creature &caster ) const;

        int range( const Creature &caster ) const;





        std::vector<tripoint_bub_ms> targetable_locations( const Character &source ) const;

        int energy_cost( const Character &guy ) const;

        int duration( const Creature &caster ) const;
        time_duration duration_turns( const Creature &caster ) const;


        float spell_fail( const Character &guy ) const;
        std::string colorized_fail_percent( const Character &guy ) const;

        int casting_time( const Character &guy, bool ignore_encumb = false ) const;

        const requirement_data &components() const;
        bool has_components() const;

        bool can_cast( const Character &guy ) const;
        bool can_cast( const Character &guy, std::map<magic_type_id, bool> &success_tracker );

        bool can_learn( const Character &guy ) const;

        int get_amount_of_projectiles( const Creature &guy ) const;

        bool is_valid() const;
        int bps_affected() const;

        bool bp_is_affected( const bodypart_id &bp ) const;

        bool has_flag( const spell_flag &flag ) const;
        bool has_flag( const std::string &flag ) const;
        bool no_hands() const;
        bool is_channeling_spell() const;

        bool is_spell_class( const trait_id &mid ) const;

        bool in_aoe( const tripoint_bub_ms &source, const tripoint_bub_ms &target,
                     const Creature &caster ) const;

        bool casting_time_encumbered( const Character &guy ) const;
        bool energy_cost_encumbered( const Character &guy ) const;
        std::string enumerate_spell_data( const Character &guy ) const;


        spell_id id() const;

        trait_id spell_class() const;

        skill_id skill() const;

        std::string effect() const;

        std::string effect_data() const;

        vproto_id summon_vehicle_id() const;

        std::string name() const;

        std::string description() const;

        std::string message() const;

        std::string energy_string() const;

        std::string energy_cost_string( const Character &guy ) const;

        std::string energy_cur_string( const Character &guy ) const;

        std::string enumerate_targets() const;


        std::string list_targeted_monster_names() const;

        std::string list_targeted_species_names() const;

        std::string list_ignored_species_names() const;

        std::string damage_string( const Character &caster ) const;
        std::string aoe_string( const Creature &caster ) const;
        std::string duration_string( const Creature &caster ) const;


        magic_energy_type energy_source() const;
        std::optional<vitamin_id> vitamin_energy_source() const;
        nc_color energy_color() const;

        nc_color damage_type_color() const;
        std::string damage_type_string() const;

        int get_level() const;

        int get_effective_level() const;

        int get_difficulty( const Creature &caster ) const;
        mod_id get_src() const;

        std::optional<int> max_book_level() const;
        double get_failure_cost_percent( Creature &caster ) const;
        double get_failure_exp_percent( Creature &caster ) const;
        void consume_spell_cost( Character &caster, bool cast_success = true ) const;
        std::vector<effect_on_condition_id> get_failure_eoc_ids() const;


        void create_field( const tripoint_bub_ms &at, Creature &caster ) const;

        int sound_volume( const Creature &caster ) const;

        void make_sound( const tripoint_bub_ms &target, Creature &caster ) const;
        void make_sound( const tripoint_bub_ms &target, int loudness ) const;

        int heal( const tripoint_bub_ms &target, Creature &caster ) const;


        void cast_spell_effect( const tripoint_bub_ms &target ) const;

        void cast_spell_effect( Creature &source, const tripoint_bub_ms &target ) const;

        void cast_all_effects( const tripoint_bub_ms &target ) const;

        void cast_all_effects( Creature &source, const tripoint_bub_ms &target ) const;

        void cast_extra_spell_effects( const tripoint_bub_ms &target ) const;

        void cast_extra_spell_effects( Creature &source, const tripoint_bub_ms &target ) const;

        void use_components( Character &guy ) const;

        bool check_if_component_in_hand( Character &guy ) const;

        bool is_target_in_range( const Creature &caster, const tripoint_bub_ms &p ) const;


        bool is_valid_target( const Creature &caster, const tripoint_bub_ms &p ) const;
        bool is_valid_target( spell_target t ) const;
        bool target_by_monster_id( const tripoint_bub_ms &p ) const;
        bool target_by_species_id( const tripoint_bub_ms &p ) const;
        bool ignore_by_species_id( const tripoint_bub_ms &p ) const;
        bool valid_caster_condition( const Creature &caster ) const;
        bool valid_target_condition( const Creature &caster, const Creature &target ) const;
        bool valid_target_condition( const Creature &caster, const vehicle &veh ) const;


        std::string failed_caster_condition_message() const;
        std::string failed_target_condition_message() const;


        std::optional<tripoint_bub_ms> random_valid_target( const Creature &caster,
                const tripoint_bub_ms &caster_pos ) const;
};

class known_magic
{
    private:

        std::map<spell_id, spell> spellbook;

        std::map<spell_id, int> spells_to_invlets;

        std::map<int, spell_id> invlets_to_spells; // NOLINT(cata-serialize)

        std::unordered_set<spell_id> favorites;

        int mana_base = 0; // NOLINT(cata-serialize)

        int mana = 0;

    public:

        bool casting_ignore = false; // NOLINT(cata-serialize)

        known_magic();

        void learn_spell( const std::string &sp, Character &guy, bool force = false );
        void learn_spell( const spell_id &sp, Character &guy, bool force = false );
        void learn_spell( const spell_type *sp, Character &guy, bool force = false );
        void forget_spell( const std::string &sp );
        void forget_spell( const spell_id &sp );
        void set_spell_level( const spell_id &, int, const Character * );
        void set_spell_exp( const spell_id &, int, const Character * );

        time_duration time_to_learn_spell( const Character &guy, const spell_id &sp ) const;
        time_duration time_to_learn_spell( const Character &guy, const std::string &str ) const;
        bool can_learn_spell( const Character &guy, const spell_id &sp, bool improved_spell = false ) const;
        bool knows_spell( const std::string &sp ) const;
        bool knows_spell( const spell_id &sp ) const;

        bool knows_spell() const;

        std::vector<spell_id> spells() const;

        bool can_cast_any_spell( const Character &guy,
                                 std::map<magic_type_id, bool> &success_tracker );

        spell &get_spell( const spell_id &sp );


        spell &select_spell( Character &guy );

        std::vector<spell *> get_spells();

        spell_id last_spell = spell_id::NULL_ID(); // NOLINT(cata-serialize)

        std::map<spell_id, spell> &get_spellbook() {
            return spellbook;
        }

        int available_mana() const;

        int max_mana( const Character &guy ) const;
        void mod_mana( const Character &guy, int add_mana );
        void set_mana( int new_mana );
        void update_mana( const Character &guy, float turns );


        bool has_enough_energy( const Character &guy, const spell &sp ) const;

        void clear_opens_spellbook_data();

        void evaluate_opens_spellbook_data();
        void channel_magic( Character &guy );
        void break_channeling( Character &guy );

        void on_mutation_gain( const trait_id &mid, Character &guy );
        void on_mutation_loss( const trait_id &mid, Character &guy );


        double caster_level_adjustment; // NOLINT(cata-serialize)
        std::map<spell_id, double> caster_level_adjustment_by_spell; // NOLINT(cata-serialize)
        std::map<trait_id, double> caster_level_adjustment_by_school; // NOLINT(cata-serialize)

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );



        int get_invlet( const spell_id &sp );

        bool set_invlet( const spell_id &sp, int invlet );

        void swap_invlet( const spell_id &new_sp, int invlet );
        void rem_invlet( const spell_id &sp );

        void toggle_favorite( const spell_id &sp );
        bool is_favorite( const spell_id &sp );

        void migrate_spells();
    private:

        int get_spellname_max_width();

        void build_invlets_to_spells();
};

namespace spell_effect
{




struct override_parameters {
    int aoe_radius;
    int range;
    bool ignore_walls;

    explicit override_parameters( const spell &sp, const Creature &caster ) {
        aoe_radius = sp.aoe( caster );
        range = sp.range( caster );
        ignore_walls = sp.has_flag( spell_flag::IGNORE_WALLS );
    }
};

void short_range_teleport( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void pain_split( const spell &, Creature &, const tripoint_bub_ms & );
void attack( const spell &sp, Creature &caster, const tripoint_bub_ms &epicenter );
void targeted_polymorph( const spell &sp, Creature &caster, const tripoint_bub_ms &target );

void area_pull( const spell &sp, Creature &caster, const tripoint_bub_ms &center );
void area_push( const spell &sp, Creature &caster, const tripoint_bub_ms &center );
void directed_push( const spell &sp, Creature &caster, const tripoint_bub_ms &target );

std::set<tripoint_bub_ms> spell_effect_blast( const override_parameters &params,
        const tripoint_bub_ms &, const tripoint_bub_ms &target );
std::set<tripoint_bub_ms> spell_effect_cone( const override_parameters &params,
        const tripoint_bub_ms &source, const tripoint_bub_ms &target );
std::set<tripoint_bub_ms> spell_effect_line( const override_parameters &params,
        const tripoint_bub_ms &source, const tripoint_bub_ms &target );

void spawn_ethereal_item( const spell &sp, Creature &, const tripoint_bub_ms & );
void recover_energy( const spell &sp, Creature &, const tripoint_bub_ms &target );
void spawn_summoned_monster( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void spawn_summoned_vehicle( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void recharge_vehicle( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void fertilize_plant( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void translocate( const spell &sp, Creature &caster, const tripoint_bub_ms &target );

void timed_event( const spell &sp, Creature &caster, const tripoint_bub_ms & );
void transform_blast( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void noise( const spell &sp, Creature &, const tripoint_bub_ms &target );
void vomit( const spell &sp, Creature &caster, const tripoint_bub_ms &target );

void pull_to_caster( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void explosion( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void flashbang( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void mod_moves( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void map( const spell &sp, Creature &caster, const tripoint_bub_ms & );
void morale( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void charm_monster( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void mutate( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void bash( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void dash( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void banishment( const spell &sp, Creature &caster, const tripoint_bub_ms &target );

void revive( const spell &sp, Creature &caster, const tripoint_bub_ms &target );

void revive_dormant( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void add_trap( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void upgrade( const spell &sp, Creature &caster, const tripoint_bub_ms &target );

void guilt( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void remove_effect( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void emit( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void fungalize( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void remove_field( const spell &sp, Creature &caster, const tripoint_bub_ms &center );
void effect_on_condition( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void pickup( const spell &sp, Creature &caster, const tripoint_bub_ms &target );
void none( const spell &sp, Creature &, const tripoint_bub_ms &target );
void slime_split_on_death( const spell &sp, Creature &, const tripoint_bub_ms &target );

inline const std::map<spell_shape, std::function<std::set<tripoint_bub_ms>
( const override_parameters &, const tripoint_bub_ms &, const tripoint_bub_ms & )>> shape_map = {
    { spell_shape::blast, spell_effect_blast },
    { spell_shape::line, spell_effect_line },
    { spell_shape::cone, spell_effect_cone }
};

inline const
std::map<std::string, std::function<void( const spell &, Creature &, const tripoint_bub_ms & )>>
effect_map{
    { "pain_split", spell_effect::pain_split },
    { "attack", spell_effect::attack },
    { "add_trap", spell_effect::add_trap},
    { "targeted_polymorph", spell_effect::targeted_polymorph },
    { "short_range_teleport", spell_effect::short_range_teleport },
    { "spawn_item", spell_effect::spawn_ethereal_item },
    { "recover_energy", spell_effect::recover_energy },
    { "summon", spell_effect::spawn_summoned_monster },
    { "summon_vehicle", spell_effect::spawn_summoned_vehicle },
    { "recharge_vehicle", spell_effect::recharge_vehicle },
    { "fertilize_plant", spell_effect::fertilize_plant },
    { "translocate", spell_effect::translocate },
    { "area_pull", spell_effect::area_pull },
    { "area_push", spell_effect::area_push },
    { "directed_push", spell_effect::directed_push },
    { "timed_event", spell_effect::timed_event },
    { "ter_transform", spell_effect::transform_blast },
    { "noise", spell_effect::noise },
    { "vomit", spell_effect::vomit },
    { "pull_target", spell_effect::pull_to_caster },
    { "explosion", spell_effect::explosion },
    { "flashbang", spell_effect::flashbang },
    { "mod_moves", spell_effect::mod_moves },
    { "map", spell_effect::map },
    { "morale", spell_effect::morale },
    { "charm_monster", spell_effect::charm_monster },
    { "mutate", spell_effect::mutate },
    { "bash", spell_effect::bash },
    { "dash", spell_effect::dash },
    { "banishment", spell_effect::banishment },
    { "revive", spell_effect::revive },
    { "revive_dormant", spell_effect::revive_dormant },
    { "upgrade", spell_effect::upgrade },
    { "guilt", spell_effect::guilt },
    { "remove_effect", spell_effect::remove_effect },
    { "emit", spell_effect::emit },
    { "fungalize", spell_effect::fungalize },
    { "remove_field", spell_effect::remove_field },
    { "effect_on_condition", spell_effect::effect_on_condition },
    { "pickup", spell_effect::pickup },
    { "slime_split", spell_effect::slime_split_on_death },
    { "none", spell_effect::none }
};
}

class spellbook_callback : public uilist_callback
{
    private:
        std::vector<spell_type> spells;
    public:
        void add_spell( const spell_id &sp );
        void refresh( uilist *menu ) override;
        float desired_extra_space_right( ) override;
};




struct area_expander {

    struct node {

        tripoint_bub_ms position;

        tripoint_bub_ms from;

        float cost = 0.0f;
    };

    int max_range = -1;
    int max_expand = -1;


    std::vector<node> area;


    std::map<tripoint_bub_ms, int> area_search;

    struct area_node_comparator {
        explicit area_node_comparator( std::vector<area_expander::node> &area ) : area( area ) {
        }

        bool operator()( int a, int b ) const {
            return area[a].cost < area[b].cost;
        }

        std::vector<area_expander::node> &area;
    };

    std::priority_queue<int, std::vector<int>, area_node_comparator> frontier;

    area_expander();

    int contains( const tripoint_bub_ms &pt ) const;


    bool enqueue( const tripoint_bub_ms &from, const tripoint_bub_ms &to, float cost );


    int run( const tripoint_bub_ms &center );


    void sort_ascending();

    void sort_descending();
};

class spell_migration
{
    public:
        spell_id id_old;
        std::optional<spell_id> id_new;

        static void load( const JsonObject &jo );

        static void reset();

        static void check();

        static const spell_migration *find_migration( const spell_id &original );
};

#endif
