#include "a11y_world.h"

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "a11y_input.h"
#include "a11y_body.h"
#include "a11y_perception.h"
#include "avatar.h"
#include "bodypart.h"
#include "character.h"
#include "coordinates.h"
#include "display.h"
#include "input_context.h"
#include "map.h"
#include "messages.h"
#include "output.h"
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
    std::string hunger_text;
    std::string thirst_text;
    std::string sleepiness_text;
    std::string pain_text;
    std::string stamina_text;
    std::vector<bodypart_status> bodyparts;
};

std::optional<monitored_status> previous_status;

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

void add_transition( std::vector<std::string> &changes, const std::string &label,
                     const std::string &value, const int old_level, const int new_level,
                     const int threshold, const int urgent_level, bool &urgent )
{
    if( new_level > old_level && new_level >= threshold ) {
        changes.push_back( string_format( _( "%1$s is worsening: %2$s" ), label, value ) );
        urgent = urgent || new_level >= urgent_level;
    } else if( old_level > new_level && old_level >= threshold ) {
        const std::string detail = value.empty() ? _( "No current warning" ) : value;
        changes.push_back( string_format( _( "%1$s is improving: %2$s" ), label, detail ) );
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
    for( const bodypart_id &part : viewer.get_all_body_parts( get_body_part_flags::only_main ) ) {
        result += ". " + body_part_name( part ) + ": " + limb_health_description( viewer, part );
    }
    if( display::vehicle_driven( viewer ) != nullptr ) {
        result += ". " + std::string( _( "Vehicle controls" ) ) + ". " +
                  display::vehicle_azimuth_text( viewer ) + ". " +
                  display::vehicle_cruise_text_color( viewer ).first;
    }
    return remove_color_tags( result );
}
} // namespace

void reset_status_monitor()
{
    previous_status.reset();
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
    add_transition( changes, _( "Hunger" ), current.hunger_text, previous_status->hunger,
                    current.hunger, 1, 3, urgent );
    add_transition( changes, _( "Thirst" ), current.thirst_text, previous_status->thirst,
                    current.thirst, 1, 3, urgent );
    add_transition( changes, _( "Sleepiness" ), current.sleepiness_text,
                    previous_status->sleepiness, current.sleepiness, 1, 3, urgent );
    add_transition( changes, _( "Pain" ), current.pain_text, previous_status->pain,
                    current.pain, 2, 7, urgent );
    add_transition( changes, _( "Stamina" ), current.stamina_text, previous_status->stamina,
                    current.stamina, 2, 3, urgent );
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
            changes.push_back( string_format( _( "Bleeding is worsening at %s." ), name ) );
            urgent = urgent || part.bleeding >= 2;
        } else if( old->bleeding > 0 && part.bleeding == 0 ) {
            changes.push_back( string_format( _( "Bleeding stopped at %s." ), name ) );
        }
        if( !old->broken && part.broken ) {
            changes.push_back( string_format( _( "%s is broken." ), name ) );
            urgent = true;
        } else if( old->broken && !part.broken ) {
            changes.push_back( string_format( _( "%s is no longer broken." ), name ) );
        }
        if( !old->bitten && part.bitten ) {
            changes.push_back( string_format( _( "A bite was detected on %s." ), name ) );
        }
        if( !old->infected && part.infected ) {
            changes.push_back( string_format( _( "An infection was detected in %s." ), name ) );
            urgent = true;
        } else if( old->infected && !part.infected ) {
            changes.push_back( string_format( _( "The infection cleared in %s." ), name ) );
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
                                  const int scan_depth, const world_scan_filter filter )
{
    semantic_snapshot result;
    result.title = _( "World" );
    result.category = context.category_id();
    result.summary = remove_color_tags( display::move_mode_letter_color( get_avatar() ).first );
    result.focus = "world:here";
    const auto origin = get_avatar().pos_bub( get_map() );
    semantic_node here = document( "world:here", _( "Current tile" ), describe_tile( observe_tile( origin ) ) );
    here.value = here.detail;
    result.nodes.push_back( std::move( here ) );
    for( int distance = 1; distance <= scan_depth; ++distance ) {
        for( const int dz : { distance, -distance } ) {
            const tile_observation vertical = observe_memory_tile(
                    origin + tripoint_rel_ms( 0, 0, dz ) );
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
    const avatar &viewer = get_avatar();
    if( viewer.has_effect( effect_sleep ) ) {
        semantic_node activity_node = document( "world:activity", _( "Current activity" ),
                                               _( "Sleeping. The game will continue until you wake up." ) );
        activity_node.role = "status";
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
    for( int distance = 1; distance <= scan_radius; ++distance ) {
        for( int dy = -distance; dy <= distance; ++dy ) {
            for( int dx = -distance; dx <= distance; ++dx ) {
                if( std::max( std::abs( dx ), std::abs( dy ) ) != distance ) { continue; }
                const auto tile = observe_tile( origin + tripoint_rel_ms( dx, dy, 0 ) );
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
                node.value = tile.source == observation_source::unknown ? _( "Unknown" ) : tile.terrain;
                if( tile.source == observation_source::remembered ) {
                    node.value += ". " + std::string( _( "Remembered" ) );
                }
                result.nodes.push_back( std::move( node ) );
            }
        }
    }
    auto commands = command_snapshot( context, result.title );
    for( auto &node : commands.nodes ) { result.nodes.push_back( std::move( node ) ); }
    return result;
}
} // namespace a11y
