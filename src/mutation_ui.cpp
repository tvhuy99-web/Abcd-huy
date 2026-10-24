#include "character.h" // IWYU pragma: associated

#include <algorithm> //std::min
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "a11y_input.h"
#include "a11y_runtime.h"
#include "avatar.h"
#include "color.h"
#include "cursesdef.h"
#include "enums.h"
#include "input.h"
#include "input_context.h"
#include "inventory.h"
#include "magic.h"
#include "mutation.h"
#include "output.h"
#include "popup.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"
namespace
{
enum class mutation_menu_mode {
    activating,
    examining,
    reassigning,
    hiding
};
enum class mutation_tab_mode {
    active,
    passive,
    none
};
} // namespace
// '!' and '=' are uses as default bindings in the menu
static const invlet_wrapper
mutation_chars( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\"#&()*+./:;@[\\]^_{|}" );

static void draw_exam_window( const catacurses::window &win, const int border_y )
{
    const int width = getmaxx( win );
    wattron( win, BORDER_COLOR );
    mvwaddch( win, point( 0, border_y ), LINE_XXXO );
    mvwhline( win, point( 1, border_y ), LINE_OXOX, width - 2 );
    mvwaddch( win, point( width - 1, border_y ), LINE_XOXX );
    wattroff( win, BORDER_COLOR );
}

static const auto shortcut_desc = []( std::string_view comment, const std::string &keys )
{
    return string_format( comment, string_format( "[<color_yellow>%s</color>]", keys ) );
};

// needs extensive improvement

static std::optional<trait_id> GetTrait( const std::vector<trait_id> &active,
        const std::vector<trait_id> &passive,
        int cursor, mutation_tab_mode tab_mode )
{
    std::optional<trait_id> mut_id;
    if( tab_mode == mutation_tab_mode::active ) {
        mut_id = active[cursor];
    } else if( tab_mode == mutation_tab_mode::passive ) {
        mut_id = passive[cursor];
    }
    return mut_id;
}

