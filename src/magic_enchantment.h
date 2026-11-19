#pragma once
#ifndef CATA_SRC_MAGIC_ENCHANTMENT_H
#define CATA_SRC_MAGIC_ENCHANTMENT_H

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "body_part_set.h"
#include "calendar.h"
#include "color.h"
#include "dialogue_helpers.h"
#include "magic.h"
#include "translation.h"
#include "type_id.h"
#include "units_fwd.h"

class Character;
class Creature;
class JsonObject;
class JsonOut;
class JsonValue;
class item;
class monster;
class vehicle;
struct const_dialogue;

namespace enchant_vals
{


enum class mod : int {

    ARTIFACT_RESONANCE,

    STRENGTH,
    DEXTERITY,
    PERCEPTION,
    INTELLIGENCE,
    SPEED,
    ATTACK_SPEED,
    AIMING_SPEED,
    RELOADING_SPEED,
    MOVE_COST,
    METABOLISM,
    MAX_MANA,
    REGEN_MANA,
    BIONIC_POWER,
    POWER_TRICKLE,
    MAX_STAMINA,
    REGEN_STAMINA,
    FAT_TO_MAX_HP,
    CARDIO_MULTIPLIER,
    MUT_INSTABILITY_MOD,
    MUT_ADDITIONAL_OPTIONS,
    RANGE_DODGE,
    MAX_HP,
    REGEN_HP,
    REGEN_HP_AWAKE,
    HUNGER,
    THIRST,
    SLEEPINESS,
    SLEEPINESS_REGEN,
    PAIN,
    PAIN_REMOVE,
    PAIN_PENALTY_MOD_STR,
    PAIN_PENALTY_MOD_DEX,
    PAIN_PENALTY_MOD_INT,
    PAIN_PENALTY_MOD_PER,
    PAIN_PENALTY_MOD_SPEED,
    DODGE_CHANCE,
    BONUS_DODGE,
    BONUS_BLOCK,
    FREE_DODGES,
    MELEE_DAMAGE,
    MELEE_RANGE_MODIFIER,
    MELEE_TO_HIT,
    SMASH_BONUS,
    RANGED_DAMAGE,
    RANGED_ARMOR_PENETRATION,
    ATTACK_NOISE,
    SHOUT_NOISE,
    FOOTSTEP_NOISE,
    VISION_RANGE,
    CARRY_WEIGHT,
    WEAPON_DISPERSION,
    SOCIAL_LIE,
    SOCIAL_PERSUADE,
    SOCIAL_INTIMIDATE,
    SLEEPY,
    BODYTEMP_SLEEP,
    LUMINATION,
    EFFECTIVE_HEALTH_MOD,
    MOD_HEALTH,
    MOD_HEALTH_CAP,
    HEALTHY_RATE,
    READING_EXP,
    SKILL_RUST_RESIST,
    READING_SPEED_MULTIPLIER,
    OVERMAP_SIGHT,
    KCAL,
    VITAMIN_ABSORB_MOD,
    MELEE_STAMINA_CONSUMPTION,
    OBTAIN_COST_MULTIPLIER,
    CASTING_TIME_MULTIPLIER,
    CRAFTING_SPEED_MULTIPLIER,
    BIONIC_MANA_PENALTY,
    STEALTH_MODIFIER,
    WEAKNESS_TO_WATER,
    WEIGHT,
    MENDING_MODIFIER,
    STOMACH_SIZE_MULTIPLIER,
    LEARNING_FOCUS,
    ARMOR_ALL,
    EXTRA_ELEC_PAIN,
    RECOIL_MODIFIER,
    ITEM_ATTACK_SPEED,
    EQUIPMENT_DAMAGE_CHANCE,
    THEORETICAL_SKILL_CATCHUP_BONUS,
    CLIMATE_CONTROL_HEAT,
    CLIMATE_CONTROL_CHILL,
    COMBAT_CATCHUP,
    KNOCKBACK_RESIST,
    KNOCKDOWN_RESIST,
    FALL_DAMAGE,
    FORCEFIELD,
    EVASION,
    OVERKILL_DAMAGE,
    RANGE,
    AVOID_FRIENDRY_FIRE,
    MOVECOST_SWIM_MOD,
    MOVECOST_OBSTACLE_MOD,
    MOVECOST_FLATGROUND_MOD,
    PHASE_DISTANCE,
    SHOUT_NOISE_STR_MULT,
    NIGHT_VIS,
    HEARING_MULT,
    BANDAGE_BONUS,
    DISINFECTANT_BONUS,
    BLEED_STOP_BONUS,
    UGLINESS,
    VOMIT_MUL,
    SCENT_MASK,
    CONSUME_TIME_MOD,
    THROW_STR,
    THROW_DAMAGE,
    SWEAT_MULTIPLIER,
    STAMINA_REGEN_MOD,
    MOVEMENT_EXERTION_MODIFIER,
    WEAKPOINT_ACCURACY,
    BLEEDING_RATE,
    MOTION_ALARM,
    TOTAL_WEIGHT,
    FUEL_USAGE,
    TURNING_DIFFICULTY,
    NUM_MOD
};
}



