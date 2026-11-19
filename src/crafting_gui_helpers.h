#pragma once
#ifndef CATA_SRC_CRAFTING_GUI_HELPERS_H
#define CATA_SRC_CRAFTING_GUI_HELPERS_H

#include <cstddef>
#include "type_id.h"
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "color.h"
#include "item.h"
#include "proficiency.h"
#include "translation.h"

class Character;
class recipe;
class recipe_subset;
class temp_crafting_inventory;

struct crafting_cost_context;
struct tool_comp;




bool cannot_gain_skill_or_prof( const Character &crafter, const recipe &recp );



struct availability {
        explicit availability( Character &_crafter, const recipe *recp, int batch_size = 1,
                               bool camp_crafting = false, temp_crafting_inventory *inventory_override = nullptr );
        Character &crafter;
        bool can_craft_recipe;

        bool crafter_has_primary_skill;
        bool would_use_rotten;
        bool would_use_favorite;
        bool useless_practice;
        bool apparently_craftable;
        bool has_proficiencies;
        bool has_all_skills;
        bool is_nested_category;


        temp_crafting_inventory *inv_override;
    private:
        const recipe *rec;
        mutable float proficiency_time_maluses = -1.0f;
        mutable float max_proficiency_time_maluses = -1.0f;
        mutable float proficiency_skill_maluses = -1.0f;
        mutable float max_proficiency_skill_maluses = -1.0f;
        mutable std::optional<book_proficiency_bonuses> cached_book_bonuses;
        const book_proficiency_bonuses &get_book_bonuses() const;
    public:
        float get_proficiency_time_maluses() const;
        float get_max_proficiency_time_maluses() const;
        float get_proficiency_skill_maluses() const;
        float get_max_proficiency_skill_maluses() const;

        nc_color selected_color() const;
        nc_color color( bool ignore_missing_skills = false ) const;

        static bool check_can_craft_nested( Character &_crafter, const recipe &r, bool camp_crafting,
                                            temp_crafting_inventory *inventory_override );
};



const availability &cached_availability( std::map<const recipe *, availability> &cache,
        Character &crafter, const recipe &rec, bool camp_crafting,
        temp_crafting_inventory *inventory_override );

enum class craft_confirm_result {
    ok,
    cannot_craft,
    too_many_results,
    too_dark
};



craft_confirm_result can_start_craft(
    const recipe &rec,
    const availability &avail,
    const Character &crafter,
    int batch_size = 1 );








bool recipe_sort_compare(
    const recipe *a, const recipe *b,
    const availability &avail_a, const availability &avail_b,
    const Character &crafter, const crafting_cost_context &ctx,
    bool a_read, bool b_read,
    bool unread_first );



std::vector<std::string> recipe_info(
    const recipe &recp, const availability &avail,
    Character &guy, std::string_view qry_comps,
    int batch_size, int fold_width, const nc_color &color,
    const std::vector<Character *> &crafting_group,
    const std::map<itype_id, int> *display_byproducts = nullptr );



std::string practice_recipe_description( const recipe &recp,
        const Character &crafter );






recipe_subset filter_recipes( const recipe_subset &available_recipes,
                              std::string_view qry, const Character &crafter,
                              const std::function<void( size_t, size_t )> &progress_callback );


struct SearchPrefix {
    char key;
    translation example;
    translation description;
};

extern const std::vector<SearchPrefix> prefixes;
extern const translation filter_help_start;



item get_recipe_result_item( const recipe &rec, Character &crafter );






std::vector<iteminfo> recipe_result_info( const recipe &rec, Character &crafter,
        int batch_size, int panel_width, bool full_info = true );


struct recipe_list_data {
    std::vector<const recipe *> entries;
    std::vector<int> indent;
    std::vector<availability> available;
    size_t num_hidden = 0;
};







recipe_list_data build_recipe_list(
    std::vector<const recipe *> picking,
    bool skip_hidden_filter,
    bool skip_sort,
    Character &crafter,
    bool camp_crafting,
    temp_crafting_inventory *inventory_override,
    bool highlight_unread,
    bool unread_first,
    std::map<const recipe *, availability> &availability_cache,
    const recipe_subset &available_recipes );



std::string list_nested( Character &crafter, const recipe *rec,
                         const recipe_subset &available_recipes,
                         int indent_level = 0 );


std::string approx_craft_time( int turns );


nc_color activity_level_color( float exertion );



const std::string &find_tool_group_name( const std::vector<tool_comp> &alts );

#endif
