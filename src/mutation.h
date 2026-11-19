#pragma once
#ifndef CATA_SRC_MUTATION_H
#define CATA_SRC_MUTATION_H

#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "damage.h"
#include "enums.h"
#include "memory_fast.h"
#include "point.h"
#include "sleep.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

class Character;
class JsonArray;
class JsonObject;
class JsonOut;
class JsonValue;
class Trait_group;
class item;
class nc_color;
struct const_dialogue;
struct dream;
template <typename E> struct enum_traits;

extern std::vector<dream> dreams;
extern std::map<mutation_category_id, std::vector<trait_id> > mutations_category;

struct dream {
    private:
        std::vector<translation> raw_messages;

    public:
        std::vector<std::string> messages() const;

        mutation_category_id category;
        int strength;

        dream() {
            strength = 0;
        }

        static void load( const JsonObject &jsobj );
};

struct mut_attack {

    translation attack_text_u;

    translation attack_text_npc;

    std::set<trait_id> required_mutations;

    std::set<trait_id> blocker_mutations;


    bodypart_str_id bp;


    int chance = 0;

    damage_instance base_damage;

    damage_instance strength_damage;


    bool hardcoded_effect = false;
};

struct mut_transform {

    trait_id target;


    translation msg_transform;

    bool active = false;

    int moves = 0;

    bool safe = false;
    mut_transform();
    bool load( const JsonObject &jsobj, std::string_view member );
};

struct mut_personality_score {


    int min_aggression = -10;
    int max_aggression = 10;
    int min_bravery = -10;
    int max_bravery = 10;
    int min_collector = -10;
    int max_collector = 10;
    int min_altruism = -10;
    int max_altruism = 10;
    mut_personality_score();
    bool load( const JsonObject &jsobj, std::string_view member );
};

struct reflex_activation_data {


    std::function<bool( const_dialogue const & )>trigger;

    std::pair<translation, game_message_type> msg_on;
    std::pair<translation, game_message_type> msg_off;

    bool is_trigger_true( Character &guy ) const;

    bool was_loaded = false;
    void load( const JsonObject &jsobj );
    void deserialize( const JsonObject &jo );
};

struct trait_and_var {
    trait_id trait;
    std::string variant;

    trait_and_var() = default;
    trait_and_var( const trait_id &t, const std::string &v ) : trait( t ), variant( v ) {}

    void deserialize( const JsonValue &jv );
    void serialize( JsonOut &jsout ) const;

    std::string name() const;
    std::string desc() const;

    bool operator==( const trait_and_var &other ) const {
        return trait == other.trait && variant == other.variant;
    }
};

struct trait_replacement {
    std::optional<proficiency_id> prof;
    std::optional<trait_and_var> trait;
    bool error = false;
};

struct mutation_variant {
    std::string id;

    translation alt_name;
    translation alt_description;


    bool append_desc = false;



    int weight = 0;



    trait_id parent;

    void deserialize( const JsonObject &jo );
    void load( const JsonObject &jo );
};

struct mutation_branch {
        trait_id id;
        std::vector<std::pair<trait_id, mod_id>> src;
        bool was_loaded = false;

        bool valid = false;

        bool purifiable = false;

        bool threshold = false;

        std::vector<trait_id> threshold_substitutes;

        bool strict_threshreq = false;

        bool profession = false;

        bool debug = false;

        bool player_display = true;

        bool vanity = false;

        bool dummy = false;

        bool mixed_effect  = false;
        bool startingtrait = false;

        bool chargen_allow_npc = true;
        bool random_start_allowed = true;
        bool activated     = false;
        translation activation_msg;

        bool starts_active = false;

        bool destroys_gear = false;

        bool allow_soft_gear  = false;

        bool sleepiness       = false;
        bool hunger        = false;
        bool thirst        = false;
        bool mana       = false;
        bool stamina       = false;

        int points     = 0;

        int vitamin_cost = 100;
        int visibility = 0;
        int ugliness   = 0;
        int cost       = 0;

        time_duration cooldown   = 0_turns;

