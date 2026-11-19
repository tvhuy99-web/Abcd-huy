#pragma once
#ifndef CATA_SRC_DIALOGUE_H
#define CATA_SRC_DIALOGUE_H

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "cata_lazy.h"
#include "dialogue_helpers.h"
#include "global_vars.h"
#include "npc_opinion.h"
#include "talker.h"
#include "type_id.h"













class dialogue_imgui_impl;
class JsonArray;
class JsonObject;
class martialart;
class mission;
class npc;
class input_context;
struct dialogue;
struct input_event;

enum talk_trial_type : unsigned char {
    TALK_TRIAL_NONE,
    TALK_TRIAL_LIE,
    TALK_TRIAL_PERSUADE,
    TALK_TRIAL_INTIMIDATE,
    TALK_TRIAL_SKILL_CHECK,
    TALK_TRIAL_CONDITION,
    NUM_TALK_TRIALS
};

enum dialogue_consequence : unsigned char {
    none = 0,
    hostile,
    helpless,
    action
};

using talkfunction_ptr = std::add_pointer_t<void ( npc & )>;
using dialogue_fun_ptr = std::add_pointer_t<void( npc & )>;

using trial_mod = std::pair<std::string, int>;

struct talk_data {
    nc_color color;
    std::string hotkey_desc;
    std::string text;
};

namespace dialog_helper
{
std::string bye_message( const npc *npc_actor );
}







struct talk_trial {
    talk_trial_type type = TALK_TRIAL_NONE;
    int difficulty = 0;
    std::function<bool( const_dialogue const & )> condition;


    std::string skill_required;

    int calc_chance( const_dialogue const &d ) const;



    std::string name() const;
    std::vector<trial_mod> modifiers;
    explicit operator bool() const {
        return type != TALK_TRIAL_NONE;
    }



    bool roll( dialogue &d ) const;

    talk_trial() = default;
    explicit talk_trial( const JsonObject & );
};




struct talk_topic {
    explicit talk_topic( std::string i, itype_id const &it = itype_id::NULL_ID(),
                         std::string r = {} )
        : id( std::move( i ) ), item_type( it ), reason( std::move( r ) ) {
    }

    std::string id;

    itype_id item_type = itype_id::NULL_ID();

    std::string reason;
};





struct talk_effect_t {




        npc_opinion opinion;




        npc_opinion mission_opinion;



        talk_topic next_topic = talk_topic( "TALK_NONE" );

        talk_topic apply( dialogue &d )
        const;
        static void update_missions( dialogue &d );
        dialogue_consequence get_consequence( dialogue const &d ) const;




        void set_effect( talkfunction_ptr );
        void set_effect( const talk_effect_fun_t & );



        void set_effect_consequence( const talk_effect_fun_t &fun, dialogue_consequence con );
        void set_effect_consequence( const std::function<void( npc &p )> &ptr, dialogue_consequence con );

        void load_effect( const JsonObject &jo, const std::string &member_name,
                          std::string_view src );
        void parse_sub_effect( const JsonObject &jo, std::string_view src );
        void parse_string_effect( const std::string &effect_id, const JsonObject &jo,
                                  std::string_view src );

        talk_effect_t() = default;
        explicit talk_effect_t( const JsonObject &, const std::string &, std::string_view src );




        std::vector<talk_effect_fun_t> effects;
    private:
        dialogue_consequence guaranteed_consequence = dialogue_consequence::none;
};






struct talk_response {




    std::string text;



    translation truetext;
    translation falsetext;
    std::function<bool( const_dialogue const & )> truefalse_condition;

    talk_trial trial;





    std::function<bool( const_dialogue const & )> condition;


    bool show_always = false;

    std::string show_reason;

    std::function<bool( const_dialogue const & )> show_condition;


    bool ignore_conditionals = false;

    mission *mission_selected = nullptr;
    skill_id skill;
    matype_id style;
    spell_id dialogue_spell;
    proficiency_id proficiency;

    talk_effect_t success;
    talk_effect_t failure;

    talk_data create_option_line( dialogue &d, const input_event &hotkey,
                                  bool is_computer = false );
    std::set<dialogue_consequence> get_consequences( dialogue &d ) const;

