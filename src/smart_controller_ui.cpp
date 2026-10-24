#include "smart_controller_ui.h"

#include "a11y_input.h"
#include "a11y_runtime.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "cata_utility.h"
#include "color.h"
#include "cursesdef.h"
#include "output.h"
#include "point.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "uilist.h"
#include "ui_manager.h"

static catacurses::window init_window()
{
    const int width = smart_controller_ui::WIDTH;
    const int height = smart_controller_ui::HEIGHT;
    const point p( std::max( 0, ( TERMX - width ) / 2 ), std::max( 0, ( TERMY - height ) / 2 ) );
    return catacurses::newwin( height, width, p );
}

smart_controller_settings::smart_controller_settings( bool &enabled, int &battery_lo,
        int &battery_hi ) : enabled( enabled ), battery_lo( battery_lo ), battery_hi( battery_hi ) {}

smart_controller_ui::smart_controller_ui( smart_controller_settings initial_settings ) :
    win( init_window() ), ctxt( "SMART_ENGINE_CONTROLLER" ), settings( initial_settings )
{
    ctxt.register_directions();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "NEXT_TAB" );
}

void smart_controller_ui::refresh()
{
    werase( win );
    draw_border( win );

    const nc_color gray = c_light_gray;
    const nc_color white = c_white;
    const nc_color lgreen = c_light_green;
    const nc_color red = c_red;

    // header
    const std::string title =  _( "Smart Engine Controller ® Interface" );
    mvwprintz( win, point( ( WIDTH - title.length() ) / 2, 1 ), white, title );
    mvwhline( win, point( 1, 2 ), BORDER_COLOR, LINE_OXOX, WIDTH - 2 );

    // for menu items, y points to the center of the menu item vertical space
    int y = 3 + MENU_ITEM_HEIGHT / 2;

    // enabled flag
    mvwprintz( win, point( LEFT_MARGIN, y ), gray,  "[ ]" );
    if( settings.enabled ) {
        mvwprintz( win, point( LEFT_MARGIN + 1, y ), white, "X" );
    }
    mvwprintz( win, point( LEFT_MARGIN + 4, y ),
               selection == smart_controller_ui_selection::enabled ? hilite( white ) : gray,
               _( "Enabled" ) );

    // battery % slider
    y += MENU_ITEM_HEIGHT;
    mvwprintz( win, point( LEFT_MARGIN, y - 1 ),
               selection == smart_controller_ui_selection::lo_and_hi_slider ? white : gray,
               _( "Electric motor use (%% battery)" ) );

    int lo_slider_x = settings.battery_lo * SLIDER_W / 100;
    int hi_slider_x = settings.battery_hi * SLIDER_W / 100 ;
    // print selected % numbers
    std::string battery_low_text = string_format( "%d%%", settings.battery_lo );
    mvwprintz( win, point( LEFT_MARGIN + lo_slider_x - battery_low_text.length() + 1, y + 2 ),
               selection == smart_controller_ui_selection::lo_and_hi_slider &&
               slider == 0 ? hilite( white ) : red, battery_low_text );
    mvwprintz( win, point( LEFT_MARGIN + hi_slider_x, y + 2 ),
               selection == smart_controller_ui_selection::lo_and_hi_slider &&
               slider == 1 ? hilite( white ) : lgreen, "%d%%", settings.battery_hi );
    // draw slider horizontal line
    for( int i = 0; i < SLIDER_W; ++i ) {
        nc_color col = selection == smart_controller_ui_selection::lo_and_hi_slider ? white : gray;
        mvwprintz( win, point( LEFT_MARGIN + i, y + 1 ), col, "-" );
    }
    // print LO and HI on the slider line
    //~ abbreviation for the "LOW" battery zone displayed on the Smart Controller UI slider. Keep it short (2-3 chars)
    std::string lo_text = _( "LO" );
    mvwprintz( win, point( LEFT_MARGIN + ( lo_slider_x - lo_text.length() ) / 2, y + 1 ), red,
               lo_text );
    //~ abbreviation for the "HIGH" battery zone displayed on the Smart Controller UI slider. Keep it short (2-3 chars)
    std::string hi_text = _( "HI" );
    int hi_text_l = hi_text.length();
    mvwprintz( win, point( LEFT_MARGIN + hi_slider_x + std::max( ( SLIDER_W - hi_slider_x -
                           hi_text_l ) / 2, 1 ), y + 1 ), lgreen, hi_text );
    // print bars on the slider
    mvwprintz( win, point( LEFT_MARGIN + lo_slider_x, y + 1 ),
               selection == smart_controller_ui_selection::lo_and_hi_slider &&
               slider == 0 ? hilite( white ) : red, "|" );
    mvwprintz( win, point( LEFT_MARGIN + hi_slider_x, y + 1 ),
               selection == smart_controller_ui_selection::lo_and_hi_slider &&
               slider == 1 ? hilite( white ) : lgreen, "|" );

    // user manual
    y += MENU_ITEM_HEIGHT;
    mvwprintz( win, point( LEFT_MARGIN, y ),
               selection == smart_controller_ui_selection::manual ? hilite( white ) : gray,
               _( "User manual" ) );

    // key descriptions
    std::string keys_text = string_format(
                                _( "Use [<color_yellow>%s</color> and <color_yellow>%s</color>] to select option.\n"
                                   "Use [<color_yellow>%s</color>] to change value.\n"
                                   "Use [<color_yellow>%s</color> or <color_yellow>%s</color>] to switch between sliders.\n"
                                   "Use [<color_yellow>%s</color> and <color_yellow>%s</color>] to move sliders."
                                   "Use [<color_yellow>%s</color>] to apply changes and quit." ),
                                ctxt.get_desc( "UP" ),
                                ctxt.get_desc( "DOWN" ),
                                ctxt.get_desc( "CONFIRM" ),
                                ctxt.get_desc( "NEXT_TAB" ),
                                ctxt.get_desc( "CONFIRM" ),
                                ctxt.get_desc( "LEFT" ),
                                ctxt.get_desc( "RIGHT" ),
                                ctxt.get_desc( "QUIT" ) );

    int keys_text_w =  WIDTH - 2;
    int keys_text_lines_n = foldstring( keys_text, keys_text_w ).size();
    fold_and_print( win, point( 1, HEIGHT - 1 - keys_text_lines_n ), keys_text_w, gray, keys_text );

    wnoutrefresh( win );
}

