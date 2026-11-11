#include "a11y_input.h"
#include <algorithm>
#include <array>
#include <utility>
#include <vector>
#include "input_context.h"
#include "avatar.h"
#include "game.h"
#include "move_mode.h"
#include "options.h"
#include "string_formatter.h"
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
// DEFAULTMODE registrations are stable across ordinary footsteps. Reuse
// translated command names and key descriptions until a modal or lifecycle
// transition invalidates them. This cache is accessed only on the game thread.
// State belongs to the live native game, not to a particular Android menu.
// Annotate command *values* instead of rewriting action labels/IDs, so the
// same state is available in combat, automation, search, favorites and any
// screen using the shared command snapshot. The base command cache is never
// stateful: these values must be refreshed whenever a snapshot is requested.
void annotate_live_command_states( semantic_snapshot &snapshot )
{
    for( semantic_node &node : snapshot.nodes ) {
        if( node.role != "command" ||
            std::find( node.actions.begin(), node.actions.end(), "activate" ) == node.actions.end() ) {
            continue;
        }
        const std::string &id = node.id;
        if( id == "toggle_auto_features" || id == "toggle_auto_mining" ||
            id == "toggle_auto_pickup" || id == "autosafe" ) {
            const char *const setting = id == "toggle_auto_features" ? "AUTO_FEATURES" :
                                        id == "toggle_auto_mining" ? "AUTO_MINING" :
                                        id == "toggle_auto_pickup" ? "AUTO_PICKUP" : "AUTOSAFEMODE";
            const bool enabled = get_option<bool>( setting );
            node.value = enabled ? _( "Currently on" ) : _( "Currently off" );
            if( id == "toggle_auto_mining" && enabled && !get_option<bool>( "AUTO_FEATURES" ) ) {
                node.value += ". " + std::string( _( "Inactive: additional auto features are off" ) );
            }
        } else if( id == "toggle_auto_pulp_butcher" || id == "toggle_auto_foraging" ) {
            // These are multi-valued modes, not boolean switches.
            const char *const setting = id == "toggle_auto_pulp_butcher" ?
                                        "AUTO_PULP_BUTCHER" : "AUTO_FORAGING";
            const auto &option = get_options().get_option( setting );
            node.value = option.getValue() == "off" ? _( "Currently off" ) : option.getValueName();
            if( option.getValue() != "off" && !get_option<bool>( "AUTO_FEATURES" ) ) {
                node.value += ". " + std::string( _( "Inactive: additional auto features are off" ) );
            }
        } else if( id == "autoattack" ) {
            // Despite the translated name, this action makes a single
            // automatic melee attack. It has no persistent on/off state.
            node.value = _( "One attack, not an on/off setting" );
            node.detail = _( "Perform one automatic melee attack. This is not an on/off setting." );
        } else if( id == "safemode" && g ) {
            node.value = g->safe_mode == SAFE_MODE_OFF ? _( "Currently off" ) :
                         g->safe_mode == SAFE_MODE_STOP ? _( "On, danger detected" ) :
                         _( "Currently on" );
        } else if( id == "auto_travel_mode" && g ) {
            node.value = g->auto_travel_mode ? _( "Currently on" ) : _( "Currently off" );
        } else if( id == "toggle_thief_mode" && g ) {
            const std::string selected = get_avatar().get_value( "THIEF_MODE" ).str();
            node.value = selected == "THIEF_HONEST" ? _( "Always honest" ) :
                         selected == "THIEF_STEAL" ? _( "Always steal" ) :
                         _( "Ask before taking owned items" );
        } else if( ( id == "haul" || id == "haul_toggle" ) && g ) {
            node.value = get_avatar().is_hauling() ? _( "Currently on" ) : _( "Currently off" );
        } else if( ( id == "toggle_run" || id == "toggle_crouch" ||
                     id == "toggle_prone" || id == "cycle_move" ||
                     id == "cycle_move_reverse" || id == "reset_move" ||
                     id == "open_movement" ) && g ) {
            node.value = string_format( _( "Movement mode: %s" ),
                                        get_avatar().get_desired_move_mode()->name() );
        }
    }
}

std::vector<std::string> cached_world_action_ids;
semantic_snapshot cached_world_commands;
bool cached_world_commands_valid = false;
} // namespace

void invalidate_command_snapshot_cache()
{
    cached_world_commands_valid = false;
    cached_world_action_ids.clear();
    cached_world_commands = semantic_snapshot{};
}

semantic_snapshot command_snapshot( const input_context &context, const std::string &title )
{
    const auto &actions = context.registered_action_ids();
    const bool cacheable = context.category_id() == "DEFAULTMODE";
    if( cacheable && cached_world_commands_valid && cached_world_action_ids == actions ) {
        semantic_snapshot cached = cached_world_commands;
        cached.title = title;
        annotate_live_command_states( cached );
        return cached;
    }
    semantic_snapshot snapshot;
    snapshot.title = title;
    std::vector<semantic_node> commands;
    for( const std::string &action : actions ) {
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
    if( cacheable ) {
        cached_world_action_ids = actions;
        cached_world_commands = snapshot;
        cached_world_commands_valid = true;
    }
    annotate_live_command_states( snapshot );
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
