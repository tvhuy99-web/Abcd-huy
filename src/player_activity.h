#pragma once
#ifndef CATA_SRC_PLAYER_ACTIVITY_H
#define CATA_SRC_PLAYER_ACTIVITY_H

#include <climits>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "activity_actor.h"
#include "calendar.h"
#include "clone_ptr.h"
#include "compatibility.h"
#include "coordinates.h"
#include "enums.h"
#include "item_location.h"
#include "memory_fast.h"
#include "point.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;
class avatar;
class monster;
class translation;

class player_activity
{
    private:
        activity_id type;

        std::set<distraction_type> ignored_distractions; // NOLINT(cata-serialize)


        bool ignore_followup_distractions = false; // NOLINT(cata-serialize)

        bool ignoreQuery = false; // NOLINT(cata-serialize)


        std::set<std::string> dismissed_prompts; // NOLINT(cata-serialize)

    public:
        cata::clone_ptr<activity_actor> actor;


        int moves_total = 0;

        int moves_left = calendar::INDEFINITELY_LONG;

        bool interruptable = true;

        bool interruptable_with_kb = true;



        int index = 0;




        int position = 0;
        std::string name;
        std::vector<item_location> targets;
        std::vector<int> values;
        std::vector<std::string> str_values;
        std::vector<tripoint_abs_ms> coords;
        std::unordered_set<tripoint_abs_ms> coord_set;
        std::vector<weak_ptr_fast<monster>> monsters;
        static constexpr const tripoint_abs_ms &invalid_place = tripoint_abs_ms::invalid;
        tripoint_abs_ms placement;


        tripoint_rel_ms relative_placement;

        bool no_drink_nearby_for_auto_consume = false; // NOLINT(cata-serialize)
        bool no_food_nearby_for_auto_consume = false; // NOLINT(cata-serialize)



        bool auto_resume = false;



        bool have_fire = true; // NOLINT(cata-serialize)

        player_activity();


        explicit player_activity( activity_id, int turns = 0, int Index = -1, int pos = INT_MIN,
                                  const std::string &name_in = "" );



        explicit player_activity( const activity_actor &actor );

        player_activity( player_activity && ) noexcept = default;
        player_activity( const player_activity & ) = default;
        player_activity &operator=( player_activity && ) noexcept( list_is_noexcept ) = default;
        player_activity &operator=( const player_activity & ) = default;

        explicit operator bool() const {
            return !type.is_null();
        }
        bool is_null() const {
            return type.is_null();
        }
        bool is_multi_type() const;

        void set_to_null();




        void synchronize_type_with_actor();

        const activity_id &id() const {
            return type;
        }
        bool rooted() const;



        std::string get_stop_phrase() const;

        const translation &get_verb() const;

        int get_value( size_t index, int def = 0 ) const;
        std::string get_str_value( size_t index, std::string_view def = {} ) const;




        std::optional<std::string> get_progress_message( const avatar &u ) const;







        bool can_resume() const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );

        void deserialize_legacy_type( int legacy_type, activity_id &dest );






        void start_or_resume( Character &who, bool resuming );






        void do_turn( Character &you );




        void canceled( Character &who );





        bool can_resume_with( const player_activity &other, const Character &who ) const;





        void set_resume_values( const player_activity &other, const Character &who ) ;

        bool is_interruptible() const;
        bool is_interruptible_with_kb() const;
        bool is_distraction_ignored( distraction_type ) const;
        void ignore_distraction( distraction_type );
        void ignore_followup_interruptions();
        void allow_distractions();
        void inherit_distractions( const player_activity & );
        bool is_prompt_dismissed( const std::string &prompt ) const;
        void dismiss_prompt( const std::string &prompt );

        float exertion_level() const;

        bool do_drop_invalid_inventory() const {
            return !actor || actor->do_drop_invalid_inventory();
        }

        std::map<distraction_type, std::string> get_distractions() const;
};

#endif