void smart_controller_ui::control()
{
    ui_adaptor ui;
    ui.on_screen_resize( [this]( ui_adaptor & ui ) {
        win = init_window();
        ui.position_from_window( win );
    } );
    ui.mark_resize();
    ui.on_redraw( [this]( const ui_adaptor & ) {
        refresh();
    } );

    // Reader adapter (R15.4): this screen is drawn by hand, so its input
    // context would otherwise only get the generic command fallback,
    // which never publishes the thresholds themselves. The settings are
    // published as R02 form fields with the domains the slider math in
    // this same function enforces: multiples of 5 percent, the low
    // threshold in [5, 90], the high threshold in [10, 95], and always
    // at least 5 points between them. Reader edits follow the form
    // contract -- set_value/select_choice only record a draft, apply
    // writes it through the same settings references the sliders write,
    // with the same coupling (raising low pushes high up, lowering high
    // pulls low down). The one exception mirrors the original screen:
    // activating the Enabled field is the screen's own CONFIRM, which
    // toggles the flag immediately, exactly as the keypress does.
    std::optional<bool> draft_enabled;
    std::optional<int> draft_lo;
    std::optional<int> draft_hi;
    a11y::screen_scope reader( &ctxt, "smart-engine-controller",
    [&]( const a11y::semantic_intent & intent ) -> std::optional<std::string> {
        if( intent.node == "smart:enabled" ) {
            if( intent.action == "focus" ) {
                selection = smart_controller_ui_selection::enabled;
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "select_choice" ) {
                draft_enabled = intent.value == "true";
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "apply" ) {
                if( draft_enabled ) {
                    settings.enabled = *draft_enabled;
                    draft_enabled.reset();
                }
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "cancel_draft" ) {
                draft_enabled.reset();
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "activate" ) {
                selection = smart_controller_ui_selection::enabled;
                draft_enabled.reset();
                return std::string( "CONFIRM" );
            }
            return std::nullopt;
        }
        if( intent.node == "smart:battery_lo" || intent.node == "smart:battery_hi" ) {
            const bool is_lo = intent.node == "smart:battery_lo";
            std::optional<int> &draft = is_lo ? draft_lo : draft_hi;
            if( intent.action == "focus" ) {
                selection = smart_controller_ui_selection::lo_and_hi_slider;
                slider = is_lo ? 0 : 1;
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "set_value" ) {
                draft = std::stoi( intent.value );
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "apply" ) {
                if( draft ) {
                    if( is_lo ) {
                        settings.battery_lo = clamp( ( *draft / 5 ) * 5, 5, 90 );
                        settings.battery_hi = std::max( settings.battery_lo + 5,
                                                        settings.battery_hi );
                    } else {
                        settings.battery_hi = clamp( ( *draft / 5 ) * 5, 10, 95 );
                        settings.battery_lo = std::min( settings.battery_hi - 5,
                                                        settings.battery_lo );
                    }
                    draft.reset();
                }
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "cancel_draft" ) {
                draft.reset();
                return std::string( "TIMEOUT" );
            }
            return std::nullopt;
        }
        if( intent.node == "smart:manual" ) {
            if( intent.action == "focus" ) {
                selection = smart_controller_ui_selection::manual;
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "activate" ) {
                selection = smart_controller_ui_selection::manual;
                return std::string( "CONFIRM" );
            }
            return std::nullopt;
        }
        return a11y::registered_intent( ctxt, intent );
    } );

    const auto make_reader_snapshot = [&]() {
        a11y::semantic_snapshot snapshot = a11y::command_snapshot( ctxt,
                                           _( "Smart Engine Controller" ) );
        std::vector<a11y::semantic_node> commands = std::move( snapshot.nodes );
        snapshot.nodes.clear();
        snapshot.summary = string_format(
                               _( "Smart engine controller: %1$s. Battery low threshold: %2$d%%. "
                                  "Battery high threshold: %3$d%%." ),
                               settings.enabled ? _( "enabled" ) : _( "disabled" ),
                               settings.battery_lo, settings.battery_hi );

        a11y::semantic_node enabled_node;
        enabled_node.id = "smart:enabled";
        enabled_node.role = "checkbox";
        enabled_node.label = _( "Enabled" );
        enabled_node.field_type = "bool";
        enabled_node.applied_value = settings.enabled ? "true" : "false";
        enabled_node.value = settings.enabled ? _( "Enabled" ) : _( "Disabled" );
        enabled_node.checked = settings.enabled;
        enabled_node.choices = { { "true", _( "Enabled" ) }, { "false", _( "Disabled" ) } };
        enabled_node.actions = { "focus", "activate", "select_choice", "apply", "cancel_draft" };
        enabled_node.selected = selection == smart_controller_ui_selection::enabled;
        if( draft_enabled ) {
            enabled_node.has_draft = true;
            enabled_node.draft_value = *draft_enabled ? "true" : "false";
        }
        if( enabled_node.selected ) {
            snapshot.focus = enabled_node.id;
        }
        snapshot.nodes.push_back( enabled_node );

        const auto threshold_node = [&]( const char *id, const std::string & label,
        const std::string & detail, int applied, int minimum, int maximum,
        const std::optional<int> &draft, const char *depends ) {
            a11y::semantic_node node;
            node.id = id;
            node.role = "slider";
            node.label = label;
            node.detail = detail;
            node.field_type = "int";
            node.applied_value = std::to_string( applied );
            node.value = string_format( "%d%%", applied );
            node.has_field_range = true;
            node.field_min = minimum;
            node.field_max = maximum;
            node.field_step = 5;
            node.depends_on = { depends };
            node.actions = { "focus", "set_value", "apply", "cancel_draft" };
            if( draft ) {
                node.has_draft = true;
                node.draft_value = std::to_string( *draft );
            }
            return node;
        };
        a11y::semantic_node lo_node = threshold_node( "smart:battery_lo",
                                      _( "Battery low threshold" ),
                                      _( "Hard limit: at or below this battery charge, the controller forbids discharging the battery for the electric motors." ),
                                      settings.battery_lo, 5, 90, draft_lo, "smart:battery_hi" );
        lo_node.selected = selection == smart_controller_ui_selection::lo_and_hi_slider &&
                           slider == 0;
        if( lo_node.selected ) {
            snapshot.focus = lo_node.id;
        }
        snapshot.nodes.push_back( lo_node );
        a11y::semantic_node hi_node = threshold_node( "smart:battery_hi",
                                      _( "Battery high threshold" ),
                                      _( "Soft limit: at or below this battery charge, the controller charges the battery toward this level and limits discharging; above it, discharging is allowed." ),
                                      settings.battery_hi, 10, 95, draft_hi, "smart:battery_lo" );
        hi_node.selected = selection == smart_controller_ui_selection::lo_and_hi_slider &&
                           slider == 1;
        if( hi_node.selected ) {
            snapshot.focus = hi_node.id;
        }
        snapshot.nodes.push_back( hi_node );

        a11y::semantic_node manual;
        manual.id = "smart:manual";
        manual.role = "button";
        manual.label = _( "User manual" );
        manual.detail = _( "Open the smart engine controller user manual." );
        manual.actions = { "focus", "activate" };
        manual.selected = selection == smart_controller_ui_selection::manual;
        if( manual.selected ) {
            snapshot.focus = manual.id;
        }
        snapshot.nodes.push_back( manual );

        snapshot.nodes.insert( snapshot.nodes.end(),
                               std::make_move_iterator( commands.begin() ),
                               std::make_move_iterator( commands.end() ) );
        for( size_t i = 0; i < snapshot.nodes.size(); ++i ) {
            snapshot.nodes[i].position = static_cast<int>( i ) + 1;
            snapshot.nodes[i].total = static_cast<int>( snapshot.nodes.size() );
        }
        return snapshot;
    };

    std::string action;

    do {
        ui_manager::redraw();
        if( a11y::enabled() ) {
            reader.update( make_reader_snapshot() );
        }
        action = ctxt.handle_input();

        if( action == "CONFIRM" || ( action == "NEXT_TAB" &&
                                     selection == smart_controller_ui_selection::lo_and_hi_slider ) ) {
            switch( selection ) {
                case smart_controller_ui_selection::enabled:
                    settings.enabled = !settings.enabled;
                    break;
                case smart_controller_ui_selection::lo_and_hi_slider:
                    slider = 1 - slider;
                    break;
                case smart_controller_ui_selection::manual:
                    const translation manual =
                        SNIPPET.random_from_category( "smart_engine_controller_manual" ).value_or( translation() );

                    scrollable_text( [&] {
                        //note: ui.on_screen_resize will be called first and reinit `win`
                        return win;
                    }, _( "Smart Engine Controller ® User Manual" ), manual.translated() );

                    break;
            }
        } else if( action == "UP" || action == "DOWN" ) {
            selection = inc_clamp_wrap( selection, action == "DOWN", MENU_ITEMS_N );
        } else if( selection == smart_controller_ui_selection::lo_and_hi_slider && ( action == "LEFT" ||
                   action == "RIGHT" ) ) {
            const int dx = action == "RIGHT" ? 1 : -1;

            if( slider == 0 ) {
                settings.battery_lo = clamp( ( settings.battery_lo / 5 + dx ) * 5, 5, 90 );
                // keep the min space of 5 between this and other slider
                settings.battery_hi = std::max( settings.battery_lo + 5, settings.battery_hi );
            } else {
                settings.battery_hi = clamp( ( settings.battery_hi / 5 + dx ) * 5, 10, 95 );
                // keep the min space of 5 between this and other slider
                settings.battery_lo = std::min( settings.battery_hi - 5, settings.battery_lo );
            }
        }

    } while( action != "QUIT" );
}
