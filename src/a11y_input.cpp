#include "a11y_input.h"
#include <algorithm>
#include <utility>
#include "input_context.h"
namespace a11y
{
semantic_snapshot command_snapshot( const input_context &context, const std::string &title )
{
    semantic_snapshot snapshot;
    snapshot.title = title;
    for( const std::string &action : context.registered_action_ids() ) {
        if( action == "ANY_INPUT" || action == "COORDINATE" || action == "TIMEOUT" ||
            action == "MOUSE_MOVE" || action == "SELECT" ) {
            continue;
        }
        semantic_node node;
        node.id = action;
        node.role = "command";
        node.label = context.get_action_name( action );
        snapshot.nodes.push_back( std::move( node ) );
    }
    for( std::size_t i = 0; i < snapshot.nodes.size(); ++i ) {
        snapshot.nodes[i].position = i + 1;
        snapshot.nodes[i].total = snapshot.nodes.size();
    }
    return snapshot;
}

std::optional<std::string> registered_intent( const input_context &context,
        const semantic_intent &intent )
{
    const auto &actions = context.registered_action_ids();
    if( intent.action == "cancel" ) {
        for( const std::string &candidate : { "QUIT", "UILIST.QUIT", "TEXT.QUIT", "CANCEL" } ) {
            if( std::find( actions.begin(), actions.end(), candidate ) != actions.end() ) {
                return candidate;
            }
        }
    } else if( intent.action == "activate" &&
               std::find( actions.begin(), actions.end(), intent.node ) != actions.end() ) {
        return intent.node;
    }
    return std::nullopt;
}
} // namespace a11y
