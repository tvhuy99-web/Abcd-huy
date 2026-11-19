#pragma once
#ifndef CATA_SRC_WOUND_H
#define CATA_SRC_WOUND_H

#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "calendar.h"
#include "requirements.h"
#include "translation.h"
#include "type_id.h"
#include "value_ptr.h"

class JsonObject;
class JsonOut;
enum class bp_type;


struct wound_progress {
    wound_type_id id;
    int chance;

    void deserialize( const JsonObject &jo );
};

struct wound_limb_score {
    limb_score_id score;
    float value;

    void deserialize( const JsonObject &jo );
};


struct wound_type_progress_timer {
    wound_type_id id;
    std::pair<time_duration, time_duration> time;
    translation message;

    void deserialize( const JsonObject &jo );
};

class wound_type
{
    public:
        bool was_loaded = false;
        wound_type_id id;

        static void load_wounds( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string_view &src );
        static void reset();
        static void check_consistency();
        void check() const;
        void deserialize( const JsonObject &jo );
        static void finalize_all();
        void finalize();
        static const std::vector<wound_type> &get_all();

        bool allowed_on_bodypart( bodypart_str_id bp_id ) const;
        std::string get_name() const;
        std::string get_description() const;
        int evaluate_pain() const;
        time_duration evaluate_healing_time() const;
        std::optional<std::pair<wound_type_id, time_duration>> evaluate_progress_timer() const;

        std::string get_progress_timer_message() const;
        int get_limit() const;

        std::vector<wound_limb_score> get_limb_scores() const;


        std::vector<damage_type_id> damage_types;

        std::pair<int, int> damage_required;

        int weight;


        std::vector<json_character_flag> whitelist_bp_with_flag;
        std::vector<json_character_flag> blacklist_bp_with_flag;

        std::vector<bp_type> whitelist_body_part_types;
        std::vector<bp_type> blacklist_body_part_types;

        std::set<wound_fix_id> fixes;


        std::vector<wound_progress> wound_progression;

        std::optional<wound_type_progress_timer> progress_timer;
    private:
        translation name_;
        translation description_;



        unsigned int limit = 0;

        std::vector<wound_limb_score> limb_scores;


        std::pair<time_duration, time_duration> healing_time_;

        std::pair<int, int> pain_;
};

enum class wound_update_ret {
    DO_NOTHING,
    ADVANCE,
    REMOVE,
    LAST
};

class wound
{
    public:
        wound() = default;
        explicit wound( wound_type_id wd );

        wound_type_id type;

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonObject &jsin );

        float healing_percentage() const;

        int get_pain() const;

        time_duration get_healing_time() const;
        time_duration get_healing_progress() const;

        wound_update_ret update_wound( time_duration time_passed );

        wound &operator+=( const wound &wd ) {
            healing_time += wd.get_healing_time();
            pain += wd.get_pain();

            healing_progress = 0_seconds;

            return *this;
        }

        bool operator==( const wound &wd ) {
            return type == wd.type && healing_progress == wd.healing_progress;
        }

    private:

        time_duration healing_time;

        time_duration healing_progress;

        int pain;

    public:


        std::optional<std::pair<wound_type_id, time_duration>> wound_progress_timer;
};

struct wound_proficiency {
    proficiency_id prof;
    float time_save = 1.f;
    bool is_mandatory = false;

    void deserialize( const JsonObject &jo );
};

class wound_fix
{
    public:
        wound_fix_id id;
        translation name;
        translation description;
        translation success_msg;
        time_duration time = 0_seconds;

        int mod_hp = 0;
        std::map<skill_id, int> skills;

        std::vector<wound_proficiency> proficiencies;
        std::set<wound_type_id> wounds_removed;
        std::set<wound_type_id> wounds_added;

        const requirement_data &get_requirements() const;

        bool was_loaded = false;
        static void load_wound_fixes( const JsonObject &jo, const std::string &src );


        std::string get_name() const;
        std::string get_description() const;

        void finalize();
        void load( const JsonObject &jo, const std::string_view & );
        void check() const;
        static void reset();
        static void finalize_all();
        static void check_consistency();
    private:

        friend class wound;
        cata::value_ptr<requirement_data> requirements = cata::make_value<requirement_data>();
};

#endif
