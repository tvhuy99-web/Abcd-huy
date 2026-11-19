#pragma once
#ifndef CATA_SRC_CRAFTING_GUI_H
#define CATA_SRC_CRAFTING_GUI_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "type_id.h"

class Character;
class JsonObject;
class temp_crafting_inventory;
class recipe;
class recipe_subset;









std::pair<Character *, const recipe *> select_crafter_and_crafting_recipe( int &batch_size_out,
        const recipe_id &goto_recipe, Character *crafter, std::string filterstring = "",
        bool camp_crafting = false, temp_crafting_inventory *inventory_override = nullptr );
std::pair<std::vector<const recipe *>, bool> recipes_from_cat( const recipe_subset
        &available_recipes, const crafting_category_id &cat, const std::string &subcat );

void load_recipe_category( const JsonObject &jsobj, const std::string &src );
void reset_recipe_categories();



const std::vector<std::string> *subcategories_for_category( const std::string &category );

struct crafting_category {
    crafting_category_id id;
    bool was_loaded = false;

    bool is_hidden;
    bool is_practice;
    bool is_building;
    bool is_wildcard;
    std::vector<std::string> subcategories;

    void load( const JsonObject &jo, std::string_view src );
    static void finalize_all();
};

#endif
