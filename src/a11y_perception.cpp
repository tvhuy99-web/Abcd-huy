#include "a11y_perception.h"

#include <map>
#include <utility>
#include "avatar.h"
#include "catacharset.h"
#include "creature.h"
#include "creature_tracker.h"
#include "coordinates.h"
#include "dialogue.h"
#include "field.h"
#include "field_type.h"
#include "flag.h"
#include "item.h"
#include "lightmap.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_memory.h"
#include "mapdata.h"
#include "output.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "vehicle.h"
#include "vpart_position.h"

namespace a11y
{
namespace
{
void add_known_items( std::map<std::string, int> &names, const item &value )
{
    ++names[value.tname()];
    // This is the same knowledge-aware container traversal as tile information.
    // all_items_ptr() would expose contents the character has not discovered.
    for( const item *content : value.all_known_contents() ) {
        add_known_items( names, *content );
    }
}

std::string describe_special_perception( const avatar &viewer, const Creature &creature,
        const tripoint_bub_ms &position )
{
    if( viewer.enchantment_cache->special_vision_vector.empty() ) {
        return {};
    }
    const_dialogue dialogue( get_const_talker_for( viewer ), get_const_talker_for( creature ) );
    const enchant_cache::special_vision vision = viewer.enchantment_cache->get_vision( dialogue );
    // Match the special-vision visibility gate used by the tile renderer.
    if( vision.is_empty() || viewer.cant_see( position ) || vision.ignores_aiming_cone ) {
        return {};
    }
    const enchant_cache::special_vision_descriptions description =
        viewer.enchantment_cache->get_vision_description_struct( vision, dialogue );
    std::string text = remove_color_tags( description.description.translated() );
    if( text.empty() ) {
        return {};
    }
    // parse_tags expands random snippets. Keep scanning read-only and avoid
    // resolving dynamic placeholders using information outside the cue.
    if( text.find( '<' ) != std::string::npos || text.find( '>' ) != std::string::npos ) {
        text = _( "You sense something here." );
    }
    return string_format( _( "Special perception: %s" ), remove_color_tags( text ) );
}
} // namespace

tile_observation observe_tile( const tripoint_bub_ms &position )
{
    tile_observation result;
    map &here = get_map();
    avatar &viewer = get_avatar();
    if( !here.inbounds( position ) ) {
        return result;
    }
    const auto &cache = here.get_visibility_variables_cache();
    const bool visible = here.get_visibility( here.apparent_light_at( position, cache ), cache ) ==
                         visibility_type::CLEAR;
    // sound_at is the player's heard-marker description, not a lookup of a
    // creature's true location or identity.
    result.heard_marker = sounds::sound_at( position );
    if( !visible ) {
        const auto absolute = here.get_abs( position );
        if( viewer.has_memory_at( absolute ) ) {
            const memorized_tile &memory = viewer.get_memorized_tile( absolute );
            const ter_str_id terrain( memory.get_ter_id() );
            const furn_str_id furniture( memory.get_dec_id() );
            if( terrain.is_valid() ) {
                result.source = observation_source::remembered;
                result.terrain = terrain.obj().name();
                result.route_feature = terrain.obj().has_flag( ter_furn_flag::TFLAG_GOES_UP ) ||
                                       terrain.obj().has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ||
                                       terrain.obj().has_flag( ter_furn_flag::TFLAG_DOOR );
                if( furniture.is_valid() && !furniture.is_null() ) {
                    result.furniture = furniture.obj().name();
                    result.route_feature = result.route_feature ||
                                           furniture.obj().has_flag( ter_furn_flag::TFLAG_GOES_UP ) ||
                                           furniture.obj().has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ||
                                           furniture.obj().has_flag( ter_furn_flag::TFLAG_DOOR );
                }
            }
        }
        if( !viewer.enchantment_cache->special_vision_vector.empty() ) {
            const Creature *creature = get_creature_tracker().creature_at( position, true );
            if( creature != nullptr && creature != &viewer ) {
                const std::string perception = describe_special_perception( viewer, *creature, position );
                if( !perception.empty() ) { result.special_perceptions.push_back( perception ); }
            }
        }
        return result;
    }
    result.source = observation_source::visible;
    result.terrain = here.tername( position );
    if( here.has_furn( position ) ) { result.furniture = here.furnname( position ); }
    result.route_feature = here.has_flag( ter_furn_flag::TFLAG_GOES_UP, position ) ||
                           here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, position ) ||
                           here.has_flag( ter_furn_flag::TFLAG_DOOR, position );
    result.impassable = here.impassable( position );
    for( const auto &entry : here.field_at( position ) ) {
        result.fields.push_back( entry.second.name() );
    }
    const trap &known_trap = here.tr_at( position );
    if( !known_trap.is_null() && known_trap.can_see( position, viewer ) ) {
        result.known_traps.push_back( known_trap.name() );
    }
    const Creature *creature = get_creature_tracker().creature_at( position, true );
    if( creature != nullptr && creature != &viewer && viewer.sees( here, *creature ) ) {
        result.seen_creatures.push_back( creature->disp_name() );
    } else if( creature != nullptr && creature != &viewer ) {
        const std::string perception = describe_special_perception( viewer, *creature, position );
        if( !perception.empty() ) { result.special_perceptions.push_back( perception ); }
    }
    const optional_vpart_position vehicle_part = here.veh_at( position );
    if( vehicle_part ) { result.vehicle = vehicle_part->vehicle().name; }
    const json_character_flag blind( "BLIND" );
    if( here.sees_some_items( position, viewer ) &&
        !( here.has_flag( ter_furn_flag::TFLAG_CONTAINER, position ) && !here.could_see_items( position, viewer ) ) &&
        !viewer.has_effect_with_flag( blind ) && !viewer.worn_with_flag( flag_BLIND ) ) {
        std::map<std::string, int> names;
        for( const item &value : here.i_at( position ) ) { add_known_items( names, value ); }
        for( const auto &name : names ) {
            result.seen_items.push_back( name.second > 1 ? string_format( _( "%s [%d]" ), name.first, name.second ) : name.first );
        }
    }
    return result;
}

