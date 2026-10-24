#include "clzones.h" // IWYU pragma: associated

#include <algorithm>
#include <iterator>

#include "a11y_input.h"
#include "a11y_runtime.h"
#include "avatar.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "faction.h"
#include "game.h"
#include "line.h"
#include "input_context.h"
#include "map.h"
#include "options.h"
#include "output.h"
#include "overmap_ui.h"
#include "panels.h"
#include "popup.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "uilist.h"
#include "ui_manager.h"

static const zone_type_id zone_type_LOOT_CUSTOM( "LOOT_CUSTOM" );

shared_ptr_fast<game::draw_callback_t> zone_manager_ui::create_zone_callback(
    const std::optional<tripoint_bub_ms> &zone_start,
    const std::optional<tripoint_bub_ms> &zone_end,
    const bool &zone_blink,
    const bool &zone_cursor,
    const bool &is_moving_zone
)
{
    map &here = get_map();

    return make_shared_fast<game::draw_callback_t>(
    [&]() {
        if( zone_cursor ) {
            if( is_moving_zone ) {
                g->draw_cursor( tripoint_bub_ms( ( zone_start.value().raw() + zone_end.value().raw() ) / 2 ) );
            } else {
                if( zone_end ) {
                    g->draw_cursor( zone_end.value() );
                } else if( zone_start ) {
                    g->draw_cursor( zone_start.value() );
                }
            }
        }
        if( zone_blink && zone_start && zone_end ) {
            avatar &player_character = get_avatar();
            const tripoint_bub_ms pos = player_character.pos_bub( here );
            const point_rel_ms offset2( player_character.view_offset.xy() +
                                        point( pos.x() - getmaxx( g->w_terrain ) / 2,
                                               pos.y() - getmaxy( g->w_terrain ) / 2 ) );

            tripoint_rel_ms offset;
#if defined(TILES)
            if( use_tiles ) {
                offset = tripoint_rel_ms::zero; //TILES
            } else {
#endif
                offset = { offset2, 0 }; //CURSES
#if defined(TILES)
            }
#endif

            const tripoint_bub_ms start( std::min( zone_start->x(), zone_end->x() ),
                                         std::min( zone_start->y(), zone_end->y() ),
                                         zone_end->z() );
            const tripoint_bub_ms end( std::max( zone_start->x(), zone_end->x() ),
                                       std::max( zone_start->y(), zone_end->y() ),
                                       zone_end->z() );
            g->draw_zones( start, end, offset );
        }
    } );
}

void zone_manager_ui::zones_manager_shortcuts( const catacurses::window &w_info,
        faction_id const &faction,
        bool show_all_zones, const input_context &ctxt, const int width )
{
    werase( w_info );

    std::vector<std::string> keybinding_tips;
    std::vector<std::string> act_descs;
    std::string show_zones_text = show_all_zones ? "Showing all zones" : "Hiding distant zones";
    std::string zone_faction = string_format( _( "Shown faction: %s" ), faction.str() );
    const auto add_action_desc = [&]( const std::string & act, const std::string & txt ) {
        act_descs.emplace_back( ctxt.get_desc( act, txt, input_context::allow_all_keys ) );
    };

    add_action_desc( "ADD_ZONE", pgettext( "zones manager", "Add" ) );
    add_action_desc( "ADD_PERSONAL_ZONE", pgettext( "zones manager", "Personal" ) );
    add_action_desc( "REMOVE_ZONE", pgettext( "zones manager", "Remove" ) );
    add_action_desc( "ENABLE_ZONE", pgettext( "zones manager", "Enable" ) );
    add_action_desc( "DISABLE_ZONE", pgettext( "zones manager", "Disable" ) );
    add_action_desc( "TOGGLE_ZONE_DISPLAY", pgettext( "zones manager", "Toggle zone display" ) );
    add_action_desc( "ENABLE_PERSONAL_ZONES", pgettext( "zones manager", "Enable personal" ) );
    add_action_desc( "DISABLE_PERSONAL_ZONES", pgettext( "zones manager", "Disable personal" ) );
    add_action_desc( "ENABLE_VEHICLE_ZONES", pgettext( "zones manager", "Enable vehicle" ) );
    add_action_desc( "DISABLE_VEHICLE_ZONES", pgettext( "zones manager", "Disable vehicle" ) );
    add_action_desc( "MOVE_ZONE_UP", pgettext( "zones manager", "Move up" ) );
    add_action_desc( "MOVE_ZONE_DOWN", pgettext( "zones manager", "Move down" ) );
    add_action_desc( "CONFIRM", pgettext( "zones manager", "Edit" ) );
    add_action_desc( "SHOW_ALL_ZONES", pgettext( "zones manager", show_zones_text.c_str() ) );
    add_action_desc( "SHOW_ZONE_ON_MAP", pgettext( "zones manager", "Map" ) );
    if( debug_mode ) {
        add_action_desc( "CHANGE_FACTION", pgettext( "zones manager", zone_faction.c_str() ) );
    }
    keybinding_tips = foldstring( enumerate_as_string( act_descs, enumeration_conjunction::none ),
                                  width - 2 );
    for( size_t i = 0; i < keybinding_tips.size(); ++i ) {
        nc_color dummy = c_white;
        print_colored_text( w_info, point( 1, 1 + i ), dummy, c_white, keybinding_tips[i] );
    }

    wnoutrefresh( w_info );
}

