#pragma once
#ifndef CATA_SRC_RECIPE_H
#define CATA_SRC_RECIPE_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "build_reqs.h"
#include "calendar.h"
#include "crafting_enums.h"
#include "proficiency.h"
#include "requirements.h"
#include "translation.h"
#include "type_id.h"
#include "value_ptr.h"

class Character;
class JsonObject;
class JsonValue;
class cata_variant;
class item;
class item_components;
class recipe;
class temp_crafting_inventory;
template <typename E> struct enum_traits;

enum scaling_stat : int;

enum class recipe_filter_flags : int {
    none = 0,
    no_rotten = 1,
    no_favorite = 2,
};

enum class recipe_time_flag : int {
    none = 0,
    ignore_proficiencies = 1,
};

template<>
struct enum_traits<recipe_time_flag> {
    static constexpr bool is_flag_enum = true;
};

template<>
struct enum_traits<recipe_filter_flags> {
    static constexpr bool is_flag_enum = true;
};


struct recipe_proficiency {
    proficiency_id id;
    bool _skill_penalty_assigned = false;
    bool required = false;
    float time_multiplier = 0.0f;
    float skill_penalty = 0.0f;
    float learning_time_mult = 1.0f;
    std::optional<time_duration> max_experience = std::nullopt;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};


struct vitamin_resource_cost {
    vitamin_id vitamin;
    int value = 0;
    std::optional<int> safe_level;

    void deserialize( const JsonObject &jo );
};

struct character_resource_costs {
    int mana = 0;
    int stamina = 0;
    std::vector<vitamin_resource_cost> vitamins;

    bool empty() const {
        return mana == 0 && stamina == 0 && vitamins.empty();
    }

    void deserialize( const JsonObject &jo );
};

