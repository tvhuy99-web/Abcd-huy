#include "iuse_software_minesweeper.h"

#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "a11y_document.h"
#include "a11y_input.h"
#include "a11y_runtime.h"
#include "catacharset.h"
#include "color.h"
#include "coordinates.h"
#include "cursesdef.h"
#include "input_context.h"
#include "output.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui_helpers.h"
#include "ui_manager.h"
#include "uilist.h"

minesweeper_game::minesweeper_game()
{
    min = point( 8, 8 );
    max = point::zero;
    offset = point::zero;
}

void minesweeper_game::new_level()
{
    auto set_num = [&]( const std::string & sType, int &iVal, const int iMin, const int iMax ) {
        const std::string desc = string_format( _( "Min: %d Max: %d" ), iMin, iMax );

        do {
            if( iVal < iMin || iVal > iMax ) {
                iVal = iMin;
            }

            string_input_popup()
            .title( sType )
            .width( 5 )
            .description( desc )
            .edit( iVal );
        } while( iVal < iMin || iVal > iMax );
    };

    uilist difficulty;
    difficulty.allow_cancel = false;
    difficulty.text = _( "Game Difficulty" );
    difficulty.entries.emplace_back( 0, true, 'b', _( "Beginner" ) );
    difficulty.entries.emplace_back( 1, true, 'i', _( "Intermediate" ) );
    difficulty.entries.emplace_back( 2, true, 'e', _( "Expert" ) );
    difficulty.entries.emplace_back( 3, true, 'c', _( "Custom" ) );
    difficulty.query();

    switch( difficulty.ret ) {
        case 0:
            level = point( 8, 8 );
            iBombs = 10;
            break;
        case 1:
            level = point( 16, 16 );
            iBombs = 40;
            break;
        case 2:
            level = point( 30, 16 );
            iBombs = 99;
            break;
        case 3:
        default:
            point new_level = min;

            set_num( _( "Level width:" ), new_level.x, min.x, max.x );
            set_num( _( "Level height:" ), new_level.y, min.y, max.y );

            const int area = new_level.x * new_level.y;
            int new_ibombs = area * 0.1;

            set_num( _( "Number of bombs:" ), new_ibombs, new_ibombs, area * 0.6 );

            level = new_level;
            iBombs = new_ibombs;
            break;
    }

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    offset = ( max - level ) / 2 + point( 1, 1 );

    mLevel.clear();
    mLevelReveal.clear();

    point iRand;
    for( int i = 0; i < iBombs; i++ ) {
        do {
            iRand.x = rng( 0, level.x - 1 );
            iRand.y = rng( 0, level.y - 1 );
        } while( mLevel[iRand.y][iRand.x] == bomb );

        mLevel[iRand.y][iRand.x] = bomb;
    }

    for( int y = 0; y < level.y; y++ ) {
        for( int x = 0; x < level.x; x++ ) {
            if( mLevel[y][x] == bomb ) {
                for( const point &p : closest_points_first( {x, y}, 1 ) ) {
                    if( p.x >= 0 && p.x < level.x && p.y >= 0 && p.y < level.y ) {
                        if( mLevel[p.y][p.x] != bomb ) {
                            mLevel[p.y][p.x]++;
                        }
                    }
                }
            }
        }
    }
}

bool minesweeper_game::check_win()
{
    for( int y = 0; y < level.y; y++ ) {
        for( int x = 0; x < level.x; x++ ) {
            if( ( mLevelReveal[y][x] == flag || mLevelReveal[y][x] == unknown ) &&
                mLevel[y][x] != bomb ) {
                return false;
            }
        }
    }

    return true;
}

