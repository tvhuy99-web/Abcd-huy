#include "iuse_software_kitten.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>  // Needed for rand()
#include <iterator>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "a11y_document.h"
#include "a11y_input.h"
#include "a11y_runtime.h"
#include "a11y_semantics.h"
#include "color.h"
#include "cuboid_rectangle.h"
#include "input.h"
#include "input_context.h"
#include "input_enums.h"
#include "output.h"
#include "rng.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "ui_manager.h"

static constexpr int EMPTY = -1;
static constexpr int ROBOT = 0;
static constexpr int KITTEN = 1;

robot_finds_kitten::robot_finds_kitten() : ctxt( "IUSE_SOFTWARE_KITTEN" )
{
    // Register every action once, on the persistent member context. The
    // original code registered QUIT/HELP_KEYBINDINGS/ANY_INPUT (and, for
    // the playing states, the cardinal directions) on a fresh local
    // context inside process_input() on every call; the cardinal
    // directions must be registered here too, because the reader's
    // command snapshot is built from this context's registrations and
    // because a11y::screen_scope has to bind to this instance.
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_cardinal();

    ret = false;
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char ktile[83] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#&()*+./:;=?![]{|}y";

    nummessages = 201;
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    empty.pos = point( -1, -1 );
    empty.color = nc_color();
    empty.character = ' ';
    rfkscreen.fill( EMPTY );
    /* Create an array to ensure we don't get duplicate messages. */
    for( int c = 0; c < nummessages; c++ ) {
        bogus[c] = empty;
    }
    /* Now we initialize the various game OBJECTs.
       * Assign a position to the player. */
    robot.pos.x = rng( 0, rfkCOLS - 1 );
    robot.pos.y = rng( 0, rfkLINES - 3 - 1 ) + 3;
    robot.character = '#';
    robot.color = c_white;
    rfkscreen[robot.pos.x][robot.pos.y] = ROBOT;

    /* Assign the kitten a unique position. */
    do {
        kitten.pos.x = rng( 0, rfkCOLS - 1 );
        kitten.pos.y = rng( 0, rfkLINES - 3 - 1 ) + 3;
    } while( rfkscreen[kitten.pos.x][kitten.pos.y] != EMPTY );

    /* Assign the kitten a character and a color. */
    do {
        kitten.character = ktile[rng( 0, 81 )];
    } while( kitten.character == '#' || kitten.character == ' ' );

    do {
        kitten.color = all_colors.get_random();
    } while( kitten.color == c_black );

    rfkscreen[kitten.pos.x][kitten.pos.y] = KITTEN;

    /* Now, initialize non-kitten OBJECTs. */
    for( int c = 0; c < numbogus; c++ ) {
        /* Assign a unique position. */
        do {
            bogus[c].pos.x = rng( 0, rfkCOLS - 1 );
            bogus[c].pos.y = rng( 0, rfkLINES - 3 - 1 ) + 3;
        } while( rfkscreen[bogus[c].pos.x][bogus[c].pos.y] != EMPTY );
        rfkscreen[bogus[c].pos.x][bogus[c].pos.y] = c + 2;

        /* Assign a character. */
        do {
            bogus[c].character = ktile[rng( 0, 81 )];
        } while( bogus[c].character == '#' || bogus[c].character == ' ' );

        do {
            bogus[c].color = all_colors.get_random();
        } while( bogus[c].color == c_black );

        /* Assign a unique message. */
        bool got_nki = false;
        while( !got_nki ) {
            std::string nki = SNIPPET.random_from_category( "non_kitten_items" ).value_or(
                                  translation() ).translated();
            // Add NKI if it does not already exist in the list
            if( std::find( bogus_messages.begin(), bogus_messages.end(), nki ) == bogus_messages.end() ) {
                bogus_messages.push_back( nki );
                got_nki = true;
            }
        }
    }

    ui_adaptor ui;
    ui.on_screen_resize( [this]( ui_adaptor & ui ) {
        bkatwin = catacurses::newwin( rfkLINES + 2, rfkCOLS + 2,
                                      point( ( TERMX - rfkCOLS - 2 ) / 2, ( TERMY - rfkLINES - 2 ) / 2 ) );
        w = catacurses::newwin( rfkLINES, rfkCOLS,
                                point( ( TERMX - rfkCOLS ) / 2, ( TERMY - rfkLINES ) / 2 ) );
        ui.position_from_window( bkatwin );
    } );
    ui.mark_resize();

    ui.on_redraw( [this]( const ui_adaptor & ) {
        show();
    } );

    // Reader adapter: publish the field without ever revealing which
    // object is kitten. That identity is the game's secret: the whole
    // point is that robot must touch an object to learn what it is, so
    // every object is published in exactly the same form -- the symbol
    // drawn on screen, its color name, and its position -- and its
    // bogus message is published only after robot has touched it (the
    // bogus_message state below, where the game itself shows the text).
    // The objects are also numbered in scan order (row by row, then
    // column by column) over rfkscreen, never in storage order, so the
    // numbering cannot single out the kitten either. Object nodes are
    // observation only (focus); movement stays on the direction
    // commands of the command snapshot.
    const auto reader_position = []( const point & pos ) {
        return string_format( _( "column %d of %d, row %d of %d" ),
                              pos.x + 1, rfkCOLS, pos.y - 2, rfkLINES - 3 );
    };
    const auto reader_relative = [this]( const point & pos ) {
        const int dx = pos.x - robot.pos.x;
        const int dy = pos.y - robot.pos.y;
        std::string horizontal;
        if( dx == 1 ) {
            horizontal = _( "1 column right" );
        } else if( dx == -1 ) {
            horizontal = _( "1 column left" );
        } else if( dx > 1 ) {
            horizontal = string_format( _( "%d columns right" ), dx );
        } else if( dx < -1 ) {
            horizontal = string_format( _( "%d columns left" ), -dx );
        }
        std::string vertical;
        if( dy == 1 ) {
            vertical = _( "1 row down" );
        } else if( dy == -1 ) {
            vertical = _( "1 row up" );
        } else if( dy > 1 ) {
            vertical = string_format( _( "%d rows down" ), dy );
        } else if( dy < -1 ) {
            vertical = string_format( _( "%d rows up" ), -dy );
        }
        if( horizontal.empty() ) {
            return vertical;
        }
        if( vertical.empty() ) {
            return horizontal;
        }
        return string_format( _( "%1$s, %2$s" ), horizontal, vertical );
    };
    const auto make_reader_snapshot = [&]() {
        a11y::semantic_snapshot snapshot = a11y::command_snapshot( ctxt,
                                           _( "Robot Finds Kitten" ) );
        std::vector<a11y::semantic_node> commands = std::move( snapshot.nodes );
        snapshot.nodes.clear();

        const auto add_field_nodes = [&]() {
            a11y::semantic_node robot_node;
            robot_node.id = "kitten:robot";
            robot_node.role = "status";
            robot_node.label = _( "Robot" );
            robot_node.detail = string_format( _( "Robot at %s." ),
                                               reader_position( robot.pos ) );
            robot_node.actions = { "focus" };
            snapshot.nodes.push_back( robot_node );

            int object_number = 0;
            for( int y = 0; y < rfkLINES; ++y ) {
                for( int x = 0; x < rfkCOLS; ++x ) {
                    const int cell = rfkscreen[x][y];
                    if( cell == EMPTY || cell == ROBOT ) {
                        continue;
                    }
                    // Internal lookup only: whether this cell is the
                    // kitten or a bogus object decides nothing that is
                    // published -- both are described solely by their
                    // on-screen symbol, color and position.
                    const kobject &object = cell == KITTEN ? kitten : bogus[cell - 2];
                    ++object_number;
                    a11y::semantic_node node;
                    node.id = string_format( "kitten:object:%d", object_number );
                    node.role = "status";
                    node.label = string_format( _( "Object: symbol '%c'" ),
                                                static_cast<char>( object.character ) );
                    const std::string relative = reader_relative( object.pos );
                    if( relative.empty() ) {
                        node.detail = string_format( _( "Color: %1$s. At %2$s." ),
                                                     string_from_color( object.color ),
                                                     reader_position( object.pos ) );
                    } else {
                        node.detail = string_format( _( "Color: %1$s. At %2$s, %3$s from robot." ),
                                                     string_from_color( object.color ),
                                                     reader_position( object.pos ), relative );
                    }
                    node.actions = { "focus" };
                    snapshot.nodes.push_back( node );
                }
            }
        };

        switch( current_ui_state ) {
            case ui_state::instructions: {
                snapshot.summary = _( "Robot Finds Kitten: instructions. "
                                       "Read the rules, then start the game." );
                a11y::append_document( snapshot, "kitten:rules", _( "How to play" ),
                                       string_format(
                                           _( "Your job is to find kitten.  This task is complicated by the existence of various things "
                                              "which are not kitten.  Robot must touch items to determine if they are kitten or not.  "
                                              "The game ends when robot finds kitten.  Alternatively, you may end the game by hitting %s." ),
                                           ctxt.get_desc( "QUIT" ) ) );
                a11y::semantic_node start;
                start.id = "kitten:start";
                start.label = _( "Start game" );
                start.detail = _( "Start the game." );
                start.actions = { "focus", "activate" };
                snapshot.nodes.push_back( start );
                snapshot.focus = start.id;
                break;
            }
            case ui_state::main:
                snapshot.summary = string_format(
                                       _( "Robot at %1$s. %2$d objects on the field. "
                                          "Move with the direction commands; touch an object to learn whether it is kitten." ),
                                       reader_position( robot.pos ), numbogus + 1 );
                add_field_nodes();
                break;
            case ui_state::invalid_input:
                snapshot.summary = string_format(
                                       _( "Invalid command: use direction keys or press %1$s to quit. "
                                          "Robot at %2$s. %3$d objects on the field." ),
                                       ctxt.get_desc( "QUIT" ), reader_position( robot.pos ),
                                       numbogus + 1 );
                add_field_nodes();
                break;
            case ui_state::bogus_message:
                // The touched object's message is no longer secret: the
                // game itself is displaying it in this state.
                snapshot.summary = _( "That is not kitten." );
                a11y::append_document( snapshot, "kitten:message", _( "Not kitten" ),
                                       this_bogus_message );
                add_field_nodes();
                break;
            case ui_state::end_animation:
                snapshot.summary = _( "You found kitten! Way to go, robot!" );
                a11y::append_document( snapshot, "kitten:result", _( "You found kitten!" ),
                                       _( "You found kitten! Way to go, robot!" ) );
                break;
            case ui_state::exit:
                break;
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

    a11y::screen_scope reader( &ctxt, "robot-finds-kitten",
    [this]( const a11y::semantic_intent & intent ) -> std::optional<std::string> {
        if( intent.node == "kitten:start" && intent.action == "activate" ) {
            // Any direction starts the game from the instructions state
            // (see the instructions branch of process_input()).
            return std::string( "UP" );
        }
        if( intent.action == "focus" && intent.node.rfind( "kitten:", 0 ) == 0 ) {
            // Pure observation: inject no action at all. An injected
            // action would be consumed by process_input() like any
            // keypress, which in the bogus_message state would dismiss
            // the message the player is still reading, and on the last
            // end_animation frame would leave the end screen.
            return std::nullopt;
        }
        return a11y::registered_intent( ctxt, intent );
    } );

    /* Now the fun begins. */
    current_ui_state = ui_state::instructions;
    while( current_ui_state != ui_state::exit ) {
        ui_manager::redraw();
        if( a11y::enabled() ) {
            reader.update( make_reader_snapshot() );
        }
        process_input();
    }
}

void robot_finds_kitten::show() const
{
    draw_border( bkatwin );
    wnoutrefresh( bkatwin );

    werase( w );
    if( current_ui_state != ui_state::instructions ) {
        mvwhline( w, point( 0, 2 ), BORDER_COLOR, '_', rfkCOLS );
        wmove( w, kitten.pos );
        draw_kitten();

        for( int c = 0; c < numbogus; c++ ) {
            mvwputch( w, bogus[c].pos, bogus[c].color, bogus[c].character );
        }

        wmove( w, robot.pos );
        draw_robot();
    }
    switch( current_ui_state ) {
        case ui_state::instructions: {
            int pos = 1;
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            pos += fold_and_print( w, point( 1, 0 ), getmaxx( w ) - 4, c_light_gray,
                                   _( "robotfindskitten v22July2008" ) );
            pos += 1 + fold_and_print( w, point( 1, pos ), getmaxx( w ) - 4, c_light_gray,
                                       _( "Originally by the illustrious Leonard Richardson, "
                                          "rewritten in PDCurses by Joseph Larson, "
                                          "ported to CDDA gaming system by a nutcase." ) );

            pos += 1 + fold_and_print( w, point( 1, pos ), getmaxx( w ) - 4, c_light_gray,
                                       _( "In this game, you are robot (" ) );
            draw_robot();
            wprintz( w, c_light_gray, _( ")." ) );
            pos += 1 + fold_and_print( w, point( 1, pos ), getmaxx( w ) - 4, c_light_gray,
                                       _( "Your job is to find kitten.  This task is complicated by the existence of various things "
                                          "which are not kitten.  Robot must touch items to determine if they are kitten or not.  "
                                          "The game ends when robot finds kitten.  Alternatively, you may end the game by hitting %s." ),
                                       ctxt.get_desc( "QUIT" ) );
            fold_and_print( w, point( 1, pos ), getmaxx( w ) - 4, c_light_gray,
                            _( "Press any key to start." ) );
            break;
        }
        case ui_state::main:
            mvwprintz( w, point::zero, c_white, _( "robotfindskitten v22July2008 - press %s to quit." ),
                       ctxt.get_desc( "QUIT" ) );
            break;
        case ui_state::invalid_input:
            mvwprintz( w, point::zero, c_white, _( "Invalid command: Use direction keys or press %s to quit." ),
                       ctxt.get_desc( "QUIT" ) );
            break;
        case ui_state::bogus_message: {
            std::vector<std::string> bogusvstr = foldstring( this_bogus_message, rfkCOLS );
            wattron( w, c_white );
            for( size_t c = 0; c < bogusvstr.size(); c++ ) {
                mvwprintw( w, point( 0, c ), bogusvstr[c] );
            }
            wattroff( w, c_white );
            break;
        }
        case ui_state::end_animation: {
            /* The grand cinema scene. */
            const int c = std::min( end_animation_frame, 3 );
            wmove( w, point( rfkCOLS / 2 - 4 + c, 1 ) );
            if( end_animation_last_input_left_or_up ) {
                draw_kitten();
            } else {
                draw_robot();
            }
            wmove( w, point( rfkCOLS / 2 + 3 - c, 1 ) );
            if( end_animation_last_input_left_or_up ) {
                draw_robot();
            } else {
                draw_kitten();
            }
            if( end_animation_frame >= 4 ) {
                /* They're in love! */
                mvwprintz( w, point( ( rfkCOLS - 6 ) / 2 - 1, 0 ), c_light_red, "<3<3<3" );
            }
            if( end_animation_frame >= 5 ) {
                mvwprintz( w, point::zero, c_white, _( "You found kitten!  Way to go, robot!" ) );
            }
            break;
        }
        case ui_state::exit:
            break;
    }
    wnoutrefresh( w );
}

void robot_finds_kitten::process_input()
{
    switch( current_ui_state ) {
        case ui_state::instructions: {
            const std::string action = ctxt.handle_input();
            if( action == "QUIT" ) {
                current_ui_state = ui_state::exit;
            } else if( ( action == "ANY_INPUT" && !ctxt.get_raw_input().sequence.empty() ) ||
                       action == "UP" || action == "DOWN" || action == "LEFT" ||
                       action == "RIGHT" ) {
                // Originally the cardinal directions were not registered
                // on this screen, so a direction key fell through to
                // ANY_INPUT like any other key. The member context
                // registers them once (see the constructor), so a
                // direction key now reports its direction action here;
                // it still starts the game, exactly as before.
                current_ui_state = ui_state::main;
            }
            break;
        }
        case ui_state::main:
        case ui_state::invalid_input:
        case ui_state::bogus_message: {
            const std::string action = ctxt.handle_input();
            if( action == "QUIT" ) {
                current_ui_state = ui_state::exit;
            } else if( action == "ANY_INPUT" ) {
                if( !ctxt.get_raw_input().sequence.empty() ) {
                    current_ui_state = ui_state::invalid_input;
                }
            } else if( action != "HELP_KEYBINDINGS" ) {
                current_ui_state = ui_state::main;
                point check = robot.pos;

                if( action == "UP" ) {
                    check.y--;
                } else if( action == "DOWN" ) {
                    check.y++;
                } else if( action == "LEFT" ) {
                    check.x--;
                } else if( action == "RIGHT" ) {
                    check.x++;
                }

                constexpr half_open_rectangle<point> bounds(
                    point( 0, 3 ), point( rfkCOLS, rfkLINES ) );
                if( !bounds.contains( check ) ) {
                    /* Can't move past edge */
                } else if( rfkscreen[check.x][check.y] != EMPTY ) {
                    switch( rfkscreen[check.x][check.y] ) {
                        case ROBOT:
                            /* We didn't move. */
                            break;
                        case KITTEN:
                            /* Found it! */
                            ret = true;
                            current_ui_state = ui_state::end_animation;
                            end_animation_frame = 0;
                            end_animation_last_input_left_or_up = action == "UP" || action == "LEFT";
                            break;
                        default:
                            current_ui_state = ui_state::bogus_message;
                            this_bogus_message = bogus_messages.at( rfkscreen[check.x][check.y] - 2 );
                            break;
                    }
                } else {
                    /* Otherwise, move the robot. */
                    rfkscreen[robot.pos.x][robot.pos.y] = EMPTY;
                    robot.pos = check;
                    rfkscreen[robot.pos.x][robot.pos.y] = ROBOT;
                }
            }
            break;
        }
        case ui_state::end_animation:
            if( end_animation_frame + 1 == num_end_animation_frames ) {
                const std::string action = ctxt.handle_input();
                if( action != "HELP_KEYBINDINGS" ) {
                    current_ui_state = ui_state::exit;
                }
            } else {
                refresh_display();
                // Sleep for 1 s
                const auto sleep_till = std::chrono::steady_clock::now()
                                        + std::chrono::nanoseconds( 1000000000 );
                do {
                    const auto sleep_for = std::min( sleep_till - std::chrono::steady_clock::now(),
                                                     // Pump events every 100 ms
                                                     std::chrono::nanoseconds( 100000000 ) );
                    if( sleep_for > std::chrono::nanoseconds( 0 ) ) {
                        std::this_thread::sleep_for( sleep_for );
                        inp_mngr.pump_events();
                    } else {
                        break;
                    }
                } while( true );
                end_animation_frame++;
            }
            break;
        case ui_state::exit:
            break;
    }
}

/* Draws robot at current position */
void robot_finds_kitten::draw_robot() const
{
    wputch( w, robot.color, robot.character );
}

/* Draws kitten at current position */
void robot_finds_kitten::draw_kitten() const
{
    wputch( w, kitten.color, kitten.character );
}
