#pragma once
#ifndef CATA_SRC_EFFECT_ON_CONDITION_H
#define CATA_SRC_EFFECT_ON_CONDITION_H

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "dialogue.h"
#include "dialogue_helpers.h"
#include "event_subscriber.h"
#include "global_vars.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonValue;
class time_duration;
enum class event_type : int;
struct effect_on_condition;
template <typename E> struct enum_traits;
template <typename T> class generic_factory;

enum eoc_type {
    ACTIVATION,
    RECURRING,
    AVATAR_DEATH,
    NPC_DEATH,
    PREVENT_DEATH,
    EVENT,
    NUM_EOC_TYPES
};

class eoc_events : public event_subscriber
{
    public:
        void notify( const cata::event &e ) override;
        void notify( const cata::event &, std::unique_ptr<talker>, std::unique_ptr<talker> ) override;
        void clear();

    private:
        std::map<event_type, std::vector<effect_on_condition>> event_EOCs;
        bool has_cached = false;
};

struct effect_on_condition {
    public:
        friend class generic_factory<effect_on_condition>;
        bool was_loaded = false;

        bool run_for_npcs = false;

        bool global = false;
        effect_on_condition_id id;
        std::vector<std::pair<effect_on_condition_id, mod_id>> src;
        eoc_type type;
        std::function<bool( const_dialogue const & )> condition;
        std::function<bool( const_dialogue const & )> deactivate_condition;
        talk_effect_t true_effect;
        talk_effect_t false_effect;
        bool has_deactivate_condition = false;
        bool has_condition = false;
        bool has_false_effect = false;
        event_type required_event;
        duration_or_var recurrence;
        bool activate( dialogue &d, bool require_callstack_check = true ) const;
        bool activate_activation_only( dialogue &d, const std::string &text1, const std::string &text2 = "",
                                       const std::string &text3 = "", bool require_callstack_check = true ) const;
        bool check_deactivate( const_dialogue const &d ) const;
        bool test_condition( const_dialogue const &d ) const;
        void apply_true_effects( dialogue &d ) const;
        void load( const JsonObject &jo, std::string_view src );
        void finalize();
        void check() const;
        effect_on_condition() = default;
};
namespace effect_on_conditions
{

const std::vector<effect_on_condition> &get_all();

void finalize_all();

void reset();

void load( const JsonObject &jo, const std::string &src );

void check_consistency();

void load_new_character( Character &you );

void load_existing_character( Character &you );

effect_on_condition_id load_inline_eoc( const JsonValue &jv, std::string_view src );

void queue_effect_on_condition( time_duration duration, effect_on_condition_id eoc,
                                Character &you, global_variables::impl_t const &context );

void process_effect_on_conditions( Character &you );

void process_reactivate( Character &you );
void process_reactivate();

void clear( Character &you );

void write_eocs_to_file( Character &you );
void write_global_eocs_to_file();

void prevent_death();

void avatar_death();
}

template<>
struct enum_traits<eoc_type> {
    static constexpr eoc_type last = eoc_type::NUM_EOC_TYPES;
};
#endif
