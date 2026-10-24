#include "iuse_software_lightson.h"

#include "a11y_document.h"
#include "a11y_input.h"
#include "a11y_runtime.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "coordinates.h"
#include "cursesdef.h"
#include "input_context.h"
#include "output.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_helpers.h"
#include "ui_manager.h"

void lightson_game::new_level()
{
    win = false;

    // level generation
    const int half_perimeter = rng( 8, 11 );
    const int lvl_width = rng( 4, 6 );
    const int lvl_height = half_perimeter - lvl_width;
    level_size = point( lvl_width, lvl_height );
    level.resize( static_cast<size_t>( lvl_height ) * static_cast<size_t>( lvl_width ) );

    const int steps_rng = half_perimeter / 2.0 + rng_float( 0.0, 2.0 );
    generate_change_coords( steps_rng );

    reset_level();
}

void lightson_game::reset_level()
{
    std::fill( level.begin(), level.end(), true );

    // change level
    std::for_each( change_coords.begin(), change_coords.end(), [this]( point & p ) {
        toggle_lights_at( p );
    } );

    position = point::zero;
}

bool lightson_game::get_value_at( const point &pt )
{
    return level[pt.y * level_size.x + pt.x];
}

void lightson_game::set_value_at( const point &pt, bool value )
{
    level[pt.y * level_size.x + pt.x] = value;
}

void lightson_game::toggle_value_at( const point &pt )
{
    set_value_at( pt, !get_value_at( pt ) );
}

void lightson_game::draw_level()
{
    werase( w );
    draw_border( w );
    for( int i = 0; i < level_size.y; i++ ) {
        for( int j = 0; j < level_size.x; j++ ) {
            point current = point( j, i );
            bool selected = position == current;
            bool on = get_value_at( current );
            const nc_color fg = on ? c_white : c_dark_gray;
            const char symbol = on ? '#' : '-';
            mvwputch( w, current + point::south_east, selected ? hilite( c_white ) : fg, symbol );
        }
    }
    wnoutrefresh( w );
}

void lightson_game::generate_change_coords( int changes )
{
    change_coords.resize( changes );
    const int size = level_size.y * level_size.x;

    point candidate;
    for( int k = 0; k < changes; k++ ) {
        do {
            const int candidate_index = rng( 0, size - 1 );

            candidate.x = candidate_index % level_size.x;
            candidate.y = ( candidate_index - candidate.x ) / level_size.x;
            // not accept repeatable coordinates
        } while( !( k == 0 ||
                    std::find( change_coords.begin(), change_coords.end(), candidate ) == change_coords.end() ) );
        change_coords[k] = candidate;
    }
}

bool lightson_game::check_win()
{
    return std::all_of( level.begin(), level.end(), []( bool i ) {
        return i;
    } );
}

void lightson_game::toggle_lights()
{
    toggle_lights_at( position );
}

void lightson_game::toggle_lights_at( const point &pt )
{
    toggle_value_at( pt );

    if( pt.y > 0 ) {
        toggle_value_at( pt + point::north );
    }
    if( pt.y < level_size.y - 1 ) {
        toggle_value_at( pt + point::south );
    }

    if( pt.x > 0 ) {
        toggle_value_at( pt + point::west );
    }
    if( pt.x < level_size.x - 1 ) {
        toggle_value_at( pt + point::east );
    }
}

