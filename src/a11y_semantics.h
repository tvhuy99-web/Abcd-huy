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

// One table cell of a table row node (R02.2). The column refers to a
// semantic_column of the table declared in the snapshot. "compare" is an
// optional machine-sortable rendering of the value (for example a plain
// number without thousands separators or units) so the Java side can order
// or compare rows without parsing presentation text.
struct semantic_cell {
    std::string column;
    std::string value;
    std::string unit;
    std::string compare;
};

// One selectable value of a form field (R02.3). "value" is the raw value an
// intent carries; "label" is the translated display text.
struct semantic_choice {
    std::string value;
    std::string label;
};

// Column declaration of a table (R02.2), published once per snapshot in
// semantic_snapshot::tables. Row nodes reference it by table id and carry
// one semantic_cell per column they fill.
struct semantic_column {
    std::string id;
    std::string header;
    std::string unit;
};

struct semantic_table {
    std::string id;
    std::vector<semantic_column> columns;
};

// Immutable copies of information the current UI is entitled to display.
// Never place game pointers, hidden map data or raw key sequences in this model.
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

    // Tree contract (R02.1). depth < 0 means the node is not part of a tree.
    // For tree nodes, position/total count the node's sibling set under the
    // same parent (the published set, after pagination), not the flat list.
    // Children of a collapsed node are never published. Expanding or
    // collapsing uses the dedicated "expand"/"collapse"/"open_tree"
    // intents, never a gameplay "activate".
    std::string parent_id;
    int depth = -1;
    bool has_children = false;
    bool expanded = false;

    // Table contract (R02.2). table_id empty means the node is not a table
    // row. Cells are presentation only: a cell never carries its own action.
    std::string table_id;
    int row_index = -1;
    std::vector<semantic_cell> cells;

    // Form contract (R02.3). field_type empty means the node is not a form
    // field. Known types: "bool", "int", "float", "choice", "text".
    // "value" stays the translated display of the applied value, while
    // applied_value carries its raw form. A draft is set with the
    // "set_value"/"select_choice" intents, committed with "apply" and
    // dropped with "cancel_draft"; editing a draft never changes the
    // applied value. field_error is builder-localized text describing why
    // the current draft cannot be applied. depends_on lists node IDs of
    // fields whose applied value constrains this field.
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
    // Engine permission for fast-movement (walking) swipe gestures, decided
    // by the game thread when the snapshot is built (R00.2). Only the world
    // snapshot may set this, and only while the avatar is on foot: not
    // driving or seated in control of a vehicle, not remotely controlling
    // one, not in the death camera, and not busy with an activity or
    // automatic travel. Every other snapshot leaves it false, so a missing
    // or stale flag can never enable walking gestures.
    bool swipe_movement_allowed = false;
    // Table declarations for row nodes (R02.2). Serialized inside the
    // content JSON, so changing a header or unit bumps the revision.
    std::vector<semantic_table> tables;
    std::vector<semantic_node> nodes;
};

// Structural guardrails for one published snapshot (R02.4). Builders with
// more data must paginate (the Java side renders in pages); a snapshot
// beyond these limits is rejected instead of truncated silently.
inline constexpr std::size_t max_snapshot_nodes = 4096;
inline constexpr std::size_t max_snapshot_tables = 16;
inline constexpr std::size_t max_table_columns = 16;
inline constexpr std::size_t max_node_cells = 16;
inline constexpr std::size_t max_form_choices = 128;
inline constexpr int max_tree_depth = 16;
inline constexpr std::size_t max_intent_value_chars = 4096;

// Validation entry points are declared after semantic_intent below.

struct semantic_intent {
    context_token token;
    std::uint64_t revision = 0;
    std::uint64_t request = 0;
    std::string node;
    std::string action;
    std::string value;
};

// Validates the structural contracts above: unique IDs, tree parentage
// without cycles or impossible depths, declared table/column references,
// form ranges and draft values inside their domain. On failure returns
// false and sets reason to a stable machine code (for example
// "duplicate_node_id", "tree_cycle", "value_out_of_range"); builders and
// the Java side map codes to user-facing text, never the reverse.
bool validate_snapshot( const semantic_snapshot &snapshot, std::string &reason );

// Validates the value of a "set_value"/"select_choice" intent against the
// target node's form contract (R02.4). Called on the game thread before the
// intent handler runs, so an out-of-domain value can never reach a builder.
bool validate_intent_value( const semantic_node &node, const semantic_intent &intent,
                            std::string &reason );

// IDs/counters are serialized as decimal strings so JavaScript/JSON consumers
// cannot truncate uint64_t identities to a floating point number.
std::string snapshot_json( const semantic_snapshot &snapshot );
std::string snapshot_content_json( const semantic_snapshot &snapshot );

} // namespace a11y
#endif // CATA_SRC_A11Y_SEMANTICS_H