struct book_recipe_data {
    int skill_req = -1;
    std::optional<translation> alt_name = std::nullopt;
    bool hidden = false;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct practice_recipe_data {



    int min_difficulty;
    int max_difficulty;


    int skill_limit;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct batch_savings {


    struct linear {
        int64_t offset;
        std::optional<int> max_batch;
    };
    struct logistic {


        double rscale;
        int rsize;
    };
    struct none { };

    std::variant<linear, logistic, none> data;
    void deserialize( const JsonValue &jv );

    double apply( double time, int batch_size ) const;
    std::string savings_string() const;

    batch_savings() : data( none{} ) {}
};

struct recipe_step {
    translation name;
    int64_t time = 0;
    float exertion = 0.0f;
    std::vector<recipe_proficiency> proficiencies;
    batch_savings batch_info;

    std::vector<std::pair<requirement_id, int>> reqs_internal;

    std::vector<std::pair<requirement_id, int>> reqs_external;

    requirement_data requirements;

    step_attention attention = step_attention::none;

    std::optional<time_duration> max_time;

    std::optional<time_duration> grace_period;
    translation unattend_message;

    void load( const JsonObject &jo, const std::string &recipe_name, int step_index );
};









struct crafting_cost_context {
    book_proficiency_bonuses books;
    std::vector<float> tool_speeds;


    static crafting_cost_context for_recipe( const Character &guy, const recipe &rec );

    static crafting_cost_context for_proficiencies( const Character &guy );
};

class recipe
{
        friend class recipe_dictionary;
        friend struct mod_tracker;

    private:
        itype_id result_ = itype_id::NULL_ID();
        std::string variant_;

        int64_t time = 0;

        std::string exertion_str;
        float exertion = 0.0f;

    public:
        recipe() = default;

        bool is_null() const {
            return id.is_null();
        }

        explicit operator bool() const {
            return !is_null();
        }

        const itype_id &result() const {
            return result_;
        }

        const std::string &variant() const {
            return variant_;
        }

        const itype_id &container_id() const {
            return container;
        }

        bool was_loaded = false;
        bool obsolete = false;

        crafting_category_id category;
        std::string subcategory;

        translation description;

        std::string get_description( const Character &crafter ) const;

        translation name_;

        int difficulty = 0;



        int get_difficulty( const Character &crafter ) const;


        int get_skill_cap() const;






        const requirement_data &simple_requirements() const {
            return requirements_;
        }

        const deduped_requirement_data &deduped_requirements() const {
            return deduped_requirements_;
        }



        const requirement_data &root_requirements() const {
            return root_requirements_;
        }

        const recipe_id &ident() const {
            return id;
        }


        const character_resource_costs &get_character_resources() const {
            return character_resources;
        }

        bool is_blacklisted() const {
            return requirements_.is_blacklisted();
        }



        bool will_be_blacklisted() const;

        std::function<bool( const item & )> get_component_filter(
            recipe_filter_flags = recipe_filter_flags::none ) const;

        bool npc_can_craft( std::string &reason ) const;

        void apply_all_morale_mods( Character &guy ) const;
        void apply_negative_morale_mods( Character &guy ) const;
        void apply_positive_morale_mods( Character &guy ) const;


        bool never_learn = false;


        requirement_data disassembly_requirements() const {
            if( reversible ) {
                return simple_requirements().disassembly_requirements();
            } else {
                return {};
            }
        }



        std::string result_name( bool decorated = false ) const;
        std::vector<effect_on_condition_id> result_eocs;
        std::pair<int, time_duration> morale_modifier;
        skill_id skill_used;
        std::map<skill_id, int> required_skills;

        character_resource_costs character_resources;

        std::vector<recipe_proficiency> proficiencies;

        std::map<skill_id, int> autolearn_requirements;
        std::map<skill_id, int> learn_by_disassembly;

        std::map<itype_id, book_recipe_data> booksets;

        std::optional<practice_recipe_data> practice_data;



        std::string practice_source_name;
        std::string practice_source_description;

        std::set<recipe_id> nested_category_data;

        std::set<flag_id> flags_to_delete;


        bool character_meets_requirements( const Character &character ) const;

        bool has_character_requirements() const;

        const std::map<scaling_stat, int> &get_character_requirements() const;







        std::string primary_skill_string( const Character &c ) const;

        std::string required_skills_string( const Character &c ) const;

        std::string required_proficiencies_string( const Character *c ) const;
        std::string used_proficiencies_string( const Character *c ) const;
        std::string missing_proficiencies_string( const Character *c ) const;

        std::string recipe_proficiencies_string() const;

        std::vector<proficiency_id> required_proficiencies() const;

        bool character_has_required_proficiencies( const Character &c ) const;

        std::vector<proficiency_id> used_proficiencies() const;


        float proficiency_time_maluses( const Character &crafter,
                                        const book_proficiency_bonuses &books = {} ) const;

        float max_proficiency_time_maluses( const Character &crafter ) const;


        float proficiency_skill_maluses( const Character &crafter,
                                         const book_proficiency_bonuses &books = {} ) const;

        float max_proficiency_skill_maluses( const Character &crafter ) const;


        float exertion_level() const;


        bool has_steps() const {
            return !steps_.empty();
        }
        const std::vector<recipe_step> &steps() const {
            return steps_;
        }
        bool has_attention_steps() const;
        bool has_remaining_attention_steps( int from_step ) const;




        const std::vector<recipe_proficiency> &get_proficiencies() const {
            return has_steps() ? aggregate_proficiencies_ : proficiencies;
        }


        static float proficiency_time_maluses_for_step(
            const Character &crafter, const recipe_step &step,
            const book_proficiency_bonuses &books );




        double step_budget_moves( const Character &guy, size_t step_idx, int batch,
                                  const crafting_cost_context &ctx,
                                  recipe_time_flag flags = recipe_time_flag::none ) const;


        std::string required_all_skills_string( const std::map<skill_id, int> & ) const;



        std::string batch_savings_string() const;



    private:
        std::vector<item> create_result( bool set_components, bool is_food,
                                         item_components *used = nullptr ) const;
    public:
        std::vector<item> create_results( int batch = 1, item_components *used = nullptr ) const;


        std::vector<item> create_byproducts( int batch = 1 ) const;
        std::map<itype_id, int> get_byproducts() const;
        bool in_byproducts( const itype_id &it ) const;
        bool has_byproducts() const;




        const std::map<itype_id, int> &legacy_byproducts() const;
        bool has_byproduct_group() const;

        int64_t batch_time( const Character &guy, int batch, float multiplier, size_t assistants,
                            const crafting_cost_context &ctx ) const;
        time_duration batch_duration( const Character &guy, const crafting_cost_context &ctx,
                                      int batch = 1, float multiplier = 1.0,
                                      size_t assistants = 0 ) const;

        time_duration time_to_craft( const Character &guy, const crafting_cost_context &ctx,
                                     recipe_time_flag flags = recipe_time_flag::none ) const;
        int64_t time_to_craft_moves( const Character &guy, const crafting_cost_context &ctx,
                                     recipe_time_flag flags = recipe_time_flag::none ) const;

        bool has_flag( const std::string &flag_name ) const;

        bool is_reversible() const {
            return reversible;
        }

        void load( const JsonObject &jo, std::string_view src );
        void finalize();


        std::string get_consistency_error() const;

        bool is_practice() const;

        bool is_nested() const;
        bool is_blueprint() const;
        const update_mapgen_id &get_blueprint() const;
        const translation &blueprint_name() const;
        const translation &blueprint_parameter_ui_string(
            const std::string &param_name, const cata_variant &arg_value ) const;
        const std::vector<itype_id> &blueprint_resources() const;
        const std::vector<std::pair<std::string, int>> &blueprint_provides() const;
        const std::vector<std::pair<std::string, int>> &blueprint_requires() const;
        const std::vector<std::pair<std::string, int>> &blueprint_excludes() const;
        const parameterized_build_reqs &blueprint_build_reqs() const;






        void check_blueprint_requirements();

        bool hot_result() const;

        bool removes_raw() const;


        int makes_amount() const;

    private:
        void incorporate_build_reqs();
        void add_requirements( const std::vector<std::pair<requirement_id, int>> &reqs );
        void finalize_step_proficiencies();

        recipe_id id = recipe_id::NULL_ID();
        std::vector<std::pair<recipe_id, mod_id>> src;


        bool abstract = false;


        bool autolearn = false;


        bool contained = false;


        bool sealed = true;


        itype_id container = itype_id::NULL_ID();


        std::string container_variant;


        bool reversible = false;


        int64_t uncraft_time = 0;


        std::vector<std::pair<requirement_id, int>> reqs_external;


        std::vector<std::pair<requirement_id, int>> reqs_internal;


        std::map<scaling_stat, int> character_requirements_;


        requirement_data requirements_;


        requirement_data root_requirements_;


        deduped_requirement_data deduped_requirements_;


        std::vector<recipe_step> steps_;

        std::vector<recipe_proficiency> aggregate_proficiencies_;

        std::set<std::string> flags;


        std::optional<int> charges;


        std::map<itype_id, int> byproducts;


        std::optional<item_group_id> byproduct_group;

        batch_savings batch_info;
        int result_mult = 1;
        update_mapgen_id blueprint;
        translation bp_name;
        using TranslationMap = std::map<std::string, translation>;
        std::map<std::string, TranslationMap> bp_parameter_names;
        std::vector<itype_id> bp_resources;
        std::vector<std::pair<std::string, int>> bp_provides;



        std::vector<std::pair<std::string, int>> bp_requires;
        std::vector<std::pair<std::string, int>> bp_excludes;




        bool bp_autocalc = false;
        bool check_blueprint_needs = false;
        cata::value_ptr<parameterized_build_reqs> bp_build_reqs;
};






float best_quality_speed_modifier( const temp_crafting_inventory &inv,
                                   const Character &crafter, const quality_id &qual, int level );




std::vector<float> compute_tool_speeds( const recipe &rec, const Character &crafter );

#endif
