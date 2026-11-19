#pragma once
#ifndef CATA_SRC_RECIPE_DICTIONARY_H
#define CATA_SRC_RECIPE_DICTIONARY_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "input_context.h"
#include "recipe.h"
#include "type_id.h"

class JsonArray;
class JsonObject;
class JsonOut;
class Character;

class recipe_dictionary
{
        friend class Item_factory;
        friend recipe_id;

    public:

        const std::set<const recipe *> &all_autolearn() const {
            return autolearn;
        }


        const std::set<const recipe *> &all_nested() const {
            return nested;
        }


        const std::set<const recipe *> &all_blueprints() const {
            return blueprints;
        }

        std::map<recipe_id, const recipe *> find_obsoletes( const itype_id &item_id ) const;

        size_t size() const;
        std::map<recipe_id, recipe>::const_iterator begin() const;
        std::map<recipe_id, recipe>::const_iterator end() const;

        bool is_item_on_loop( const itype_id & ) const;


        static const recipe &get_uncraft( const itype_id &id );

        static const recipe &get_craft( const itype_id &id );

        static void load_recipe( const JsonObject &jo, const std::string &src );
        static void load_uncraft( const JsonObject &jo, const std::string &src );
        static void load_practice( const JsonObject &jo, const std::string &src );
        static void load_nested_category( const JsonObject &jo, const std::string &src );

        static void finalize();
        static void check_consistency();
        static void reset();

    protected:




        static void delete_if( const std::function<bool( const recipe & )> &pred );

        static recipe &load( const JsonObject &jo, const std::string &src,
                             std::map<recipe_id, recipe> &out );

    private:
        std::map<recipe_id, recipe> recipes;
        std::map<recipe_id, recipe> uncraft;
        std::set<const recipe *> autolearn;
        std::set<const recipe *> nested;
        std::set<const recipe *> blueprints;
        std::map<const itype_id, const recipe *> obsoletes;
        std::unordered_set<itype_id> items_on_loops;

        static void finalize_internal( std::map<recipe_id, recipe> &obj );
        void find_items_on_loops();
};

extern recipe_dictionary recipe_dict;

class recipe_subset
{
    public:
        recipe_subset() = default;
        recipe_subset( const recipe_subset &src, const std::vector<const recipe *> &recipes );





        void include( const recipe *r, int custom_difficulty = -1 );
        void include( const recipe_subset &subset );
        void remove( const recipe *r );





        template<class Predicate>
        void include_if( const recipe_subset &subset, Predicate pred ) {
            for( const recipe * const &elem : subset ) {
                if( pred( *elem ) ) {
                    include( elem );
                }
            }
        }


        bool contains( const recipe *r ) const {
            return std::any_of( recipes.begin(), recipes.end(), [r]( const recipe * elem ) {
                return elem->ident() == r->ident();
            } );
        }





        int get_custom_difficulty( const recipe *r ) const;


        bool empty_category( const crafting_category_id &cat,
                             const std::string &subcat = std::string() ) const;


        std::vector<const recipe *> in_category(
            const crafting_category_id &cat,
            const std::string &subcat = std::string() ) const;


        const std::set<const recipe *> &of_component( const itype_id &id ) const;

        enum class search_type : int {
            name,
            exclude_name,
            skill,
            primary_skill,
            component,
            tool,
            quality,
            quality_result,
            length,
            volume,
            mass,
            covers,
            layer,
            description_result,
            proficiency,
            difficulty,
            activity_level,
            book
        };


        std::vector<const recipe *> favorite() const;


        std::vector<const recipe *> recent() const;


        std::vector<const recipe *> hidden() const;


        std::vector<const recipe *> expanded() const;


        std::vector<const recipe *> search(
            std::string_view txt, search_type key = search_type::name,
            const Character *crafter = nullptr,
            const std::function<void( size_t, size_t )> &progress_callback = {} ) const;

        recipe_subset reduce(
            std::string_view txt, search_type key = search_type::name,
            const std::function<void( size_t, size_t )> &progress_callback = {} ) const;
        recipe_subset reduce(
            std::string_view txt, const Character &crafter, search_type key = search_type::name,
            const std::function<void( size_t, size_t )> &progress_callback = {} ) const;

        recipe_subset intersection( const recipe_subset &subset ) const;

        recipe_subset difference( const recipe_subset &subset ) const;
        recipe_subset difference( const std::set<const recipe *> &recipe_set ) const;

        std::vector<const recipe *> recipes_that_produce( const itype_id &item ) const;

        size_t size() const {
            return recipes.size();
        }

        void clear() {
            component.clear();
            category.clear();
            recipes.clear();
        }

        std::set<const recipe *>::const_iterator begin() const {
            return recipes.begin();
        }

        std::set<const recipe *>::const_iterator end() const {
            return recipes.end();
        }

    private:
        std::set<const recipe *> recipes;
        std::map<const recipe *, int> difficulties;
        std::map<crafting_category_id, std::set<const recipe *>> category;
        std::map<itype_id, std::set<const recipe *>> component;
        mutable input_context ctxt;
};

void serialize( const recipe_subset &value, JsonOut &jsout );
void deserialize( recipe_subset &value, const JsonArray &ja );

#endif