static void show_mutations_titlebar( const catacurses::window &window,
                                     const mutation_menu_mode menu_mode, const input_context &ctxt )
{
    werase( window );
    std::string desc;
    if( menu_mode == mutation_menu_mode::reassigning ) {
        desc += std::string( _( "Reassigning." ) ) + "  " +
                _( "Select a mutation to reassign or press [<color_yellow>SPACE</color>] to cancel. " );
    }
    if( menu_mode == mutation_menu_mode::activating ) {
        desc += colorize( _( "Activating" ),
                          c_green ) + "  " + shortcut_desc( _( "%s Examine, " ),
                                  ctxt.get_desc( "TOGGLE_EXAMINE" ) );
    }
    if( menu_mode == mutation_menu_mode::examining ) {
        desc += colorize( _( "Examining" ),
                          c_light_blue ) + "  " + shortcut_desc( _( "%s Activate, " ),
                                  ctxt.get_desc( "TOGGLE_EXAMINE" ) );
    }
    if( menu_mode == mutation_menu_mode::hiding ) {
        desc += colorize( _( "Hiding" ), c_cyan ) + "  " + shortcut_desc( _( "%s Activate, " ),
                ctxt.get_desc( "TOGGLE_EXAMINE" ) );
    }
    if( menu_mode != mutation_menu_mode::reassigning ) {
        desc += shortcut_desc( _( "%s Reassign, " ), ctxt.get_desc( "REASSIGN" ) );
    }
    desc += shortcut_desc( _( "%s Toggle sprite visibility, " ), ctxt.get_desc( "TOGGLE_SPRITE" ) );
    desc += shortcut_desc( _( "%s Change keybindings." ), ctxt.get_desc( "HELP_KEYBINDINGS" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    fold_and_print( window, point( 1, 0 ), getmaxx( window ) - 1, c_white, desc );
    wnoutrefresh( window );
}

void avatar::power_mutations()
{
    std::vector<trait_id> passive;
    std::vector<trait_id> active;
    for( std::pair<const trait_id, trait_data> &mut : cached_mutations ) {
        if( mut.second.corrupted > 0 ) {
            continue;
        }
        if( !mut.first->player_display ) {
            continue;
        }
        if( !mut.first->activated ) {
            passive.push_back( mut.first );
        } else {
            active.push_back( mut.first );
        }
        // New mutations are initialized with no key at all, so we have to do this here.
        if( mut.second.key == ' ' ) {
            for( const char &letter : mutation_chars ) {
                if( trait_by_invlet( letter ).is_null() && mut.first->activated ) {
                    mut.second.key = letter;
                    break;
                }
            }
        }
    }

    // maximal number of rows in both columns
    const int mutations_count = std::max( passive.size(), active.size() );
    const int TITLE_HEIGHT = 2;

    const int DESCRIPTION_HEIGHT = 5;
    // + lines with text in titlebar, local
    const int HEADER_LINE_Y = TITLE_HEIGHT + 1;
    const int list_start_y = HEADER_LINE_Y + 2;

    // Main window
    /** Total required height is:
    * top frame line:                                         + 1
    * height of title window:                                 + TITLE_HEIGHT
    * line after the title:                                   + 1
    * line with active/passive mutation captions:               + 1
    * height of the biggest list of active/passive mutations:   + mutations_count
    * line before mutation description:                         + 1
    * height of description window:                           + DESCRIPTION_HEIGHT
    * bottom frame line:                                      + 1
    * TOTAL: TITLE_HEIGHT + mutations_count + DESCRIPTION_HEIGHT + 5
    */

    int HEIGHT = 0;
    int WIDTH = 0;
    catacurses::window wBio;

    int DESCRIPTION_LINE_Y = 0;
    catacurses::window w_description;

    catacurses::window w_title;

    int second_column = 0;

    int scroll_position = 0;
    int examine_pos = 0;
    int cursor = 0;
    int max_scroll_position = 0;
    int list_height = 0;
    int half_list_view_location = 0;
    mutation_menu_mode menu_mode = mutation_menu_mode::activating;
    mutation_tab_mode tab_mode;
    if( !active.empty() ) {
        tab_mode = mutation_tab_mode::active;
    } else if( !passive.empty() ) {
        tab_mode = mutation_tab_mode::passive;
    } else {
        tab_mode = mutation_tab_mode::none;
    }

    const auto recalc_max_scroll_position = [&]() {
        list_height = ( menu_mode == mutation_menu_mode::examining ?
                        DESCRIPTION_LINE_Y : HEIGHT - 1 ) - list_start_y;
        max_scroll_position = mutations_count - list_height;
        half_list_view_location = list_height / 2;
        if( max_scroll_position < 0 ) {
            scroll_position = 0;
        } else if( scroll_position > max_scroll_position ) {
            scroll_position = max_scroll_position;
        }
    };

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        HEIGHT = std::min( TERMY, std::max( FULL_SCREEN_HEIGHT,
                                            TITLE_HEIGHT + mutations_count + DESCRIPTION_HEIGHT + 5 ) );
        WIDTH = FULL_SCREEN_WIDTH + ( TERMX - FULL_SCREEN_WIDTH ) / 2;
        const point START( ( TERMX - WIDTH ) / 2, ( TERMY - HEIGHT ) / 2 );
        wBio = catacurses::newwin( HEIGHT, WIDTH, START );

        // Description window @ the bottom of the bionic window
        const int DESCRIPTION_START_Y = START.y + HEIGHT - DESCRIPTION_HEIGHT - 1;
        DESCRIPTION_LINE_Y = DESCRIPTION_START_Y - START.y - 1;
        w_description = catacurses::newwin( DESCRIPTION_HEIGHT, WIDTH - 2,
                                            point( START.x + 1, DESCRIPTION_START_Y ) );

        // Title window
        const int TITLE_START_Y = START.y + 1;
        w_title = catacurses::newwin( TITLE_HEIGHT, WIDTH - 2,
                                      point( START.x + 1, TITLE_START_Y ) );

        recalc_max_scroll_position();
        examine_pos = 0;

        // X-coordinate of the list of active mutations
        second_column = 32 + ( TERMX - FULL_SCREEN_WIDTH ) / 4;

        ui.position_from_window( wBio );
    } );
    ui.mark_resize();

    input_context ctxt( "MUTATIONS", keyboard_mode::keychar );
    ctxt.register_updown();
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "TOGGLE_EXAMINE" );
    ctxt.register_action( "TOGGLE_SPRITE" );
    ctxt.register_action( "REASSIGN" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "SCROLL_TRAIT_INFO_UP" );
    ctxt.register_action( "SCROLL_TRAIT_INFO_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
#if defined(__ANDROID__)
    for( const auto &p : passive ) {
        ctxt.register_manual_key( cached_mutations[p].key, p.obj().name() );
    }
    for( const auto &a : active ) {
        ctxt.register_manual_key( cached_mutations[a].key, a.obj().name() );
    }
#endif

    std::optional<trait_id> examine_id;

    // Reader adapter (R17): this screen previously had no a11y points at
    // all. The snapshot below publishes exactly what the original UI
    // prints: the titlebar mode word, the active/passive captions with
    // their counts, and for the current tab's column the row text as
    // drawn (assigned key, name, and for active traits the cost/cooldown
    // text and the " - Active" suffix). Two states are carried only by
    // color or symbol in the original -- powered (green versus red) and,
    // for passive traits, the sprite-hidden " H" suffix -- so the entry
    // detail restates them in words, next to the same description text
    // the examining window shows (mutation_desc plus the purifier note).
    // Nothing the screen does not print is synthesized here.
    // Intents never touch mutation state directly: focus only moves the
    // original cursor (and the examining target, exactly as UP/DOWN do)
    // and never activates; activate returns the loop's own CONFIRM, so
    // the original branch for the current menu mode runs --
    // choose_key_from_user when reassigning, activation/deactivation
    // with the engine's own resource checks and popups when activating,
    // examining, or the sprite-visibility toggle when hiding. Tab
    // activation returns the original NEXT_TAB (PREV_TAB does the same
    // toggle, and the loop itself resets cursor/scroll); tab focus only
    // observes.
    const auto sync_reader_cursor = [&]( const int index ) {
        cursor = index;
        examine_pos = 0;
        examine_id = GetTrait( active, passive, cursor, tab_mode );
        if( cursor < scroll_position ) {
            scroll_position = cursor;
        } else if( list_height > 0 && cursor - scroll_position >= list_height ) {
            scroll_position = cursor - list_height + 1;
        }
        scroll_position = std::max( 0, std::min( scroll_position,
                                                  std::max( 0, max_scroll_position ) ) );
    };
    a11y::screen_scope reader( &ctxt, "mutations",
    [&]( const a11y::semantic_intent &intent ) -> std::optional<std::string> {
        if( intent.node == "mutations:tab:active" || intent.node == "mutations:tab:passive" ) {
            if( intent.action == "focus" ) {
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "activate" ) {
                const mutation_tab_mode target = intent.node == "mutations:tab:active"
                        ? mutation_tab_mode::active : mutation_tab_mode::passive;
                return target == tab_mode ? std::optional<std::string>( "TIMEOUT" )
                       : std::optional<std::string>( "NEXT_TAB" );
            }
        }
        if( intent.node == "mutations:empty" &&
            ( intent.action == "focus" || intent.action == "activate" ) ) {
            return std::string( "TIMEOUT" );
        }
        if( intent.node.compare( 0, 16, "mutations:entry:" ) == 0 &&
            ( intent.action == "focus" || intent.action == "activate" ) ) {
            // Re-resolve the row against the lists the loop is using at
            // intent time; an index minted against an older snapshot
            // must never act on a trait that is no longer there.
            if( tab_mode == mutation_tab_mode::none ) {
                return std::nullopt;
            }
            const std::string index_text = intent.node.substr( 16 );
            int index = 0;
            bool valid = !index_text.empty();
            for( const char digit : index_text ) {
                if( digit < '0' || digit > '9' ) {
                    valid = false;
                    break;
                }
                index = index * 10 + ( digit - '0' );
                if( index > 100000 ) {
                    valid = false;
                    break;
                }
            }
            const std::vector<trait_id> &current = tab_mode == mutation_tab_mode::active
                                                   ? active : passive;
            if( !valid || index >= static_cast<int>( current.size() ) ||
                cached_mutations.find( current[index] ) == cached_mutations.end() ) {
                return std::nullopt;
            }
            sync_reader_cursor( index );
            return intent.action == "activate" ? std::optional<std::string>( "CONFIRM" )
                   : std::optional<std::string>( "TIMEOUT" );
        }
        return a11y::registered_intent( ctxt, intent );
    } );

    const auto make_reader_snapshot = [&]() {
        a11y::semantic_snapshot snapshot;
        snapshot.title = _( "Mutations" );
        std::string mode_word;
        switch( menu_mode ) {
            case mutation_menu_mode::activating:
                mode_word = _( "Activating" );
                break;
            case mutation_menu_mode::examining:
                mode_word = _( "Examining" );
                break;
            case mutation_menu_mode::reassigning:
                mode_word = _( "Reassigning" );
                break;
            case mutation_menu_mode::hiding:
                mode_word = _( "Hiding" );
                break;
        }
        snapshot.summary = string_format( _( "Mutations: %1$s. %2$d active, %3$d passive." ),
                                          mode_word, static_cast<int>( active.size() ),
                                          static_cast<int>( passive.size() ) );
        if( menu_mode == mutation_menu_mode::reassigning ) {
            // The one titlebar sentence that is not a keybinding hint:
            // the reassigning instructions, including the SPACE cancel.
            snapshot.summary += " " + remove_color_tags(
                                      _( "Select a mutation to reassign or press [<color_yellow>SPACE</color>] to cancel." ) );
        }
        const auto append_tab = [&]( const char *id, const std::string &label,
                                     const mutation_tab_mode tab, const std::size_t count,
                                     const int position ) {
            a11y::semantic_node node;
            node.id = id;
            node.role = "tab";
            node.label = label;
            node.value = std::to_string( count );
            node.detail = string_format( _( "%d mutations" ), static_cast<int>( count ) );
            node.selected = tab_mode == tab;
            node.position = position;
            node.total = 2;
            node.actions = { "focus", "activate" };
            snapshot.nodes.push_back( std::move( node ) );
        };
        append_tab( "mutations:tab:active", _( "Active" ), mutation_tab_mode::active,
                    active.size(), 1 );
        append_tab( "mutations:tab:passive", _( "Passive" ), mutation_tab_mode::passive,
                    passive.size(), 2 );

        const std::vector<trait_id> &current = tab_mode == mutation_tab_mode::active
                                               ? active : passive;
        if( tab_mode == mutation_tab_mode::none || current.empty() ) {
            // The original columns print "None" for an empty list.
            a11y::semantic_node node;
            node.id = "mutations:empty";
            node.role = "status";
            node.label = _( "None" );
            node.actions = { "focus", "activate" };
            snapshot.nodes.push_back( std::move( node ) );
            snapshot.focus = tab_mode == mutation_tab_mode::none ? "mutations:empty"
                             : tab_mode == mutation_tab_mode::active ? "mutations:tab:active"
                             : "mutations:tab:passive";
        } else {
            const int selected_index = std::clamp( cursor, 0,
                                                   static_cast<int>( current.size() ) - 1 );
            for( std::size_t i = 0; i < current.size(); ++i ) {
                const trait_id &tid = current[i];
                const mutation_branch &md = tid.obj();
                const trait_data &td = cached_mutations[tid];
                a11y::semantic_node node;
                node.id = "mutations:entry:" + std::to_string( i );
                node.role = "listitem";
                if( tab_mode == mutation_tab_mode::passive ) {
                    // Row as drawn: "<key> <name>", plus " H" when the
                    // sprite is hidden.
                    node.label = string_format( "%c %s", td.key, mutation_name( md.id ) );
                    if( !td.show_sprite ) {
                        node.label += _( " H" );
                    }
                } else {
                    // Row as drawn: the key, then the text built by
                    // on_redraw (name, cost/cooldown, " - Active"). The
                    // resource-unit assembly is copied verbatim from
                    // on_redraw so the spoken row matches the printed
                    // one, quirks included.
                    std::string mut_desc;
                    std::string resource_unit;
                    int number_of_resource = 0;
                    if( md.hunger ) {
                        resource_unit += _( " kcal" );
                        number_of_resource++;
                    }
                    if( md.thirst ) {
                        if( number_of_resource > 0 ) {
                            resource_unit += _( " &" );
                        }
                        resource_unit += _( " thirst" );
                        number_of_resource++;
                    }
                    if( md.sleepiness ) {
                        if( number_of_resource > 0 ) {
                            resource_unit += _( " &" );
                        }
                        resource_unit += _( " sleepiness" );
                    }
                    if( md.stamina ) {
                        if( number_of_resource > 0 ) {
                            resource_unit += _( " &" );
                        }
                        resource_unit += _( " stamina" );
                    }
                    if( md.mana ) {
                        resource_unit += _( " mana" );
                        number_of_resource++;
                    }
                    mut_desc += mutation_name( md.id );
                    if( md.cost > 0 && md.cooldown > 0_turns ) {
                        mut_desc += string_format( _( " - %d%s / %s" ),
                                                   md.cost, resource_unit,
                                                   to_string_clipped( md.cooldown ) );
                    } else if( md.cost > 0 ) {
                        mut_desc += string_format( _( " - %d%s" ), md.cost, resource_unit );
                    } else if( md.cooldown > 0_turns ) {
                        mut_desc += string_format( _( " - %s" ), to_string_clipped( md.cooldown ) );
                    }
                    if( td.powered ) {
                        mut_desc += _( " - Active" );
                    }
                    node.label = string_format( "%c %s", td.key, mut_desc );
                }
                node.label = remove_color_tags( node.label );
                // Detail: the description the examining window shows
                // for this trait, with the states the original only
                // encodes in color or the " H" symbol restated in words.
                std::string detail = mutation_desc( tid );
                if( !purifiable( tid ) ) {
                    detail +=
                        _( "\n<color_yellow>This trait is an intrinsic part of you now, purifier won't be able to remove it.</color>" );
                }
                if( tab_mode == mutation_tab_mode::active ) {
                    detail += "\n" + std::string( td.powered ? _( "Active" ) : _( "Not active" ) );
                } else if( !td.show_sprite ) {
                    detail += "\n" + std::string( _( "Hidden" ) );
                }
                node.detail = remove_color_tags( detail );
                node.checked = tab_mode == mutation_tab_mode::active && td.powered;
                node.selected = static_cast<int>( i ) == selected_index;
                node.position = static_cast<int>( i ) + 1;
                node.total = static_cast<int>( current.size() );
                if( node.selected ) {
                    snapshot.focus = node.id;
                }
                snapshot.nodes.push_back( std::move( node ) );
            }
        }
        a11y::semantic_snapshot commands = a11y::command_snapshot( ctxt, _( "Mutations" ) );
        snapshot.nodes.insert( snapshot.nodes.end(),
                               std::make_move_iterator( commands.nodes.begin() ),
                               std::make_move_iterator( commands.nodes.end() ) );
        return snapshot;
    };

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( wBio );
        draw_border( wBio, BORDER_COLOR, _( "Mutations" ) );
        wattron( wBio, BORDER_COLOR );
        // Draw line under title
        mvwhline( wBio, point( 1, HEADER_LINE_Y ), LINE_OXOX, WIDTH - 2 );
        // Draw symbols to connect additional lines to border
        mvwaddch( wBio, point( 0, HEADER_LINE_Y ), LINE_XXXO ); // |-
        mvwaddch( wBio, point( WIDTH - 1, HEADER_LINE_Y ), LINE_XOXX ); // -|
        wattroff( wBio, BORDER_COLOR );

        // Captions
        mvwprintz( wBio, point( 2, HEADER_LINE_Y + 1 ), c_light_blue, _( "Passive:" ) );
        mvwprintz( wBio, point( second_column, HEADER_LINE_Y + 1 ), c_light_blue, _( "Active:" ) );

        if( menu_mode == mutation_menu_mode::examining ) {
            draw_exam_window( wBio, DESCRIPTION_LINE_Y );
        }
        nc_color type;
        if( passive.empty() ) {
            mvwprintz( wBio, point( 2, list_start_y ), c_light_gray, _( "None" ) );
        } else {
            for( int i = scroll_position; static_cast<size_t>( i ) < passive.size(); i++ ) {
                const mutation_branch &md = passive[i].obj();
                const trait_data &td = cached_mutations[passive[i]];
                const bool is_highlighted = cursor == static_cast<int>( i );
                if( i - scroll_position == list_height ) {
                    break;
                }
                if( is_highlighted && tab_mode == mutation_tab_mode::passive ) {
                    type = has_base_trait( passive[i] ) ? c_cyan_yellow : c_light_cyan_yellow;
                } else {
                    type = has_base_trait( passive[i] ) ? c_cyan : c_light_cyan;
                }
                mvwprintz( wBio, point( 2, list_start_y + i - scroll_position ),
                           type, "%c %s", td.key, mutation_name( md.id ) );
                if( !td.show_sprite ) {
                    //~ Hint: Letter to show which mutation is Hidden in the mutation menu
                    wprintz( wBio, c_cyan, _( " H" ) );
                }
            }
        }

        if( active.empty() ) {
            mvwprintz( wBio, point( second_column, list_start_y ), c_light_gray, _( "None" ) );
        } else {
            for( int i = scroll_position; static_cast<size_t>( i ) < active.size(); i++ ) {
                const mutation_branch &md = active[i].obj();
                const trait_data &td = cached_mutations[active[i]];
                const bool is_highlighted = cursor == static_cast<int>( i );
                if( i - scroll_position == list_height ) {
                    break;
                }
                if( td.powered ) {
                    if( is_highlighted && tab_mode == mutation_tab_mode::active ) {
                        type = has_base_trait( active[i] ) ? c_green_yellow : c_light_green_yellow;
                    } else {
                        type = has_base_trait( active[i] ) ? c_green : c_light_green;
                    }
                } else {
                    if( is_highlighted && tab_mode == mutation_tab_mode::active ) {
                        type = has_base_trait( active[i] ) ? c_red_yellow : c_light_red_yellow;
                    } else {
                        type = has_base_trait( active[i] ) ? c_red : c_light_red;
                    }
                }
                // TODO: track resource(s) used and specify
                mvwputch( wBio, point( second_column, list_start_y + i - scroll_position ),
                          type, td.key );
                std::string mut_desc;
                std::string resource_unit;
                int number_of_resource = 0;
                if( md.hunger ) {
                    resource_unit += _( " kcal" );
                    number_of_resource++;
                }
                if( md.thirst ) {
                    if( number_of_resource > 0 ) {
                        //~ Resources consumed by a mutation: "kcal & thirst & sleepiness & mana & stamina"
                        resource_unit += _( " &" );
                    }
                    resource_unit += _( " thirst" );
                    number_of_resource++;
                }
                if( md.sleepiness ) {
                    if( number_of_resource > 0 ) {
                        //~ Resources consumed by a mutation: "kcal & thirst & sleepiness & mana & stamina"
                        resource_unit += _( " &" );
                    }
                    resource_unit += _( " sleepiness" );
                }
                if( md.stamina ) {
                    if( number_of_resource > 0 ) {
                        //~ Resources consumed by a mutation: "kcal & thirst & sleepiness & mana & stamina"
                        resource_unit += _( " &" );
                    }
                    resource_unit += _( " stamina" );
                }
                if( md.mana ) {
                    resource_unit += _( " mana" );
                    number_of_resource++;
                }
                mut_desc += mutation_name( md.id );
                if( md.cost > 0 && md.cooldown > 0_turns ) {
                    mut_desc += string_format( _( " - %d%s / %s" ),
                                               md.cost, resource_unit, to_string_clipped( md.cooldown ) );
                } else if( md.cost > 0 ) {
                    mut_desc += string_format( _( " - %d%s" ), md.cost, resource_unit );
                } else if( md.cooldown > 0_turns ) {
                    mut_desc += string_format( _( " - %s" ), to_string_clipped( md.cooldown ) );
                }
                if( td.powered ) {
                    mut_desc += _( " - Active" );
                }
                mvwprintz( wBio, point( second_column + 2, list_start_y + i - scroll_position ),
                           type, mut_desc );
            }
        }

        draw_scrollbar( wBio, scroll_position, list_height, mutations_count,
                        point( 0, list_start_y ), c_white, true );

        if( menu_mode == mutation_menu_mode::examining && examine_id.has_value() ) {
            werase( w_description );
            std::string description = mutation_desc( examine_id.value() );
            if( !purifiable( examine_id.value() ) ) {
                description +=
                    _( "\n<color_yellow>This trait is an intrinsic part of you now, purifier won't be able to remove it.</color>" );
            }
            std::vector<std::string> desc = foldstring( description, WIDTH - 2 );
            const int winh = catacurses::getmaxy( w_description );
            const bool do_scroll = desc.size() > static_cast<unsigned>( std::abs( winh ) );
            const int fline = do_scroll ? examine_pos % ( desc.size() + 1 - winh ) : 0;
            const int lline = do_scroll ? fline + winh : desc.size();
            for( int i = fline; i < lline; i++ ) {
                trim_and_print( w_description, point( 0, i - fline ), WIDTH - 2, c_light_blue, desc[i] );
            }
            draw_scrollbar( wBio, fline, winh, desc.size(), point( 0, catacurses::getmaxy( wBio ) - winh - 1 ),
                            c_white, true );
        }
        wnoutrefresh( wBio );
        show_mutations_titlebar( w_title, menu_mode, ctxt );
        if( menu_mode == mutation_menu_mode::examining && examine_id.has_value() ) {
            wnoutrefresh( w_description );
        }
    } );

    bool exit = false;
    while( !exit ) {
        recalc_max_scroll_position();
        ui_manager::redraw();
        if( a11y::enabled() ) {
            reader.update( make_reader_snapshot() );
        }
        bool handled = false;
        const std::string action = ctxt.handle_input();
        const input_event evt = ctxt.get_raw_input();
        if( evt.type == input_event_t::keyboard_char && !evt.sequence.empty() ) {
            const int ch = evt.get_first_input();
            if( ch == ' ' ) { //skip if space is pressed (space is used as an empty hotkey)
                continue;
            }
            const trait_id mut_id = trait_by_invlet( ch );
            if( !mut_id.is_null() ) {
                const mutation_branch &mut_data = mut_id.obj();
                switch( menu_mode ) {
                    case mutation_menu_mode::reassigning: {
                        int newch = KEY_ESCAPE;
                        do {
                            newch = choose_key_from_user(
                                        string_format( _( "%s; enter new letter." ), mutation_name( mut_id ) ),
                                        mutation_chars.get_allowed_chars() );
                            if( newch != KEY_ESCAPE && !mutation_chars.valid( newch ) ) {
                                popup( _( "Invalid mutation letter.  Only those characters are valid:\n\n%s" ),
                                       mutation_chars.get_allowed_chars() );
                            }
                        } while( newch != KEY_ESCAPE && !mutation_chars.valid( newch ) );
                        if( newch != KEY_ESCAPE ) {
                            const trait_id other_mut_id = trait_by_invlet( newch );
                            if( !other_mut_id.is_null() ) {
                                std::swap( cached_mutations[mut_id].key, cached_mutations[other_mut_id].key );
                            } else {
                                cached_mutations[mut_id].key = newch;
                            }
                        }

                        menu_mode = mutation_menu_mode::activating;
                        examine_id = std::nullopt;
                        // TODO: show a message like when reassigning a key to an item?
                        break;
                    }
                    case mutation_menu_mode::activating: {
                        if( mut_data.activated ) {
                            if( cached_mutations[mut_id].powered ) {
                                add_msg_if_player( m_neutral, _( "You stop using your %s." ), mutation_name( mut_data.id ) );
                                // Reset menu in advance
                                ui.reset();
                                deactivate_mutation( mut_id );
                                // Action done, leave screen
                                exit = true;
                            } else if( ( !mut_data.hunger || get_kcal_percent() >= 0.8f ) &&
                                       ( !mut_data.thirst || get_thirst() <= 400 ) &&
                                       ( !mut_data.sleepiness || get_sleepiness() <= 400 ) &&
                                       ( !mut_data.mana || magic->available_mana() >= mut_data.cost ) &&
                                       ( !mut_data.stamina || get_stamina() >= mut_data.cost ) ) {
                                add_msg_if_player( m_neutral,
                                                   string_format( mut_data.activation_msg.translated(), mutation_name( mut_data.id ) ) );
                                // Reset menu in advance
                                ui.reset();
                                activate_mutation( mut_id );
                                // Action done, leave screen
                                exit = true;
                            } else {
                                popup( _( "You don't have enough in you to activate your %s!" ), mutation_name( mut_data.id ) );
                            }
                        } else {
                            popup( _( "You cannot activate %1$s!  To read a description of "
                                      "%1$s, press '%2$s', then '%3$c'." ),
                                   mutation_name( mut_data.id ), ctxt.get_desc( "TOGGLE_EXAMINE" ), cached_mutations[mut_id].key );
                        }
                        break;
                    }
                    case mutation_menu_mode::examining:
                        // Describing mutations, not activating them!
                        examine_id = mut_id;
                        break;
                    case mutation_menu_mode::hiding:
                        cached_mutations[mut_id].show_sprite = !cached_mutations[mut_id].show_sprite;
                        break;
                }
                handled = true;
            } else if( mutation_chars.valid( ch ) ) {
                handled = true;
            }
        }
        if( !handled ) {

            // Essentially, up-down navigation adapted from the bionics_ui.cpp, with a bunch of extra workarounds to keep functionality

            if( action == "DOWN" ) {

                int lowerlim;

                if( tab_mode == mutation_tab_mode::passive ) {
                    lowerlim = static_cast<int>( passive.size() ) - 1;
                } else if( tab_mode == mutation_tab_mode::active ) {
                    lowerlim = static_cast<int>( active.size() ) - 1;
                } else {
                    continue;
                }

                if( cursor < lowerlim ) {
                    cursor++;
                } else {
                    cursor = 0;
                }
                if( scroll_position < max_scroll_position &&
                    cursor - scroll_position > list_height - half_list_view_location ) {
                    scroll_position++;
                }
                if( scroll_position > 0 && cursor - scroll_position < half_list_view_location ) {
                    scroll_position = std::max( cursor - half_list_view_location, 0 );
                }
                examine_pos = 0;

                // Draw the description, shabby workaround
                examine_id = GetTrait( active, passive, cursor, tab_mode );

            } else if( action == "UP" ) {

                int lim;
                if( tab_mode == mutation_tab_mode::passive ) {
                    lim = passive.size() - 1;
                } else if( tab_mode == mutation_tab_mode::active ) {
                    lim = active.size() - 1;
                } else {
                    continue;
                }
                if( cursor > 0 ) {
                    cursor--;
                } else {
                    cursor = lim;
                }
                if( scroll_position > 0 && cursor - scroll_position < half_list_view_location ) {
                    scroll_position--;
                }
                if( scroll_position < max_scroll_position &&
                    cursor - scroll_position > list_height - half_list_view_location ) {
                    scroll_position =
                        std::max( std::min<int>( lim + 1 - list_height,
                                                 cursor - half_list_view_location ), 0 );
                }
                examine_pos = 0;

                examine_id = GetTrait( active, passive, cursor, tab_mode );
            } else if( ( action == "SCROLL_TRAIT_INFO_UP" || action == "SCROLL_TRAIT_INFO_DOWN" ) &&
                       menu_mode == mutation_menu_mode::examining ) {
                examine_pos += action == "SCROLL_TRAIT_INFO_UP" ? -1 : 1;
            } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
                if( tab_mode == mutation_tab_mode::active && !passive.empty() ) {
                    tab_mode = mutation_tab_mode::passive;
                } else if( tab_mode == mutation_tab_mode::passive && !active.empty() ) {
                    tab_mode = mutation_tab_mode::active;
                } else {
                    continue;
                }
                scroll_position = 0;
                examine_pos = 0;
                cursor = 0;
                examine_id = GetTrait( active, passive, cursor, tab_mode );
            } else if( action == "CONFIRM" ) {
                trait_id mut_id;
                if( tab_mode == mutation_tab_mode::active ) {
                    mut_id = active[cursor];
                } else if( tab_mode == mutation_tab_mode::passive ) {
                    mut_id = passive[cursor];
                } else {
                    continue;
                }
                if( !mut_id.is_null() ) {
                    const mutation_branch &mut_data = mut_id.obj();
                    switch( menu_mode ) {
                        case mutation_menu_mode::reassigning: {
                            int newch = KEY_ESCAPE;
                            do {
                                newch = choose_key_from_user(
                                            string_format( _( "%s; enter new letter." ), mutation_name( mut_id ) ),
                                            mutation_chars.get_allowed_chars(), ' ' );
                                if( newch != KEY_ESCAPE && newch != ' ' && !mutation_chars.valid( newch ) ) {
                                    popup( _( "Invalid mutation letter.  Only those characters are valid:\n\n%s" ),
                                           mutation_chars.get_allowed_chars() );
                                }
                            } while( newch != KEY_ESCAPE && newch != ' ' && !mutation_chars.valid( newch ) );
                            if( newch == ' ' ) {
                                cached_mutations[mut_id].key = newch;
                            } else if( newch != KEY_ESCAPE && mutation_chars.valid( newch ) ) {
                                const trait_id other_mut_id = trait_by_invlet( newch );
                                if( !other_mut_id.is_null() ) {
                                    std::swap( cached_mutations[mut_id].key, cached_mutations[other_mut_id].key );
                                } else {
                                    cached_mutations[mut_id].key = newch;
                                }
                            }

                            menu_mode = mutation_menu_mode::activating;
                            examine_id = std::nullopt;
                            // TODO: show a message like when reassigning a key to an item?
                            break;
                        }
                        case mutation_menu_mode::activating: {
                            if( mut_data.activated ) {
                                if( cached_mutations[mut_id].powered ) {
                                    add_msg_if_player( m_neutral, _( "You stop using your %s." ), mutation_name( mut_data.id ) );
                                    // Reset menu in advance
                                    ui.reset();
                                    deactivate_mutation( mut_id );
                                    // Action done, leave screen
                                    exit = true;
                                } else if( ( !mut_data.hunger || get_kcal_percent() >= 0.8f ) &&
                                           ( !mut_data.thirst || get_thirst() <= 400 ) &&
                                           ( !mut_data.sleepiness || get_sleepiness() <= 400 ) &&
                                           ( !mut_data.mana || magic->available_mana() >= mut_data.cost ) ) {
                                    add_msg_if_player( m_neutral,
                                                       string_format( mut_data.activation_msg.translated(), mutation_name( mut_data.id ) ) );
                                    // Reset menu in advance
                                    ui.reset();
                                    activate_mutation( mut_id );
                                    // Action done, leave screen
                                    exit = true;
                                } else {
                                    popup( _( "You don't have enough in you to activate your %s!" ), mutation_name( mut_data.id ) );
                                }
                            } else {
                                popup( _( "You cannot activate %1$s!  To read a description of "
                                          "%1$s, press '%2$s'." ),
                                       mutation_name( mut_data.id ), ctxt.get_desc( "TOGGLE_EXAMINE" ) );
                            }
                            break;
                        }
                        case mutation_menu_mode::examining:
                            // Describing mutations, not activating them!
                            examine_id = mut_id;
                            break;
                        case mutation_menu_mode::hiding:
                            cached_mutations[mut_id].show_sprite = !cached_mutations[mut_id].show_sprite;
                            break;
                    }
                }
            } else if( action == "REASSIGN" ) {
                menu_mode = mutation_menu_mode::reassigning;
                examine_id = std::nullopt;
            } else if( action == "TOGGLE_EXAMINE" ) {
                // switches between activation and examination
                menu_mode = menu_mode == mutation_menu_mode::activating ?
                            mutation_menu_mode::examining : mutation_menu_mode::activating;
                if( menu_mode == mutation_menu_mode::examining ) {
                    examine_id = GetTrait( active, passive, cursor, tab_mode );
                } else {
                    examine_id = std::nullopt;
                }
                examine_pos = 0;
            } else if( action == "TOGGLE_SPRITE" ) {
                menu_mode = mutation_menu_mode::hiding;
                examine_id = std::nullopt;
            } else if( action == "QUIT" ) {
                exit = true;
            }
        }
    }
}
