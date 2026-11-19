#pragma once
#ifndef CATA_SRC_WEAKPOINT_H
#define CATA_SRC_WEAKPOINT_H

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "translation.h"
#include "type_id.h"

class Character;
class Creature;
class JsonArray;
class JsonObject;
class JsonValue;
class item;
struct const_dialogue;
struct damage_instance;
struct resistances;


struct weakpoint_attack {
    enum class attack_type : int {
        NONE,
        MELEE_BASH,
        MELEE_CUT,
        MELEE_STAB,
        PROJECTILE,

        NUM,
    };


    Creature *source;

    Creature *target;

    const item *weapon;

    attack_type type;

    bool is_thrown;




    double accuracy = -1.0;

    bool is_crit;

    float wp_skill;

    weakpoint_attack();

    static attack_type type_of_melee_attack( const damage_instance &damage );

    void compute_wp_skill();
};


struct weakpoint_effect {

    efftype_id effect;

    std::vector<effect_on_condition_id> effect_on_conditions;

    float chance;

    bool permanent;

    std::pair<int, int> instant_death_chance;

    std::pair<time_duration, time_duration> duration;

    std::pair<int, int> intensity;

    std::pair<float, float> damage_required;

    translation message;

    weakpoint_effect();

    std::string get_message() const;

    void apply_to( Creature &target, int total_damage, const weakpoint_attack &attack ) const;
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo ) {
        load( jo );
    }
};

struct weakpoint_difficulty {
    std::array<float, static_cast<int>( weakpoint_attack::attack_type::NUM )> difficulty;

    explicit weakpoint_difficulty( float default_value );
    float of( const weakpoint_attack &attack ) const;
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo ) {
        load( jo );
    }
};

struct weakpoint_family {

    std::string id;

    proficiency_id proficiency;

    std::optional<float> bonus;

    std::optional<float> penalty;

    float modifier( const Character &attacker ) const;
    void load( const JsonValue &jsin );
    void deserialize( const JsonValue &jsin ) {
        load( jsin );
    }
};

struct weakpoint_families {

    std::vector<weakpoint_family> families;


    bool practice( Character &learner, const time_duration &amount ) const;
    bool practice_hit( Character &learner ) const;
    bool practice_kill( Character &learner ) const;
    bool practice_dissect( Character &learner ) const;
    float modifier( const Character &attacker ) const;

    void clear();
    void load( const JsonArray &ja );
    void remove( const JsonArray &ja );

    void deserialize( const JsonValue &jv );
    bool handle_extend( const JsonValue &jv );
    bool handle_delete( const JsonValue &jv );
};

struct weakpoint {

    std::string id;

    translation name;

    float coverage = 100.0f;

    bool is_good = true;

    std::unordered_map<damage_type_id, float> armor_mult;

    std::unordered_map<damage_type_id, float> armor_penalty;

    std::unordered_map<damage_type_id, float> damage_mult;

    std::unordered_map<damage_type_id, float> crit_mult;

    std::function<bool( const_dialogue const & )> condition;
    bool has_condition = false;

    std::vector<weakpoint_effect> effects;

    weakpoint_difficulty coverage_mult;

    weakpoint_difficulty difficulty;
    bool is_head = false;

    weakpoint();

    std::string get_name() const;

    void apply_to( resistances &resistances ) const;

    void apply_to( damage_instance &damage, bool is_crit ) const;
    void apply_effects( Creature &target, int total_damage, const weakpoint_attack &attack ) const;

    float hit_chance( const weakpoint_attack &attack ) const;
    void load( const JsonObject &jo );
    void check() const;
};

struct weakpoints {

    weakpoints_id id;
    std::vector<std::pair<weakpoints_id, mod_id>> src;

    std::vector<weakpoint> weakpoint_list;

    weakpoint default_weakpoint;

    bool was_loaded = false;


    const weakpoint *select_weakpoint( const weakpoint_attack &attack ) const;

    void clear();

    void load( const JsonArray &ja );
    void remove( const JsonArray &ja );
    void finalize();
    void check() const;

    void deserialize( const JsonValue &jv );
    bool handle_extend( const JsonValue &jv );
    bool handle_delete( const JsonValue &jv );



    void load( const JsonObject &jo, std::string_view src );
    void add_from_set( const weakpoints_id &set_id, bool replace_id );
    void add_from_set( const weakpoints &set, bool replace_id );
    void del_from_set( const weakpoints_id &set_id );
    void del_from_set( const weakpoints &set );
    static void load_weakpoint_sets( const JsonObject &jo, const std::string &src );
    static void reset();
    static void finalize_all();
    static const std::vector<weakpoints> &get_all();
};

#endif
