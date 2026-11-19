#pragma once
#ifndef CATA_SRC_BODYPART_H
#define CATA_SRC_BODYPART_H

#include <array>
#include <climits>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "damage.h"
#include "enums.h"
#include "flat_set.h"
#include "subbodypart.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"
#include "weighted_list.h"
#include "wound.h"

class Character;
class Creature;
class JsonObject;
class JsonOut;
class time_duration;
struct body_part_type;
struct localized_comparator;
template <typename E> struct enum_traits;
template <typename T> class generic_factory;

using bodypart_id = int_id<body_part_type>;

extern const bodypart_str_id body_part_head;
extern const bodypart_str_id body_part_eyes;
extern const bodypart_str_id body_part_mouth;
extern const bodypart_str_id body_part_torso;
extern const bodypart_str_id body_part_arm_l;
extern const bodypart_str_id body_part_arm_r;
extern const bodypart_str_id body_part_hand_l;
extern const bodypart_str_id body_part_hand_r;
extern const bodypart_str_id body_part_leg_l;
extern const bodypart_str_id body_part_foot_l;
extern const bodypart_str_id body_part_leg_r;
extern const bodypart_str_id body_part_foot_r;


enum body_part : int {
    bp_torso = 0,
    bp_head,
    bp_eyes,
    bp_mouth,
    bp_arm_l,
    bp_arm_r,
    bp_hand_l,
    bp_hand_r,
    bp_leg_l,
    bp_leg_r,
    bp_foot_l,
    bp_foot_r,
    num_bp
};

inline auto format_as( body_part bp )
{
    return static_cast<std::underlying_type_t<body_part>>( bp );
}

template<>
struct enum_traits<body_part> {
    static constexpr body_part last = body_part::num_bp;
};


enum water_tolerance {
    WT_IGNORED = 0,
    WT_NEUTRAL,
    WT_GOOD,
    NUM_WATER_TOLERANCE
};





constexpr std::array<body_part, 12> all_body_parts = {{
        bp_torso, bp_head, bp_eyes, bp_mouth,
        bp_arm_l, bp_arm_r, bp_hand_l, bp_hand_r,
        bp_leg_l, bp_leg_r, bp_foot_l, bp_foot_r
    }
};

struct stat_hp_mods {

    float str_mod = 3.0f;
    float dex_mod = 0.0f;
    float int_mod = 0.0f;
    float per_mod = 0.0f;

    float health_mod = 0.0f;

    bool was_loaded = false;
    void load( const JsonObject &jsobj );
    void deserialize( const JsonObject &jo );
};

struct limb_score {
    public:
        static void load_limb_scores( const JsonObject &jo, const std::string &src );
        static void finalize_all();
        static void reset();
        void load( const JsonObject &jo, std::string_view src );
        static const std::vector<limb_score> &get_all();

        const limb_score_id &getId() const {
            return id;
        }
        const translation &name() const {
            return _name;
        }
        bool affected_by_wounds() const {
            return wound_affect;
        }
        bool affected_by_encumb() const {
            return encumb_affect;
        }
    private:
        limb_score_id id;
        std::vector<std::pair<limb_score_id, mod_id>> src;
        translation _name;
        bool wound_affect = true;
        bool encumb_affect = true;
        bool was_loaded = false;
        friend class generic_factory<limb_score>;
        friend struct mod_tracker;
};

struct bp_limb_score {
    float score = 0.0f;
    float max = 0.0f;
};

struct bp_wounds {
    wound_type_id id;

    std::vector<damage_type_id> damage_type;

    std::pair<int, int> damage_required;
};

struct bp_onhit_effect {

    efftype_id id;

    bool global = false;

    damage_type_id dtype = damage_type_id::NULL_ID();

    int dmg_threshold = 100;

    float scale_increment = 0.0f;

    int chance = 100;

    float chance_dmg_scaling = 0.0f;

    int intensity = 1;

    float intensity_dmg_scaling = 0.0f;

    int duration = 1;

    float duration_dmg_scaling = 0.0f;

    int max_intensity = INT_MAX;

    int max_duration = INT_MAX;

    void load( const JsonObject &jo );
};

struct bp_qualities_provided {

    quality_id quality;
    int level;

    float disable_percent = 0;

    void load( const JsonObject &jo );
};

struct body_part_type {
    public:





        std::vector<std::pair<bodypart_str_id, mod_id>> src;



        std::vector<sub_bodypart_str_id> sub_parts;

        std::map<units::mass, int> encumbrance_per_weight;

        cata::flat_set<json_character_flag> flags;
        cata::flat_set<json_character_flag> conditional_flags;

    private:

        std::map<limb_score_id, bp_limb_score> limb_scores;
        damage_instance damage;

    public:
        bodypart_str_id id;

        std::string legacy_id;

        bodypart_str_id main_part;


