#include "a11y_world.h"

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "a11y_input.h"
#include "a11y_body.h"
#include "a11y_outcome.h"
#include "a11y_perception.h"
#include "a11y_runtime.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "display.h"
#include "effect.h"
#include "game.h"
#include "input_context.h"
#include "map.h"
#include "messages.h"
#include "output.h"
#include "options.h"
#include "point.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "vehicle.h"

namespace a11y
{
namespace
{
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_sleep( "sleep" );

struct bodypart_status {
    bodypart_id id;
    int bleeding = 0;
    bool broken = false;
    bool bitten = false;
    bool infected = false;
};

struct monitored_status {
    int hunger = 0;
    int thirst = 0;
    int sleepiness = 0;
    int pain = 0;
    int stamina = 0;
    // R10.4: dimensions the game never writes to the message log, so the
    // status monitor is their only spoken channel. Temperature is tracked
    // on both sides of comfortable because the sidebar word is two-sided;
    // morale uses the same total the morale screen prints as a number.
    int temp_cold = 0;
    int temp_heat = 0;
    int morale_risk = 0;
    int weariness = 0;
    std::string hunger_text;
    std::string thirst_text;
    std::string sleepiness_text;
    std::string pain_text;
    std::string stamina_text;
    std::string temp_text;
    std::string morale_text;
    std::string weariness_text;
    std::vector<bodypart_status> bodyparts;
};

std::optional<monitored_status> previous_status;

// R10.4 alert bookkeeping for this session only. The cooldown stops a
// scalar level that flaps around a threshold from being re-announced on
// every crossing; a strictly worse level, or reaching the key's urgent
// level, always bypasses it. The history is what the world snapshot
// publishes as "Recent status alerts"; both are cleared by
// reset_status_monitor(), which replace_game_session() and the lifecycle
// synchronisation already call on load / character change (R08 epoch).
struct status_alert_record {
    std::string key;
    std::string text;
    time_point when = calendar::turn_zero;
};
std::vector<status_alert_record> status_alert_history;
std::map<std::string, std::pair<int, time_point>> status_alert_cooldown;

bool status_alert_allowed( const std::string &key, const int level, const int urgent_level )
{
    if( level >= urgent_level ) {
        return true;
    }
    const auto found = status_alert_cooldown.find( key );
    if( found == status_alert_cooldown.end() ) {
        return true;
    }
    if( level > found->second.first ) {
        return true;
    }
    return calendar::turn - found->second.second >= 30_minutes;
}

void record_status_alert( const std::string &key, const int level, const std::string &text )
{
    status_alert_cooldown[key] = { level, calendar::turn };
    status_alert_history.push_back( { key, text, calendar::turn } );
    if( status_alert_history.size() > 20 ) {
        status_alert_history.erase( status_alert_history.begin(),
                                    status_alert_history.begin() +
                                    ( status_alert_history.size() - 20 ) );
    }
}

int hunger_severity( const std::string &text )
{
    if( text == _( "Hungry" ) ) { return 1; }
    if( text == _( "Near starving" ) ) { return 2; }
    if( text == _( "Starving!" ) ) { return 3; }
    if( text == _( "Famished" ) ) { return 4; }
    return 0;
}

int thirst_severity( const int thirst )
{
    if( thirst > 520 ) { return 4; }
    if( thirst > 240 ) { return 3; }
    if( thirst > 80 ) { return 2; }
    if( thirst > 40 ) { return 1; }
    return 0;
}

int sleepiness_severity( const int sleepiness )
{
    if( sleepiness >= static_cast<int>( sleepiness_levels::EXHAUSTED ) ) { return 3; }
    if( sleepiness >= static_cast<int>( sleepiness_levels::DEAD_TIRED ) ) { return 2; }
    if( sleepiness >= static_cast<int>( sleepiness_levels::TIRED ) ) { return 1; }
    return 0;
}

monitored_status current_status()
{
    const avatar &viewer = get_avatar();
    monitored_status result;
    result.hunger_text = display::hunger_text_color( viewer ).first;
    result.hunger = hunger_severity( result.hunger_text );
    result.thirst_text = display::thirst_text_color( viewer ).first;
    result.thirst = thirst_severity( viewer.get_instant_thirst() );
    result.sleepiness_text = display::sleepiness_text_color( viewer ).first;
    result.sleepiness = sleepiness_severity( viewer.get_sleepiness() );
    result.temp_text = display::temp_text_color( viewer ).first;
    if( result.temp_text == _( "Freezing!" ) ) { result.temp_cold = 3; }
    if( result.temp_text == _( "Very cold!" ) ) { result.temp_cold = 2; }
    if( result.temp_text == _( "Chilly" ) ) { result.temp_cold = 1; }
    if( result.temp_text == _( "Scorching!" ) ) { result.temp_heat = 3; }
    if( result.temp_text == _( "Very hot!" ) ) { result.temp_heat = 2; }
    if( result.temp_text == _( "Warm" ) ) { result.temp_heat = 1; }
    const int morale = viewer.get_morale_level();
    result.morale_risk = morale <= -100 ? 3 : morale <= -50 ? 2 : morale < 0 ? 1 : 0;
    result.morale_text = string_format( _( "Morale %d" ), morale );
    result.weariness = static_cast<int>( viewer.weariness_level() );
    result.weariness_text = display::weariness_text_color( viewer ).first;
    result.pain_text = display::pain_text_color( viewer ).first;
    const int perceived_pain = viewer.get_perceived_pain();
    result.pain = perceived_pain > 70 ? 7 : perceived_pain > 60 ? 6 :
                  perceived_pain > 50 ? 5 : perceived_pain > 40 ? 4 :
                  perceived_pain > 30 ? 3 : perceived_pain > 20 ? 2 :
                  perceived_pain > 10 ? 1 : 0;
    if( viewer.get_stamina_max() > 0 ) {
        const int stamina_percent = 100 * viewer.get_stamina() / viewer.get_stamina_max();
        result.stamina = stamina_percent <= 10 ? 3 : stamina_percent <= 25 ? 2 :
                         stamina_percent <= 50 ? 1 : 0;
        result.stamina_text = result.stamina >= 3 ? _( "Critically low" ) :
                              result.stamina >= 2 ? _( "Low" ) : _( "No current warning" );
    } else {
        result.stamina_text = _( "No current warning" );
    }
    for( const bodypart_id &part : viewer.get_all_body_parts( get_body_part_flags::only_main ) ) {
        result.bodyparts.push_back( { part, viewer.get_effect_int( effect_bleed, part ),
                                      viewer.is_limb_broken( part ),
                                      viewer.has_effect( effect_bite, part.id() ),
                                      viewer.has_effect( effect_infected, part.id() ) } );
    }
    return result;
}

void add_transition( std::vector<std::string> &changes, const std::string &key,
                     const std::string &label,
                     const std::string &value, const int old_level, const int new_level,
                     const int threshold, const int urgent_level, bool &urgent )
{
    if( new_level > old_level && new_level >= threshold ) {
        if( !status_alert_allowed( key, new_level, urgent_level ) ) {
            return;
        }
        const std::string text = string_format( _( "%1$s is worsening: %2$s" ), label, value );
        changes.push_back( text );
        record_status_alert( key, new_level, text );
        urgent = urgent || new_level >= urgent_level;
    } else if( old_level > new_level && old_level >= threshold ) {
        if( !status_alert_allowed( key, new_level, urgent_level ) ) {
            return;
        }
        const std::string detail = value.empty() ? _( "No current warning" ) : value;
        const std::string text = string_format( _( "%1$s is improving: %2$s" ), label, detail );
        changes.push_back( text );
        record_status_alert( key, new_level, text );
    }
}

semantic_node document( std::string id, std::string label, std::string detail )
{
    semantic_node node;
    node.id = std::move( id );
    node.label = std::move( label );
    node.detail = remove_color_tags( detail );
    node.role = "document";
    return node;
}

semantic_node button( std::string id, std::string label, std::string detail )
{
    semantic_node node;
    node.id = std::move( id );
    node.label = std::move( label );
    node.detail = remove_color_tags( detail );
    node.role = "button";
    node.actions = { "focus", "activate" };
    return node;
}

std::string relative_name( const int dx, const int dy )
{
    if( dx == 0 && dy == 0 ) { return _( "Current tile" ); }
    std::string result;
    if( dy != 0 ) {
        result += string_format( dy < 0 ? _( "North %d" ) : _( "South %d" ), std::abs( dy ) );
    }
    if( dx != 0 ) {
        if( !result.empty() ) { result += ", "; }
        result += string_format( dx < 0 ? _( "West %d" ) : _( "East %d" ), std::abs( dx ) );
    }
    return result;
}

// Position relative to the avatar: relative_name for the horizontal part,
// plus the level difference when the tile is on another level.
std::string relative_position_name( const int dx, const int dy, const int dz )
{
    std::string result = relative_name( dx, dy );
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
}

bool has_noteworthy_information( const tile_observation &tile )
{
    return !tile.heard_marker.empty() ||
           ( tile.source == observation_source::visible &&
             ( tile.impassable || !tile.fields.empty() || !tile.known_traps.empty() ||
               !tile.furniture.empty() || !tile.seen_creatures.empty() || !tile.seen_items.empty() ||
               !tile.special_perceptions.empty() || !tile.vehicle.empty() || tile.route_feature ) ) ||
           ( tile.source == observation_source::remembered &&
             ( !tile.furniture.empty() || !tile.special_perceptions.empty() || tile.route_feature ) ) ||
           ( tile.source == observation_source::unknown && !tile.special_perceptions.empty() );
}

int noteworthy_priority( const tile_observation &tile )
{
    if( !tile.known_traps.empty() ) { return 0; }
    if( !tile.seen_creatures.empty() ) { return 1; }
    if( !tile.special_perceptions.empty() ) { return 2; }
    if( !tile.fields.empty() ) { return 3; }
    if( tile.impassable ) { return 4; }
    if( tile.route_feature ) { return 5; }
    if( !tile.heard_marker.empty() ) { return 6; }
    if( !tile.vehicle.empty() ) { return 7; }
    if( !tile.seen_items.empty() ) { return 8; }
    if( !tile.furniture.empty() ) { return 9; }
    return 10;
}

std::string noteworthy_category( const tile_observation &tile )
{
    if( !tile.known_traps.empty() ) { return _( "Known trap" ); }
    if( !tile.seen_creatures.empty() ) { return _( "Visible creature" ); }
    if( !tile.special_perceptions.empty() ) { return _( "Special perception" ); }
    if( !tile.fields.empty() ) { return _( "Visible field" ); }
    if( tile.impassable ) { return _( "Visible obstacle" ); }
    if( tile.route_feature ) { return _( "Known route feature" ); }
    if( !tile.heard_marker.empty() ) { return _( "Approximate sound marker" ); }
    if( !tile.vehicle.empty() ) { return _( "Visible vehicle" ); }
    if( !tile.seen_items.empty() ) { return _( "Visible items" ); }
    if( !tile.furniture.empty() ) { return _( "Known furniture" ); }
    return _( "Noteworthy information" );
}

std::string player_status()
{
    const avatar &viewer = get_avatar();
    std::string result = display::time_string( viewer ) + ". " +
                         string_format( _( "Hunger: %s" ), display::hunger_text_color( viewer ).first ) + ". " +
                         string_format( _( "Thirst: %s" ), display::thirst_text_color( viewer ).first ) + ". " +
                         string_format( _( "Sleepiness: %s" ), display::sleepiness_text_color( viewer ).first ) + ". " +
                         string_format( _( "Pain: %s" ), display::pain_text_color( viewer ).first );
    const int stamina = viewer.get_stamina_max() > 0 ? 100 * viewer.get_stamina() / viewer.get_stamina_max() : 0;
    result += ". " + string_format( _( "Stamina: %d%%" ), stamina );
    // R10.1: the remaining sidebar facts. Temperature and weariness are
    // published by the game as words (their colors only repeat severity),
    // morale as the same total the morale screen prints, and effect names
    // are exactly the ones the character sheet's EFFECTS tab lists. No
    // exact hit points or other unpublished numbers are added here.
    result += ". " + string_format( _( "Temperature: %s" ),
                                    display::temp_text_color( viewer ).first );
    result += ". " + string_format( _( "Morale: %d" ), viewer.get_morale_level() );
    result += ". " + string_format( _( "Weariness: %s" ),
                                    display::weariness_text_color( viewer ).first );
    std::string effect_names;
    for( const auto &eff_ref : viewer.get_effects() ) {
        const std::string name = eff_ref.get().disp_name();
        if( name.empty() ||
            effect_names.find( name ) != std::string::npos ) {
            continue;
        }
        if( !effect_names.empty() ) {
            effect_names += ", ";
        }
        effect_names += name;
    }
    result += ". " + string_format( _( "Effects: %s" ),
                                    effect_names.empty() ? _( "none" ) : effect_names );
    // R11.1: safe mode is a game-side state (the global game's safe_mode,
    // the same value avatar_action::move checks before a melee attack);
    // this file otherwise reaches the game only through get_avatar() /
    // get_map(), so read it from g exactly as avatar_action.cpp does. An
    // unrecognised value is omitted rather than guessed at.
    if( g ) {
        std::string safe_mode_name;
        switch( g->safe_mode ) {
            case SAFE_MODE_ON:
                safe_mode_name = _( "On" );
                break;
            case SAFE_MODE_OFF:
                safe_mode_name = _( "Off" );
                break;
            case SAFE_MODE_STOP:
                safe_mode_name = _( "Stop" );
                break;
            default:
                break;
        }
        if( !safe_mode_name.empty() ) {
            result += ". " + string_format( _( "Safe mode: %s" ), safe_mode_name );
        }
    }
    for( const bodypart_id &part : viewer.get_all_body_parts( get_body_part_flags::only_main ) ) {
        result += ". " + body_part_name( part ) + ": " + limb_health_description( viewer, part );
    }
    if( vehicle *veh = display::vehicle_driven( viewer ) ) {
        // R15.4: one published source for the driving state, extending
        // the azimuth/cruise text the sidebar itself prints. The mode
        // distinguishes sitting at the controls from remote control
        // (vehicle_driven returns the remotely controlled vehicle
        // first); fuel and battery are the sidebar's own percentages;
        // engine strain is the verbal equivalent of the color the
        // cruise text is drawn in, from the same strain value and the
        // same bands display.cpp uses (none up to 0, then low, moderate
        // and high at 0.2 / 0.4).
        result += ". " + string_format( _( "Vehicle: %s" ), veh->name );
        result += ". " + std::string( g && g->remoteveh() == veh
                                      ? _( "Remote control" ) : _( "Driving" ) );
        result += ". " + display::vehicle_azimuth_text( viewer );
        result += ". " + display::vehicle_cruise_text_color( viewer ).first;
        const std::string fuel_text = display::vehicle_fuel_percent_text_color( viewer ).first;
        if( !fuel_text.empty() ) {
            result += ". " + string_format( _( "Fuel: %s" ), fuel_text );
        }
        const std::string battery_text = display::vehicle_battery_percent_text_color(
                                             viewer ).first;
        if( !battery_text.empty() ) {
            result += ". " + string_format( _( "Battery: %s" ), battery_text );
        }
        const float strain = veh->strain( get_map() );
        result += ". " + string_format( _( "Engine strain: %s" ),
                                        strain <= 0.0f ? _( "none" ) :
                                        strain <= 0.2f ? _( "low" ) :
                                        strain <= 0.4f ? _( "moderate" ) : _( "high" ) );
    }
    return remove_color_tags( result );
}
} // namespace

void reset_status_monitor()
{
    previous_status.reset();
    status_alert_history.clear();
    status_alert_cooldown.clear();
}

void announce_status_changes()
{
    if( !enabled() || suspended() ) {
        reset_status_monitor();
        return;
    }
    const monitored_status current = current_status();
    if( !previous_status ) {
        previous_status = current;
        return;
    }

    std::vector<std::string> changes;
    bool urgent = false;
    add_transition( changes, "hunger", _( "Hunger" ), current.hunger_text, previous_status->hunger,
                    current.hunger, 1, 3, urgent );
    add_transition( changes, "thirst", _( "Thirst" ), current.thirst_text, previous_status->thirst,
                    current.thirst, 1, 3, urgent );
    add_transition( changes, "sleepiness", _( "Sleepiness" ), current.sleepiness_text,
                    previous_status->sleepiness, current.sleepiness, 1, 3, urgent );
    add_transition( changes, "pain", _( "Pain" ), current.pain_text, previous_status->pain,
                    current.pain, 2, 7, urgent );
    add_transition( changes, "stamina", _( "Stamina" ), current.stamina_text, previous_status->stamina,
                    current.stamina, 2, 3, urgent );
    // Temperature, morale and weariness have no game-log channel at all:
    // without these transitions they would only ever change silently in
    // the sidebar (temperature and weariness as words whose color carries
    // the severity, morale as an ASCII face). Thresholds: cold/heat level
    // 2 ("Very cold!"/"Very hot!"), morale risk level 1 (total below 0,
    // the same total the morale screen prints), weariness level 2.
    add_transition( changes, "temp_cold", _( "Temperature (cold)" ), current.temp_text,
                    previous_status->temp_cold, current.temp_cold, 2, 3, urgent );
    add_transition( changes, "temp_heat", _( "Temperature (heat)" ), current.temp_text,
                    previous_status->temp_heat, current.temp_heat, 2, 3, urgent );
    add_transition( changes, "morale", _( "Morale" ), current.morale_text,
                    previous_status->morale_risk, current.morale_risk, 1, 3, urgent );
    add_transition( changes, "weariness", _( "Weariness" ), current.weariness_text,
                    previous_status->weariness, current.weariness, 2, 99, urgent );
    for( const bodypart_status &part : current.bodyparts ) {
        const auto old = std::find_if( previous_status->bodyparts.begin(), previous_status->bodyparts.end(),
        [&part]( const bodypart_status &entry ) {
            return entry.id == part.id;
        } );
        if( old == previous_status->bodyparts.end() ) {
            continue;
        }
        const std::string name = body_part_name( part.id );
        if( part.bleeding > old->bleeding ) {
            const std::string part_alert = string_format( _( "Bleeding is worsening at %s." ), name );
            changes.push_back( part_alert );
            record_status_alert( "limb:" + part.id.id().str(), 1, part_alert );
            urgent = urgent || part.bleeding >= 2;
        } else if( old->bleeding > 0 && part.bleeding == 0 ) {
            const std::string part_alert = string_format( _( "Bleeding stopped at %s." ), name );
            changes.push_back( part_alert );
            record_status_alert( "limb:" + part.id.id().str(), 1, part_alert );
        }
        if( !old->broken && part.broken ) {
            const std::string part_alert = string_format( _( "%s is broken." ), name );
            changes.push_back( part_alert );
            record_status_alert( "limb:" + part.id.id().str(), 1, part_alert );
            urgent = true;
        } else if( old->broken && !part.broken ) {
            const std::string part_alert = string_format( _( "%s is no longer broken." ), name );
            changes.push_back( part_alert );
            record_status_alert( "limb:" + part.id.id().str(), 1, part_alert );
        }
        if( !old->bitten && part.bitten ) {
            const std::string part_alert = string_format( _( "A bite was detected on %s." ), name );
            changes.push_back( part_alert );
            record_status_alert( "limb:" + part.id.id().str(), 1, part_alert );
        }
        if( !old->infected && part.infected ) {
            const std::string part_alert = string_format( _( "An infection was detected in %s." ), name );
            changes.push_back( part_alert );
            record_status_alert( "limb:" + part.id.id().str(), 1, part_alert );
            urgent = true;
        } else if( old->infected && !part.infected ) {
            const std::string part_alert = string_format( _( "The infection cleared in %s." ), name );
            changes.push_back( part_alert );
            record_status_alert( "limb:" + part.id.id().str(), 1, part_alert );
        }
    }
    previous_status = current;
    if( !changes.empty() ) {
        std::string message;
        for( const std::string &change : changes ) {
            if( !message.empty() ) { message += ". "; }
            message += change;
        }
        announce_game_message( message, urgent );
    }
}

semantic_snapshot world_snapshot( const input_context &context, const int scan_radius,
                                  const int scan_depth, const world_scan_filter filter,
                                  const std::optional<point> camera_offset,
                                  const std::optional<tripoint_rel_ms> explore_cursor )
{
    semantic_snapshot result;
    result.title = _( "World" );
    result.category = context.category_id();
    result.summary = remove_color_tags( display::move_mode_letter_color( get_avatar() ).first );
    const bool explore_active = explore_cursor.has_value();
    result.focus = explore_active ? "explore:cursor" : "world:here";
    const tripoint_rel_ms offset = camera_offset ? tripoint_rel_ms( camera_offset->x,
                                   camera_offset->y, 0 ) : tripoint_rel_ms( 0, 0, 0 );
    const tripoint_bub_ms avatar_origin = get_avatar().pos_bub( get_map() );
    const tripoint_bub_ms origin = avatar_origin + offset;
    // The explore cursor is a reading position relative to the avatar. While
    // it is active the nearby scan is centered on the cursor; on another
    // level only map memory may be read, exactly like the vertical scan.
    const tripoint_bub_ms scan_origin = explore_active ? avatar_origin + *explore_cursor : origin;
    const bool cursor_on_other_level = explore_active && explore_cursor->z() != 0;
    const auto observe_scan_tile = [&]( const tripoint_bub_ms & position ) {
        return cursor_on_other_level ? observe_memory_tile( position ) : observe_tile( position );
    };
    semantic_node here = document( "world:here", _( "Current tile" ), describe_tile( observe_tile( origin ) ) );
    if( camera_offset ) {
        here.label = _( "Death camera center" );
        result.summary = _( "Death camera is active. Use Look to inspect the view, move or center the camera, "
                            "or accept your fate to finish watching." );
    }
    here.value = here.detail;
    result.nodes.push_back( std::move( here ) );
    for( int distance = 1; distance <= scan_depth; ++distance ) {
        for( const int dz : { distance, -distance } ) {
            const tile_observation vertical = observe_memory_tile(
                    scan_origin + tripoint_rel_ms( 0, 0, dz ) );
            std::string id;
            if( distance == 1 ) {
                id = dz > 0 ? "world:above" : "world:below";
            } else {
                id = "world:level:" + std::to_string( dz );
            }
            const std::string label = string_format(
                                          dz > 0 ? _( "Level above %d" ) : _( "Level below %d" ), distance );
            semantic_node node = document( id, label, describe_tile( vertical ) );
            node.value = vertical.source == observation_source::unknown ? _( "Unknown" ) :
                         vertical.terrain;
            if( vertical.source == observation_source::remembered ) {
                if( !vertical.furniture.empty() ) { node.value += ". " + vertical.furniture; }
                node.value += ". " + std::string( _( "Remembered" ) );
            }
            result.nodes.push_back( std::move( node ) );
        }
    }
    result.nodes.push_back( document( "world:status", _( "Character status" ), player_status() ) );
    if( !status_alert_history.empty() ) {
        // Session alert history (R10.4): the last status transitions the
        // monitor announced, newest last, with approximate ages. This is a
        // reading node only; it never re-announces anything by itself.
        std::string history;
        const std::size_t first = status_alert_history.size() > 10 ?
                                  status_alert_history.size() - 10 : 0;
        for( std::size_t i = first; i < status_alert_history.size(); ++i ) {
            if( !history.empty() ) {
                history += " ";
            }
            history += status_alert_history[i].text + " (" +
                       string_format( _( "%s ago" ),
                                      to_string_approx( calendar::turn - status_alert_history[i].when ) ) + ").";
        }
        result.nodes.push_back( document( "world:alert_history", _( "Recent status alerts" ), history ) );
    }
    const action_outcome &outcome = last_action_outcome();
    if( outcome.kind != action_outcome_kind::none ) {
        result.nodes.push_back( document( "world:last_outcome", _( "Last action outcome" ),
                                          describe_action_outcome( outcome ) ) );
    }
    const avatar &viewer = get_avatar();
    if( viewer.has_effect( effect_sleep ) ) {
        semantic_node activity_node = document( "world:activity", _( "Current activity" ),
                                               _( "Sleeping. The game will continue until you wake up." ) );
        activity_node.role = "status";
        activity_node.value = _( "Sleeping. The game will continue until you wake up." );
        result.nodes.push_back( std::move( activity_node ) );
    } else if( viewer.activity ) {
        const player_activity &current = viewer.activity;
        const std::optional<std::string> progress = current.get_progress_message( viewer );
        std::string summary = progress.value_or( current.get_verb().translated() );
        if( summary.empty() ) { summary = _( "Ongoing activity." ); }
        summary = remove_color_tags( summary );
        std::string description = summary;
        if( current.is_interruptible_with_kb() ) {
            description += ". ";
            description += _( "Pause can request interruption; the game will ask before canceling." );
        } else {
            description += ". ";
            description += _( "Pause cannot interrupt this activity right now." );
        }
        semantic_node activity_node = document( "world:activity", _( "Current activity" ), description );
        activity_node.role = "status";
        activity_node.value = summary;
        result.nodes.push_back( std::move( activity_node ) );
    }
    semantic_node radius;
    radius.id = "world:radius";
    radius.label = _( "Scan radius, from 1 to 16" );
    radius.value = std::to_string( scan_radius );
    radius.role = "number";
    radius.actions = { "focus", "set_text" };
    result.nodes.push_back( std::move( radius ) );
    semantic_node depth;
    depth.id = "world:depth";
    depth.label = _( "Vertical memory scan depth, from 0 to 10" );
    depth.value = std::to_string( scan_depth );
    depth.detail = _( "This scan reads only remembered terrain and furniture on other levels. "
                      "Reading the map does not advance game time." );
    depth.role = "number";
    depth.actions = { "focus", "set_text" };
    result.nodes.push_back( std::move( depth ) );
    const std::vector<std::pair<world_scan_filter, std::string>> filters = {
        { world_scan_filter::all, "world:filter:all" },
        { world_scan_filter::known, "world:filter:known" },
        { world_scan_filter::noteworthy, "world:filter:noteworthy" },
        { world_scan_filter::heard, "world:filter:heard" }
    };
    const std::vector<std::string> filter_labels = {
        _( "All nearby tiles" ), _( "Known tiles only" ),
        _( "Noteworthy tiles only" ), _( "Heard sound markers only" )
    };
    const std::vector<std::string> filter_details = {
        _( "Show all nearby tiles and label unexplored tiles as unknown." ),
        _( "Show visible or remembered tiles and tiles with a sound marker." ),
        _( "Show visible fields, traps, creatures, special perceptions, items, vehicles, obstacles, known doors and stairs, remembered furniture, and heard markers." ),
        _( "Show only the game's heard sound markers. A marker is approximate and does not reveal a creature's true position." )
    };
    for( std::size_t i = 0; i < filters.size(); ++i ) {
        semantic_node node;
        node.id = filters[i].second;
        node.role = "radio";
        node.label = filter_labels[i];
        node.detail = filter_details[i];
        node.selected = filter == filters[i].first;
        node.actions = { "focus", "activate" };
        result.nodes.push_back( std::move( node ) );
    }
    std::string recent;
    for( const auto &message : Messages::recent_messages( 12 ) ) {
        recent += message.first + ": " + message.second + ". ";
    }
    result.nodes.push_back( document( "world:messages", _( "Recent messages" ), recent ) );
    // Nearby observations are read-only. A tile selection never walks, attacks
    // or constructs a route. Direction commands below use the normal engine.
    struct nearby_tile_node {
        int priority = 0;
        int distance = 0;
        semantic_node node;
    };
    std::vector<nearby_tile_node> nearby_nodes;
    for( int distance = 1; distance <= scan_radius; ++distance ) {
        for( int dy = -distance; dy <= distance; ++dy ) {
            for( int dx = -distance; dx <= distance; ++dx ) {
                if( std::max( std::abs( dx ), std::abs( dy ) ) != distance ) { continue; }
                const auto tile = observe_scan_tile( scan_origin + tripoint_rel_ms( dx, dy, 0 ) );
                const bool include = filter == world_scan_filter::all ||
                                     ( filter == world_scan_filter::known &&
                                       ( tile.source != observation_source::unknown ||
                                         !tile.special_perceptions.empty() || !tile.heard_marker.empty() ) ) ||
                                     ( filter == world_scan_filter::noteworthy && has_noteworthy_information( tile ) ) ||
                                     ( filter == world_scan_filter::heard && !tile.heard_marker.empty() );
                if( !include ) { continue; }
                const std::string direction = relative_name( dx, dy );
                semantic_node node = document( "tile:" + std::to_string( dx ) + ':' + std::to_string( dy ),
                                               direction, describe_tile( tile ) );
                if( filter == world_scan_filter::noteworthy ) {
                    node.label = string_format( _( "%1$s: %2$s" ), noteworthy_category( tile ), direction );
                }
                node.value = tile.source == observation_source::unknown ? _( "Unknown" ) : tile.terrain;
                if( tile.source == observation_source::remembered ) {
                    node.value += ". " + std::string( _( "Remembered" ) );
                }
                nearby_nodes.push_back( { filter == world_scan_filter::noteworthy ? noteworthy_priority( tile ) : 0,
                                          distance, std::move( node ) } );
            }
        }
    }
    std::stable_sort( nearby_nodes.begin(), nearby_nodes.end(), []( const nearby_tile_node &lhs,
    const nearby_tile_node &rhs ) {
        if( lhs.priority != rhs.priority ) { return lhs.priority < rhs.priority; }
        return lhs.distance < rhs.distance;
    } );
    for( std::size_t i = 0; i < nearby_nodes.size(); ++i ) {
        nearby_nodes[i].node.position = i + 1;
        nearby_nodes[i].node.total = nearby_nodes.size();
        result.nodes.push_back( std::move( nearby_nodes[i].node ) );
    }
    // Explore mode: a read-only cursor the reader can move without moving the
    // avatar. Every explore intent is handled locally by the reader plumbing
    // in handle_action; none of these nodes ever reach the game engine, so
    // exploring costs no moves and no game time.
    if( !explore_active ) {
        result.nodes.push_back( button( "explore:enter", _( "Explore mode" ),
                                        _( "Move a read-only cursor away from your character to inspect distant tiles. "
                                           "Exploring never moves your character and does not use game time." ) ) );
    } else {
        const tripoint_rel_ms cursor = *explore_cursor;
        const tile_observation cursor_tile = observe_scan_tile( scan_origin );
        std::string cursor_detail = describe_tile( cursor_tile );
        cursor_detail += ". " + string_format( _( "Relative to you: %s" ),
                                               relative_position_name( cursor.x(), cursor.y(), cursor.z() ) );
        semantic_node cursor_node = document( "explore:cursor", _( "Explore cursor" ), cursor_detail );
        cursor_node.value = cursor_tile.source == observation_source::unknown ? _( "Unknown" ) :
                            cursor_tile.terrain;
        if( cursor_tile.source == observation_source::remembered ) {
            cursor_node.value += ". " + std::string( _( "Remembered" ) );
        }
        result.nodes.push_back( std::move( cursor_node ) );
        const std::string cursor_move_detail =
            _( "Moves only the explore cursor. Your character does not move and no game time passes." );
        for( int dy = -1; dy <= 1; ++dy ) {
            for( int dx = -1; dx <= 1; ++dx ) {
                if( dx == 0 && dy == 0 ) { continue; }
                result.nodes.push_back( button(
                                            "explore:dir:" + std::to_string( dx ) + ':' + std::to_string( dy ),
                                            string_format( _( "Move explore cursor: %s" ), relative_name( dx, dy ) ),
                                            cursor_move_detail ) );
            }
        }
        result.nodes.push_back( button( "explore:up", _( "Move explore cursor up one level" ),
                                        cursor_move_detail ) );
        result.nodes.push_back( button( "explore:down", _( "Move explore cursor down one level" ),
                                        cursor_move_detail ) );
        result.nodes.push_back( button( "explore:recenter", _( "Recenter explore cursor" ),
                                        _( "Returns the explore cursor to your character's tile." ) ) );
        result.nodes.push_back( button( "explore:exit", _( "Exit explore mode" ),
                                        _( "Returns the scan to your character." ) ) );
        // Jump targets are computed around the avatar on the avatar's own
        // level, independently of where the cursor currently is, so a jump
        // always lands on a tile noteworthy from the character's position.
        struct jump_candidate {
            int priority = 0;
            int distance = 0;
            semantic_node node;
        };
        std::vector<jump_candidate> jumps;
        for( int distance = 1; distance <= scan_radius; ++distance ) {
            for( int dy = -distance; dy <= distance; ++dy ) {
                for( int dx = -distance; dx <= distance; ++dx ) {
                    if( std::max( std::abs( dx ), std::abs( dy ) ) != distance ) { continue; }
                    const auto tile = observe_tile( avatar_origin + tripoint_rel_ms( dx, dy, 0 ) );
                    if( !has_noteworthy_information( tile ) ) { continue; }
                    semantic_node node = button(
                                             "explore:jump:" + std::to_string( dx ) + ':' + std::to_string( dy ),
                                             string_format( _( "%1$s: %2$s" ), noteworthy_category( tile ),
                                                     relative_name( dx, dy ) ),
                                             describe_tile( tile ) );
                    jumps.push_back( { noteworthy_priority( tile ), distance, std::move( node ) } );
                }
            }
        }
        std::stable_sort( jumps.begin(), jumps.end(), []( const jump_candidate & lhs,
        const jump_candidate & rhs ) {
            if( lhs.priority != rhs.priority ) { return lhs.priority < rhs.priority; }
            return lhs.distance < rhs.distance;
        } );
        if( jumps.size() > 24 ) { jumps.resize( 24 ); }
        for( auto &jump : jumps ) {
            result.nodes.push_back( std::move( jump.node ) );
        }
    }
    // Fast-movement swipes send ordinary walking actions to the engine, so
    // they are only permitted while the avatar is actually on foot (R00.2).
    // In a vehicle the same directional actions steer or operate the vehicle,
    // in remote control they drive another vehicle, in the death camera they
    // move the camera, and during an activity or automatic travel a walking
    // action would be queued against the wrong mode. An active explore
    // cursor also disables swipes: the scan is centered away from the
    // avatar, so a walking gesture would act on tiles the reader is not
    // describing as the avatar's surroundings. Remote control has two
    // engine forms, both reached by those same movement actions:
    // "remote_controlling" drives a radio-controlled car (handle_action
    // rcdrive) and "remote_controlling_vehicle" backs game::remoteveh() for
    // a full vehicle; either one disables walking gestures. Any doubt stays
    // false; the Java side additionally requires the user's fast-movement
    // preference and its own editor/modal/practice checks.
    result.swipe_movement_allowed = !explore_active
                                    && !camera_offset.has_value()
                                    && !viewer.is_dead_state()
                                    && !viewer.in_vehicle
                                    && !viewer.controlling_vehicle
                                    && !viewer.is_driving()
                                    && viewer.maybe_get_value( "remote_controlling" ) == nullptr
                                    && viewer.maybe_get_value( "remote_controlling_vehicle" ) == nullptr
                                    && !viewer.activity
                                    && !viewer.has_destination()
                                    && !viewer.has_destination_activity()
                                    && !viewer.has_effect( effect_sleep );
    auto commands = command_snapshot( context, result.title );
    for( auto &node : commands.nodes ) { result.nodes.push_back( std::move( node ) ); }
    return result;
}

semantic_snapshot activity_snapshot( const input_context &context )
{
    const avatar &viewer = get_avatar();
    semantic_snapshot result;
    result.title = _( "Activity in progress" );
    result.category = context.category_id();
    result.focus = "activity:current";
    semantic_node current;
    current.id = "activity:current";
    current.role = "document";
    current.actions = { "focus" };
    semantic_node interruption;
    interruption.id = "activity:interruptibility";
    interruption.label = _( "Interruptibility" );
    interruption.role = "document";
    interruption.actions = { "focus" };

    if( viewer.activity ) {
        const player_activity &activity = viewer.activity;
        current.label = remove_color_tags( activity.get_verb().translated() );
        if( current.label.empty() ) { current.label = _( "Current activity" ); }
        const std::optional<std::string> progress = activity.get_progress_message( viewer );
        current.value = remove_color_tags( progress.value_or( current.label ) );
        if( activity.is_interruptible_with_kb() ) {
            interruption.value = _( "Can request interruption with Pause" );
            interruption.detail = _( "The game will ask before canceling." );
        } else if( viewer.has_destination() ) {
            interruption.value = _( "Can request cancellation with Pause" );
            interruption.detail = _( "The game will ask before canceling." );
        } else {
            interruption.value = _( "Cannot interrupt with Pause right now" );
        }
    } else {
        current.label = _( "Automatic travel" );
        current.value = _( "Travel is in progress." );
        interruption.value = _( "Can request cancellation with Pause" );
        interruption.detail = _( "The game will ask before canceling." );
    }
    current.detail = interruption.value;
    if( !interruption.detail.empty() ) { current.detail += ". " + interruption.detail; }
    // R10.3: coarse progress only. The engine itself publishes a percent
    // for move-based activities (get_progress_message), computed from
    // moves_total/moves_left; quantising that same percent down to whole
    // tens means the snapshot content -- and therefore the reader's
    // revision and any re-reading -- changes at most ten times over a
    // long activity instead of on every tick. The word "about" keeps the
    // published value honest about the quantisation.
    std::optional<semantic_node> progress_node;
    if( viewer.activity && viewer.activity.moves_total > 0 ) {
        const int percent = std::max( 0, std::min( 100,
                                  ( viewer.activity.moves_total - viewer.activity.moves_left ) * 100 /
                                  viewer.activity.moves_total ) );
        const int coarse = percent / 10 * 10;
        progress_node = document( "activity:progress", _( "Progress" ),
                                  string_format( _( "About %d%% complete." ), coarse ) );
        progress_node->value = std::to_string( coarse );
    }
    if( get_option<float>( "TURN_DURATION" ) > 0.005f ) {
        const std::string realtime_warning = _( "Real-time turns are enabled. The world may continue to update while you "
                                                "listen to this activity screen." );
        result.summary = realtime_warning;
        current.detail += ". " + result.summary;
    }
    result.nodes.push_back( std::move( current ) );
    result.nodes.push_back( std::move( interruption ) );
    if( progress_node ) {
        result.nodes.push_back( std::move( *progress_node ) );
    }

    const std::vector<std::string> supported_actions = {
        "pause", "zoom_in", "zoom_out", "player_data", "messages", "help"
    };
    auto commands = command_snapshot( context, result.title );
    for( semantic_node &node : commands.nodes ) {
        if( std::find( supported_actions.begin(), supported_actions.end(), node.id ) == supported_actions.end() ) {
            continue;
        }
        if( node.id == "pause" && viewer.activity && !viewer.activity.is_interruptible_with_kb() &&
            !viewer.has_destination() ) {
            continue;
        }
        result.nodes.push_back( std::move( node ) );
    }
    for( std::size_t i = 0; i < result.nodes.size(); ++i ) {
        result.nodes[i].position = i + 1;
        result.nodes[i].total = result.nodes.size();
    }
    return result;
}
} // namespace a11y
