#pragma once
#ifndef CATA_SRC_A11Y_WORLD_INPUT_GATE_H
#define CATA_SRC_A11Y_WORLD_INPUT_GATE_H

#include <algorithm>
#include <string>

#include "a11y_semantics.h"

namespace a11y
{

// A character step is an input command, not a selection of a map description.
// Map and speech updates can advance the semantic revision while a finger
// swipe waits for the next game turn. The native *current* walking permission
// and the independently checked context token remain authoritative.
inline bool is_cardinal_step_node( const std::string &node )
{
    return node == "UP" || node == "DOWN" || node == "LEFT" || node == "RIGHT";
}

inline bool is_world_step_node( const std::string &node )
{
    return is_cardinal_step_node( node ) || node == "LEFTUP" ||
           node == "LEFTDOWN" || node == "RIGHTUP" || node == "RIGHTDOWN";
}

// Only the first matching node may authorize a walking step.
inline bool enabled_world_step_node( const semantic_snapshot &snapshot,
                                     const std::string &node_id )
{
    const auto node = std::find_if( snapshot.nodes.begin(), snapshot.nodes.end(),
    [&node_id]( const semantic_node & n ) { return n.id == node_id; } );
    return node != snapshot.nodes.end() && node->enabled &&
           std::find( node->actions.begin(), node->actions.end(), "activate" ) != node->actions.end();
}

inline bool live_world_walk_command( const semantic_snapshot &snapshot,
                                     const semantic_intent &intent )
{
    return snapshot.category == "DEFAULTMODE" && snapshot.swipe_movement_allowed &&
           intent.action == "activate" && is_world_step_node( intent.node ) &&
           intent.value.empty() && intent.revision != 0 && intent.revision <= snapshot.revision &&
           enabled_world_step_node( snapshot, intent.node );
}

} // namespace a11y
#endif // CATA_SRC_A11Y_WORLD_INPUT_GATE_H