        bodypart_str_id connected_to;

        bodypart_str_id opposite_part;


        std::map<bp_type, float> limbtypes;


        std::set<matec_id> techniques;

        std::vector<bp_qualities_provided> qualities;


        efftype_id windage_effect;

        efftype_id no_power_effect;


        std::vector<bp_onhit_effect> effects_on_hit;


        translation name;
        translation name_multiple;
        translation accusative;
        translation accusative_multiple;
        translation name_as_heading;
        translation name_as_heading_multiple;
        translation smash_message;
        translation hp_bar_ui_text;
        translation encumb_text;

        body_part token = num_bp;

        float hit_size = 0.0f;






        float hit_difficulty = 0.0f;


        side part_side = side::BOTH;


        int encumbrance_threshold = 0;

        int encumbrance_limit = 0;

        int health_limit = 0;


        int bmi_encumbrance_threshold = 999;

        float bmi_encumbrance_scalar = 0;
        float smash_efficiency = 0.5f;


        float pain_mod = 1.f;


        float hot_morale_mod = 0.0f;
        float cold_morale_mod = 0.0f;
        int squeamish_penalty = 0;
        bool feels_discomfort = true;

        units::temperature_delta fire_warmth_bonus = 0_C_delta;


        int env_protection = 0;

        int base_hp = 60;

        int heal_bonus = 0;
        float mend_rate = 1.0f;


        int ugliness = 0;

        int ugliness_mandatory = 0;


        units::temperature_delta temp_min = 0_C_delta;

        units::temperature_delta temp_max = 0_C_delta;
        int drench_max = 0;
        int drench_increment = 2;
        float drying_rate = 1.0f;

        int wet_morale = 0;
        int technique_enc_limit = 50;


        int power_efficiency = 0;



        std::optional<bodypart_str_id> similar_bodypart;

        weighted_int_list<bp_wounds> potential_wounds;

    private:
        int bionic_slots_ = 0;
        bp_type _primary_limb_type = bp_type::num_types;

        resistances armor;

    public:
        stat_hp_mods hp_mods;
        bool unarmed_bonus = false;

        bool is_vital = false;
        bool is_limb = false;

        bool was_loaded = false;

        bool has_flag( const json_character_flag &flag ) const;
        bp_type primary_limb_type() const;
        bool has_type( const bp_type &type ) const;



        sub_bodypart_id random_sub_part( bool secondary ) const;

        void load( const JsonObject &jo, std::string_view src );
        void finalize();
        void check() const;

        static void load_bp( const JsonObject &jo, const std::string &src );
        static const std::vector<body_part_type> &get_all();


        static void reset();

        static void finalize_all();

        static void check_consistency();

        float get_limb_score( const limb_score_id &id ) const;
        float get_limb_score_max( const limb_score_id &id ) const;
        bool has_limb_score( const limb_score_id &id ) const;

        int bionic_slots() const {
            return bionic_slots_;
        }

        damage_instance unarmed_damage_instance() const;
        float unarmed_damage( const damage_type_id &dt ) const;

        float total_unarmed_damage() const;
        float unarmed_arpen( const damage_type_id &dt ) const;

        float damage_resistance( const damage_type_id &dt ) const;
        float damage_resistance( const damage_unit &du ) const;


        static std::set<translation, localized_comparator> consolidate( std::vector<sub_bodypart_id>
                &covered );


        static std::set<translation, localized_comparator> consolidate( std::vector<bodypart_id>
                &covered );

        std::vector<bodypart_str_id> get_all_combined_similar_bodyparts() const;
};

struct layer_details {

    std::vector<int> pieces;
    int max = 0;
    int total = 0;


    bool is_conflicting = false;

    std::vector<sub_bodypart_id> covered_sub_parts;

    void reset();
    int layer( int encumbrance, bool conflicts );

    bool operator ==( const layer_details &rhs ) const {
        return max == rhs.max &&
               total == rhs.total &&
               pieces == rhs.pieces;
    }
};

struct encumbrance_data {

    int encumbrance = 0;
    int armor_encumbrance = 0;
    int layer_penalty = 0;

    std::array<layer_details, static_cast<size_t>( layer_level::NUM_LAYER_LEVELS )>
    layer_penalty_details;

    bool add_sub_location( layer_level level, sub_bodypart_id sbp );

    bool add_sub_location( layer_level level, sub_bodypart_str_id sbp );

    void layer( const layer_level level, const int encumbrance, bool conflicts ) {
        layer_penalty_details[static_cast<size_t>( level )].layer( encumbrance, conflicts );
    }

    void reset() {
        *this = encumbrance_data();
    }

    bool operator ==( const encumbrance_data &rhs ) const {
        return encumbrance == rhs.encumbrance &&
               armor_encumbrance == rhs.armor_encumbrance &&
               layer_penalty == rhs.layer_penalty &&
               layer_penalty_details == rhs.layer_penalty_details;
    }
};

