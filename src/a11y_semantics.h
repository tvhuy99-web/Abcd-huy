#pragma once
#ifndef CATA_SRC_A11Y_SEMANTICS_H
#define CATA_SRC_A11Y_SEMANTICS_H

#include <cstdint>
#include <string>
#include <vector>

#include "a11y_context.h"

namespace a11y
{

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
};

struct semantic_snapshot {
    context_token token;
    std::uint64_t revision = 0;
    std::string category;
    std::string title;
    std::string summary;
    std::string focus;
    std::vector<semantic_node> nodes;
};

struct semantic_intent {
    context_token token;
    std::uint64_t revision = 0;
    std::uint64_t request = 0;
    std::string node;
    std::string action;
    std::string value;
};

// IDs/counters are serialized as decimal strings so JavaScript/JSON consumers
// cannot truncate uint64_t identities to a floating point number.
std::string snapshot_json( const semantic_snapshot &snapshot );
std::string snapshot_content_json( const semantic_snapshot &snapshot );

} // namespace a11y
#endif // CATA_SRC_A11Y_SEMANTICS_H
