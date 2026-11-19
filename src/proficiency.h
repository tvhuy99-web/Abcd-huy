#pragma once
#ifndef CATA_SRC_PROFICIENCY_H
#define CATA_SRC_PROFICIENCY_H

#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "flat_set.h"
#include "translation.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;
struct display_proficiency;
struct learning_proficiency;
template<typename E> struct enum_traits;
template<typename T>
class generic_factory;

enum class proficiency_bonus_type : int {
    strength,
    dexterity,
    intelligence,
    perception,
    stamina,
    last
};

template<>
struct enum_traits<proficiency_bonus_type> {
    static constexpr proficiency_bonus_type last = proficiency_bonus_type::last;
};

struct proficiency_bonus {
    proficiency_bonus_type type = proficiency_bonus_type::last;
    float value = 0;

    void deserialize( const JsonObject &jo );
};

struct proficiency_category {
    proficiency_category_id id;
    translation _name;
    translation _description;
    bool was_loaded = false;

    static void load_proficiency_categories( const JsonObject &jo, const std::string &src );
    static void finalize_all();
    static void reset();
    void load( const JsonObject &jo, std::string_view src );
    static const std::vector<proficiency_category> &get_all();
};

class proficiency
{
        friend class generic_factory<proficiency>;
        friend struct mod_tracker;

        proficiency_id id;
        proficiency_category_id _category;
        std::vector<std::pair<proficiency_id, mod_id>> src;
        bool was_loaded = false;

        bool _can_learn = false;
        bool _ignore_focus = false;
        bool _teachable = true;

        translation _name;
        translation _description;

        float _default_time_multiplier = 2.0f;
        float _default_skill_penalty = 1.0f;

        float _default_weakpoint_bonus = 0.0f;
        float _default_weakpoint_penalty = 0.0f;

        time_duration _time_to_learn = 9999_hours;
        std::set<proficiency_id> _required;

        std::map<std::string, std::vector<proficiency_bonus>> _bonuses;

    public:
        static void load_proficiencies( const JsonObject &jo, const std::string &src );
        static void finalize_all();
        static void reset();
        void load( const JsonObject &jo, std::string_view src );

        static const std::vector<proficiency> &get_all();

        bool can_learn() const;
        bool ignore_focus() const;
        bool is_teachable() const;
        proficiency_id prof_id() const;
        proficiency_category_id prof_category() const;
        std::string name() const;
        std::string description() const;

        float default_time_multiplier() const;
        float default_skill_penalty() const;

        float default_weakpoint_bonus() const;
        float default_weakpoint_penalty() const;

        time_duration time_to_learn() const;
        std::set<proficiency_id> required_proficiencies() const;

        std::vector<proficiency_bonus> get_bonuses( const std::string &category ) const;
        std::optional<float> bonus_for( const std::string &category, proficiency_bonus_type type ) const;
};


class proficiency_set
{

        cata::flat_set<proficiency_id> known;


        std::vector<learning_proficiency> learning;

        learning_proficiency &fetch_learning( const proficiency_id &target );

    public:
        void clear();

        std::vector<display_proficiency> display() const;

        bool practice( const proficiency_id &practicing, const time_duration &amount,
                       float remainder, const std::optional<time_duration> &max );
        void learn( const proficiency_id &learned, bool recursive = false );
        void remove( const proficiency_id &lost );


        void direct_learn( const proficiency_id &learned );
        void direct_remove( const proficiency_id &lost );


        bool has_learned( const proficiency_id &query ) const;
        bool has_practiced( const proficiency_id &query ) const;
        bool has_prereqs( const proficiency_id &query ) const;

        float pct_practiced( const proficiency_id &query ) const;
        time_duration pct_practiced_time( const proficiency_id &query ) const;
        void set_time_practiced( const proficiency_id &practicing, const time_duration &amount );
        time_duration training_time_needed( const proficiency_id &query ) const;
        std::vector<proficiency_id> known_profs() const;
        std::vector<proficiency_id> learning_profs() const;
        float get_proficiency_bonus( const std::string &category,
                                     proficiency_bonus_type prof_bonus ) const;

        void migrate_proficiencies();

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonObject &jsobj );
};

struct learning_proficiency {
    proficiency_id id;


    time_duration practiced;

    float remainder = 0.f;

    learning_proficiency() = default;
    learning_proficiency( const proficiency_id &id, const time_duration &practiced ) : id( id ),
        practiced( practiced ) {}

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonObject &jo );
};

struct display_proficiency {

    proficiency_id id;


    nc_color color;


    float practice = 0.0f;


    time_duration spent = 0_turns;


    bool known = false;
};


struct book_proficiency_bonus {
        proficiency_id id;
        float time_factor = default_time_factor;
        float fail_factor = default_fail_factor;
        bool include_prereqs = default_include_prereqs;

        bool was_loaded = false;
        void deserialize( const JsonObject &jo );

    private:
        static const float default_time_factor;
        static const float default_fail_factor;
        static const bool default_include_prereqs;
};


class book_proficiency_bonuses
{
    private:
        std::vector<book_proficiency_bonus> bonuses;

        void add( const book_proficiency_bonus &bonus, std::set<proficiency_id> &already_included );
    public:
        void add( const book_proficiency_bonus &bonus );
        book_proficiency_bonuses &operator+=( const book_proficiency_bonuses &rhs );


        float fail_factor( const proficiency_id &id ) const;


        float time_factor( const proficiency_id &id ) const;
};

class proficiency_migration
{
    public:
        proficiency_id id_old;
        std::optional<proficiency_id> id_new;

        static void load( const JsonObject &jo );

        static void reset();

        static void check();


        static const proficiency_migration *find_migration( const proficiency_id &original );
};

void show_proficiencies_window( const Character &u,
                                std::optional<proficiency_id> default_selection = std::nullopt );

#endif