        units::temperature_delta bodytemp_min = 0_C_delta;
        units::temperature_delta bodytemp_max = 0_C_delta;

        std::optional<int> scent_intensity;

        std::map<quality_id, int> provided_qualities;

        cata::value_ptr<mut_transform> transform;

        cata::value_ptr<mut_personality_score> personality_score;


        std::map<std::string, mutation_variant> variants;

        std::vector<std::vector<reflex_activation_data>> trigger_list;


        std::map<skill_id, int> craft_skill_bonus;


        std::optional<scenttype_id> scent_typeid;


        std::map<bodypart_str_id, float> lumination;


        social_modifiers social_mods;


        itype_id spawn_item;


        std::vector<species_id> ignored_by;


        std::map<species_id, int> anger_relations;

        std::vector<species_id> empathize_with;
        std::vector<species_id> no_empathize_with;


        std::set<material_id> can_only_eat;


        std::set<itype_id> can_only_heal_with;
        std::set<itype_id> can_heal_with;


        std::set<mutation_category_id> allowed_category;


        std::set<bodypart_str_id> no_cbm_on_bp;


        std::map<spell_id, int> spells_learned;

        std::optional<bool> hide_on_activated = std::nullopt;

        std::optional<bool> hide_on_deactivated = std::nullopt;

        std::map<mtype_id, int> moncams;

        std::vector<effect_on_condition_id> activated_eocs;

        bool activated_is_setup = false;

        std::vector<effect_on_condition_id> processed_eocs;

        std::vector<effect_on_condition_id> deactivated_eocs;

        std::vector<enchantment_id> enchantments;


        std::vector<comfort_data> comfort;

        struct OverrideLook {
            std::string id;
            std::string tile_category;
            OverrideLook( const std::string &_id, const std::string &_tile_category )
                : id( _id ), tile_category( _tile_category ) {}
        };



        std::optional<OverrideLook> override_look;
    private:
        translation raw_spawn_item_message;
    public:
        std::string spawn_item_message() const;


        itype_id ranged_mutation;
    private:
        translation raw_ranged_mutation_message;
    public:
        std::string ranged_mutation_message() const;


        std::vector<mut_attack> attacks_granted;


        std::map<vitamin_id, time_duration> vitamin_rates;


        std::map<material_id, std::map<vitamin_id, double>> vitamin_absorb_multi;

        std::vector<trait_id> prereqs;
        std::vector<trait_id> prereqs2;
        std::vector<trait_id> threshreq;
        std::set<std::string> types;
        std::vector<trait_id> cancels;
        std::vector<trait_id> replacements;
        std::vector<trait_id> additions;
        std::vector<mutation_category_id> category;
        std::set<json_character_flag> flags;
        std::set<json_character_flag> active_flags;
        std::set<json_character_flag> inactive_flags;
        std::map<bodypart_str_id, tripoint> protection;
        std::map<bodypart_str_id, int> encumbrance_always;

        std::map<bodypart_str_id, int> encumbrance_covered;

        std::map<bodypart_str_id, float> encumbrance_multiplier_always;

        std::set<bodypart_str_id> restricts_gear;
        std::set<sub_bodypart_str_id> restricts_gear_subparts;

        std::set<bodypart_str_id> remove_rigid;
        std::set<sub_bodypart_str_id> remove_rigid_subparts;

        std::set<flag_id> allowed_items;
        std::map<bodypart_str_id, resistances> armor;
        std::vector<itype_id> integrated_armor;
        std::vector<matype_id>
        initial_ma_styles;
    private:
        std::map<bodypart_str_id, int> bionic_slot_bonuses;
        translation raw_name;
        translation raw_desc;
    public:
        std::string name( const std::string &variant = "" ) const;

        std::string desc( const std::string &variant = "" ) const;




        nc_color get_display_color() const;




        const mutation_variant *pick_variant() const;



        const mutation_variant *variant( const std::string &id ) const;





        const mutation_variant *pick_variant_menu() const;



        bool conflicts_with_item( const item &it ) const;



        bool conflicts_with_item_rigid( const item &it ) const;