class enchantment
{
    public:
        enum has {
            WIELD,
            WORN,
            HELD,
            NUM_HAS
        };

        enum condition {
            ALWAYS,
            ACTIVE,
            INACTIVE,
            DIALOG_CONDITION,
            NUM_CONDITION
        };

        static void load_enchantment( const JsonObject &jo, const std::string &src );
        static void finalize_all();
        static void reset();
        void load( const JsonObject &jo, std::string_view src = {},
                   const std::optional<std::string> &inline_id = std::nullopt, bool is_child = false );



        static enchantment_id load_inline_enchantment( const JsonValue &jv, std::string_view src,
                std::string &inline_id );


        bool is_active( const Character &guy, const item &parent ) const;



        bool is_active( const Character &guy, bool active ) const;


        bool is_active( const monster &mon ) const;

        bool is_monster_relevant() const;


        bool is_active( const vehicle &veh, bool active ) const;

        bool is_vehicle_relevant() const;



        bool active_wield() const;
        enchantment_id id;
        // NOLINTNEXTLINE(cata-serialize)
        std::vector<std::pair<enchantment_id, mod_id>> src;

        bool was_loaded = false;

        const std::vector<trait_id> &get_mutations() const {
            return mutations;
        }
        double get_value_add( enchant_vals::mod value, const Character &guy ) const;
        double get_value_multiply( enchant_vals::mod value, const Character &guy ) const;

        body_part_set modify_bodyparts( const body_part_set &unmodified ) const;

        bool modifies_bodyparts() const;
        struct bodypart_changes {
            bodypart_str_id gain;
            bodypart_str_id lose;
            bool was_loaded = false;
            void serialize( JsonOut &jsout ) const;
            void deserialize( const JsonObject &jo );
            void load( const JsonObject &jo );
        };
        std::vector<bodypart_changes> modified_bodyparts;

        std::vector<trait_id> mutations;
        std::optional<emit_id> emitter;
        std::map<efftype_id, int> ench_effects;


        translation name;


        translation description;


        std::map<enchant_vals::mod, dbl_or_var> values_add; // NOLINT(cata-serialize)


        std::map<enchant_vals::mod, dbl_or_var> values_multiply; // NOLINT(cata-serialize)


        std::map<skill_id, dbl_or_var> skill_values_add; // NOLINT(cata-serialize)
        std::map<skill_id, dbl_or_var> skill_values_multiply; // NOLINT(cata-serialize)

        std::map<bodypart_str_id, dbl_or_var> encumbrance_values_add; // NOLINT(cata-serialize)
        std::map<bodypart_str_id, dbl_or_var> encumbrance_values_multiply; // NOLINT(cata-serialize)

        std::map<damage_type_id, dbl_or_var> damage_values_add; // NOLINT(cata-serialize)
        std::map<damage_type_id, dbl_or_var> damage_values_multiply; // NOLINT(cata-serialize)