void zone_manager_ui::zones_manager_draw_borders( const catacurses::window &w_border,
        const catacurses::window &w_info_border,
        const int iInfoHeight, const int width )
{
    wattron( w_border, c_light_gray );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwhline( w_border, point( 1, 0 ), LINE_OXOX, width - 1 ); // -
    mvwhline( w_border, point( 1, TERMY - iInfoHeight - 1 ), LINE_OXOX, width - 1 ); // -
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwvline( w_border, point( 0, 1 ), LINE_XOXO, TERMY - iInfoHeight - 1 ); // |
    mvwvline( w_border, point( width - 1, 1 ), LINE_XOXO, TERMY - iInfoHeight - 1 ); // |

    mvwaddch( w_border, point::zero, LINE_OXXO ); // |^
    mvwaddch( w_border, point( width - 1, 0 ), LINE_OOXX ); // ^|

    mvwaddch( w_border, point( 0, TERMY - iInfoHeight - 1 ), LINE_XXXO ); // |-
    mvwaddch( w_border, point( width - 1, TERMY - iInfoHeight - 1 ), LINE_XOXX ); // -|
    wattroff( w_border, c_light_gray );

    mvwprintz( w_border, point( 2, 0 ), c_white, _( "Zones manager" ) );

    wnoutrefresh( w_border );

    wattron( w_info_border, c_light_gray );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwvline( w_info_border, point( 0, 0 ), LINE_XOXO, iInfoHeight - 1 );
    mvwvline( w_info_border, point( width - 1, 0 ), LINE_XOXO, iInfoHeight - 1 );
    mvwhline( w_info_border, point( 0, iInfoHeight - 1 ), LINE_OXOX, width - 1 );

    mvwaddch( w_info_border, point( 0, iInfoHeight - 1 ), LINE_XXOO );
    mvwaddch( w_info_border, point( width - 1, iInfoHeight - 1 ), LINE_XOOX );
    wattroff( w_info_border, c_light_gray );

    wnoutrefresh( w_info_border );
}

