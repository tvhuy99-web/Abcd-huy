#pragma once
#ifndef CATA_SRC_BEHAVIOR_H
#define CATA_SRC_BEHAVIOR_H

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "type_id.h"

class JsonObject;

namespace behavior
{

class node_t;
class oracle_t;
class strategy_t;

enum class status_t : char { running, success, failure };
struct behavior_return {
    status_t result;
    const node_t *selection;
    float score = 0.0f;
};
















class tree
{
    public:

        std::string tick( const oracle_t *subject );


        std::pair<std::string, float> tick_full( const oracle_t *subject );

        std::string goal() const;

        float last_score() const;

        void add( const node_t *new_node );
    private:
        const node_t *root = nullptr;
        const node_t *active_node = nullptr;
        float active_score = 0.0f;
};

class node_t
{
    public:
        node_t() = default;

        behavior_return tick( const oracle_t *subject ) const;
        std::string goal() const;


        void set_strategy( const strategy_t *new_strategy );
        void add_predicate( const std::function < status_t ( const oracle_t *, const std::string & )> &
                            new_predicate, const std::string &argument = "", const bool &invert_result = false );
        void set_goal( const std::string &new_goal );
        void add_child( const node_t *new_child );
        using score_type = std::function<float( const oracle_t *, std::string_view )>;
        void set_score_function( const score_type &func, const std::string &argument = "" );


        void load( const JsonObject &jo, std::string_view src );
        void check() const;
        string_id<node_t> id;
        std::vector<std::pair<string_id<node_t>, mod_id>> src;
        bool was_loaded = false;
    private:
        std::vector<const node_t *> children;
        const strategy_t *strategy = nullptr;
        using predicate_type = std::function<status_t( const oracle_t *, const std::string & )>;
        std::vector<std::tuple<predicate_type, std::string, bool>> conditions;
        status_t process_predicates( const oracle_t *subject ) const;

        std::string _goal;
        std::optional<std::pair<score_type, std::string>> score_function_;
};


void load_behavior( const JsonObject &jo, const std::string &src );

void reset();

void finalize();

void check_consistency();

}

#endif