        std::map<damage_type_id, dbl_or_var> armor_values_add; // NOLINT(cata-serialize)
        std::map<damage_type_id, dbl_or_var> armor_values_multiply; // NOLINT(cata-serialize)

        std::map<damage_type_id, dbl_or_var> extra_damage_add; // NOLINT(cata-serialize)
        std::map<damage_type_id, dbl_or_var> extra_damage_multiply; // NOLINT(cata-serialize)

        std::vector<fake_spell> hit_me_effect;
        std::vector<fake_spell> hit_you_effect;

        struct special_vision_descriptions {
            std::string id = "infrared_creature";
            nc_color color = c_red;
            std::string symbol = "?";
            std::string text = "You see a medium figure radiating heat.";
            translation description;
            std::function<bool( const_dialogue const & )> condition;
        };

        struct special_vision {
            std::vector<special_vision_descriptions> special_vision_descriptions_vector;
            std::function<bool( const_dialogue const & )> condition;
            dbl_or_var range;
            bool precise = false;
            bool ignores_aiming_cone = false;
            bool is_empty( const_dialogue &d ) const {
                return range.evaluate( d ) <= 0;
            }
        };

        std::vector<special_vision> special_vision_vector;

        special_vision get_vision( const const_dialogue &d ) const;
        bool get_vision_can_see( const enchantment::special_vision &vision_struct,
                                 const_dialogue &d ) const;
        special_vision_descriptions get_vision_description_struct(
            const enchantment::special_vision &vision_struct, const_dialogue &d ) const;

        std::map<time_duration, std::vector<fake_spell>> intermittent_activation;

        std::pair<has, condition> active_conditions;
        std::function<bool( const_dialogue const & )> dialog_condition; // NOLINT(cata-serialize)

        void add_activation( const time_duration &dur, const fake_spell &fake );
};

class enchant_cache : public enchantment
{
    public:

        double modify_value( enchant_vals::mod mod_val, double value ) const;
        double modify_value( const skill_id &mod_val, double value ) const;
        units::energy modify_value( enchant_vals::mod mod_val, units::energy value ) const;
        units::mass modify_value( enchant_vals::mod mod_val, units::mass value ) const;
        units::volume modify_value( enchant_vals::mod mod_val, units::volume value ) const;
        units::power modify_value( enchant_vals::mod mod_val, units::power value ) const;
        units::temperature_delta modify_value( enchant_vals::mod mod_val,
                                               units::temperature_delta value ) const;
        time_duration modify_value( enchant_vals::mod mod_val, time_duration value ) const;

        double modify_encumbrance( const bodypart_str_id &mod_val, double value ) const;
        double modify_melee_damage( const damage_type_id &mod_val, double value ) const;
        double modify_damage_units_by_armor_protection( const damage_type_id &mod_val, double value ) const;
        double modify_damage_units_by_extra_damage( const damage_type_id &mod_val, double value ) const;

        void force_add( const enchantment &rhs, const Character &guy );
        void force_add( const enchantment &rhs, const monster &mon );
        void force_add( const enchantment &rhs, const vehicle &veh );
        void force_add( const enchantment &rhs );
        void force_add( const enchant_cache &rhs );
        void force_add_with_dialogue( const enchantment &rhs, const const_dialogue &d );

        void force_add_mutation( const enchantment &rhs );


        void activate_passive( Character &guy ) const;
        template<typename TKey>
        double get_value( const TKey &value, const std::map<TKey, double> &value_map ) const;
        double get_value_add( enchant_vals::mod value ) const;
        double get_value_multiply( enchant_vals::mod value ) const;
        int mult_bonus( enchant_vals::mod value_type, int base_value ) const;

        double get_skill_value_add( const skill_id &value ) const;
        int get_damage_add( const damage_type_id &value ) const;
        int get_armor_add( const damage_type_id &value ) const;
        int get_encumbrance_add( const bodypart_str_id &value ) const;
        int get_extra_damage_add( const damage_type_id &value ) const;
        double get_skill_value_multiply( const skill_id &value ) const;
        double get_damage_multiply( const damage_type_id &value ) const;
        double get_armor_multiply( const damage_type_id &value ) const;
        double get_encumbrance_multiply( const bodypart_str_id &value ) const;
        double get_extra_damage_multiply( const damage_type_id &value ) const;
        int skill_mult_bonus( const skill_id &value_type, int base_value ) const;