void zone_manager_ui::display_zone_manager()
{
    map &here = get_map();
    avatar &u = get_avatar();

    const tripoint_bub_ms pos = u.pos_bub();

    const tripoint_rel_ms stored_view_offset = u.view_offset;

    u.view_offset = tripoint_rel_ms::zero;

    const int zone_ui_height = 14;
    const int zone_options_height = debug_mode ? 6 : 7;

    const bool sidebar_on_right = get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
    const int sidebar_width = sidebar_on_right ?
                              panel_manager::get_manager().get_width_right() :
                              panel_manager::get_manager().get_width_left();
    const int minimum_zone_ui_width = 45;
    const int zone_ui_width = std::max( sidebar_width, minimum_zone_ui_width );

    int offsetX = 0;
    int max_rows = 0;

    catacurses::window w_zones;
    catacurses::window w_zones_border;
    catacurses::window w_zones_info;
    catacurses::window w_zones_info_border;
    catacurses::window w_zones_options;

    bool show = true;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        if( !show ) {
            ui.position( point::zero, point::zero );
            return;
        }
        offsetX = get_option<std::string>( "SIDEBAR_POSITION" ) != "left" ?
                  TERMX - zone_ui_width : 0;
        const int w_zone_height = TERMY - zone_ui_height;
        max_rows = w_zone_height - 2;
        w_zones = catacurses::newwin( w_zone_height - 2, zone_ui_width - 2,
                                      point( offsetX + 1, 1 ) );
        w_zones_border = catacurses::newwin( w_zone_height, zone_ui_width,
                                             point( offsetX, 0 ) );
        w_zones_info = catacurses::newwin( zone_ui_height - zone_options_height - 1,
                                           zone_ui_width - 2, point( offsetX + 1, w_zone_height ) );
        w_zones_info_border = catacurses::newwin( zone_ui_height, zone_ui_width,
                              point( offsetX, w_zone_height ) );
        w_zones_options = catacurses::newwin( zone_options_height - 1, zone_ui_width - 2,
                                              point( offsetX + 1, TERMY - zone_options_height ) );

        ui.position( point( offsetX, 0 ), point( zone_ui_width, TERMY ) );
    } );
    ui.mark_resize();

    input_context ctxt( "ZONES_MANAGER" );
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "ADD_ZONE" );
    ctxt.register_action( "ADD_PERSONAL_ZONE" );
    ctxt.register_action( "REMOVE_ZONE" );
    ctxt.register_action( "MOVE_ZONE_UP" );
    ctxt.register_action( "MOVE_ZONE_DOWN" );
    ctxt.register_action( "SHOW_ZONE_ON_MAP" );
    ctxt.register_action( "ENABLE_ZONE" );
    ctxt.register_action( "DISABLE_ZONE" );
    ctxt.register_action( "TOGGLE_ZONE_DISPLAY" );
    ctxt.register_action( "ENABLE_PERSONAL_ZONES" );
    ctxt.register_action( "DISABLE_PERSONAL_ZONES" );
    ctxt.register_action( "ENABLE_VEHICLE_ZONES" );
    ctxt.register_action( "DISABLE_VEHICLE_ZONES" );
    ctxt.register_action( "SHOW_ALL_ZONES" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    if( debug_mode ) {
        ctxt.register_action( "CHANGE_FACTION" );
    }

    zone_manager &mgr = zone_manager::get_manager();
    int start_index = 0;
    int active_index = 0;
    bool blink = false;
    bool stuff_changed = false;
    bool show_all_zones = false;
    int zone_cnt = 0;
    faction_id zones_faction( your_fac );

    // reset any zones that were temporarily disabled for an activity
    mgr.reset_disabled();

    // cache the players location for person zones
    if( mgr.has_personal_zones() ) {
        mgr.cache_avatar_location();
    }

    // get zones with distance between player and
    // zone center point <= MAX_VIEW_DISTANCE or all zones, if show_all_zones is true
    auto get_zones = [&]() {
        std::vector<zone_manager::ref_zone_data> zones;
        if( show_all_zones ) {
            zones = mgr.get_zones( zones_faction );
        } else {
            const tripoint_abs_ms u_abs_pos = u.pos_abs();
            for( zone_manager::ref_zone_data &ref : mgr.get_zones( zones_faction ) ) {
                const tripoint_abs_ms &zone_abs_pos = ref.get().get_center_point();
                if( rl_dist( u_abs_pos, zone_abs_pos ) <= MAX_VIEW_DISTANCE ) {
                    zones.emplace_back( ref );
                }
            }
        }
        zones.erase( std::remove_if( zones.begin(), zones.end(),
        []( zone_manager::ref_zone_data const & it ) {
            zone_type_id const type = it.get().get_type();
            return !debug_mode && type.is_valid() && type->hidden;
        } ),
        zones.end() );
        zone_cnt = static_cast<int>( zones.size() );
        return zones;
    };

    auto zones = get_zones();

    // Reader adapter (R16): this screen is a ui_adaptor that draws the
    // zone list itself, so the generic command fallback would never
    // publish the zones. Rows are published as one R02 table; selecting
    // a row only moves active_index (focus) or returns the screen's own
    // CONFIRM (activate), which opens the original edit uilist. A row's
    // identity is re-resolved against the live zones vector at intent
    // time (veh_interact reader_session precedent): the list is rebuilt
    // by get_zones() after most actions, so a stale row must not act on
    // whatever happens to sit at its old position.
    struct zone_reader_binding {
        std::string id;
        std::string name;
        std::string type;
        tripoint_abs_ms start;
        tripoint_abs_ms end;
    };
    std::vector<zone_reader_binding> zone_reader_bindings;
    std::optional<zone_reader_binding> zone_reader_armed_remove;
    const auto zone_reader_make_binding = []( const std::string &id, const zone_data &zone ) {
        return zone_reader_binding { id, zone.get_name(), zone.get_type().str(),
                                     zone.get_start_point(), zone.get_end_point()
                                   };
    };
    const auto zone_reader_find = [&zones]( const zone_reader_binding & binding )
    -> std::optional<int> {
        for( int i = 0; i < static_cast<int>( zones.size() ); ++i ) {
            const zone_data &zone = zones[i].get();
            if( zone.get_name() == binding.name && zone.get_type().str() == binding.type &&
                zone.get_start_point() == binding.start && zone.get_end_point() == binding.end ) {
                return i;
            }
        }
        return std::nullopt;
    };
    a11y::screen_scope reader( &ctxt, "zone-manager",
    [&]( const a11y::semantic_intent & intent ) -> std::optional<std::string> {
        if( intent.node == "zone:remove:confirm" ) {
            if( intent.action == "focus" ) {
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "activate" ) {
                if( !zone_reader_armed_remove ) {
                    return std::nullopt;
                }
                const std::optional<int> armed_index =
                    zone_reader_find( *zone_reader_armed_remove );
                zone_reader_armed_remove.reset();
                if( !armed_index ) {
                    return std::nullopt;
                }
                // The original REMOVE_ZONE handling removes
                // zones[active_index], so point it at the armed zone.
                active_index = *armed_index;
                return std::string( "REMOVE_ZONE" );
            }
            return std::nullopt;
        }
        if( intent.node == "REMOVE_ZONE" && intent.action == "activate" ) {
            if( zone_cnt == 0 || active_index < 0 || active_index >= zone_cnt ) {
                return std::nullopt;
            }
            // Removal has its own confirmation node (published by the
            // builder while this zone is armed and active); the first
            // activation only arms it and never returns REMOVE_ZONE.
            zone_reader_armed_remove = zone_reader_make_binding(
                                           "zone:row:" + std::to_string( active_index ),
                                           zones[active_index].get() );
            return std::string( "TIMEOUT" );
        }
        if( intent.action == "activate" ) {
            // Activating anything else disarms a pending removal.
            // Focusing does not.
            zone_reader_armed_remove.reset();
        }
        for( const zone_reader_binding &binding : zone_reader_bindings ) {
            if( intent.node != binding.id ) {
                continue;
            }
            if( intent.action != "focus" && intent.action != "activate" ) {
                return std::nullopt;
            }
            const std::optional<int> found = zone_reader_find( binding );
            if( !found ) {
                return std::nullopt;
            }
            active_index = *found;
            return intent.action == "activate" ? std::optional<std::string>( "CONFIRM" )
                   : std::optional<std::string>( "TIMEOUT" );
        }
        return a11y::registered_intent( ctxt, intent );
    } );

    const auto build_snapshot = [&]() {
        a11y::semantic_snapshot snapshot = a11y::command_snapshot( ctxt, _( "Zone manager" ) );
        std::vector<a11y::semantic_node> commands = std::move( snapshot.nodes );
        snapshot.nodes.clear();

        const tripoint_abs_ms player_absolute_pos = u.pos_abs();
        const bool has_selection = zone_cnt > 0 && active_index >= 0 && active_index < zone_cnt;
        const std::string selected_name = has_selection ? zones[active_index].get().get_name()
                                          : std::string();

        for( a11y::semantic_node &command : commands ) {
            if( command.id == "SHOW_ALL_ZONES" ) {
                command.role = "checkbox";
                command.checked = show_all_zones;
                command.detail = show_all_zones ? _( "Showing all zones." )
                                 : _( "Showing only zones within sight range." );
            } else if( command.id == "ENABLE_ZONE" ) {
                command.detail = has_selection ? string_format(
                                     _( "Enable the selected zone, %s." ), selected_name )
                                 : _( "No zone is selected." );
            } else if( command.id == "DISABLE_ZONE" ) {
                command.detail = has_selection ? string_format(
                                     _( "Disable the selected zone, %s." ), selected_name )
                                 : _( "No zone is selected." );
            } else if( command.id == "TOGGLE_ZONE_DISPLAY" ) {
                command.detail = has_selection ? string_format(
                                     _( "Toggle whether the selected zone, %s, is shown on the map." ),
                                     selected_name )
                                 : _( "No zone is selected." );
            } else if( command.id == "ADD_ZONE" ) {
                command.detail = _( "Add a zone: choose a zone type, set its options, "
                                    "enter a name, then select the two corners of the zone." );
            } else if( command.id == "ADD_PERSONAL_ZONE" ) {
                command.detail = _( "Add a personal zone: choose a zone type, set its options, "
                                    "enter a name, then select the two corners of the zone." );
            }
        }

        if( zone_cnt == 0 ) {
            snapshot.summary = _( "No Zones defined." );
        } else {
            snapshot.summary = string_format( _( "Showing %1$d of %2$d zones." ), zone_cnt,
                                              static_cast<int>( mgr.get_zones( zones_faction ).size() ) );
        }
        snapshot.summary += " " + std::string( show_all_zones ? _( "Showing all zones." )
                                               : _( "Showing only zones within sight range." ) );
        if( stuff_changed ) {
            snapshot.summary += " " + std::string( _( "There are unsaved changes." ) );
        }

        a11y::semantic_table zones_table;
        zones_table.id = "zones:list";
        zones_table.columns = { { "name", _( "Name" ), "" },
                                { "type", _( "Type" ), "" },
                                { "status", _( "Status" ), "" },
                                { "personal", _( "Personal" ), "" },
                                { "vehicle", _( "Vehicle" ), "" },
                                { "direction", _( "Direction" ), "" },
                                { "distance", _( "Distance" ), "tiles" },
                                { "displayed", _( "Displayed" ), "" }
                              };
        snapshot.tables.push_back( zones_table );

        // Axis-by-axis offset of an absolute point from the player, in
        // the same wording a11y_world's relative_position_name uses.
        const auto corner_offset = [&]( const tripoint_abs_ms & point_abs ) -> std::string {
            const int dx = point_abs.x() - player_absolute_pos.x();
            const int dy = point_abs.y() - player_absolute_pos.y();
            const int dz = point_abs.z() - player_absolute_pos.z();
            if( dx == 0 && dy == 0 && dz == 0 ) {
                return _( "Current tile" );
            }
            std::string result;
            if( dy != 0 ) {
                result += string_format( dy < 0 ? _( "North %d" ) : _( "South %d" ),
                                         std::abs( dy ) );
            }
            if( dx != 0 ) {
                if( !result.empty() ) {
                    result += ", ";
                }
                result += string_format( dx < 0 ? _( "West %d" ) : _( "East %d" ),
                                         std::abs( dx ) );
            }
            if( dz != 0 ) {
                if( dx != 0 || dy != 0 ) {
                    result += ", ";
                } else {
                    result.clear();
                }
                result += string_format( dz > 0 ? _( "Level above %d" ) : _( "Level below %d" ),
                                         std::abs( dz ) );
            }
            return result;
        };

        zone_reader_bindings.clear();
        for( size_t i = 0; i < zones.size(); ++i ) {
            const zone_data &zone = zones[i].get();
            const std::string row_id = "zone:row:" + std::to_string( i );
            const std::string type_name = mgr.get_name_from_type( zone.get_type() );
            const std::string status = zone.get_enabled() ? _( "Enabled" ) : _( "Disabled" );
            const tripoint_abs_ms center = zone.get_center_point();
            const std::string direction = direction_name_short( direction_from(
                                              player_absolute_pos, center ) );
            const int distance = static_cast<int>( trig_dist( player_absolute_pos, center ) );

            std::string detail = string_format( _( "Type: %s." ), type_name );
            detail += " " + status + ".";
            if( zone.get_temporarily_disabled() ) {
                detail += " " + std::string( _( "Temporarily disabled." ) );
            }
            if( zone.get_is_personal() ) {
                detail += " " + std::string( _( "Personal zone: it moves with you." ) );
            }
            if( zone.get_is_vehicle() ) {
                detail += " " + std::string( _( "Vehicle zone." ) );
            }
            detail += " " + string_format( _( "Center: %1$s, %2$d tiles away." ), direction,
                                           distance );
            detail += " " + string_format( _( "First corner: %s." ),
                                           corner_offset( zone.get_start_point() ) );
            detail += " " + string_format( _( "Second corner: %s." ),
                                           corner_offset( zone.get_end_point() ) );
            const int size_x = std::abs( zone.get_end_point().x() - zone.get_start_point().x() ) + 1;
            const int size_y = std::abs( zone.get_end_point().y() - zone.get_start_point().y() ) + 1;
            const int size_z = std::abs( zone.get_end_point().z() - zone.get_start_point().z() ) + 1;
            detail += " " + string_format( _( "Size: %1$d by %2$d tiles, %3$d levels." ),
                                           size_x, size_y, size_z );
            detail += " " + std::string( zone.get_is_displayed() ? _( "Shown on the map." )
                                          : _( "Not shown on the map." ) );
            if( zone.has_options() ) {
                for( const auto &desc : zone.get_options().get_descriptions() ) {
                    detail += " " + desc.first + ": " + desc.second + ".";
                }
            }

            a11y::semantic_node node;
            node.id = row_id;
            node.role = "listitem";
            node.label = zone.get_name();
            node.value = string_format( _( "%1$s, %2$s" ), type_name, status );
            node.detail = detail;
            node.selected = static_cast<int>( i ) == active_index;
            node.actions = { "focus", "activate" };
            node.table_id = "zones:list";
            node.row_index = static_cast<int>( i );
            node.cells = { { "name", zone.get_name(), "", "" },
                           { "type", type_name, "", "" },
                           { "status", status, "", "" },
                           { "personal", zone.get_is_personal() ? _( "Yes" ) : _( "No" ), "", "" },
                           { "vehicle", zone.get_is_vehicle() ? _( "Yes" ) : _( "No" ), "", "" },
                           { "direction", direction, "", "" },
                           { "distance", std::to_string( distance ), "", std::to_string( distance ) },
                           { "displayed", zone.get_is_displayed() ? _( "Yes" ) : _( "No" ), "", "" }
                         };
            snapshot.nodes.push_back( std::move( node ) );
            zone_reader_bindings.push_back( zone_reader_make_binding( row_id, zone ) );
        }

        if( zone_reader_armed_remove ) {
            const std::optional<int> armed_index = zone_reader_find( *zone_reader_armed_remove );
            if( armed_index && *armed_index == active_index ) {
                a11y::semantic_node confirm;
                confirm.id = "zone:remove:confirm";
                confirm.role = "button";
                confirm.label = string_format( _( "Confirm removal of zone %s" ),
                                               zones[*armed_index].get().get_name() );
                confirm.detail = _( "This zone will be removed from the list immediately. "
                                    "The change is only saved when you confirm saving when you "
                                    "leave; choosing not to save restores it." );
                confirm.actions = { "focus", "activate" };
                snapshot.nodes.push_back( std::move( confirm ) );
            }
        }

        if( has_selection ) {
            snapshot.focus = "zone:row:" + std::to_string( active_index );
        } else if( zone_cnt == 0 ) {
            snapshot.focus = "ADD_ZONE";
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

    auto zones_manager_options = [&]() {
        werase( w_zones_options );

        if( zone_cnt > 0 ) {
            const zone_data &zone = zones[active_index].get();

            // NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwprintz( w_zones_options, point( 1, 0 ), c_white, mgr.get_name_from_type( zone.get_type() ) );

            if( zone.has_options() ) {
                const auto &descriptions = zone.get_options().get_descriptions();

                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwprintz( w_zones_options, point( 1, 1 ), c_white, _( "Options" ) );

                int y = 2;
                wattron( w_zones_options, c_white );
                for( const auto &desc : descriptions ) {
                    mvwprintw( w_zones_options, point( 3, y ), desc.first );
                    mvwprintw( w_zones_options, point( 20, y ), desc.second );
                    y++;
                }
                wattroff( w_zones_options, c_white );
            }
        }

        wnoutrefresh( w_zones_options );
    };

    std::optional<tripoint_bub_ms> zone_start;
    std::optional<tripoint_bub_ms> zone_end;
    bool zone_blink = false;
    bool zone_cursor = false;
    shared_ptr_fast<game::draw_callback_t> zone_cb = create_zone_callback(
                zone_start, zone_end, zone_blink, zone_cursor );
    g->add_draw_callback( zone_cb );

    // This lambda returns either absolute coordinates or relative-to-player
    // coordinates, depending on whether personal is false or true respectively.
    // In C++20 we could have the return type depend on the parameter using
    // if constexpr( personal ) but for now it will just return tripoints.
    auto query_position =
    [&]() -> std::optional<std::pair<tripoint_abs_ms, tripoint_abs_ms>> {
        on_out_of_scope invalidate_current_ui( [&]()
        {
            ui.mark_resize();
        } );
        restore_on_out_of_scope show_prev( show );
        restore_on_out_of_scope zone_start_prev( zone_start );
        restore_on_out_of_scope zone_end_prev( zone_end );
        show = false;
        zone_start = std::nullopt;
        zone_end = std::nullopt;
        ui.mark_resize();

        static_popup popup;
        popup.on_top( true );
        popup.message( "%s", _( "Select first point." ) );

        tripoint_bub_ms center = pos + u.view_offset;

        const look_around_result first =
        g->look_around( /*show_window=*/false, center, center, false, true, false );
        if( first.position )
        {
            popup.message( "%s", _( "Select second point." ) );

            const look_around_result second = g->look_around( /*show_window=*/false, center, *first.position,
                    true, true, false );
            if( second.position ) {
                tripoint_abs_ms first_abs =
                here.get_abs(
                    tripoint_bub_ms(
                        std::min( first.position->x(), second.position->x() ),
                        std::min( first.position->y(), second.position->y() ),
                        std::min( first.position->z(), second.position->z() ) ) );
                tripoint_abs_ms second_abs =
                here.get_abs(
                    tripoint_bub_ms(
                        std::max( first.position->x(), second.position->x() ),
                        std::max( first.position->y(), second.position->y() ),
                        std::max( first.position->z(), second.position->z() ) ) );

                return { { first_abs, second_abs } };
            }
        }

        return std::nullopt;
    };

    auto query_personal_position =
    [&]() -> std::optional<std::pair<tripoint_rel_ms, tripoint_rel_ms>> {
        on_out_of_scope invalidate_current_ui( [&]()
        {
            ui.mark_resize();
        } );
        restore_on_out_of_scope show_prev( show );
        restore_on_out_of_scope zone_start_prev( zone_start );
        restore_on_out_of_scope zone_end_prev( zone_end );
        show = false;
        zone_start = std::nullopt;
        zone_end = std::nullopt;
        ui.mark_resize();

        static_popup popup;
        popup.on_top( true );
        popup.message( "%s", _( "Select first point." ) );

        tripoint_bub_ms center = pos + u.view_offset;

        const look_around_result first =
        g->look_around( /*show_window=*/false, center, center, false, true, false );
        if( first.position )
        {
            popup.message( "%s", _( "Select second point." ) );

            const look_around_result second = g->look_around( /*show_window=*/false, center, *first.position,
                    true, true, false );
            if( second.position ) {
                tripoint_rel_ms first_rel(
                    std::min( first.position->x() - pos.x(), second.position->x() - pos.x() ),
                    std::min( first.position->y() - pos.y(), second.position->y() - pos.y() ),
                    std::min( first.position->z() - pos.z(), second.position->z() - pos.z() ) );
                tripoint_rel_ms second_rel(
                    std::max( first.position->x() - pos.x(), second.position->x() - pos.x() ),
                    std::max( first.position->y() - pos.y(), second.position->y() - pos.y() ),
                    std::max( first.position->z() - pos.z(), second.position->z() - pos.z() ) );
                return { { first_rel, second_rel } };
            }
        }

        return std::nullopt;
    };

    ui.on_redraw( [&]( const ui_adaptor & ) {
        if( !show ) {
            return;
        }
        zones_manager_draw_borders( w_zones_border, w_zones_info_border, zone_ui_height, zone_ui_width );
        zones_manager_shortcuts( w_zones_info, zones_faction, show_all_zones, ctxt, zone_ui_width );

        if( zone_cnt == 0 ) {
            werase( w_zones );
            mvwprintz( w_zones, point( 2, 5 ), c_white, _( "No Zones defined." ) );

        } else {
            werase( w_zones );

            calcStartPos( start_index, active_index, max_rows, zone_cnt );

            draw_scrollbar( w_zones_border, active_index, max_rows, zone_cnt, point::south );
            wnoutrefresh( w_zones_border );

            int iNum = 0;

            tripoint_abs_ms player_absolute_pos = u.pos_abs();

            //Display saved zones
            for( auto &i : zones ) {
                if( iNum >= start_index &&
                    iNum < start_index + ( ( max_rows > zone_cnt ) ? zone_cnt : max_rows ) ) {
                    const zone_data &zone = i.get();

                    nc_color colorLine = zone.get_enabled() ? c_white : c_light_gray;

                    if( iNum == active_index ) {
                        mvwprintz( w_zones, point( 0, iNum - start_index ), c_yellow, "%s", ">>" );
                        colorLine = zone.get_enabled() ? c_light_green : c_green;
                    }

                    //Draw Zone name
                    mvwprintz( w_zones, point( 3, iNum - start_index ), colorLine,
                               //~ "P: <Zone Name>" represents a personal zone
                               trim_by_length( ( zone.get_is_personal() ? _( "P: " ) : "" ) + zone.get_name(),
                                               zone_ui_width - 17 ) );

                    tripoint_abs_ms center = zone.get_center_point();

                    //Draw direction + distance
                    mvwprintz( w_zones, point( zone_ui_width - 13, iNum - start_index ), colorLine, "%*d %s",
                               5, static_cast<int>( trig_dist( player_absolute_pos, center ) ),
                               direction_name_short( direction_from( player_absolute_pos,
                                                     center ) ) );

                    //Draw Vehicle Indicator
                    mvwprintz( w_zones, point( zone_ui_width - 4, iNum - start_index ), colorLine,
                               zone.get_is_vehicle() ? "*" : "" );
                }
                iNum++;
            }

            // Display zone options
            zones_manager_options();
        }

        wnoutrefresh( w_zones );
    } );

    const int scroll_rate = zone_cnt > 20 ? 10 : 3;
    bool quit = false;
    bool save = false;
    g->set_zones_manager_open( true );
    zone_manager::get_manager().save_zones( "zmgr-temp" );
    while( !quit ) {
        if( zone_cnt > 0 ) {
            blink = !blink;
            const zone_data &zone = zones[active_index].get();
            zone_start = here.get_bub( zone.get_start_point() );
            zone_end = here.get_bub( zone.get_end_point() );
            ctxt.set_timeout( get_option<int>( "BLINK_SPEED" ) );
        } else {
            blink = false;
            zone_start = zone_end = std::nullopt;
            ctxt.reset_timeout();
        }

        // Actually accessed from the terrain overlay callback `zone_cb` in the
        // call to `ui_manager::redraw`.
        //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
        zone_blink = blink;
        g->invalidate_main_ui_adaptor();

        ui_manager::redraw();

        if( a11y::enabled() ) {
            reader.update( build_snapshot() );
        }

        //Wait for input
        const std::string action = ctxt.handle_input();

        if( action == "ADD_ZONE" ) {
            do { // not a loop, just for quick bailing out if canceled
                const auto maybe_id = mgr.query_type();
                if( !maybe_id.has_value() ) {
                    break;
                }

                const zone_type_id &id = maybe_id.value();
                auto options = zone_options::create( id );

                if( !options->query_at_creation() ) {
                    break;
                }

                std::string default_name = options->get_zone_name_suggestion();
                if( default_name.empty() ) {
                    default_name = mgr.get_name_from_type( id );
                }
                const auto maybe_name = mgr.query_name( default_name );
                if( !maybe_name.has_value() ) {
                    break;
                }
                const std::string &name = maybe_name.value();

                const auto position = query_position();
                if( !position ) {
                    break;
                }

                int vehicle_zones_pre = 0;
                for( zone_manager::ref_zone_data zone : get_zones() ) {
                    if( zone.get().get_is_vehicle() ) {
                        vehicle_zones_pre++;
                    }
                }

                mgr.add( name, id, get_player_character().get_faction()->id, false, true,
                         position->first, position->second, options );

                zones = get_zones();
                active_index = zone_cnt - 1;

                int vehicle_zones_post = 0;
                for( zone_manager::ref_zone_data zone : zones ) {
                    if( zone.get().get_is_vehicle() ) {
                        vehicle_zones_post++;
                    }
                }

                if( vehicle_zones_post == vehicle_zones_pre ) {
                    active_index -= vehicle_zones_post;
                }

                stuff_changed = true;
            } while( false );

            blink = false;
        } else if( action == "ADD_PERSONAL_ZONE" ) {
            do { // not a loop, just for quick bailing out if canceled
                const auto maybe_id = mgr.query_type( true );
                if( !maybe_id.has_value() ) {
                    break;
                }

                const zone_type_id &id = maybe_id.value();
                auto options = zone_options::create( id );

                if( !options->query_at_creation() ) {
                    break;
                }

                std::string default_name = options->get_zone_name_suggestion();
                if( default_name.empty() ) {
                    default_name = mgr.get_name_from_type( id );
                }
                const auto maybe_name = mgr.query_name( default_name );
                if( !maybe_name.has_value() ) {
                    break;
                }
                const std::string &name = maybe_name.value();

                const std::optional<std::pair<tripoint_rel_ms, tripoint_rel_ms>> position =
                            query_personal_position();
                if( !position ) {
                    break;
                }

                //add a zone that is relative to the avatar position
                mgr.add( name, id, get_player_character().get_faction()->id, false, true,
                         position->first, position->second, options );
                zones = get_zones();
                active_index = zone_cnt - 1;

                int vehicle_zones = 0;
                for( zone_manager::ref_zone_data zone : zones ) {
                    if( zone.get().get_is_vehicle() ) {
                        vehicle_zones++;
                    }
                }

                active_index -= vehicle_zones;

                stuff_changed = true;
            } while( false );

            blink = false;
        } else if( action == "SHOW_ALL_ZONES" ) {
            show_all_zones = !show_all_zones;
            zones = get_zones();
            active_index = 0;
        } else if( action == "CHANGE_FACTION" ) {
            ui.invalidate_ui();
            std::string facname = zones_faction.str();
            string_input_popup()
            .description( _( "Show zones for faction:" ) )
            .width( 55 )
            .max_length( 256 )
            .edit( facname );
            zones_faction = faction_id( facname );
            zones = get_zones();
        } else if( action == "QUIT" ) {
            if( stuff_changed ) {
                const query_ynq_result res = query_ynq( _( "Save changes?" ) );
                switch( res ) {
                    case query_ynq_result::quit:
                        break;
                    case query_ynq_result::no:
                        save = false;
                        quit = true;
                        break;
                    case query_ynq_result::yes:
                        save = true;
                        quit = true;
                        break;
                }
            } else {
                save = false;
                quit = true;
            }
        } else if( zone_cnt > 0 ) {
            if( navigate_ui_list( action, active_index, scroll_rate, zone_cnt, true ) ) {
                blink = false;
            } else if( action == "REMOVE_ZONE" ) {
                if( active_index < zone_cnt ) {
                    mgr.remove( zones[active_index] );
                    zones = get_zones();
                    active_index--;

                    if( active_index < 0 ) {
                        active_index = 0;
                    }
                }
                blink = false;
                stuff_changed = true;

            } else if( action == "CONFIRM" ) {
                zone_data &zone = zones[active_index].get();

                uilist as_m;
                as_m.text = _( "What do you want to change:" );
                as_m.entries.emplace_back( 1, true, '1', _( "Edit name" ) );
                as_m.entries.emplace_back( 2, true, '2', _( "Edit type" ) );
                as_m.entries.emplace_back( 3, zone.get_options().has_options(), '3',
                                           zone.get_type() == zone_type_LOOT_CUSTOM ? _( "Edit filter" ) : _( "Edit options" ) );
                as_m.entries.emplace_back( 4, !zone.get_is_vehicle(), '4', _( "Edit position" ) );
                as_m.entries.emplace_back( 5, !zone.get_is_vehicle(), '5', _( "Move position" ) );
                as_m.query();

                switch( as_m.ret ) {
                    case 1:
                        if( zone.set_name() ) {
                            stuff_changed = true;
                        }
                        break;
                    case 2:
                        if( zone.set_type() ) {
                            stuff_changed = true;
                        }
                        break;
                    case 3:
                        if( zone.get_options().query() ) {
                            stuff_changed = true;
                        }
                        break;
                    case 4: {
                        if( zone.get_is_personal() ) {
                            const std::optional<std::pair<tripoint_rel_ms, tripoint_rel_ms>> pos = query_personal_position();
                            if( pos && ( u.pos_abs() + pos->first != zone.get_start_point() ||
                                         u.pos_abs() + pos->second != zone.get_end_point() ) ) {
                                zone.set_position( { pos->first, pos->second } );
                                stuff_changed = true;

                            }
                        } else {
                            const std::optional<std::pair<tripoint_abs_ms, tripoint_abs_ms>> pos = query_position();
                            if( pos && ( pos->first != zone.get_start_point() ||
                                         pos->second != zone.get_end_point() ) ) {
                                zone.set_position( { pos->first, pos->second } );
                                stuff_changed = true;
                            }
                        }
                        break;
                    }
                    case 5: {
                        on_out_of_scope invalidate_current_ui( [&]() {
                            ui.mark_resize();
                        } );
                        restore_on_out_of_scope show_prev( show );
                        restore_on_out_of_scope zone_start_prev( zone_start );
                        restore_on_out_of_scope zone_end_prev( zone_end );
                        show = false;
                        zone_start = std::nullopt;
                        zone_end = std::nullopt;
                        ui.mark_resize();
                        static_popup message_pop;
                        message_pop.on_top( true );
                        message_pop.message( "%s", _( "Moving zone." ) );
                        const tripoint_bub_ms zone_local_start_point = here.get_bub( zone.get_start_point() );
                        const tripoint_bub_ms zone_local_end_point = here.get_bub( zone.get_end_point() );
                        // local position of the zone center, used to calculate the u.view_offset,
                        // could center the screen to the position it represents
                        tripoint_bub_ms view_center = here.get_bub( zone.get_center_point() );
                        const look_around_result result_local = g->look_around( false, view_center,
                                                                zone_local_start_point, false, false,
                                                                false, true, zone_local_end_point );
                        if( result_local.position ) {
                            const tripoint_abs_ms new_start_point = here.get_abs( *result_local.position );
                            const tripoint_abs_ms new_end_point = zone.get_end_point() - zone.get_start_point() +
                                                                  new_start_point;
                            if( new_start_point == zone.get_start_point() ) {
                                break; // Nothing changed, don't save
                            }
                            if( zone.get_is_personal() ) {
                                const tripoint_rel_ms new_start_point_rl = new_start_point - u.pos_abs();
                                const tripoint_rel_ms new_end_point_rl = new_end_point - u.pos_abs();
                                zone.set_position( std::make_pair( new_start_point_rl, new_end_point_rl ) );
                            } else {
                                zone.set_position( std::make_pair( new_start_point, new_end_point ) );
                            }
                            stuff_changed = true;
                        }
                    }
                    break;
                    default:
                        break;
                }

                blink = false;
            } else if( action == "MOVE_ZONE_DOWN" && zone_cnt > 1 ) {
                if( active_index < zone_cnt - 1 ) {
                    mgr.swap( zones[active_index], zones[active_index + 1] );
                    zones = get_zones();
                    active_index++;
                }
                blink = false;
                stuff_changed = true;

            } else if( action == "MOVE_ZONE_UP" && zone_cnt > 1 ) {
                if( active_index > 0 ) {
                    mgr.swap( zones[active_index], zones[active_index - 1] );
                    zones = get_zones();
                    active_index--;
                }
                blink = false;
                stuff_changed = true;

            } else if( action == "SHOW_ZONE_ON_MAP" ) {
                //show zone position on overmap;
                tripoint_abs_omt player_overmap_position = u.pos_abs_omt();
                tripoint_abs_omt zone_overmap =
                    project_to<coords::omt>( zones[active_index].get().get_center_point() );

                ui::omap::display_zones( player_overmap_position, zone_overmap, active_index );
            } else if( action == "ENABLE_ZONE" ) {
                zones[active_index].get().set_enabled( true );

                stuff_changed = true;

            } else if( action == "DISABLE_ZONE" ) {
                zones[active_index].get().set_enabled( false );

                stuff_changed = true;

            } else if( action == "TOGGLE_ZONE_DISPLAY" ) {
                zones[active_index].get().toggle_display();
                stuff_changed = true;

            } else if( action == "ENABLE_PERSONAL_ZONES" ) {
                bool zones_changed = false;

                for( const auto &i : zones ) {
                    zone_data &zone = i.get();
                    if( zone.get_enabled() ) {
                        continue;
                    }
                    if( zone.get_is_personal() ) {
                        zone.set_enabled( true );
                        zones_changed = true;
                    }
                }

                stuff_changed = zones_changed;
            } else if( action == "DISABLE_PERSONAL_ZONES" ) {
                bool zones_changed = false;

                for( const auto &i : zones ) {
                    zone_data &zone = i.get();
                    if( !zone.get_enabled() ) {
                        continue;
                    }
                    if( zone.get_is_personal() ) {
                        zone.set_enabled( false );
                        zones_changed = true;
                    }
                }

                stuff_changed = zones_changed;
            } else if( action == "ENABLE_VEHICLE_ZONES" ) {
                bool zones_changed = false;

                for( const auto &i : zones ) {
                    zone_data &zone = i.get();
                    if( zone.get_enabled() ) {
                        continue;
                    }
                    if( zone.get_is_vehicle() ) {
                        zone.set_enabled( true );
                        zones_changed = true;
                    }
                }

                stuff_changed = zones_changed;
            } else if( action == "DISABLE_VEHICLE_ZONES" ) {
                bool zones_changed = false;

                for( const auto &i : zones ) {
                    zone_data &zone = i.get();
                    if( !zone.get_enabled() ) {
                        continue;
                    }
                    if( zone.get_is_vehicle() ) {
                        zone.set_enabled( false );
                        zones_changed = true;
                    }
                }

                stuff_changed = zones_changed;
            }
        }
    }
    g->set_zones_manager_open( false );
    ctxt.reset_timeout();
    zone_cb = nullptr;

    if( stuff_changed ) {
        zone_manager &zones = zone_manager::get_manager();
        if( !save ) {
            zones.load_zones( "zmgr-temp" );
        }

        zones.cache_data();
    }

    u.view_offset = stored_view_offset;
}