        const resistances &damage_resistance( const bodypart_id &bp ) const;



        int bionic_slot_bonus( const bodypart_str_id &part ) const;




        static const std::vector<mutation_branch> &get_all();

        static void reset_all();

        void load( const JsonObject &jo, std::string_view src );
        static void load_trait( const JsonObject &jo, const std::string &src );

        static void check_consistency();




        static void load_trait_blacklist( const JsonObject &jsobj );




        static bool trait_is_blacklisted( const trait_id &tid );




        static void load_trait_migration( const JsonObject &jo );




        static const trait_replacement &trait_migration( const trait_id &tid );


        static void finalize_all();
        static void finalize_trait_blacklist();
        void finalize();















        static void load_trait_group( const JsonObject &jsobj );















        static void load_trait_group( const JsonObject &jsobj, const trait_group::Trait_group_tag &gid,
                                      const std::string &subtype );



















        static void load_trait_group( const JsonArray &entries, const trait_group::Trait_group_tag &gid,
                                      bool is_collection );





        static void add_entry( Trait_group &tg, const JsonObject &obj );





        static shared_ptr_fast<Trait_group> get_group( const trait_group::Trait_group_tag &gid );




        static std::vector<trait_group::Trait_group_tag> get_all_group_names();
};

struct mutation_category_trait {
    private:
        translation raw_name;

        translation raw_mutagen_message;

        std::string raw_memorial_message;

    public:
        std::string name() const;
        std::string mutagen_message() const;
        std::string memorial_message_male() const;
        std::string memorial_message_female() const;


        bool wip = false;

        bool skip_test = false;

        mutation_category_id id;

        trait_id threshold_mut;

        int threshold_min = 2200;

        vitamin_id vitamin;

        int base_removal_chance = 100;

        float base_removal_cost_mul = 3.0f;

        static const std::map<mutation_category_id, mutation_category_trait> &get_all();
        static const mutation_category_trait &get_category(
            const mutation_category_id &category_id );
        static void reset();
        static void check_consistency();

        static void load( const JsonObject &jsobj );
};

void load_mutation_type( const JsonObject &jsobj );
bool mutation_category_is_valid( const mutation_category_id &cat );
bool mutation_type_exists( const std::string &id );
std::vector<trait_id> get_mutations_in_types( const std::set<std::string> &ids );
std::vector<trait_id> get_mutations_in_type( const std::string &id );
bool mutation_is_in_category( const trait_id &mut, const mutation_category_id &cat );
std::vector<trait_and_var> mutations_var_in_type( const std::string &id );
bool trait_var_display_sort( const trait_and_var &a, const trait_and_var &b ) noexcept;
bool trait_var_display_nocolor_sort( const trait_and_var &a, const trait_and_var &b ) noexcept;
bool trait_display_sort( const trait_id &a, const trait_id &b ) noexcept;
bool trait_display_nocolor_sort( const trait_id &a, const trait_id &b ) noexcept;

bool are_conflicting_traits( const trait_id &trait_a, const trait_id &trait_b );
bool b_is_lower_trait_of_a( const trait_id &trait_a, const trait_id &trait_b );
bool b_is_higher_trait_of_a( const trait_id &trait_a, const trait_id &trait_b );
bool are_opposite_traits( const trait_id &trait_a, const trait_id &trait_b );
bool are_same_type_traits( const trait_id &trait_a, const trait_id &trait_b );
bool contains_trait( std::vector<string_id<mutation_branch>> traits, const trait_id &trait );
enum class mutagen_technique : int {
    consumed_mutagen,
    injected_mutagen,
    consumed_purifier,
    injected_purifier,
    injected_smart_purifier,
    num_mutagen_techniques
};

struct traits_sorter {
    bool sort_by_points = false;

    bool operator()( const trait_id *a, const trait_id *b ) {
        return std::abs( ( *a )->points ) > std::abs( ( *b )->points );
    }
};

template<>
struct enum_traits<mutagen_technique> {
    static constexpr mutagen_technique last = mutagen_technique::num_mutagen_techniques;
};

#endif
