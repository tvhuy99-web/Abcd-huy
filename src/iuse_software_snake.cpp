#include "iuse_software_snake.h"
#include "a11y_semantics.h"

#include <cstddef>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "a11y_document.h"
#include "a11y_input.h"
#include "a11y_runtime.h"
#include "catacharset.h"  // utf8_width()
#include "color.h"
#include "cursesdef.h"
#include "input_context.h"
#include "output.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_helpers.h"
#include "ui_manager.h"

snake_game::snake_game() = default;

void snake_game::print_score( const catacurses::window &w_snake, int iScore )
{
    mvwprintz( w_snake, point( 5, 0 ), c_white, string_format( _( "Score: %d" ), iScore ) );
}

void snake_game::print_header( const catacurses::window &w_snake, bool show_shortcut )
{
    draw_border( w_snake, BORDER_COLOR, _( "S N A K E" ), c_white );
    if( show_shortcut ) {
        std::string shortcut = _( "<q>uit" );
        shortcut_print( w_snake, point( FULL_SCREEN_WIDTH - utf8_width( shortcut ) - 2, 0 ),
                        c_white, c_light_green, shortcut );
    }
}

void snake_game::snake_over( const catacurses::window &w_snake, int iScore )
{
    werase( w_snake );
    print_header( w_snake, false );

    // Body of dead snake
    size_t body_length = 3;
    wattron( w_snake, c_green );
    for( size_t i = 1; i <= body_length; i++ ) {
        for( size_t j = 0; j <= 1; j++ ) {
            mvwprintw( w_snake, point( 4 + j * 65, i ),  "|   |" );
        }
    }
    wattroff( w_snake, c_green );

    // Head of dead snake
    mvwprintz( w_snake, point( 3, body_length + 1 ), c_green, "(     )" );
    mvwprintz( w_snake, point( 4, body_length + 1 ), c_dark_gray, "x   x" );
    mvwprintz( w_snake, point( 3, body_length + 2 ), c_green, " \\___/ " );
    mvwputch( w_snake, point( 6, body_length + 3 ), c_red, '|' );
    mvwputch( w_snake, point( 6, body_length + 4 ), c_red, '^' );

    // Tail of dead snake
    mvwprintz( w_snake, point( 70, body_length + 1 ), c_green, "\\ /" );
    mvwputch( w_snake, point( 71, body_length + 2 ), c_green, 'v' );

    std::vector<std::string> game_over_text;
    game_over_text.emplace_back( R"(  ________    _____      _____   ___________)" );
    game_over_text.emplace_back( R"( /  _____/   /  _  \    /     \  \_   _____/)" );
    game_over_text.emplace_back( R"(/   \  ___  /  /_\  \  /  \ /  \  |    __)_ )" );
    game_over_text.emplace_back( R"(\    \_\  \/    |    \/    Y    \ |        \)" );
    game_over_text.emplace_back( R"( \______  /\____|__  /\____|__  //_______  /)" );
    game_over_text.emplace_back( R"(        \/         \/         \/         \/ )" );
    game_over_text.emplace_back( R"( ________ ____   _________________________  )" );
    game_over_text.emplace_back( R"( \_____  \\   \ /   /\_   _____/\______   \ )" );
    game_over_text.emplace_back( R"(  /   |   \\   Y   /  |    __)_  |       _/ )" );
    game_over_text.emplace_back( R"( /    |    \\     /   |        \ |    |   \ )" );
    game_over_text.emplace_back( R"( \_______  / \___/   /_______  / |____|_  / )" );
    game_over_text.emplace_back( R"(         \/                  \/         \/  )" );

    wattron( w_snake, c_light_red );
    for( size_t i = 0; i < game_over_text.size(); i++ ) {
        mvwprintw( w_snake, point( 17, i + 3 ), game_over_text[i] );
    }
    wattroff( w_snake, c_light_red );

    center_print( w_snake, 17, c_yellow, string_format( _( "TOTAL SCORE: %d" ), iScore ) );
    // TODO: print actual bound keys
    center_print( w_snake, 21, c_white, _( "Press 'q' or ESC to exit." ) );
    wnoutrefresh( w_snake );
}