    std::map<std::string, std::string> debug_info;

    talk_response();
    explicit talk_response( const JsonObject &, std::string_view );
};




struct const_dialogue {
        const_talker *const_actor( bool is_beta ) const;
        bool has_actor( bool is_beta ) const;

        const_dialogue() = default;
        ~const_dialogue() = default;
        const_dialogue( const const_dialogue & );
        const_dialogue( const_dialogue && ) = default;
        const_dialogue &operator=( const const_dialogue & ) = delete;
        const_dialogue &operator=( const_dialogue && ) = default;
        const_dialogue(
            std::unique_ptr<const_talker> alpha_in, std::unique_ptr<const_talker> beta_in,
            const std::unordered_map<std::string, std::function<bool( const_dialogue const & )>> &cond = {},
            global_variables::impl_t const &ctx = {} );

        bool has_beta{};
        bool has_alpha{};

        mutable itype_id cur_item;
        mutable std::string reason;

        std::vector<mission *> missions_assigned;

        bool by_radio = false;


        void set_value( const std::string &key, diag_value value );
        template <typename... Args>
        void set_value( const std::string &key, Args... args ) {
            set_value( key, diag_value{ std::forward<Args>( args )... } );
        }
        void remove_value( const std::string &key );

        void set_conditional( const std::string &key,
                              const std::function<bool( const_dialogue const & )> &value );
        diag_value const &get_value( const std::string &key ) const;
        diag_value const *maybe_get_value( const std::string &key ) const;

        bool evaluate_conditional( const std::string &key, const_dialogue const &d ) const;

        global_variables::impl_t const &get_context() const;
        const std::unordered_map<std::string, std::function<bool( const_dialogue const & )>>
                &get_conditionals() const;
        void amend_callstack( const std::string &value );
        std::string get_callstack() const;

    private:
        std::unique_ptr<const_talker> alpha, beta;

        lazy<global_variables::impl_t> context;
        mutable std::string callstack;

        lazy<std::unordered_map<std::string, std::function<bool( const_dialogue const & )>>> conditionals;
};

struct dialogue: public const_dialogue {



        bool done = false;
        std::vector<talk_topic> topic_stack;

        talk_topic opt_imgui( dialogue_imgui_impl &d_img, const talk_topic &topic, input_context &ctxt );
        dialogue() = default;
        ~dialogue() = default;
        dialogue( const dialogue &d );
        dialogue( dialogue && ) = default;
        dialogue &operator=( const dialogue & );
        dialogue &operator=( dialogue && ) = default;
        dialogue( std::unique_ptr<talker> alpha_in, std::unique_ptr<talker> beta_in,
                  const std::unordered_map<std::string, std::function<bool( const_dialogue const & )>> &cond = {},
                  global_variables::impl_t const &ctx = {} );
        talker *actor( bool is_beta ) const;

        std::string dynamic_line( const talk_topic &topic );
        void apply_speaker_effects( const talk_topic &the_topic );


        std::optional<character_portrait_id> portrait_or_nullopt() const;



        std::string speaker_name( const dialogue_imgui_impl &d_img ) const;




        std::vector<talk_response> responses;
        void gen_responses( const talk_topic &topic );


        std::vector<bool> response_condition_exists;
        std::vector<bool> response_condition_eval;


        void add_gen_response( const talk_response &resp, bool insert_front, bool condition_exists = true,
                               bool condition_result = true );

        void add_topic( const std::string &topic );
        void add_topic( const talk_topic &topic );

        bool debug_conditionals = true;
        bool debug_effects = true;
        bool debug_ignore_conditionals = false;

    private:



        std::unique_ptr<talker> alpha;



        std::unique_ptr<talker> beta;





        talk_response &add_response( const std::string &text, const std::string &r,
                                     bool first = false );



        talk_response &add_response_done( const std::string &text );



        talk_response &add_response_none( const std::string &text );




        talk_response &add_response( const std::string &text, const std::string &r,
                                     const dialogue_fun_ptr &effect_success, bool first = false );





