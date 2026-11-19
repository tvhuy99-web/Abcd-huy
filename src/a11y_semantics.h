#pragma once
#ifndef CATA_SRC_A11Y_SEMANTICS_H
#define CATA_SRC_A11Y_SEMANTICS_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "a11y_context.h"

namespace a11y
{






struct semantic_cell {
    std::string column;
    std::string value;
    std::string unit;
    std::string compare;
};



struct semantic_choice {
    std::string value;
    std::string label;
};




struct semantic_column {
    std::string id;
    std::string header;
    std::string unit;
};

struct semantic_table {
    std::string id;
    std::vector<semantic_column> columns;
};



struct semantic_node {
    std::string id;
    std::string role = "button";
    std::string label;
    std::string value;
    std::string detail;
    std::string disabled_reason;
    bool enabled = true;
    bool selected = false;
    bool checked = false;
    int position = 0;
    int total = 0;
    std::vector<std::string> actions{ "focus", "activate" };







    std::string parent_id;
    int depth = -1;
    bool has_children = false;
    bool expanded = false;



    std::string table_id;
    int row_index = -1;
    std::vector<semantic_cell> cells;










    std::string field_type;
    std::string applied_value;
    bool has_field_range = false;
    double field_min = 0.0;
    double field_max = 0.0;
    double field_step = 0.0;
    std::vector<semantic_choice> choices;
    bool has_draft = false;
    std::string draft_value;
    std::string field_error;
    std::vector<std::string> depends_on;
};

struct semantic_snapshot {
    context_token token;
    std::uint64_t revision = 0;
    std::string category;
    std::string title;
    std::string summary;
    std::string focus;







    bool swipe_movement_allowed = false;


    std::vector<semantic_table> tables;
    std::vector<semantic_node> nodes;
};




inline constexpr std::size_t max_snapshot_nodes = 4096;
inline constexpr std::size_t max_snapshot_tables = 16;
inline constexpr std::size_t max_table_columns = 16;
inline constexpr std::size_t max_node_cells = 16;
inline constexpr std::size_t max_form_choices = 128;
inline constexpr int max_tree_depth = 16;
inline constexpr std::size_t max_intent_value_chars = 4096;



struct semantic_intent {
    context_token token;
    std::uint64_t revision = 0;
    std::uint64_t request = 0;
    std::string node;
    std::string action;
    std::string value;


    std::uint64_t received_at_ms = 0;
};







bool validate_snapshot( const semantic_snapshot &snapshot, std::string &reason );




bool validate_intent_value( const semantic_node &node, const semantic_intent &intent,
                            std::string &reason );



std::string snapshot_json( const semantic_snapshot &snapshot );
std::string snapshot_content_json( const semantic_snapshot &snapshot );

}
#endif
