#include "iuse_software_sokoban.h"

#include <algorithm>
#include <functional>
#include <istream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "a11y_document.h"
#include "a11y_input.h"
#include "a11y_runtime.h"
#include "a11y_semantics.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "coordinates.h"
#include "cursesdef.h"
#include "input_context.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_helpers.h"
#include "ui_manager.h"

sokoban_game::sokoban_game() = default;

void sokoban_game::print_score( const catacurses::window &w_sokoban, int iScore, int iMoves )
{
    mvwprintz( w_sokoban, point( 3, 1 ), c_white, _( "Level: %d/%d" ), iCurrentLevel + 1, iNumLevel );
    wprintw( w_sokoban, "    " );

    mvwprintz( w_sokoban, point( 3, 2 ), c_white, _( "Score: %d" ), iScore );

    mvwprintz( w_sokoban, point( 3, 3 ), c_white, _( "Moves: %d" ), iMoves );
    wprintw( w_sokoban, "    " );

    mvwprintz( w_sokoban, point( 3, 4 ), c_white, _( "Total moves: %d" ), iTotalMoves );
}

void sokoban_game::parse_level( std::istream &fin )
{
    /*
    # Wall
    $ Package
    space Floor
    . Goal
    * Package on Goal
    @ Sokoban
    + Sokoban on Goal
    */

    iCurrentLevel = 0;
    iNumLevel = 0;

    vLevel.clear();
    vUndo.clear();
    vLevelDone.clear();

    std::string sLine;
    while( !fin.eof() ) {
        safe_getline( fin, sLine );

        if( sLine.substr( 0, 3 ) == "; #" ) {
            iNumLevel++;
            continue;
        } else if( sLine[0] == ';' ) {
            continue;
        }

        if( sLine.empty() ) {
            //Find level start
            vLevel.resize( iNumLevel + 1 );
            vLevelDone.resize( iNumLevel + 1 );
            mLevelInfo[iNumLevel]["MaxLevelY"] = 0;
            mLevelInfo[iNumLevel]["MaxLevelX"] = 0;
            mLevelInfo[iNumLevel]["PlayerY"] = 0;
            mLevelInfo[iNumLevel]["PlayerX"] = 0;
            continue;
        }

        if( mLevelInfo[iNumLevel]["MaxLevelX"] < sLine.length() ) {
            mLevelInfo[iNumLevel]["MaxLevelX"] = sLine.length();
        }

        for( size_t i = 0; i < sLine.length(); i++ ) {
            if( sLine[i] == '@' ) {
                if( mLevelInfo[iNumLevel]["PlayerY"] == 0 && mLevelInfo[iNumLevel]["PlayerX"] == 0 ) {
                    mLevelInfo[iNumLevel]["PlayerY"] = mLevelInfo[iNumLevel]["MaxLevelY"];
                    mLevelInfo[iNumLevel]["PlayerX"] = i;
                } else {
                    // TODO: describe why it's invalid
                    throw std::runtime_error( "invalid content of sokoban file" );
                }
            }

            if( sLine[i] == '.' || sLine[i] == '*' || sLine[i] == '+' ) {
                vLevelDone[iNumLevel].emplace_back( static_cast<int>
                                                    ( mLevelInfo[iNumLevel]["MaxLevelY"] ), static_cast<int>( i ) );
            }

            vLevel[iNumLevel][mLevelInfo[iNumLevel]["MaxLevelY"]][i] = sLine[i];
        }

        mLevelInfo[iNumLevel]["MaxLevelY"]++;
    }
}