int snake_game::start_game()
{
    std::vector<std::pair<int, int> > vSnakeBody;
    std::map<int, std::map<int, bool> > mSnakeBody;

    catacurses::window w_snake;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        ui_helpers::full_screen_window( ui, &w_snake );
    } );
    ui.mark_resize();

    //Snake start position
    vSnakeBody.emplace_back( FULL_SCREEN_HEIGHT / 2, FULL_SCREEN_WIDTH / 2 );
    mSnakeBody[FULL_SCREEN_HEIGHT / 2][FULL_SCREEN_WIDTH / 2] = true;

    //Snake start direction
    int iDirY = 0;
    int iDirX = 1;

    //Snake start length
    size_t iSnakeBody = 10;

    //GameSpeed aka inputdelay/timeout
    int iGameSpeed = 100;

    //Score
    int iScore = 0;
    int iFruitPosY = 0;
    int iFruitPosX = 0;

    input_context ctxt( "SNAKE" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    // Reader adapter: this scope lives across both loops below (the main
    // game loop and the game-over loop), switched by this phase flag.
    bool game_over = false;
    a11y::screen_scope reader( &ctxt, "snake",
    [&]( const a11y::semantic_intent & intent ) -> std::optional<std::string> {
        if( intent.node == "snake:dir:up" || intent.node == "snake:dir:down" ||
            intent.node == "snake:dir:left" || intent.node == "snake:dir:right" ) {
            if( intent.action == "focus" ) {
                // Browsing to a steering command must never steer -- and
                // must not even step the snake: every action this loop
                // returns from handle_input() advances the snake one
                // cell. Returning nullopt injects no action at all, so
                // the loop keeps waiting for a real steering command.
                return std::nullopt;
            }
            if( intent.action == "activate" ) {
                if( intent.node == "snake:dir:up" ) {
                    return std::string( "UP" );
                }
                if( intent.node == "snake:dir:down" ) {
                    return std::string( "DOWN" );
                }
                if( intent.node == "snake:dir:left" ) {
                    return std::string( "LEFT" );
                }
                return std::string( "RIGHT" );
            }
            return std::nullopt;
        }
        if( intent.node == "snake:fruit" || intent.node == "snake:body" ) {
            // Observation only: no action is injected (see above).
            return std::nullopt;
        }
        return a11y::registered_intent( ctxt, intent );
    } );

    const auto make_reader_snapshot = [&]() {
        a11y::semantic_snapshot snapshot = a11y::command_snapshot( ctxt, _( "Snake" ) );
        std::vector<a11y::semantic_node> commands = std::move( snapshot.nodes );
        snapshot.nodes.clear();

        // The timing policy is part of the rules in both phases: with the
        // reader on, the main loop below blocks in handle_input() with no
        // timeout instead of handle_input( iGameSpeed ), so the snake
        // waits for a direction at each step instead of moving in real
        // time. Speed, scoring and growth rules are unchanged.
        const std::string rules_text = _(
                                           "Steer the snake with the direction commands.  The snake keeps moving in its "
                                           "current direction until you steer it. Eat fruit to score 500 points per "
                                           "fruit and grow 10 segments longer; the game also speeds up slightly with "
                                           "each fruit when played in real time.  The snake wraps around the edges of "
                                           "the board: leaving one side brings it back in on the opposite side.  If "
                                           "the snake runs into its own body, the game is over.  Reader timing: with "
                                           "the screen reader on, the snake waits for your direction at each step "
                                           "instead of moving in real time.  The snake, scoring and growth rules are "
                                           "unchanged." );

        if( game_over ) {
            snapshot.summary = string_format( _( "Game over.  Total score: %d." ), iScore );
        } else {
            // vSnakeBody entries are (row, column); the head is the last entry.
            const int head_y = vSnakeBody.empty() ? 0 : vSnakeBody.back().first;
            const int head_x = vSnakeBody.empty() ? 0 : vSnakeBody.back().second;

            std::string heading;
            if( iDirX == 1 ) {
                heading = _( "east" );
            } else if( iDirX == -1 ) {
                heading = _( "west" );
            } else if( iDirY == 1 ) {
                heading = _( "south" );
            } else {
                heading = _( "north" );
            }

            const bool has_fruit = iFruitPosX != 0 && iFruitPosY != 0;
            std::string fruit_relative;
            if( has_fruit ) {
                const point delta( iFruitPosX - head_x, iFruitPosY - head_y );
                std::vector<std::string> parts;
                if( delta.x > 0 ) {
                    parts.push_back( string_format( delta.x == 1 ? _( "%d column right" )
                                                    : _( "%d columns right" ), delta.x ) );
                } else if( delta.x < 0 ) {
                    parts.push_back( string_format( delta.x == -1 ? _( "%d column left" )
                                                    : _( "%d columns left" ), -delta.x ) );
                }
                if( delta.y < 0 ) {
                    parts.push_back( string_format( delta.y == -1 ? _( "%d row up" )
                                                    : _( "%d rows up" ), -delta.y ) );
                } else if( delta.y > 0 ) {
                    parts.push_back( string_format( delta.y == 1 ? _( "%d row down" )
                                                    : _( "%d rows down" ), delta.y ) );
                }
                for( size_t i = 0; i < parts.size(); ++i ) {
                    if( i > 0 ) {
                        fruit_relative += ", ";
                    }
                    fruit_relative += parts[i];
                }
            }
            const std::string fruit_text = has_fruit
                                           ? ( fruit_relative.empty()
                                               ? _( "Fruit: at the snake's head" )
                                               : string_format( _( "Fruit: %s" ), fruit_relative ) )
                                           : _( "No fruit yet" );

            snapshot.summary = string_format(
                                   _( "Score: %1$d. Snake length: %2$d. Head at column %3$d, row %4$d. "
                                      "Heading %5$s. %6$s. Reader timing: with the screen reader on, the "
                                      "snake waits for your direction at each step instead of moving in real "
                                      "time.  The snake, scoring and growth rules are unchanged." ),
                                   iScore, static_cast<int>( vSnakeBody.size() ), head_x + 1, head_y + 1,
                                   heading, fruit_text );

            const struct {
                const char *id;
                std::string label;
                point delta;
            } directions[] = {
                { "snake:dir:up", _( "Steer north" ), point::north },
                { "snake:dir:down", _( "Steer south" ), point::south },
                { "snake:dir:left", _( "Steer west" ), point::west },
                { "snake:dir:right", _( "Steer east" ), point::east },
            };
            for( const auto &direction : directions ) {
                a11y::semantic_node node;
                node.id = direction.id;
                node.label = direction.label;
                node.actions = { "focus", "activate" };
                node.selected = point( iDirX, iDirY ) == direction.delta;
                if( node.selected ) {
                    snapshot.focus = node.id;
                }
                snapshot.nodes.push_back( node );
            }

            a11y::semantic_node fruit_node;
            fruit_node.id = "snake:fruit";
            fruit_node.label = _( "Fruit" );
            fruit_node.actions = { "focus" };
            fruit_node.detail = has_fruit
                                ? string_format( _( "Fruit at column %1$d, row %2$d: %3$s." ),
                                                 iFruitPosX + 1, iFruitPosY + 1, fruit_relative )
                                : _( "No fruit yet." );
            snapshot.nodes.push_back( fruit_node );

            // STT 100: the snapshot used to publish the head, the fruit and
            // the steering commands, but nothing about the body, so a blind
            // player could not know where their own snake was and could only
            // avoid biting it by remembering the whole path. Publish the
            // body cells next to the head, both as a clause of the summary
            // and as a browsable node. Only the cells next to the head can
            // kill on the next step, and the wrap is applied exactly as in
            // the main loop below, so the cell entered by steering off an
            // edge is reported too. Every body cell next to the head is
            // lethal: the self-collision check runs before the tail is
            // trimmed, so even the tail's current cell kills.
            const auto body_next_to_head = [&]( int dy, int dx ) -> bool {
                int ny = head_y + dy;
                int nx = head_x + dx;
                if( ny == 0 ) {
                    ny += FULL_SCREEN_HEIGHT - 2;
                } else if( ny == FULL_SCREEN_HEIGHT - 1 ) {
                    ny -= FULL_SCREEN_HEIGHT - 2;
                }
                if( nx == 0 ) {
                    nx += FULL_SCREEN_WIDTH - 2;
                } else if( nx == FULL_SCREEN_WIDTH - 1 ) {
                    nx -= FULL_SCREEN_WIDTH - 2;
                }
                const auto row_it = mSnakeBody.find( ny );
                if( row_it == mSnakeBody.end() ) {
                    return false;
                }
                const auto cell_it = row_it->second.find( nx );
                return cell_it != row_it->second.end() && cell_it->second;
            };
            std::vector<std::string> body_directions;
            if( body_next_to_head( -1, 0 ) ) {
                body_directions.push_back( _( "north" ) );
            }
            if( body_next_to_head( 1, 0 ) ) {
                body_directions.push_back( _( "south" ) );
            }
            if( body_next_to_head( 0, -1 ) ) {
                body_directions.push_back( _( "west" ) );
            }
            if( body_next_to_head( 0, 1 ) ) {
                body_directions.push_back( _( "east" ) );
            }
            std::string body_text;
            if( body_directions.empty() ) {
                body_text = _( "No part of the body is next to the head." );
            } else {
                std::string joined;
                for( size_t i = 0; i < body_directions.size(); ++i ) {
                    if( i > 0 ) {
                        joined += ", ";
                    }
                    joined += body_directions[i];
                }
                body_text = string_format( _( "Body next to the head: %s." ), joined );
            }
            // Appended, not folded into the format above, so the existing
            // summary msgid keeps its translation.
            snapshot.summary += " " + body_text;

            a11y::semantic_node body_node;
            body_node.id = "snake:body";
            body_node.role = "status";
            body_node.label = _( "Snake body" );
            body_node.detail = body_text;
            body_node.actions = { "focus" };
            snapshot.nodes.push_back( body_node );
        }

        a11y::append_document( snapshot, "snake:rules", _( "Snake rules" ), rules_text );

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
        werase( w_snake );
        print_header( w_snake );
        wattron( w_snake, c_light_gray );
        for( auto it = vSnakeBody.begin(); it != vSnakeBody.end(); ++it ) {
            if( it + 1 == vSnakeBody.end() ) {
                wattroff( w_snake, c_light_gray );
                wattron( w_snake, c_white );
            }
            mvwaddch( w_snake, point( it->second, it->first ), '#' );
        }
        wattroff( w_snake, c_white );
        if( iFruitPosX != 0 && iFruitPosY != 0 ) {
            mvwputch( w_snake, point( iFruitPosX, iFruitPosY ), c_light_red, '*' );
        }
        print_score( w_snake, iScore );
        wnoutrefresh( w_snake );
    } );

    do {
        //Check if we hit a border
        if( vSnakeBody[vSnakeBody.size() - 1].first + iDirY == 0 ) {
            vSnakeBody.emplace_back( vSnakeBody[vSnakeBody.size() - 1].first +
                                     iDirY + FULL_SCREEN_HEIGHT - 2,
                                     vSnakeBody[vSnakeBody.size() - 1].second + iDirX );

        } else if( vSnakeBody[vSnakeBody.size() - 1].first + iDirY == FULL_SCREEN_HEIGHT - 1 ) {
            vSnakeBody.emplace_back( vSnakeBody[vSnakeBody.size() - 1].first +
                                     iDirY - FULL_SCREEN_HEIGHT + 2,
                                     vSnakeBody[vSnakeBody.size() - 1].second + iDirX );

        } else if( vSnakeBody[vSnakeBody.size() - 1].second + iDirX == 0 ) {
            vSnakeBody.emplace_back( vSnakeBody[vSnakeBody.size() - 1].first + iDirY,
                                     vSnakeBody[vSnakeBody.size() - 1].second +
                                     iDirX + FULL_SCREEN_WIDTH - 2 );

        } else if( vSnakeBody[vSnakeBody.size() - 1].second + iDirX == FULL_SCREEN_WIDTH - 1 ) {
            vSnakeBody.emplace_back( vSnakeBody[vSnakeBody.size() - 1].first + iDirY,
                                     vSnakeBody[vSnakeBody.size() - 1].second +
                                     iDirX - FULL_SCREEN_WIDTH + 2 );

        } else {
            vSnakeBody.emplace_back( vSnakeBody[vSnakeBody.size() - 1].first + iDirY,
                                     vSnakeBody[vSnakeBody.size() - 1].second + iDirX );
        }

        //Check if we hit ourselves
        if( mSnakeBody[vSnakeBody[vSnakeBody.size() - 1].first]
            [vSnakeBody[vSnakeBody.size() - 1].second] ) {
            //We are dead :(
            break;
        } else {
            //Add new position to map
            mSnakeBody[vSnakeBody[vSnakeBody.size() - 1].first]
            [vSnakeBody[vSnakeBody.size() - 1].second] = true;
        }

        //Have we eaten the forbidden fruit?
        if( vSnakeBody[vSnakeBody.size() - 1].first == iFruitPosY &&
            vSnakeBody[vSnakeBody.size() - 1].second == iFruitPosX ) {
            iScore += 500;
            iSnakeBody += 10;
            iGameSpeed -= 3;

            iFruitPosY = 0;
            iFruitPosX = 0;
        }

        //Check if we are longer than our max size
        if( vSnakeBody.size() > iSnakeBody ) {
            mSnakeBody[vSnakeBody[0].first][vSnakeBody[0].second] = false;
            vSnakeBody.erase( vSnakeBody.begin(), vSnakeBody.begin() + 1 );
        }

        //On full length add a fruit
        if( iFruitPosX == 0 && iFruitPosY == 0 ) {
            do {
                iFruitPosY = rng( 1, FULL_SCREEN_HEIGHT - 2 );
                iFruitPosX = rng( 1, FULL_SCREEN_WIDTH - 2 );
            } while( mSnakeBody[iFruitPosY][iFruitPosX] );
        }

        ui_manager::redraw();

        if( a11y::enabled() ) {
            reader.update( make_reader_snapshot() );
        }

        // Reader timing policy (explicit): with the screen reader on, block
        // in handle_input() with no timeout so the snake waits for the
        // player's direction at each step instead of moving in real time.
        // Only the cadence changes from real-time to turn-based; the snake,
        // speed-up, scoring and growth rules above are unchanged. With the
        // reader off, the original timed input is kept.
        const std::string action = a11y::enabled() ? ctxt.handle_input()
                                   : ctxt.handle_input( iGameSpeed );

        if( action == "UP" ) {
            if( iDirY != 1 ) {
                iDirY = -1;
                iDirX = 0;
            }
        } else if( action == "DOWN" ) {
            if( iDirY != -1 ) {
                iDirY = 1;
                iDirX = 0;
            }
        } else if( action == "LEFT" ) {
            if( iDirX != 1 ) {
                iDirY = 0;
                iDirX = -1;
            }
        } else if( action == "RIGHT" ) {
            if( iDirX != -1 ) {
                iDirY = 0;
                iDirX = 1;
            }
        } else if( action == "QUIT" ) {
            return iScore;
        }

    } while( true );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        snake_over( w_snake, iScore );
    } );
    game_over = true;
    do {
        ui_manager::redraw();
        if( a11y::enabled() ) {
            reader.update( make_reader_snapshot() );
        }
        const std::string action = ctxt.handle_input();
        if( action == "QUIT" ) {
            break;
        }
    } while( true );

    return iScore;
}