int lightson_game::start_game()
{
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        ui_helpers::full_screen_window( ui, &w, &w_border, nullptr, nullptr, nullptr, 1, 0, 4 );
    } );
    ui.mark_resize();

    input_context ctxt( "LIGHTSON" );
    ctxt.register_directions();
    ctxt.register_action( "TOGGLE_SPACE" );
    ctxt.register_action( "TOGGLE_5" );
    ctxt.register_action( "RESET" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    // Reader adapter: the board is drawn by hand, so the generic command
    // fallback would never publish the lights themselves. Every cell is
    // public information in this game (there is no hidden state), so the
    // whole board is published as one node per cell. Focusing a cell only
    // moves the cursor and observes it; only activating a cell toggles,
    // through the same TOGGLE_SPACE action the keyboard uses.
    a11y::screen_scope reader( &ctxt, "lightson",
    [&]( const a11y::semantic_intent & intent ) -> std::optional<std::string> {
        if( intent.node.rfind( "lights:cell:", 0 ) == 0 ) {
            const std::string suffix =
                intent.node.substr( std::string( "lights:cell:" ).size() );
            const std::size_t separator = suffix.find( ':' );
            if( separator == std::string::npos ) {
                return std::nullopt;
            }
            const std::string x_text = suffix.substr( 0, separator );
            const std::string y_text = suffix.substr( separator + 1 );
            if( x_text.empty() || y_text.empty() || x_text.size() > 3 || y_text.size() > 3 ||
                x_text.find_first_not_of( "0123456789" ) != std::string::npos ||
                y_text.find_first_not_of( "0123456789" ) != std::string::npos ) {
                return std::nullopt;
            }
            const point cell( std::stoi( x_text ), std::stoi( y_text ) );
            if( cell.x < 0 || cell.x >= level_size.x || cell.y < 0 ||
                cell.y >= level_size.y ) {
                return std::nullopt;
            }
            if( intent.action == "focus" ) {
                position = cell;
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "activate" ) {
                position = cell;
                return std::string( "TOGGLE_SPACE" );
            }
            return std::nullopt;
        }
        return a11y::registered_intent( ctxt, intent );
    } );

    const auto make_reader_snapshot = [&]() {
        a11y::semantic_snapshot snapshot = a11y::command_snapshot( ctxt,
                                           _( "Lights On" ) );
        std::vector<a11y::semantic_node> commands = std::move( snapshot.nodes );
        snapshot.nodes.clear();

        int on_count = 0;
        for( int y = 0; y < level_size.y; ++y ) {
            for( int x = 0; x < level_size.x; ++x ) {
                if( get_value_at( point( x, y ) ) ) {
                    ++on_count;
                }
            }
        }
        const int total_cells = level_size.x * level_size.y;
        const bool cursor_on = total_cells > 0 && get_value_at( position );
        snapshot.summary = string_format(
                               _( "Lights On: %1$d of %2$d lights are on. Cursor at row %3$d, "
                                  "column %4$d: %5$s. Toggling a light also toggles its "
                                  "4 neighbors; turn every light on to win." ),
                               on_count, total_cells, position.y + 1, position.x + 1,
                               cursor_on ? _( "On" ) : _( "Off" ) );

        a11y::append_document( snapshot, "lights:rules", _( "Lights On rules" ),
                               _( "Game goal: switch all the lights on. # is on and - is off. "
                                  "Toggling a light switches the selected light and its "
                                  "4 neighbors (above, below, left and right); a neighbor "
                                  "outside the board is not toggled. Move the cursor with "
                                  "the direction keys, toggle the selected light with the "
                                  "space bar or 5, reset to return the board to its "
                                  "starting state, and quit to leave the game." ) );

        for( int y = 0; y < level_size.y; ++y ) {
            for( int x = 0; x < level_size.x; ++x ) {
                const point cell( x, y );
                const bool on = get_value_at( cell );
                a11y::semantic_node node;
                node.id = string_format( "lights:cell:%d:%d", x, y );
                node.role = "button";
                node.label = string_format( _( "Row %d, column %d" ), y + 1, x + 1 );
                node.value = on ? _( "On" ) : _( "Off" );
                node.selected = cell == position;
                node.actions = { "focus", "activate" };
                if( node.selected ) {
                    snapshot.focus = node.id;
                }
                snapshot.nodes.push_back( std::move( node ) );
            }
        }

        snapshot.nodes.insert( snapshot.nodes.end(),
                               std::make_move_iterator( commands.begin() ),
                               std::make_move_iterator( commands.end() ) );
        for( size_t i = 0; i < snapshot.nodes.size(); ++i ) {
            snapshot.nodes[i].position = static_cast<int>( i ) + 1;
            snapshot.nodes[i].total = static_cast<int>( snapshot.nodes.size() );
        }
        return snapshot;
    };

    ui.on_redraw( [&]( const ui_adaptor & ) {
        std::vector<std::string> shortcuts;
        shortcuts.emplace_back( _( "<spacebar or 5> toggle lights" ) );
        shortcuts.emplace_back( _( "<r>eset" ) );
        shortcuts.emplace_back( _( "<q>uit" ) );

        int iWidth = 0;
        for( auto &shortcut : shortcuts ) {
            if( iWidth > 0 ) {
                iWidth += 1;
            }
            iWidth += utf8_width( shortcut );
        }

        werase( w_border );
        draw_border( w_border );
        int iPos = FULL_SCREEN_WIDTH - iWidth - 1;
        for( auto &shortcut : shortcuts ) {
            shortcut_print( w_border, point( iPos, 0 ), c_white, c_light_green, shortcut );
            iPos += utf8_width( shortcut ) + 1;
        }

        mvwputch( w_border, point( 2, 0 ), hilite( c_white ), _( "Lights on!" ) );
        fold_and_print( w_border, point( 2, getmaxy( w_border ) - 5 ), FULL_SCREEN_WIDTH - 4, c_light_gray,
                        "%s\n%s\n%s", _( "<color_white>Game goal:</color> Switch all the lights on." ),
                        _( "<color_white>Legend: #</color> on, <color_dark_gray>-</color> off." ),
                        _( "Toggle lights switches selected light and 4 its neighbors." ) );

        wnoutrefresh( w_border );

        draw_level();
    } );

    win = true;
    int hasWon = 0;

    do {
        if( win ) {
            new_level();
        }
        ui_manager::redraw();
        if( a11y::enabled() ) {
            reader.update( make_reader_snapshot() );
        }
        std::string action = ctxt.handle_input();
        if( const std::optional<tripoint_rel_ms> vec = ctxt.get_direction_rel_ms( action ) ) {
            position.y = clamp( position.y + vec->y(), 0, level_size.y - 1 );
            position.x = clamp( position.x + vec->x(), 0, level_size.x - 1 );
        } else if( action == "TOGGLE_SPACE" || action == "TOGGLE_5" ) {
            toggle_lights();
            win = check_win();
            if( win ) {
                ui.invalidate_ui();
                popup_top( _( "Congratulations, you won!" ) );
                hasWon++;
            }
        } else if( action == "RESET" ) {
            reset_level();
        } else if( action == "QUIT" ) {
            break;
        }
    } while( true );

    return hasWon;
}