int sokoban_game::get_wall_connection( const point &i )
{
    bool bTop = false;
    bool bRight = false;
    bool bBottom = false;
    bool bLeft = false;

    if( mLevel[i.y - 1][i.x] == "#" ) {
        bTop = true;
    }

    if( mLevel[i.y][i.x + 1] == "#" ) {
        bRight = true;
    }

    if( mLevel[i.y + 1][i.x] == "#" ) {
        bBottom = true;
    }

    if( mLevel[i.y][i.x - 1] == "#" ) {
        bLeft = true;
    }

    if( !bRight && !bLeft ) {
        return LINE_XOXO; //

    } else if( !bTop && !bBottom ) {
        return LINE_OXOX;

    } else if( bTop && bRight && !bBottom && !bLeft ) {
        return LINE_XXOO;

    } else if( !bTop && bRight && bBottom && !bLeft ) {
        return LINE_OXXO;

    } else if( !bTop && !bRight && bBottom && bLeft ) {
        return LINE_OOXX;

    } else if( bTop && !bRight && !bBottom && bLeft ) {
        return LINE_XOOX;

    } else if( bTop && bRight && bBottom && !bLeft ) {
        return LINE_XXXO;

    } else if( bTop && bRight && !bBottom && bLeft ) {
        return LINE_XXOX;

    } else if( bTop && !bRight && bBottom && bLeft ) {
        return LINE_XOXX;

    } else if( !bTop && bRight && bBottom && bLeft ) {
        return LINE_OXXX;

    } else if( bTop && bRight && bBottom && bLeft ) {
        return LINE_XXXX;
    }

    return '#';
}

void sokoban_game::draw_level( const catacurses::window &w_sokoban )
{
    const point iOffset( ( FULL_SCREEN_WIDTH - 2 - mLevelInfo[iCurrentLevel]["MaxLevelX"] ) / 2,
                         ( FULL_SCREEN_HEIGHT - 2 - mLevelInfo[iCurrentLevel]["MaxLevelY"] ) / 2 );

    for( auto &elem : mLevel ) {
        for( std::map<int, std::string>::iterator iterX = elem.second.begin();
             iterX != elem.second.end(); ++iterX ) {
            std::string sTile = iterX->second;

            if( sTile == "#" ) {
                mvwputch( w_sokoban, iOffset + point( iterX->first, elem.first ),
                          c_white, get_wall_connection( point( iterX->first, elem.first ) ) );

            } else {
                nc_color cCol = c_white;

                if( sTile == "." || sTile == "*" ||  sTile == "+" ) {
                    cCol = red_background( c_white );
                }

                if( sTile == "." ) {
                    sTile = " ";
                }

                if( sTile == "*" ) {
                    sTile = "$";
                }

                if( sTile == "+" ) {
                    sTile = "@";
                }

                mvwprintz( w_sokoban, iOffset + point( iterX->first, elem.first ), cCol, sTile );
            }
        }
    }
}

bool sokoban_game::check_win()
{
    for( auto &elem : vLevelDone[iCurrentLevel] ) {
        if( mLevel[elem.first][elem.second] != "*" ) {
            return false;
        }
    }
    return true;
}

