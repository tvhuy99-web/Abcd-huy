#include "a11y_input.h"
#include <algorithm>
#include <array>
#include <utility>
#include <vector>
#include "input_context.h"
#include "output.h"
#include "translations.h"
namespace a11y
{
namespace
{
// Command-center grouping (R04.3). There is no thematic group data for
// actions anywhere in the game sources: the "category" of a keybinding is
// the input context/screen it belongs to, not a topic. Groups are
// therefore an explicit table in this shared builder, listing only action
// IDs verified against data/raw/keybindings*.json. Every registered
// action that is not listed still appears, under the final "other"
// group: an unclassified command is shown, never hidden.
struct command_group {
    const char *id;
    std::vector<const char *> actions;
};
// Group names are translated from literals so the extraction toolchain
// sees them; a _() call on a variable would silently stay in English.
std::string command_group_name( const std::string &id )
{
    if( id == "movement" ) {
        return _( "Movement and looking" );
    }
    if( id == "interaction" ) {
        return _( "Interaction and items" );
    }
    if( id == "combat" ) {
        return _( "Combat and survival" );
    }
    if( id == "character" ) {
        return _( "Character and information" );
    }
    if( id == "making" ) {
        return _( "Making and vehicles" );
    }
    if( id == "system" ) {
        return _( "Waiting and system" );
    }
    return _( "Other commands on this screen" );
}
const std::array<command_group, 7> &command_groups()
{
    static const std::array<command_group, 7> groups = {{
            { "movement",
              { "UP", "DOWN", "LEFT", "RIGHT", "LEFTUP", "RIGHTUP", "LEFTDOWN", "RIGHTDOWN",
                "LEVEL_UP", "LEVEL_DOWN", "center", "look", "peek", "toggle_run",
                "toggle_crouch", "haul"
              }
            },
            { "interaction",
              { "open", "close", "examine", "pickup", "pickup_all", "inventory", "wear",
                "wield", "drop", "grab", "eat", "read", "unload", "smash", "butcher",
                "chat", "sort_armor", "workout"
              }
            },
            { "combat",
              { "fire", "throw", "autoattack", "safemode" }
            },
            { "character",
              { "player_data", "messages", "diary", "factions", "missions", "map", "zones",
                "bionics", "mutations"
              }
            },
            { "making",
              { "craft", "construct", "disassemble", "control_vehicle" }
            },
            { "system",
              { "wait", "sleep", "save", "help", "HELP_KEYBINDINGS", "debug", "QUIT",
                "CANCEL", "CONFIRM"
              }
            },
            { "other", {} },
        }
    };
    return groups;
}
std::size_t command_group_index( const std::string &action )
{
    const auto &groups = command_groups();
    for( std::size_t i = 0; i + 1 < groups.size(); ++i ) {
        const auto &listed = groups[i].actions;
        if( std::find( listed.begin(), listed.end(), action ) != listed.end() ) {
            return i;
        }
    }
    return groups.size() - 1;
}
} // namespace

semantic_snapshot command_snapshot( const input_context &context, const std::string &title )
{
    semantic_snapshot snapshot;
    snapshot.title = title;
    std::vector<semantic_node> commands;
    for( const std::string &action : context.registered_action_ids() ) {
        if( action == "ANY_INPUT" || action == "COORDINATE" || action == "TIMEOUT" ||
            action == "MOUSE_MOVE" || action == "SELECT" ||
            action.rfind( "A11Y_INTERNAL_", 0 ) == 0 ) {
            continue;
        }
        semantic_node node;
        node.id = action;
        node.role = "command";
        node.label = context.get_action_name( action );
        // The only per-command description the game data actually has is
        // the bound-key description (the same text the keybinding help
        // shows, including its unbound states). There is no declared
        // effect text, cost or parameter schema for actions; adapters
        // that know more overwrite this detail for their own commands
        // (precedent: overmap route/travel commands). Nothing is
        // synthesized from the action ID.
        node.detail = remove_color_tags( context.get_desc( action ) );
        node.actions = { "focus", "activate" };
        commands.push_back( std::move( node ) );
    }
    // Emit commands grouped, keeping registration order inside a group.
    // A group header is a focus-only node: browsing to it reads the group
    // name, and it offers no activation, so it can never be mistaken for
    // an invocable command.
    const auto &groups = command_groups();
    std::vector<bool> emitted( commands.size(), false );
    for( std::size_t g = 0; g < groups.size(); ++g ) {
        bool header_written = false;
        for( std::size_t i = 0; i < commands.size(); ++i ) {
            if( emitted[i] || command_group_index( commands[i].id ) != g ) {
                continue;
            }
            if( !header_written ) {
                semantic_node header;
                header.id = std::string( "group:" ) + groups[g].id;
                header.role = "command";
                header.label = command_group_name( groups[g].id );
                header.actions = { "focus" };
                snapshot.nodes.push_back( std::move( header ) );
                header_written = true;
            }
            emitted[i] = true;
            snapshot.nodes.push_back( std::move( commands[i] ) );
        }
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
        for( const char *const candidate : { "QUIT", "UILIST.QUIT", "TEXT.QUIT", "CANCEL" } ) {
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