        bool add( const enchantment &rhs, Character &you );
        bool add( const enchant_cache &rhs );

        bool stacks_with( const enchantment &rhs ) const;

        void cast_enchantment_spell( Creature &caster, const Creature *target,
                                     const fake_spell &sp ) const;

        void clear();


        void cast_hit_you( Character &caster, const Creature &target ) const;
        void cast_hit_you( Creature &caster, const Creature &target ) const;

        void cast_hit_me( Character &caster, const Creature *target ) const;
        void cast_hit_me( Creature &caster, const Creature *target ) const;

        template<typename TKey>
        void save_add_and_multiply( JsonOut &jsout, const std::string_view &member_key,
                                    const std::string &type_key, const std::map<TKey, double> &add_map,
                                    const std::map<TKey, double> &mult_map ) const;

        void serialize( JsonOut &jsout ) const;
        void add_value_add( enchant_vals::mod value, float add_value );

        void set_has( enchantment::has value );
        void add_value_mult( enchant_vals::mod value, float mult_value );
        void add_hit_you( const fake_spell &sp );
        void add_hit_me( const fake_spell &sp );
        void load( const JsonObject &jo, std::string_view src = {},
                   const std::optional<std::string> &inline_id = std::nullopt );
        bool operator==( const enchant_cache &rhs ) const;


        std::vector<std::pair<std::string, std::string>> details; // NOLINT(cata-serialize)

        using special_vision_descriptions = enchantment::special_vision_descriptions;

        struct special_vision {
            std::vector<special_vision_descriptions> special_vision_descriptions_vector;
            std::function<bool( const_dialogue const & )> condition;
            double range;
            bool precise = false;
            bool ignores_aiming_cone = false;
            bool is_empty() const {
                return range <= 0;
            }
        };

        std::vector<special_vision> special_vision_vector;

        special_vision get_vision( const const_dialogue &d ) const;
        bool get_vision_can_see( const enchant_cache::special_vision &vision_struct ) const;
        enchant_cache::special_vision_descriptions get_vision_description_struct(
            const enchant_cache::special_vision &vision_struct, const_dialogue &d ) const;

    private:
        std::map<enchant_vals::mod, double> values_add; // NOLINT(cata-serialize)


        std::map<enchant_vals::mod, double> values_multiply; // NOLINT(cata-serialize)


        std::map<skill_id, double> skill_values_add; // NOLINT(cata-serialize)
        std::map<skill_id, double> skill_values_multiply; // NOLINT(cata-serialize)

        std::map<bodypart_str_id, double> encumbrance_values_add; // NOLINT(cata-serialize)
        std::map<bodypart_str_id, double> encumbrance_values_multiply; // NOLINT(cata-serialize)

        std::map<damage_type_id, double> damage_values_add; // NOLINT(cata-serialize)
        std::map<damage_type_id, double> damage_values_multiply; // NOLINT(cata-serialize)

        std::map<damage_type_id, double> armor_values_add; // NOLINT(cata-serialize)
        std::map<damage_type_id, double> armor_values_multiply; // NOLINT(cata-serialize)

        std::map<damage_type_id, double> extra_damage_add; // NOLINT(cata-serialize)
        std::map<damage_type_id, double> extra_damage_multiply; // NOLINT(cata-serialize)

};

template <typename E> struct enum_traits;

template<>
struct enum_traits<enchantment::has> {
    static constexpr enchantment::has last = enchantment::has::NUM_HAS;
};

template<>
struct enum_traits<enchantment::condition> {
    static constexpr enchantment::condition last = enchantment::condition::NUM_CONDITION;
};

template<>
struct enum_traits<enchant_vals::mod> {
    static constexpr enchant_vals::mod last = enchant_vals::mod::NUM_MOD;
};

#endif
