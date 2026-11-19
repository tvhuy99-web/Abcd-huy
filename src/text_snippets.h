#pragma once
#ifndef CATA_SRC_TEXT_SNIPPETS_H
#define CATA_SRC_TEXT_SNIPPETS_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dialogue.h"
#include "translation.h"
#include "type_id.h"

class JsonArray;
class JsonObject;
class cata_path;

class snippet_library
{
    public:



        void load_snippet( const JsonObject &jsobj, const std::string &src );





        void add_snippets_from_json( const std::string &category, const JsonArray &jarr,
                                     const std::string &src );






        void add_snippet_from_json( const std::string &category, const JsonObject &jo,
                                    const std::string &src );



        void reload_names( const cata_path &path );
        void clear_snippets();

        bool has_category( const std::string &category ) const;






        std::optional<translation> get_snippet_by_id( const snippet_id &id ) const;




        std::optional<talk_effect_t> get_EOC_by_id( const snippet_id &id ) const;




        std::optional<translation> get_name_by_id( const snippet_id &id ) const;




        const translation &get_snippet_ref_by_id( const snippet_id &id ) const;



        bool has_snippet_with_id( const snippet_id &id ) const;











        std::string expand( const std::string &str ) const;






        snippet_id random_id_from_category( const std::string &cat ) const;







        std::optional<translation> random_from_category( const std::string &cat ) const;











        std::optional<translation> random_from_category( const std::string &cat, unsigned int seed ) const;






        snippet_id migrate_hash_to_id( int hash );

        std::vector<std::pair<snippet_id, std::string>> get_snippets_by_category( const std::string &cat,
                bool add_null_id = false );

    private:
        std::unordered_map<snippet_id, translation> snippets_by_id;

        std::unordered_map<snippet_id, translation> name_by_id;
        std::unordered_map<snippet_id, talk_effect_t> EOC_by_id;

        struct weighted_id {

            uint64_t weight_acc;
            snippet_id value;
        };

        struct weighted_translation {

            uint64_t weight_acc;
            translation value;
        };

        struct category_snippets {
            std::vector<weighted_id> ids;
            std::vector<weighted_translation> no_id;
        };
        std::unordered_map<std::string, category_snippets> snippets_by_category;

        std::optional<std::map<int, snippet_id>> hash_to_id_migration;
};

extern snippet_library SNIPPET;

std::string get_hint();

#endif
