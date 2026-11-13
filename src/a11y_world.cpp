#include "a11y_world.h"
#include "color.h"
#include "enums.h"
#include "gun_mode.h"
#include "mapdata.h"
#include "omdata.h"
#include "overmap.h"
#include "player_activity.h"
#include "translation.h"
#include <functional>
#include <memory>

#include <algorithm>
#if defined(__ANDROID__)
#include <array>
#endif
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "a11y_android.h"
#include "a11y_input.h"
#include "a11y_item_text.h"
#include "a11y_body.h"
#include "a11y_outcome.h"
#include "a11y_perception.h"
#include "a11y_runtime.h"
#include "action.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "display.h"
#include "effect.h"
#include "field.h"
#include "game.h"
#include "input_context.h"
#include "item.h"
#include "item_location.h"
#include "lightmap.h"
#include "map.h"
#include "messages.h"
#include "move_mode.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "output.h"
#include "options.h"
#if defined(__ANDROID__)
#include "overmap_types.h"
#endif
#include "overmapbuffer.h"
#include "point.h"
#include "ranged.h"
#include "string_formatter.h"
#include "trap.h"
#include "translations.h"
#include "type_id.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

namespace a11y
{
namespace
{
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_incorporeal_nearby( "incorporeal" );
static const trait_id trait_shell2_nearby( "SHELL2" );
static const trait_id trait_shell3_nearby( "SHELL3" );
static const json_character_flag no_hands_nearby( "TEMPORARY_SHAPESHIFT_NO_HANDS" );

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

// Speak a short, useful event when entering an interesting tile. Ordinary
// floor tiles stay silent. Use only the already observed information: never
// reveal items hidden inside a closed container or invisible hazards.
std::string short_tile_alert( const tile_observation &tile )
{
    if( !tile.known_traps.empty() ) {
        return string_format( _( "Trap: %s." ), tile.known_traps.front() );
    }
    if( !tile.seen_creatures.empty() ) {
        return string_format( _( "Creature: %s." ), tile.seen_creatures.front() );
    }
    if( !tile.special_perceptions.empty() ) {
        return tile.special_perceptions.front();
    }
    if( !tile.fields.empty() ) {
        return string_format( _( "Hazard: %s." ), tile.fields.front() );
    }
    if( tile.impassable ) {
        return string_format( _( "Obstacle: %s." ), tile.terrain );
    }
    if( tile.route_feature ) {
        return string_format( _( "Route feature: %s." ), tile.terrain );
    }
    if( !tile.heard_marker.empty() ) {
        return string_format( _( "Sound marker: %s." ), tile.heard_marker );
    }
    if( !tile.vehicle.empty() ) {
        return string_format( _( "Vehicle: %s." ), tile.vehicle );
    }
    if( !tile.seen_items.empty() ) {
        if( tile.seen_items.size() == 1 ) {
            return string_format( _( "Item here: %s." ), tile.seen_items.front() );
        }
        if( tile.seen_items.size() == 2 ) {
            return string_format( _( "Items here: %1$s, %2$s." ),
                                  tile.seen_items[0], tile.seen_items[1] );
        }
        return string_format( _( "Items here: %1$s and %2$d other types." ),
                              tile.seen_items.front(),
                              static_cast<int>( tile.seen_items.size() - 1 ) );
    }
    if( !tile.furniture.empty() ) {
        return string_format( _( "Furniture: %s." ), tile.furniture );
    }
    return noteworthy_category( tile ) + ". " + tile.terrain;
}

std::string player_status()
{
    const avatar &viewer = get_avatar();
    std::string result = display::time_string( viewer );
    // The native sidebar uses an empty label for neutral statuses. Spoken
    // summaries must not say "Hunger: . Pain: ." or invent a severity;
    // keep only readings the game actually supplies.
    const auto append_if_present = [&result]( const char *heading, const std::string &reading ) {
        const std::string clean = remove_color_tags( reading );
        if( !clean.empty() ) {
            result += ". " + string_format( heading, clean );
        }
    };
    append_if_present( _( "Hunger: %s" ), display::hunger_text_color( viewer ).first );
    append_if_present( _( "Thirst: %s" ), display::thirst_text_color( viewer ).first );
    append_if_present( _( "Sleepiness: %s" ), display::sleepiness_text_color( viewer ).first );
    append_if_present( _( "Pain: %s" ), display::pain_text_color( viewer ).first );
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
    const tile_observation current_tile = observe_tile( origin );
    semantic_node here = document( "world:here", _( "Current tile" ), describe_tile( current_tile ) );
    // Native classification, not a language-specific substring heuristic:
    // only noteworthy terrain should interrupt movement with short speech.
    const bool noteworthy_here = has_noteworthy_information( current_tile );
    here.role = noteworthy_here ? "alert" : "document";
    here.value = current_tile.terrain;
    if( noteworthy_here ) {
        here.value = short_tile_alert( current_tile );
    }
    if( camera_offset ) {
        here.label = _( "Death camera center" );
        result.summary = _( "Death camera is active. Use Look to inspect the view, move or center the camera, "
                            "or accept your fate to finish watching." );
    }
    result.nodes.push_back( std::move( here ) );
    // A separate, read-only location node lets Android distinguish two
    // consecutive noteworthy tiles with identical descriptions. This never
    // appears in the automatic spoken alert, so footsteps remain quiet.
    const tripoint_abs_ms avatar_position = get_avatar().pos_abs();
    semantic_node location = document( "world:tile_position", _( "Current position" ), "" );
    location.value = string_format( "%d,%d,%d",
                                    avatar_position.x(), avatar_position.y(), avatar_position.z() );
    location.actions = { "focus" };
    result.nodes.push_back( std::move( location ) );
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
    // Both shortcuts read the exact same captured turn. Avoid computing
    // the fairly detailed health/effects summary twice per footstep.
    const std::string status_summary = player_status();
    result.nodes.push_back( document( "world:status", _( "Character status" ), status_summary ) );
    const avatar &character = get_avatar();
    std::string character_details = string_format(
                                        _( "Character information. %s." ), character.get_name() );
    character_details += " " + string_format(
                             _( "Strength: %1$d. Dexterity: %2$d. Intelligence: %3$d. Perception: %4$d." ),
                             character.get_str(), character.get_dex(),
                             character.get_int(), character.get_per() );
    character_details += " " + status_summary;
    // Blank labels mean neutral need states, not unavailable statistics.
    // Speak them only in this requested full character read. Keep the
    // short status summary unchanged to avoid repetitive live alerts.
    const auto append_neutral_need = [&character_details]( const char *label,
                                     const std::string &current ) {
        if( current.empty() ) {
            character_details += " " + string_format( label, _( "Normal" ) ) + ".";
        }
    };
    append_neutral_need( _( "Hunger: %s" ), display::hunger_text_color( character ).first );
    append_neutral_need( _( "Thirst: %s" ), display::thirst_text_color( character ).first );
    append_neutral_need( _( "Sleepiness: %s" ), display::sleepiness_text_color( character ).first );

    // Every number below comes from the existing game state. Nutrition is
    // a genuine percent of healthy calorie reserves; never invent hunger,
    // thirst or sleepiness percentages.
    character_details += " " + string_format( _( "Energy reserves: %d%%" ),
                         static_cast<int>( std::lround( 100.0f * character.get_kcal_percent() ) ) ) + ".";
    const std::string mood = remove_color_tags( display::morale_face_color( character ).first );
    if( !mood.empty() && mood != "ERR" ) {
        character_details += " " + string_format( _( "Mood: %s" ), mood ) + ".";
    }
    character_details += " " + string_format( _( "Focus: %d" ), character.get_focus() ) + ".";
    character_details += " " + string_format( _( "Speed: %d" ), character.get_speed() ) + ".";
    // The current moves counter can be negative. It does not mean
    // "number of moves already spent" or "maximum moves this turn".
    character_details += " " + string_format( _( "Movement points: %d" ),
                         character.get_moves() ) + ".";
    character_details += " " + string_format( _( "Flat-ground move cost: %d" ),
                         character.run_cost( 100, false ) ) + ".";
    const auto weariness_progress = character.weariness_transition_progress();
    if( weariness_progress.second > 0 ) {
        character_details += " " + string_format( _( "Weariness to next level: %d" ),
                             weariness_progress.first ) + ".";
    }
    character_details += " " + string_format( _( "Activity level: %s" ),
                         display::activity_text_color( character ).first ) + ".";
    character_details += " " + string_format( _( "Activity penalty: %s" ),
                         display::activity_malus_str( character ) ) + ".";
    // The player's existing local sound volume meter, not an invented
    // noise system or a percentage.
    character_details += " " + string_format( _( "Sound level: %d" ), character.volume ) + ".";
    const std::string held_item = remove_color_tags( character.weapname_simple() );
    if( !held_item.empty() ) {
        character_details += " " + string_format( _( "Wielding: %s." ), held_item );
    }
    result.nodes.push_back( document( "world:character_detail", _( "Character information" ),
                                      std::move( character_details ) ) );
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
    // Two-finger double-tap is a spoken recap: the newest event must be
    // first, so players hear what just happened without waiting through
    // older messages. Leave the game's underlying chronological log alone.
    std::string recent;
    const auto messages = Messages::recent_messages( 12 );
    for( auto it = messages.rbegin(); it != messages.rend(); ++it ) {
        recent += it->first + ": " + it->second + ". ";
    }
    result.nodes.push_back( document( "world:messages", _( "Recent messages" ), recent ) );
    // A small explicit command surface lets Android multi-finger gestures
    // request live information without trusting coordinates from Java.
    result.nodes.push_back( button( "reader:scan:north", _( "Scan 16 tiles north" ), "" ) );
    result.nodes.push_back( button( "reader:scan:south", _( "Scan 16 tiles south" ), "" ) );
    result.nodes.push_back( button( "reader:scan:west", _( "Scan 16 tiles west" ), "" ) );
    result.nodes.push_back( button( "reader:scan:east", _( "Scan 16 tiles east" ), "" ) );
    result.nodes.push_back( button( "reader:places", _( "Explore places" ), "" ) );
    result.nodes.push_back( button( "reader:pickup_here", _( "Pick up here" ), "" ) );
    result.nodes.push_back( button( "reader:coordinates", _( "Read current coordinates" ), "" ) );
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
    // Eight-direction combat gestures use the same engine rules as the
    // corresponding native keys. A gun fires; otherwise normal movement
    // bumps into monsters for melee, interacts with NPCs, or walks into
    // empty space. No hostile-only restriction or forced standstill.
    // The existing eligibility gate prevents use in incompatible modes.
    const int combat_steps[8][2] = {
        { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 },
        { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }
    };
    const item_location combat_held = viewer.get_wielded_item();
    const bool ranged_combat = combat_held && combat_held->is_gun() &&
                               !combat_held->gun_current_mode().melee();
    for( int i = 0; i < 8; ++i ) {
        const std::string dir_name = relative_name( combat_steps[i][0], combat_steps[i][1] );
        semantic_node attack = button( "reader:combat:attack:" + std::to_string( i ),
                                       ranged_combat ? string_format( _( "Fire %s" ), dir_name ) :
                                       string_format( _( "Move or attack %s" ), dir_name ),
                                       ranged_combat ?
                                       _( "Fire the wielded gun in this direction." ) :
                                       _( "Use normal movement: attack or interact if occupied, otherwise walk." ) );
        attack.enabled = result.swipe_movement_allowed;
        result.nodes.push_back( std::move( attack ) );
    }
    const bool precise_aim = target_handler::combat_precise_aim_enabled();
    semantic_node aim_toggle = button( "reader:combat:aim-toggle",
                                       precise_aim ? _( "Switch to quick fire" ) :
                                       _( "Switch to precise aiming" ),
                                       precise_aim ?
                                       _( "Precise aiming is active. Directional gun attacks aim and fire automatically." ) :
                                       _( "Quick fire is active. Directional gun attacks fire immediately." ) );
    aim_toggle.enabled = result.swipe_movement_allowed;
    result.nodes.push_back( std::move( aim_toggle ) );
    for( const std::pair<std::string, std::string> &command : {
                std::pair<std::string, std::string>{ "reader:combat:weapon-next", _( "Next weapon" ) },
                { "reader:combat:weapon-prev", _( "Previous weapon" ) },
                { "reader:combat:reload", _( "Reload wielded gun" ) }
            } ) {
        semantic_node node = button( command.first, command.second,
                                     _( "Uses the native weapon and ammunition rules. No walking." ) );
        node.enabled = result.swipe_movement_allowed;
        if( node.id == "reader:combat:reload" ) {
            const item_location held = viewer.get_wielded_item();
            node.value = held && held->is_gun() ?
                         string_format( _( "Ammunition remaining: %d." ), held->ammo_remaining() ) :
                         _( "No gun wielded." );
        }
        result.nodes.push_back( std::move( node ) );
    }
    // Read the actual wielded item without activating it or using ammo.
    // A right-left one-finger gesture speaks this read-only status node.
    const item_location wielded = viewer.get_wielded_item();
    std::string weapon_report;
    if( !wielded ) {
        weapon_report = _( "Your hands are empty." );
    } else if( wielded->is_gun() ) {
        weapon_report = string_format( _( "Wielding %1$s. %2$d rounds loaded." ),
                                       wielded->tname(), wielded->ammo_remaining() );
    } else {
        weapon_report = string_format( _( "Wielding %s." ), wielded->tname() );
    }
    result.nodes.push_back( document( "reader:combat:weapon-status",
                                      _( "Wielded weapon" ), weapon_report ) );
    // Names, hostility, distance and health come exclusively from the
    // game's visible/targetable creature set. Never reveal hidden enemies.
    std::vector<Creature *> enemies = viewer.get_targetable_creatures( 16, false );
    enemies.erase( std::remove_if( enemies.begin(), enemies.end(), [&]( const Creature *enemy ) {
        return enemy == nullptr || enemy == &viewer ||
               enemy->attitude_to( viewer ) != Creature::Attitude::HOSTILE;
    } ), enemies.end() );
    std::sort( enemies.begin(), enemies.end(), [&]( const Creature *first, const Creature *second ) {
        return rl_dist( viewer.pos_bub(), first->pos_bub() ) <
               rl_dist( viewer.pos_bub(), second->pos_bub() );
    } );
    std::string enemy_report;
    for( const Creature *enemy : enemies ) {
        const tripoint_rel_ms relative = enemy->pos_bub() - viewer.pos_bub();
        if( !enemy_report.empty() ) {
            enemy_report += " ";
        }
        enemy_report += string_format(
                            _( "%1$s, %2$s, %3$d HP remaining." ),
                            enemy->disp_name(), relative_name( relative.x(), relative.y() ),
                            std::max( 0, enemy->get_hp() ) );
    }
    if( enemy_report.empty() ) {
        enemy_report = _( "No visible hostile creatures nearby." );
    }
    result.nodes.push_back( document( "reader:combat:enemies",
                                      _( "Visible hostile creatures" ), enemy_report ) );

    auto commands = command_snapshot( context, result.title );
    const std::string active_mode = viewer.current_movement_mode()->name();
    const std::string chosen_mode = viewer.get_desired_move_mode()->name();
    for( auto &node : commands.nodes ) {
        if( node.id == "cycle_move" || node.id == "cycle_move_reverse" ||
            node.id == "reset_move" || node.id == "open_movement" ||
            node.id == "toggle_run" || node.id == "toggle_crouch" ||
            node.id == "toggle_prone" ) {
            // All reader screens obtain the selected movement mode from
            // command_snapshot(); only world-specific detail belongs here.
            node.detail = string_format( _( "Now: %1$s. Selected: %2$s." ),
                                         active_mode, chosen_mode );
        } else if( node.id == "auto_travel_mode" ) {
            node.detail = _( "When on, directional movement may follow a longer path. "
                             "It does not select a destination." );
        } else if( node.id == "grab" ) {
            if( viewer.get_grab_type() != object_type::NONE ) {
                const tripoint_bub_ms grabbed = viewer.pos_bub() + viewer.grab_point;
                std::string held_name;
                if( const optional_vpart_position vp = get_map().veh_at( grabbed ) ) {
                    held_name = vp->vehicle().name;
                } else if( get_map().has_furn( grabbed ) ) {
                    held_name = get_map().furnname( grabbed );
                }
                node.label = held_name.empty() ? _( "Release held object" ) :
                             string_format( _( "Release %s" ), held_name );
                node.detail = _( "Release the held object. No movement or extra confirmation." );
            } else {
                node.detail = _( "Choose a visible movable object by name and direction. "
                                 "Selecting it grabs it immediately." );
            }
        } else if( node.id == "haul" || node.id == "haul_toggle" ) {
            node.detail = string_format( _( "Items selected: %1$d. Autohaul: %2$s." ),
                                         static_cast<int>( viewer.haul_list.size() ),
                                         viewer.is_autohauling() ? _( "on" ) : _( "off" ) );
        } else if( node.id == "interact" || node.id == "examine" ||
                   node.id == "examine_and_pickup" ) {
            node.detail = _( "Opens a spoken list of visible, nearby objects. "
                             "Browse freely; selecting an object uses the original game action." );
        } else if( node.id == "smash" && viewer.controlling_vehicle ) {
            // When driving, the native Smash key engages the handbrake.
            // The contextual menu must never call that "Smash".
            node.label = _( "Apply handbrake" );
            node.detail = _( "Apply the vehicle handbrake. This is not an attack." );
        } else if( node.id == "open" || node.id == "close" || node.id == "smash" ) {
            node.detail = _( "Choose a target, then listen for the result. "
                             "The action may spend time or damage property." );
        } else if( node.id == "pickup" ) {
            node.detail = _( "Choose one tile, mark the items and quantities, then confirm." );
        } else if( node.id == "pickup_all" ) {
            node.detail = _( "Mark items from your tile and all eight neighboring tiles, "
                             "then confirm. Browsing does not pick anything up." );
        } else if( node.id == "butcher" ) {
            node.detail = _( "Process corpses, cut up materials or disassemble items on "
                             "your current tile. Requirements and time depend on the work." );
        } else if( node.id == "inventory" ) {
            node.detail = _( "Browse carried items. Select an item for its original action "
                             "menu. Browsing never uses or drops an item." );
        } else if( node.id == "advinv" ) {
            node.detail = _( "Transfer items between a source and destination. "
                             "Review both panes and the amount before moving." );
        } else if( node.id == "drop" || node.id == "drop_adj" ) {
            node.detail = _( "Choose items and amounts, then choose or confirm the "
                             "destination. Only the game activity moves them." );
        } else if( node.id == "compare" ) {
            node.detail = _( "Choose two items. Each side will be read separately "
                             "with its full item statistics." );
        } else if( node.id == "organize" ) {
            node.detail = _( "Assign or clear inventory shortcut letters. "
                             "This does not move items." );
        } else if( node.id == "apply" || node.id == "apply_wielded" ) {
            node.detail = _( "Use a selected or wielded item. Its effects and "
                             "resource costs depend on that item." );
        } else if( node.id == "item_action_menu" ) {
            node.detail = _( "Browse available item actions, with their original "
                             "requirements and disabled choices." );
        } else if( node.id == "insert" ) {
            node.detail = _( "Select a container and items to put inside. "
                             "The game checks capacity before inserting." );
        } else if( node.id == "unload_container" ) {
            node.detail = _( "Take eligible contents out of containers and drop "
                             "them at a chosen nearby tile." );
        } else if( node.id == "pause" && !viewer.activity ) {
            node.label = _( "Stand still for one turn" );
        }
        result.nodes.push_back( std::move( node ) );
    }

    // Only the second comprehensive-menu group consumes these hidden
    // capabilities. Other hubs and the engine's original actions stay intact.
    // All checks run on the game thread after visibility has been updated.
    {
        map &nearby_map = get_map();
        const tripoint_bub_ms player_pos = viewer.pos_bub( nearby_map );
        std::map<std::string, std::string> nearby_actions;
        const bool in_shell = viewer.has_active_mutation( trait_shell2_nearby ) ||
                              viewer.has_active_mutation( trait_shell3_nearby );
        const bool incorporeal = viewer.has_effect( effect_incorporeal_nearby );
        const bool handless = viewer.has_flag( no_hands_nearby );
        const bool mounted = viewer.is_mounted();
        const bool ordinary_mount = mounted &&
                                    ( !viewer.mounted_creature ||
                                      !viewer.mounted_creature->has_flag( mon_flag_RIDEABLE_MECH ) );
        const auto permitted = [&]( const std::string & id ) {
            if( in_shell && ( id == "open" || id == "close" || id == "smash" ||
                              id == "examine" || id == "examine_and_pickup" ||
                              id == "pickup" || id == "pickup_all" ||
                              id == "reader:pickup_here" || id == "grab" ||
                              id == "haul" || id == "haul_toggle" ||
                              id == "butcher" || id == "loot" ||
                              id == "control_vehicle" ) ) {
                return false;
            }
            if( incorporeal && ( id == "open" || id == "close" || id == "smash" ||
                                 id == "pickup" || id == "pickup_all" ||
                                 id == "reader:pickup_here" || id == "grab" ||
                                 id == "haul" || id == "haul_toggle" ||
                                 id == "butcher" || id == "loot" ) ) {
                return false;
            }
            if( handless && ( id == "open" || id == "close" || id == "pickup" ||
                              id == "pickup_all" || id == "reader:pickup_here" ||
                              id == "grab" || id == "haul" ||
                              id == "haul_toggle" || id == "butcher" ||
                              id == "loot" || id == "control_vehicle" ) ) {
                return false;
            }
            if( mounted && ( id == "open" || id == "pickup" || id == "pickup_all" ||
                             id == "reader:pickup_here" || id == "grab" ||
                             id == "haul" || id == "haul_toggle" ||
                             id == "butcher" || id == "loot" ) ) {
                return false;
            }
            if( ordinary_mount && id == "close" ) {
                return false;
            }
            return true;
        };
        const auto offer = [&]( const std::string & id, const std::string & hint ) {
            if( permitted( id ) ) {
                nearby_actions.emplace( id, hint );
            }
        };
        if( viewer.get_grab_type() != object_type::NONE ) {
            offer( "grab", "" );
        }
        if( viewer.is_hauling() ) {
            offer( "haul_toggle", "" );
        }
        // Native Smash is repurposed as the handbrake while driving.
        if( viewer.controlling_vehicle ) {
            offer( "smash", "" );
        }
        if( viewer.in_vehicle ) {
            if( const optional_vpart_position vp = nearby_map.veh_at( player_pos ) ) {
                if( vp.part_with_feature( "CONTROLS", true ) ) {
                    offer( "control_vehicle", vp->vehicle().name );
                }
            }
        }
        for( int dy = -1; dy <= 1; ++dy ) {
            for( int dx = -1; dx <= 1; ++dx ) {
                const tripoint_bub_ms target = player_pos + tripoint_rel_ms( dx, dy, 0 );
                const tile_observation seen = observe_tile( target );
                if( seen.source != observation_source::visible ) {
                    continue;
                }
                std::string object_name = !seen.furniture.empty() ? seen.furniture :
                                          !seen.vehicle.empty() ? seen.vehicle :
                                          !seen.seen_creatures.empty() ? seen.seen_creatures.front() :
                                          !seen.seen_items.empty() ? seen.seen_items.front() : seen.terrain;
                const std::string hint = object_name + ", " +
                                         relative_position_name( dx, dy, 0 );
                const auto offer_interact = [&]( const std::string & id ) {
                    if( permitted( id ) ) {
                        offer( id, hint );
                        offer( "interact", hint );
                    }
                };
                if( can_interact_at( ACTION_EXAMINE, nearby_map, target ) ) {
                    offer_interact( "examine" );
                }
                if( can_interact_at( ACTION_EXAMINE_AND_PICKUP, nearby_map, target ) ) {
                    offer_interact( "examine_and_pickup" );
                }
                if( can_interact_at( ACTION_PICKUP, nearby_map, target ) ) {
                    offer_interact( "pickup" );
                    offer( "pickup_all", hint );
                    offer( "haul", hint );
                    offer( "haul_toggle", hint );
                    // Loot is a zone-based activity. The game still checks whether
                    // suitable zones and items are available when launched.
                    offer( "loot", hint );
                    if( target == player_pos ) {
                        offer( "reader:pickup_here", hint );
                    }
                }
                if( can_interact_at( ACTION_OPEN, nearby_map, target ) ) {
                    offer_interact( "open" );
                }
                if( can_interact_at( ACTION_CLOSE, nearby_map, target ) ) {
                    offer_interact( "close" );
                }
                const Creature *creature = get_creature_tracker().creature_at( target );
                if( dynamic_cast<const npc *>( creature ) &&
                    can_interact_at( ACTION_CHAT, nearby_map, target ) ) {
                    offer_interact( "chat" );
                }
                if( target == player_pos ) {
                    if( can_interact_at( ACTION_BUTCHER, nearby_map, target ) ) {
                        offer_interact( "butcher" );
                    }
                    if( can_interact_at( ACTION_MOVE_UP, nearby_map, target ) ) {
                        offer( "LEVEL_UP", hint );
                        offer( "interact", hint );
                    }
                    if( can_interact_at( ACTION_MOVE_DOWN, nearby_map, target ) ) {
                        offer( "LEVEL_DOWN", hint );
                        offer( "interact", hint );
                    }
                }
                if( !viewer.controlling_vehicle &&
                    can_interact_at( ACTION_SMASH, nearby_map, target ) ) {
                    offer( "smash", hint );
                }
                if( target != player_pos &&
                    ( ( nearby_map.has_furn( target ) &&
                        nearby_map.furn( target ).obj().is_movable() ) ||
                      ( nearby_map.veh_at( target ) &&
                        !nearby_map.veh_at( target ).part_with_feature(
                            VPFLAG_WALL_MOUNTED, false ) ) ) ) {
                    offer( "grab", hint );
                }
            }
        }
        for( const std::string &id : {
                    "interact", "open", "close", "examine", "examine_and_pickup",
                    "reader:pickup_here", "pickup", "pickup_all", "chat",
                    "LEVEL_UP", "LEVEL_DOWN", "grab", "haul", "haul_toggle",
                    "butcher", "loot", "control_vehicle", "smash"
                } ) {
            semantic_node marker;
            marker.id = "reader:nearby:available:" + id;
            marker.role = "status";
            marker.label = marker.id;
            marker.actions = { "focus" };
            const auto found = nearby_actions.find( id );
            marker.enabled = found != nearby_actions.end();
            if( marker.enabled ) {
                marker.value = found->second;
            }
            result.nodes.push_back( std::move( marker ) );
        }
    }
    return result;
}

// Deliberately bypass observe_tile() and its visibility/memory restrictions:
// this explicit assistive scan reveals the live map, even behind walls.
unrestricted_tile scan_unrestricted_tile( const tripoint_bub_ms &position )
{
    unrestricted_tile tile;
    map &here = get_map();
    if( !here.inbounds( position ) ) {
        return tile;
    }
    tile.terrain = remove_color_tags( here.tername( position ) );
    if( here.has_furn( position ) ) {
        tile.furniture = remove_color_tags( here.furnname( position ) );
    }
    tile.impassable = here.impassable( position );
    for( const auto &entry : here.field_at( position ) ) {
        tile.fields.push_back( remove_color_tags( entry.second.name() ) );
    }
    const trap &hazard = here.tr_at( position );
    if( !hazard.is_null() ) {
        tile.traps.push_back( remove_color_tags( hazard.name() ) );
    }
    const Creature *creature = get_creature_tracker().creature_at( position, true );
    if( creature != nullptr && creature != &get_avatar() ) {
        tile.creatures.push_back( remove_color_tags( creature->disp_name() ) );
    }
    const optional_vpart_position vehicle = here.veh_at( position );
    if( vehicle ) {
        tile.vehicle = remove_color_tags( vehicle->vehicle().name );
    }
    std::map<std::string, int> counts;
    for( const item &obj : here.i_at( position ) ) {
        const std::string name = spoken_item_name( obj );
        if( !name.empty() ) {
            ++counts[name];
        }
    }
    for( const auto &entry : counts ) {
        tile.items.push_back( entry.second == 1 ? entry.first :
                              string_format( _( "%1$s [%2$d]" ), entry.first, entry.second ) );
    }
    return tile;
}

void announce_linear_scan( const int dx, const int dy )
{
    if( std::abs( dx ) + std::abs( dy ) != 1 ) {
        return;
    }
    map &here = get_map();
    const tripoint_bub_ms origin = get_avatar().pos_bub( here );
    std::string spoken;
    for( int distance = 1; distance <= 16; ++distance ) {
        const tripoint_bub_ms pos = origin + tripoint_rel_ms( dx * distance, dy * distance, 0 );
        std::string detail;
        if( !here.inbounds( pos ) ) {
            detail = _( "Outside loaded map" );
        } else {
            const unrestricted_tile tile = scan_unrestricted_tile( pos );
            detail = tile.terrain;
            if( !tile.furniture.empty() && tile.furniture != tile.terrain ) {
                detail += ". " + tile.furniture;
            }
            if( tile.impassable ) {
                detail += ". " + std::string( _( "Impassable" ) );
            }
            for( const std::string &field : tile.fields ) {
                detail += ". " + field;
            }
            for( const std::string &hazard : tile.traps ) {
                detail += ". " + string_format( _( "Trap: %s" ), hazard );
            }
            for( const std::string &creature : tile.creatures ) {
                detail += ". " + creature;
            }
            if( !tile.vehicle.empty() ) {
                detail += ". " + string_format( _( "Vehicle: %s" ), tile.vehicle );
            }
            if( !tile.items.empty() ) {
                // Distinguish the furniture from items on the same tile.
                // Never assert that items are inside an actual container.
                detail += ". " + std::string( _( "Items:" ) );
                for( const std::string &name : tile.items ) {
                    detail += " " + name + ".";
                }
            }
        }
        if( !spoken.empty() ) {
            spoken += " ";
        }
        spoken += string_format( _( "Tile %1$d: %2$s." ), distance, detail );
    }
    a11y::android::announce( "A11Y_SCAN:" + spoken, 1 );
}

void announce_area_transition()
{
    // Game thread only. The absolute overmap tile is independent of map
    // bubble shifts: walking across an internal submap boundary is not
    // itself a new area. Reset across world loads/lifecycle generations.
    static std::uint64_t previous_epoch = 0;
    static std::optional<tripoint_abs_omt> previous_tile;
    static std::string previous_area;
    const std::uint64_t current_epoch = a11y::android::lifecycle_epoch();
    if( current_epoch != previous_epoch ) {
        previous_epoch = current_epoch;
        previous_tile.reset();
        previous_area.clear();
    }
    const tripoint_abs_omt omt = get_avatar().pos_abs_omt();
    if( previous_tile && *previous_tile == omt ) {
        return;
    }
    const om_vision_level vision = overmap_buffer.seen( omt );
    if( vision == om_vision_level::unseen ) {
        // Never announce unseen terrain. Recheck when the observer learns it.
        return;
    }
    const oter_id &terrain = overmap_buffer.ter_existing( omt );
    if( !terrain.is_valid() ) {
        return;
    }
    const std::string area = remove_color_tags( terrain->get_name( vision ) );
    if( area.empty() ) {
        return;
    }
    if( previous_tile && !previous_area.empty() && area != previous_area ) {
        a11y::android::announce( string_format( _( "Entering area: %s." ), area ), 1 );
#if defined(__ANDROID__)
        a11y::android::diagnostic_event( "WORLD_AREA_CHANGE",
                "from=" + previous_area + " to=" + area );
#endif
    }
    previous_tile = omt;
    previous_area = area;
}

void announce_player_coordinates()
{
    const avatar &viewer = get_avatar();
    const tripoint_abs_ms pos = viewer.pos_abs();
    const std::string coordinates = string_format( _( "Coordinates. X %1$d. Y %2$d. Z %3$d." ),
                                            pos.x(), pos.y(), pos.z() );
    // Speak the known overmap place first. Never reveal undiscovered
    // terrain when the current overmap location has not been seen.
    std::string spoken = string_format( _( "Area: %1$s. %2$s" ), _( "Unknown" ), coordinates );

    // The overmap already names forests, roads, buildings and other areas.
    // Use exactly what the character knows; never expose an unseen terrain
    // or generate new overmap data merely to announce coordinates.
    const tripoint_abs_omt omt = viewer.pos_abs_omt();
    const om_vision_level vision = overmap_buffer.seen( omt );
    if( vision != om_vision_level::unseen ) {
        const oter_id &terrain = overmap_buffer.ter_existing( omt );
        if( terrain.is_valid() ) {
            const std::string area = remove_color_tags( terrain->get_name( vision ) );
            if( !area.empty() ) {
                spoken = string_format( _( "Area: %1$s. %2$s" ), area, coordinates );
            }
        }
    }
    // Native lighting classification at the player's actual tile. Use
    // the lightmap, never the brighter general outdoor sky level.
    const lit_level light = get_map().light_at( viewer.pos_bub() );
    std::string lighting;
    switch( light ) {
        case lit_level::BRIGHT:
            lighting = _( "Very bright" );
            break;
        case lit_level::LIT:
            lighting = _( "Well lit" );
            break;
        case lit_level::LOW:
            lighting = _( "Dim" );
            break;
        case lit_level::DARK:
            lighting = _( "Dark" );
            break;
        default:
            lighting = _( "Unknown" );
            break;
    }
    spoken += " " + string_format( _( "Lighting: %s" ), lighting ) + ".";
    spoken += " " + string_format( _( "Weather: %s" ),
                                   display::weather_text_color( viewer ).first ) + ".";
    // Exact with a watch, approximate if the sky is visible, otherwise
    // unavailable. The native display routine already enforces this.
    const std::string game_time = display::time_string( viewer );
    spoken += " " + string_format( _( "Time: %s" ),
                                   game_time == _( "???" ) ? _( "Unknown" ) : game_time ) + ".";
    a11y::android::announce( spoken, 1 );
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