class bodypart
{
    private:
        bodypart_str_id id;

        int hp_cur = 0;
        int hp_max = 0;

        int wetness = 0;
        units::temperature temp_cur = BODYTEMP_NORM;
        units::temperature temp_conv = BODYTEMP_NORM;
        int frostbite_timer = 0;

        int healed_total = 0;
        int damage_bandaged = 0;
        int damage_disinfected = 0;

        encumbrance_data encumb_data; // NOLINT(cata-serialize)

        std::array<int, NUM_WATER_TOLERANCE> mut_drench; // NOLINT(cata-serialize)

        std::vector<wound> wounds;


        float wound_adjusted_limb_value( float val, const limb_score_id &score ) const;

        float encumb_adjusted_limb_value( const Creature &mon, float val ) const;

        float skill_adjusted_limb_value( float val, int skill ) const;
    public:
        bodypart(): id( bodypart_str_id::NULL_ID() ), mut_drench() {}
        explicit bodypart( bodypart_str_id id ): id( id ), hp_cur( id->base_hp ), hp_max( id->base_hp ),
            mut_drench() {}

        bodypart_id get_id() const;

        void set_hp_to_max();
        bool is_at_max_hp() const;

        float get_wetness_percentage() const;

        bool compare_encumbrance_data( const bodypart &bp ) const;
        int get_final_encumbrance( const Creature &mon ) const;
        int get_layer_penalty() const;
        int get_encumbrance_threshold() const;

        bool is_limb_overencumbered( const Creature &mon ) const;
        bool has_conditional_flag( const Creature &mon, const json_character_flag &flag ) const;


        std::set<matec_id> get_limb_techs( const Creature &mon ) const;


        std::vector<bp_onhit_effect> get_onhit_effects( damage_type_id dtype ) const;



        float get_limb_score( const Creature &mon, const limb_score_id &score, int skill = -1,
                              int override_encumb = -1,
                              int override_wounds = -1 ) const;
        float get_limb_score_max( const limb_score_id &score ) const;

        const std::vector<wound> &get_wounds() const;
        std::vector<wound> &get_wounds();

        void add_or_worsen_wound( wound_type_id wd );
        void add_or_worsen_wound( const wound &wd );

        void add_wound( const wound &wd );
        void add_wound( wound_type_id wd );
        bool has_wounds() const;
        bool has_wound( wound_type_id wd ) const;
        wound *get_wound( wound_type_id wd_id );
        void remove_wound( wound wd );
        void remove_wound( wound_type_id wd );
        void remove_all_wounds_of_type( wound_type_id wd );
        void update_wounds( const Character &who, time_duration time_passed );

        int get_hp_cur() const;
        int get_hp_max() const;
        float get_hit_size() const;
        int get_healed_total() const;
        int get_damage_bandaged() const;
        int get_damage_disinfected() const;
        int get_drench_capacity() const;
        int get_wetness() const;
        int get_frostbite_timer() const;
        units::temperature get_temp_cur() const;
        units::temperature get_temp_conv() const;
        int get_bmi_encumbrance_threshold() const;
        float get_bmi_encumbrance_scalar() const;
        int get_power_efficiency() const;

        std::array<int, NUM_WATER_TOLERANCE> get_mut_drench() const;

        void set_hp_cur( int set );
        void set_hp_max( int set );
        void set_healed_total( int set );
        void set_damage_bandaged( int set );
        void set_damage_disinfected( int set );
        void set_wetness( int set );
        void set_temp_cur( units::temperature set );
        void set_temp_conv( units::temperature set );
        void set_frostbite_timer( int set );

        void set_encumbrance_data( const encumbrance_data &set );

        void set_mut_drench( const std::pair<water_tolerance, int> &set );

        void mod_hp_cur( int mod );
        void mod_hp_max( int mod );
        void mod_healed_total( int mod );
        void mod_damage_bandaged( int mod );
        void mod_damage_disinfected( int mod );
        void mod_wetness( int mod );
        void mod_temp_cur( units::temperature_delta mod );
        void mod_temp_conv( units::temperature_delta mod );
        void mod_frostbite_timer( int mod );

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );
};


const bodypart_str_id &convert_bp( body_part bp );


side opposite_side( side s );


const std::array<size_t, 12> bp_aiOther = {{0, 1, 2, 3, 5, 4, 7, 6, 9, 8, 11, 10}};


std::string body_part_name( const bodypart_id &bp, int number = 1 );



std::string body_part_name_accusative( const bodypart_id &bp, int number = 1 );



std::string body_part_name_as_heading( const bodypart_id &bp, int number );


std::string body_part_hp_bar_ui_text( const bodypart_id &bp );


std::string encumb_text( const bodypart_id &bp );

#endif