int minesweeper_game::start_game()
{
    catacurses::window w_minesweeper_border;
    catacurses::window w_minesweeper;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        ui_helpers::full_screen_window( ui, &w_minesweeper, &w_minesweeper_border );
        max = point( FULL_SCREEN_WIDTH - 4, FULL_SCREEN_HEIGHT - 4 );
    } );
    ui.mark_resize();

    input_context ctxt( "MINESWEEPER" );
    ctxt.register_cardinal();
    ctxt.register_action( "NEW" );
    ctxt.register_action( "FLAG" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    static const std::array<nc_color, 9> aColors = {{
            c_white,
            c_light_gray,
            c_cyan,
            c_blue,
            c_light_blue,
            c_green,
            c_magenta,
            c_red,
            c_yellow
        }
    };

    int iScore = 5;

    int iPlayerY = 0;
    int iPlayerX = 0;

    bool started = false;
    bool boom = false;

    // Reader adapter: this minigame draws its board by hand, so without
    // its own adapter the input context would only get the generic
    // command fallback, which never publishes the board itself.
    //
    // KNOWLEDGE BOUNDARY (hard): mLevel is hidden data. A cell node may
    // only publish what the player can already see on screen: a seen
    // cell publishes its true adjacent-mine count, a flagged cell
    // publishes "Flagged" and a covered cell publishes "Covered". The
    // snapshot builder reads mLevel for a cell only when that cell is
    // seen, or when boom is true -- after a loss the engine itself
    // reveals the whole board (reveal_all() plus the '*' drawing in
    // the redraw callback), so a mine may then be published as "Mine",
    // exactly as drawn. No cell is ever described as safe, and no
    // hidden mine is ever located or counted per position. The cell
    // text helper below is the single place where this gate lives; the
    // summary's cursor description reuses it, so there is no second
    // path around it. Flag/revealed counts come from mLevelReveal
    // (player-visible state) and the total mine count is iBombs, the
    // public parameter of the chosen difficulty.
    //
    // The handler never reveals or flags a cell itself: "activate" on
    // a cell only moves the cursor onto it and returns CONFIRM, and
    // the "flag" action only moves the cursor and returns FLAG, so the
    // loop below applies them through the original code, exactly like
    // a keypress. "focus" only moves the cursor and returns TIMEOUT.
    a11y::screen_scope reader( &ctxt, "minesweeper",
    [&]( const a11y::semantic_intent &intent ) -> std::optional<std::string> {
        if( intent.node.rfind( "mine:cell:", 0 ) == 0 &&
            ( intent.action == "focus" || intent.action == "activate" ||
              intent.action == "flag" ) ) {
            // Node IDs are "mine:cell:<x>:<y>", built by the snapshot
            // builder below. The coordinates are re-validated against
            // the live board at intent time; nothing is trusted from
            // the published snapshot.
            const std::size_t prefix_len = std::string( "mine:cell:" ).size();
            const std::size_t colon = intent.node.find( ':', prefix_len );
            if( colon == std::string::npos ) {
                return std::nullopt;
            }
            // Digit-by-digit parse: a malformed ID from the reader is
            // rejected, never an exception on the game thread.
            const auto parse_coord = []( const std::string & text, int &out ) {
                if( text.empty() || text.size() > 6 ) {
                    return false;
                }
                int value = 0;
                for( const char ch : text ) {
                    if( ch < '0' || ch > '9' ) {
                        return false;
                    }
                    value = value * 10 + ( ch - '0' );
                }
                out = value;
                return true;
            };
            int x = 0;
            int y = 0;
            if( !parse_coord( intent.node.substr( prefix_len, colon - prefix_len ), x ) ||
                !parse_coord( intent.node.substr( colon + 1 ), y ) ||
                x >= level.x || y >= level.y ) {
                return std::nullopt;
            }
            iPlayerX = x;
            iPlayerY = y;
            if( intent.action == "focus" ) {
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "flag" ) {
                return std::string( "FLAG" );
            }
            return std::string( "CONFIRM" );
        }
        return a11y::registered_intent( ctxt, intent );
    } );

    const auto make_reader_snapshot = [&]() {
        a11y::semantic_snapshot snapshot = a11y::command_snapshot( ctxt,
                                           _( "Minesweeper" ) );
        std::vector<a11y::semantic_node> commands = std::move( snapshot.nodes );
        snapshot.nodes.clear();

        if( !started ) {
            // The difficulty menu (uilist) and the custom-size prompts
            // (string_input_popup) run inside new_level() and already
            // have their own shared adapters; until a board exists this
            // screen publishes only its commands.
            snapshot.summary = _( "Choose a difficulty to start." );
            snapshot.nodes = std::move( commands );
            for( size_t i = 0; i < snapshot.nodes.size(); ++i ) {
                snapshot.nodes[i].position = static_cast<int>( i ) + 1;
                snapshot.nodes[i].total = static_cast<int>( snapshot.nodes.size() );
            }
            return snapshot;
        }

        // Sparse-map lookups that never insert: cells the engine has
        // not touched yet are simply unknown / not mines here.
        const auto reveal_at = [&]( const int x, const int y ) {
            const auto row_it = mLevelReveal.find( y );
            if( row_it != mLevelReveal.end() ) {
                const auto cell_it = row_it->second.find( x );
                if( cell_it != row_it->second.end() ) {
                    return cell_it->second;
                }
            }
            return unknown;
        };
        const auto level_at = [&]( const int x, const int y ) {
            const auto row_it = mLevel.find( y );
            if( row_it != mLevel.end() ) {
                const auto cell_it = row_it->second.find( x );
                if( cell_it != row_it->second.end() ) {
                    return cell_it->second;
                }
            }
            return 0;
        };

        // The knowledge boundary, in one place (see the comment above):
        // mLevel is consulted only for a seen cell, or after boom, when
        // the engine has already revealed the whole board on screen.
        const auto cell_value_text = [&]( const int x, const int y ) -> std::string {
            const reveal cell_reveal = reveal_at( x, y );
            if( cell_reveal == seen ) {
                const int num = level_at( x, y );
                if( num == bomb ) {
                    return _( "Mine" );
                }
                if( num == 0 ) {
                    return _( "No adjacent mines" );
                }
                return std::to_string( num );
            }
            if( boom && level_at( x, y ) == bomb ) {
                return _( "Mine" );
            }
            if( cell_reveal == flag ) {
                return _( "Flagged" );
            }
            return _( "Covered" );
        };

        int flag_count = 0;
        int revealed_count = 0;
        for( int y = 0; y < level.y; ++y ) {
            for( int x = 0; x < level.x; ++x ) {
                if( reveal_at( x, y ) == flag ) {
                    ++flag_count;
                } else if( reveal_at( x, y ) == seen ) {
                    ++revealed_count;
                }
            }
        }
        const int total_cells = level.x * level.y;

        // Board-size guard: with the default FULL_SCREEN_WIDTH (80) and
        // FULL_SCREEN_HEIGHT (24, or 25 on an odd-height terminal) the
        // largest Custom board is (80 - 4) by (25 - 4) = 1596 cells at
        // most, and Expert is 30 by 16 = 480 cells, both far below the
        // snapshot limit. Should a board
        // ever grow past the budget anyway, publish only a window
        // around the cursor (which is therefore always published) and
        // say so in the summary, instead of letting the snapshot be
        // rejected for exceeding a11y::max_snapshot_nodes.
        constexpr std::size_t node_budget = a11y::max_snapshot_nodes - 96;
        const std::size_t overhead = commands.size() + 1; // + the rules document
        const bool windowed = static_cast<std::size_t>( total_cells ) + overhead > node_budget;
        int win_x0 = 0;
        int win_y0 = 0;
        int win_x1 = level.x - 1;
        int win_y1 = level.y - 1;
        if( windowed ) {
            int radius = 30;
            while( radius > 5 ) {
                win_x0 = std::max( 0, iPlayerX - radius );
                win_x1 = std::min( level.x - 1, iPlayerX + radius );
                win_y0 = std::max( 0, iPlayerY - radius );
                win_y1 = std::min( level.y - 1, iPlayerY + radius );
                const std::size_t window_cells = static_cast<std::size_t>( win_x1 - win_x0 + 1 ) *
                                                 static_cast<std::size_t>( win_y1 - win_y0 + 1 );
                if( window_cells + overhead <= node_budget ) {
                    break;
                }
                radius -= 5;
            }
        }

        snapshot.summary = string_format(
                               _( "Minesweeper board: %1$d columns by %2$d rows. "
                                  "Mines: %3$d. Flags placed: %4$d. "
                                  "Cells revealed: %5$d of %6$d. "
                                  "Cursor: row %7$d, column %8$d: %9$s." ),
                               level.x, level.y, iBombs, flag_count, revealed_count,
                               total_cells, iPlayerY + 1, iPlayerX + 1,
                               cell_value_text( iPlayerX, iPlayerY ) );
        if( windowed ) {
            snapshot.summary += " " + string_format(
                                      _( "The board is too large to list in full; only the "
                                         "area around the cursor is listed: rows %1$d to "
                                         "%2$d, columns %3$d to %4$d." ),
                                      win_y0 + 1, win_y1 + 1, win_x0 + 1, win_x1 + 1 );
        }

        a11y::append_document( snapshot, "mine:rules", _( "Minesweeper rules" ),
                               _( "Reveal every cell that does not hide a mine to win; "
                                  "revealing a mine loses the game. The number on a "
                                  "revealed cell counts the mines in the eight "
                                  "neighboring cells. Activate a covered cell to reveal "
                                  "it. Use the flag action on a covered cell to place a "
                                  "flag where you believe a mine is hidden, or to remove "
                                  "a flag you placed; a flag only records your suspicion "
                                  "and never changes what is hidden." ) );

        for( int y = win_y0; y <= win_y1; ++y ) {
            for( int x = win_x0; x <= win_x1; ++x ) {
                a11y::semantic_node cell;
                cell.id = string_format( "mine:cell:%d:%d", x, y );
                cell.label = string_format( _( "Row %d, column %d" ), y + 1, x + 1 );
                cell.value = cell_value_text( x, y );
                cell.selected = x == iPlayerX && y == iPlayerY;
                cell.actions = { "focus", "activate", "flag" };
                if( cell.selected ) {
                    snapshot.focus = cell.id;
                }
                snapshot.nodes.push_back( std::move( cell ) );
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
        werase( w_minesweeper_border );
        draw_border( w_minesweeper_border );

        std::vector<std::string> shortcuts;
        shortcuts.emplace_back( _( "<n>ew level" ) );
        shortcuts.emplace_back( _( "<f>lag" ) );
        shortcuts.emplace_back( _( "<q>uit" ) );

        int iWidth = 0;
        for( auto &shortcut : shortcuts ) {
            if( iWidth > 0 ) {
                iWidth += 1;
            }
            iWidth += utf8_width( shortcut );
        }

        int iPos = FULL_SCREEN_WIDTH - iWidth - 1;
        for( auto &shortcut : shortcuts ) {
            shortcut_print( w_minesweeper_border, point( iPos, 0 ), c_white, c_light_green, shortcut );
            iPos += utf8_width( shortcut ) + 1;
        }

        mvwputch( w_minesweeper_border, point( 2, 0 ), hilite( c_white ), _( "Minesweeper" ) );

        wnoutrefresh( w_minesweeper_border );

        if( !started ) {
            return;
        }

        werase( w_minesweeper );
        draw_custom_border( w_minesweeper, 1, 1, 1, 1, 1, 1, 1, 1, BORDER_COLOR,
                            // NOLINTNEXTLINE(cata-use-named-point-constants)
                            offset + point( -1, -1 ), level.y + 2, level.x + 2 );
        for( int y = 0; y < level.y; ++y ) {
            for( int x = 0; x < level.x; ++x ) {
                char ch = '?';
                nc_color col = c_red;
                const int num = mLevel[y][x];
                switch( mLevelReveal[y][x] ) {
                    case unknown:
                        if( boom && num == bomb ) {
                            ch = '*';
                            col = h_red;
                        } else {
                            ch = '#';
                            col = c_white;
                        }
                        break;
                    case flag:
                        ch = '!';
                        if( boom && num == bomb ) {
                            col = h_red;
                        } else {
                            col = c_yellow;
                        }
                        break;
                    case seen: {
                        if( num == bomb ) {
                            ch = '*';
                            col = boom ? h_red : c_red;
                        } else if( num == 0 ) {
                            ch = ' ';
                            col = aColors[num];
                        } else if( num >= 1 && num <= 8 ) {
                            ch = '0' + num;
                            col = aColors[num];
                        }
                        break;
                    }
                }
                if( !boom && iPlayerX == x && iPlayerY == y ) {
                    col = hilite( col );
                }
                mvwputch( w_minesweeper, offset + point( x, y ), col, ch );
            }
        }
        wnoutrefresh( w_minesweeper );
    } );

    std::function<void ( const point & )> rec_reveal = [&]( const point & p ) {
        if( mLevelReveal[p.y][p.x] == unknown || mLevelReveal[p.y][p.x] == flag ) {
            mLevelReveal[p.y][p.x] = seen;

            if( mLevel[p.y][p.x] == 0 ) {
                for( const point &near_p : closest_points_first( p, 1 ) ) {
                    if( near_p.x >= 0 && near_p.x < level.x && near_p.y >= 0 && near_p.y < level.y ) {
                        if( mLevelReveal[near_p.y][near_p.x] != seen ) {
                            rec_reveal( near_p );
                        }
                    }
                }
            }
        }
    };

    const auto &reveal_all = [&]() {
        for( int y = 0; y < level.y; ++y ) {
            for( int x = 0; x < level.x; ++x ) {
                if( mLevelReveal[y][x] == unknown || ( mLevelReveal[y][x] == flag && mLevel[y][x] != bomb ) ) {
                    mLevelReveal[y][x] = seen;
                }
            }
        }
    };

    std::string action = "NEW";

    do {
        if( action == "NEW" ) {
            new_level();

            iPlayerY = 0;
            iPlayerX = 0;

            started = true;
            boom = false;
        }

        if( check_win() ) {
            reveal_all();
            ui.invalidate_ui();
            popup_top( _( "Congratulations, you won!" ) );

            iScore = 30;

            action = "QUIT";
        } else {
            ui_manager::redraw();
            if( a11y::enabled() ) {
                reader.update( make_reader_snapshot() );
            }
            action = ctxt.handle_input();
        }

        if( const std::optional<tripoint_rel_ms> vec = ctxt.get_direction_rel_ms( action ) ) {
            const point new_( vec->xy().raw() + point( iPlayerX, iPlayerY ) );
            if( new_.x >= 0 && new_.x < level.x && new_.y >= 0 && new_.y < level.y ) {
                iPlayerX = new_.x;
                iPlayerY = new_.y;
            }
        } else if( action == "FLAG" ) {
            if( mLevelReveal[iPlayerY][iPlayerX] == unknown ) {
                mLevelReveal[iPlayerY][iPlayerX] = flag;
            } else if( mLevelReveal[iPlayerY][iPlayerX] == flag ) {
                mLevelReveal[iPlayerY][iPlayerX] = unknown;
            }
        } else if( action == "CONFIRM" ) {
            if( mLevelReveal[iPlayerY][iPlayerX] != seen ) {
                if( mLevel[iPlayerY][iPlayerX] == bomb ) {
                    boom = true;
                    reveal_all();
                    ui.invalidate_ui();
                    popup_top( _( "Boom, you're dead!  Better luck next time." ) );
                    action = "QUIT";
                } else if( mLevelReveal[iPlayerY][iPlayerX] == unknown ) {
                    rec_reveal( point( iPlayerX, iPlayerY ) );
                }
            }
        }
    } while( action != "QUIT" );

    return iScore;
}

