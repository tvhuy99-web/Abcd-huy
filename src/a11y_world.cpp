#include "a11y_world.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>
#include "a11y_input.h"
#include "a11y_perception.h"
#include "avatar.h"
#include "bodypart.h"
#include "catacharset.h"
#include "coordinates.h"
#include "display.h"
#include "input_context.h"
#include "map.h"
#include "messages.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "vehicle.h"

namespace a11y
{
namespace
{
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
        const auto indicator = get_hp_bar( viewer.get_part_hp_cur( part ), viewer.get_part_hp_max( part ), false );
        const int bars = std::max( 0, std::min( 5, utf8_width( indicator.first ) ) );
        result += ". " + string_format( _( "%s: health indicator %d of 5" ), body_part_name( part ), bars );
    }
    if( display::vehicle_driven( viewer ) != nullptr ) {
        result += ". " + std::string( _( "Vehicle controls" ) ) + ". " +
                  display::vehicle_azimuth_text( viewer ) + ". " +
                  display::vehicle_cruise_text_color( viewer ).first;
    }
    return remove_color_tags( result );
}
} // namespace

semantic_snapshot world_snapshot( const input_context &context, const int scan_radius )
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
    result.nodes.push_back( document( "world:status", _( "Character status" ), player_status() ) );
    semantic_node radius;
    radius.id = "world:radius";
    radius.label = _( "Scan radius, from 1 to 16" );
    radius.value = std::to_string( scan_radius );
    radius.role = "number";
    radius.actions = { "focus", "set_text" };
    result.nodes.push_back( std::move( radius ) );
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