tile_observation observe_memory_tile( const tripoint_bub_ms &position )
{
    tile_observation result;
    map &here = get_map();
    avatar &viewer = get_avatar();
    if( !here.inbounds( position ) ) {
        return result;
    }
    const auto absolute = here.get_abs( position );
    if( !viewer.has_memory_at( absolute ) ) {
        return result;
    }
    const memorized_tile &memory = viewer.get_memorized_tile( absolute );
    const ter_str_id terrain( memory.get_ter_id() );
    const furn_str_id furniture( memory.get_dec_id() );
    if( !terrain.is_valid() ) {
        return result;
    }
    result.source = observation_source::remembered;
    result.terrain = terrain.obj().name();
    result.route_feature = terrain.obj().has_flag( ter_furn_flag::TFLAG_GOES_UP ) ||
                           terrain.obj().has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ||
                           terrain.obj().has_flag( ter_furn_flag::TFLAG_DOOR );
    if( furniture.is_valid() && !furniture.is_null() ) {
        result.furniture = furniture.obj().name();
        result.route_feature = result.route_feature ||
                               furniture.obj().has_flag( ter_furn_flag::TFLAG_GOES_UP ) ||
                               furniture.obj().has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ||
                               furniture.obj().has_flag( ter_furn_flag::TFLAG_DOOR );
    }
    return result;
}

std::string describe_tile( const tile_observation &observation )
{
    std::string result;
    if( observation.source == observation_source::unknown ) {
        result = _( "Unknown tile." );
    } else {
        if( observation.source == observation_source::remembered ) {
            result = _( "Remembered, not currently visible: " );
        }
        result += observation.terrain;
        if( !observation.furniture.empty() ) { result += ". " + observation.furniture; }
        if( observation.source == observation_source::visible && observation.impassable ) {
            result += ". " + std::string( _( "Impassable" ) );
        }
        for( const auto &field : observation.fields ) { result += ". " + field; }
        for( const auto &trap : observation.known_traps ) { result += ". " + string_format( _( "Trap: %s" ), trap ); }
        for( const auto &creature : observation.seen_creatures ) { result += ". " + creature; }
        for( const auto &perception : observation.special_perceptions ) { result += ". " + perception; }
        if( !observation.vehicle.empty() ) { result += ". " + string_format( _( "Vehicle: %s" ), observation.vehicle ); }
        for( const auto &item : observation.seen_items ) { result += ". " + item; }
    }
    if( observation.source == observation_source::unknown ) {
        for( const auto &perception : observation.special_perceptions ) { result += ". " + perception; }
    }
    if( !observation.heard_marker.empty() ) {
        result += ". " + string_format( _( "Near this marker, you heard %s. "
                                            "The marker is approximate and may not be the sound source's position." ),
                                         observation.heard_marker );
    }
    return remove_color_tags( result );
}
} // namespace a11y