        talk_response &add_response( const std::string &text, const std::string &r,
                                     const std::function<void( npc & )> &effect_success,
                                     dialogue_consequence consequence, bool first = false );




        talk_response &add_response( const std::string &text, const std::string &r, mission *miss,
                                     bool first = false );




        talk_response &add_response( const std::string &text, const std::string &r, const skill_id &skill,
                                     bool first = false );




        talk_response &add_response( const std::string &text, const std::string &r,
                                     const proficiency_id &proficiency,
                                     bool first = false );




        talk_response &add_response( const std::string &text, const std::string &r,
                                     const spell_id &sp, bool first = false );




        talk_response &add_response( const std::string &text, const std::string &r,
                                     const martialart &style, bool first = false );




        talk_response &add_response( const std::string &text, const std::string &r,
                                     const itype_id &item_type, bool first = false );

        int get_best_quit_response();







        std::vector<std::string> build_debug_info( dialogue_imgui_impl &d_img, const talk_topic &topic,
                int do_response = -1 );
};








struct dynamic_line_t {
    private:
        std::function<std::string( dialogue & )> function;

    public:
        dynamic_line_t() = default;
        explicit dynamic_line_t( const translation &line );
        explicit dynamic_line_t( const JsonObject &jo );
        explicit dynamic_line_t( const JsonArray &ja );
        static dynamic_line_t from_member( const JsonObject &jo, std::string_view member_name );

        std::string operator()( dialogue &d ) const {
            if( !function ) {
                return std::string{};
            }
            return function( d );
        }
};





class json_talk_response
{
    private:
        talk_response actual_response;
        std::function<bool( const_dialogue const & )> condition;
        bool has_condition_ = false;
        bool is_switch = false;
        bool is_default = false;


        translation failure_explanation;

        std::string failure_topic;

        void load_condition( const JsonObject &jo, std::string_view src );

    public:
        json_talk_response() = default;
        explicit json_talk_response( const JsonObject &jo, std::string_view src );

        const talk_response &get_actual_response() const;
        bool has_condition() const {
            return has_condition_;
        }
        bool test_condition( dialogue &d ) const;
        bool show_anyways( dialogue &d ) const;



        bool gen_responses( dialogue &d, bool switch_done );
        bool gen_repeat_response( dialogue &d, const itype_id &item_id, bool switch_done ) const;
};




class json_talk_repeat_response
{
    public:
        json_talk_repeat_response() = default;
        explicit json_talk_repeat_response( const JsonObject &jo, std::string_view src );
        bool is_npc = false;
        bool include_containers = false;
        std::vector<itype_id> for_item;
        std::vector<item_category_id> for_category;
        json_talk_response response;
};

class json_dynamic_line_effect
{
    private:
        std::function<bool( dialogue & )> condition;
        talk_effect_t effect;
    public:
        json_dynamic_line_effect( const JsonObject &jo, const std::string &id, std::string_view src );
        bool test_condition( dialogue &d ) const;
        void apply( dialogue &d ) const;

        std::map<std::string, std::string> debug_info;

};




class json_talk_topic
{
    private:
        bool replace_built_in_responses = false;
        std::vector<json_talk_response> responses;
        dynamic_line_t dynamic_line;
        std::vector<json_dynamic_line_effect> speaker_effects;
        std::vector<json_talk_repeat_response> repeat_responses;

        std::optional<character_portrait_id> portrait_override;

    public:
        json_talk_topic() = default;






        void load( const JsonObject &jo, std::string_view src );

        std::string get_dynamic_line( dialogue &d ) const;
        std::vector<json_dynamic_line_effect> get_speaker_effects() const;
        std::optional<character_portrait_id> get_portrait_override() const;

        void check_consistency() const;







        bool gen_responses( dialogue &d );

        cata::flat_set<std::string> get_directly_reachable_topics( bool only_unconditional ) const;

        std::vector<json_talk_response> get_responses();
};

void unload_talk_topics();
void load_talk_topic( const JsonObject &jo, std::string_view src );
void get_raw_debug_fields( const JsonObject &jo, std::map<std::string, std::string> &debug_info );

#endif