int sokoban_game::start_game()
{
    int iScore = 0;
    int iMoves = 0;
    iTotalMoves = 0;

    int iDirY = 0;
    int iDirX = 0;

    read_from_file( PATH_INFO::sokoban(), [this]( std::istream & is ) {
        parse_level( is );
    } );

    catacurses::window w_sokoban;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ) {
        ui_helpers::full_screen_window( ui, &w_sokoban );
    } );
    ui.mark_resize();

    input_context ctxt( "SOKOBAN" );
    ctxt.register_cardinal();
    ctxt.register_action( "NEXT" );
    ctxt.register_action( "PREV" );
    ctxt.register_action( "RESET" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UNDO" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    // Reader adapter: the whole Sokoban board is public knowledge (the
    // sighted UI draws every tile), so the snapshot below publishes the
    // player, every package and every goal, with no hidden information.
    // The custom "soko:" nodes are observation only: they offer just the
    // "focus" action, so a reader intent on one changes nothing in the
    // game and the handler answers "TIMEOUT", which keeps the original
    // ctxt.handle_input() call waiting for real input. Command nodes
    // (directions, NEXT/PREV/RESET/UNDO/QUIT, HELP_KEYBINDINGS) go
    // through the shared registered-intent mapping, so activating one
    // returns exactly the action the keypress would have returned.
    a11y::screen_scope reader( &ctxt, "sokoban",
    [&]( const a11y::semantic_intent &intent ) -> std::optional<std::string> {
        if( intent.node.rfind( "soko:", 0 ) == 0 ) {
            return std::string( "TIMEOUT" );
        }
        return a11y::registered_intent( ctxt, intent );
    } );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_sokoban );
        draw_border( w_sokoban, BORDER_COLOR, _( "Sokoban" ), hilite( c_white ) );

        std::vector<std::string> shortcuts;
        shortcuts.reserve( 5 );
        shortcuts.emplace_back( _( "<+> next" ) ); // '+': next
        shortcuts.emplace_back( _( "<-> prev" ) ); // '-': prev
        shortcuts.emplace_back( _( "<r>eset" ) ); // 'r': reset
        shortcuts.emplace_back( _( "<q>uit" ) ); // 'q': quit
        shortcuts.emplace_back( _( "<u>ndo move" ) ); // 'u': undo move

        int indent = 10;
        for( auto &shortcut : shortcuts ) {
            indent = std::max( indent, utf8_width( shortcut ) + 1 );
        }
        indent = std::min( indent, 30 );

        for( size_t i = 0; i < shortcuts.size(); i++ ) {
            shortcut_print( w_sokoban, point( FULL_SCREEN_WIDTH - indent, i + 1 ),
                            c_white, c_light_green, shortcuts[i] );
        }

        print_score( w_sokoban, iScore, iMoves );

        draw_level( w_sokoban );
        wnoutrefresh( w_sokoban );
    } );

    int iPlayerY = 0;
    int iPlayerX = 0;

    // Builds one reader snapshot from the live board. mLevel is only
    // ever read here, through find() on a const reference: the usual
    // mLevel[y][x] operator[] would insert a missing row or cell into
    // the live board, so a cell (or row) that is not in the map is
    // reported as a wall instead, which is also how cells outside the
    // drawn board behave. Rows and columns are spoken 1-based, in the
    // board's own coordinates (row = y + 1, column = x + 1).
    const auto make_reader_snapshot = [&]() {
        a11y::semantic_snapshot snapshot = a11y::command_snapshot( ctxt, _( "Sokoban" ) );
        std::vector<a11y::semantic_node> commands = std::move( snapshot.nodes );
        snapshot.nodes.clear();

        const auto &board = mLevel;
        const auto tile_at = [&board]( int y, int x ) -> std::string {
            const auto row_it = board.find( y );
            if( row_it == board.end() ) {
                return "#";
            }
            const auto cell_it = row_it->second.find( x );
            if( cell_it == row_it->second.end() ) {
                return "#";
            }
            return cell_it->second;
        };
        const auto describe_tile = []( const std::string &tile ) -> std::string {
            if( tile == "#" ) {
                return _( "wall" );
            }
            if( tile == "$" ) {
                return _( "package" );
            }
            if( tile == "*" ) {
                return _( "package on goal" );
            }
            if( tile == "." ) {
                return _( "goal" );
            }
            if( tile == "@" ) {
                return _( "you" );
            }
            if( tile == "+" ) {
                return _( "you, on a goal" );
            }
            return _( "floor" );
        };
        const auto neighbours_text = [&]( int y, int x ) {
            return string_format( _( "North: %s. South: %s. East: %s. West: %s." ),
                                  describe_tile( tile_at( y - 1, x ) ),
                                  describe_tile( tile_at( y + 1, x ) ),
                                  describe_tile( tile_at( y, x + 1 ) ),
                                  describe_tile( tile_at( y, x - 1 ) ) );
        };

        std::vector<std::pair<int, int>> goals;
        if( iCurrentLevel >= 0 &&
            static_cast<size_t>( iCurrentLevel ) < vLevelDone.size() ) {
            goals = vLevelDone[static_cast<size_t>( iCurrentLevel )];
        }
        int packages_on_goals = 0;
        for( const std::pair<int, int> &goal : goals ) {
            if( tile_at( goal.first, goal.second ) == "*" ) {
                packages_on_goals++;
            }
        }

        snapshot.summary = string_format(
                               _( "Level %d of %d. Score %d. Moves %d (total %d). "
                                  "Packages on goals: %d of %d. "
                                  "You are at row %d, column %d." ),
                               iCurrentLevel + 1, iNumLevel, iScore, iMoves, iTotalMoves,
                               packages_on_goals, static_cast<int>( goals.size() ),
                               iPlayerY + 1, iPlayerX + 1 );
        snapshot.focus = "soko:player";

        a11y::semantic_node player;
        player.id = "soko:player";
        player.role = "status";
        player.label = _( "You" );
        player.value = string_format( _( "Row %d, column %d" ), iPlayerY + 1, iPlayerX + 1 );
        player.detail = neighbours_text( iPlayerY, iPlayerX );
        player.actions = { "focus" };
        snapshot.nodes.push_back( player );

        // One node per package, numbered in scan order (row by row, then
        // column by column; the std::map board already iterates in that
        // order). The four neighbours of the package are included so the
        // player can work out a pushing route: a package can only be
        // pushed from the opposite side onto a free cell.
        int package_number = 0;
        for( const auto &row : board ) {
            for( const auto &cell : row.second ) {
                if( cell.second != "$" && cell.second != "*" ) {
                    continue;
                }
                package_number++;
                const bool on_goal = cell.second == "*";
                a11y::semantic_node package;
                package.id = "soko:package:" + std::to_string( package_number );
                package.role = "status";
                package.label = string_format( _( "Package %d" ), package_number );
                package.value = string_format( _( "Row %d, column %d. %s." ),
                                               row.first + 1, cell.first + 1,
                                               on_goal ? _( "On a goal" ) : _( "Not on a goal" ) );
                package.detail = string_format( _( "Row %d, column %d. %s. %s" ),
                                                row.first + 1, cell.first + 1,
                                                on_goal ? _( "On a goal" ) : _( "Not on a goal" ),
                                                neighbours_text( row.first, cell.first ) );
                package.actions = { "focus" };
                snapshot.nodes.push_back( std::move( package ) );
            }
        }

        // One document with the short rules and the whole goal list in
        // words: every goal, and whether a package already covers it.
        std::string board_text = _( "Rules: push every package onto a goal. "
                                    "You can push one package at a time, and a package "
                                    "cannot be pulled. Undo, reset, previous level and "
                                    "next level are available as commands on this screen." );
        board_text += "\n";
        board_text += _( "Goals:" );
        if( goals.empty() ) {
            board_text += "\n";
            board_text += _( "No goals in this level." );
        }
        for( size_t i = 0; i < goals.size(); i++ ) {
            board_text += "\n";
            board_text += string_format( _( "Goal %d: row %d, column %d: %s." ),
                                         static_cast<int>( i ) + 1,
                                         goals[i].first + 1, goals[i].second + 1,
                                         tile_at( goals[i].first, goals[i].second ) == "*"
                                         ? _( "a package is on this goal" )
                                         : _( "no package yet" ) );
        }
        a11y::append_document( snapshot, "soko:board", _( "Sokoban board and rules" ),
                               board_text );
        if( !snapshot.nodes.empty() && snapshot.nodes.back().id == "soko:board" ) {
            snapshot.nodes.back().actions = { "focus" };
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

    bool bNewLevel = true;
    bool bMoved = false;
    do {
        if( bNewLevel ) {
            bNewLevel = false;

            iMoves = 0;
            vUndo.clear();

            iPlayerY = mLevelInfo[iCurrentLevel]["PlayerY"];
            iPlayerX = mLevelInfo[iCurrentLevel]["PlayerX"];
            mLevel = vLevel[iCurrentLevel];
        }

        std::string action;
        if( check_win() ) {
            //we won yay
            if( !mAlreadyWon[iCurrentLevel] ) {
                iScore += 500;
                mAlreadyWon[iCurrentLevel] = true;
            }
            action = "NEXT";

        } else {
            ui_manager::redraw();
            if( a11y::enabled() ) {
                reader.update( make_reader_snapshot() );
            }
            //Check input
            action = ctxt.handle_input();
        }

        bMoved = false;
        if( const std::optional<tripoint_rel_ms> vec = ctxt.get_direction_rel_ms( action ) ) {
            iDirX = vec->x();
            iDirY = vec->y();
            bMoved = true;
        } else if( action == "QUIT" ) {
            return iScore;
        } else if( action == "UNDO" ) {
            int iPlayerYNew = 0;
            int iPlayerXNew = 0;
            bool bUndoSkip = false;
            //undo move
            if( !vUndo.empty() ) {
                //reset last player pos
                mLevel[iPlayerY][iPlayerX] = mLevel[iPlayerY][iPlayerX] == "+" ? "." : " ";
                iPlayerYNew = vUndo[vUndo.size() - 1].old.y;
                iPlayerXNew = vUndo[vUndo.size() - 1].old.x;
                mLevel[iPlayerYNew][iPlayerXNew] = vUndo[vUndo.size() - 1].sTileOld;

                vUndo.pop_back();

                bUndoSkip = true;
            }

            if( bUndoSkip && !vUndo.empty() ) {
                iDirY = vUndo[vUndo.size() - 1].old.y;
                iDirX = vUndo[vUndo.size() - 1].old.x;

                if( vUndo[vUndo.size() - 1].sTileOld == "$" ||
                    vUndo[vUndo.size() - 1].sTileOld == "*" ) {
                    mLevel[iPlayerY][iPlayerX] = mLevel[iPlayerY][iPlayerX] == "." ? "*" : "$";
                    mLevel[iPlayerY + iDirY][iPlayerX + iDirX] = mLevel[iPlayerY + iDirY][iPlayerX + iDirX] == "*" ?
                            "." : " ";

                    vUndo.pop_back();
                }
            }

            if( bUndoSkip ) {
                iPlayerY = iPlayerYNew;
                iPlayerX = iPlayerXNew;
            }
        } else if( action == "RESET" ) {
            //reset level
            bNewLevel = true;
        } else if( action == "NEXT" ) {
            //next level
            iCurrentLevel++;
            if( iCurrentLevel >= iNumLevel ) {
                iCurrentLevel = 0;
            }
            bNewLevel = true;
        } else if( action == "PREV" ) {
            //previous level
            iCurrentLevel--;
            if( iCurrentLevel < 0 ) {
                iCurrentLevel = iNumLevel - 1;
            }
            bNewLevel = true;
        }

        if( bMoved ) {
            //check if we can move the player
            std::string sMoveTo = mLevel[iPlayerY + iDirY][iPlayerX + iDirX];
            bool bMovePlayer = false;

            if( sMoveTo != "#" ) {
                if( sMoveTo == "$" || sMoveTo == "*" ) {
                    //Check if we can move the package
                    std::string sMovePackTo = mLevel[iPlayerY + iDirY * 2][iPlayerX + iDirX * 2];
                    if( sMovePackTo == "." || sMovePackTo == " " ) {
                        //move both
                        bMovePlayer = true;
                        mLevel[iPlayerY + iDirY * 2][iPlayerX + iDirX * 2] = sMovePackTo == "." ? "*" : "$";

                        vUndo.emplace_back( point( iDirX, iDirY ), sMoveTo );

                        iMoves--;
                    }
                } else {
                    bMovePlayer = true;
                }

                if( bMovePlayer ) {
                    //move player
                    vUndo.emplace_back( point( iPlayerX, iPlayerY ), mLevel[iPlayerY][iPlayerX] );

                    mLevel[iPlayerY][iPlayerX] = mLevel[iPlayerY][iPlayerX] == "+" ? "." : " ";
                    mLevel[iPlayerY + iDirY][iPlayerX + iDirX] = sMoveTo == "." || sMoveTo == "*" ? "+" : "@";

                    iPlayerY += iDirY;
                    iPlayerX += iDirX;

                    iMoves++;
                    iTotalMoves++;
                }
            }
        }

    } while( true );

    return iScore;
}
