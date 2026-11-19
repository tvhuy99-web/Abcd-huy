#pragma once
#ifndef CATA_SRC_CRAFT_COMMAND_H
#define CATA_SRC_CRAFT_COMMAND_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "coordinates.h"
#include "recipe.h"
#include "requirements.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;
class item;
class read_only_visitable;
class temp_crafting_inventory;
template<typename T> struct enum_traits;

const std::string flag_ALWAYS_START_ON_GROUND( "ALWAYS_START_ON_GROUND" );




enum class usage_from : int {
    none = 0,
    map = 1,
    player = 2,
    both = 1 | 2,
    cancel = 4,
    num_usages_from
};

template<>
struct enum_traits<usage_from> {
    static constexpr usage_from last = usage_from::num_usages_from;
    static constexpr bool is_flag_enum = true;
};




template<typename CompType>
struct comp_selection {

    usage_from use_from = usage_from::none;
    CompType comp;


    std::string nname() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonObject &data );
};


struct step_tool_alloc {
    comp_selection<tool_comp> sel;


    int step_count_units = 0;

    int consumed_buckets = 0;

    bool root_derived = false;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonObject &data );
};











std::vector<std::vector<step_tool_alloc>> select_step_tool_allocs(
        Character &crafter, const recipe &rec, int batch, temp_crafting_inventory &map_inv,
        bool &cancelled, int reselect_step = -1 );






class craft_command
{
    public:

        craft_command() = default;
        craft_command( const recipe *to_make, int batch_size, bool is_long, Character *crafter,
                       const std::optional<tripoint_bub_ms> &loc ) :
            rec( to_make ), batch_size( batch_size ), longcraft( is_long ), crafter( crafter ), loc( loc ) {}





        void execute( const std::optional<tripoint_bub_ms> &new_loc );

        void execute( bool only_cache_comps = false );





        item create_in_progress_craft();

        bool is_long() const {
            return longcraft;
        }


        bool always_start_on_ground() const {
            return rec->has_flag( flag_ALWAYS_START_ON_GROUND );
        }

        bool has_cached_selections() const {
            return !item_selections.empty() || !tool_selections.empty() || !step_tool_allocs.empty();
        }

        bool empty() const {
            return rec == nullptr;
        }
        skill_id get_skill_id();

        bool continue_prompt_liquids( const std::function<bool( const item & )> &filter,
                                      bool no_prompt = false );
        static bool safe_to_unload_comp( const item &it );

    private:
        const recipe *rec = nullptr;
        int batch_size = 0;




        bool longcraft = false;

        Character *crafter;

        recipe_filter_flags flags = recipe_filter_flags::none;



        std::optional<tripoint_bub_ms> loc;

        std::vector<comp_selection<item_comp>> item_selections;
        std::vector<comp_selection<tool_comp>> tool_selections;

        std::vector<std::vector<step_tool_alloc>> step_tool_allocs;


        std::vector<comp_selection<item_comp>> check_item_components_missing(
                                                const read_only_visitable &map_inv ) const;

        std::vector<comp_selection<tool_comp>> check_tool_components_missing(
                                                const read_only_visitable &map_inv ) const;


        bool query_continue( const std::vector<comp_selection<item_comp>> &missing_items,
                             const std::vector<comp_selection<tool_comp>> &missing_tools );
};

#endif
