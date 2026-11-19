#include "a11y_perception.h"
#include "enums.h"
#include "pimpl.h"
#include "point.h"
#include "translation.h"
#include <list>
#include <memory>
#include <optional>
#include "a11y_item_text.h"
#include "a11y_world.h"

#include <map>
#include <unordered_set>
#include <utility>
#include "avatar.h"
#include "creature.h"
#include "creature_tracker.h"
#include "coordinates.h"
#include "dialogue.h"
#include "field.h"
#include "flag.h"
#include "item.h"
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

static const json_character_flag json_flag_BLIND( "BLIND" );

namespace a11y
{
namespace
{
void add_known_items( std::map<std::string, int> &names, const item &value )
{
    ++names[spoken_item_name( value )];


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

    if( vision.is_empty() || viewer.cant_see( position ) || vision.ignores_aiming_cone ) {
        return {};
    }
    const enchant_cache::special_vision_descriptions description =
        viewer.enchantment_cache->get_vision_description_struct( vision, dialogue );
    std::string text = remove_color_tags( description.description.translated() );
    if( text.empty() ) {
        return {};
    }


    if( text.find( '<' ) != std::string::npos || text.find( '>' ) != std::string::npos ) {
        text = _( "You sense something here." );
    }
    return string_format( _( "Special perception: %s" ), remove_color_tags( text ) );
}






bool is_locked_door_id( const std::string &id )
{
    static const std::unordered_set<std::string> locked_doors = {
        "t_door_locked", "t_door_locked_peep", "t_door_locked_alarm",
        "t_door_locked_interior", "t_door_bar_locked"
    };
    return locked_doors.count( id ) > 0;
}

door_state derive_door_state( const bool has_door, const bool locked, const bool impassable )
{
    if( !has_door ) {
        return door_state::none;
    }
    if( locked ) {
        return door_state::locked;
    }
    return impassable ? door_state::closed : door_state::open;
}




bool ter_furn_impassable( const ter_t &terrain, const furn_t *furniture )
{
    if( terrain.movecost == 0 ) {
        return true;
    }
    const int furncost = furniture != nullptr ? furniture->movecost : 0;
    if( furncost < 0 ) {
        return true;
    }
    return terrain.movecost + furncost <= 0;
}




bool fill_remembered_tile( tile_observation &result, const memorized_tile &memory )
{
    const ter_str_id terrain( memory.get_ter_id() );
    const furn_str_id furniture( memory.get_dec_id() );
    if( !terrain.is_valid() ) {
        return false;
    }
    const ter_t &terrain_type = terrain.obj();
    const furn_t *furniture_type = nullptr;
    const bool has_furniture = furniture.is_valid() && !furniture.is_null();
    if( has_furniture ) {
        furniture_type = &furniture.obj();
    }
    result.source = observation_source::remembered;
    result.terrain = terrain_type.name();
    result.route_feature = terrain_type.has_flag( ter_furn_flag::TFLAG_GOES_UP ) ||
                           terrain_type.has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ||
                           terrain_type.has_flag( ter_furn_flag::TFLAG_DOOR );
    bool has_door = terrain_type.has_flag( ter_furn_flag::TFLAG_DOOR );
    bool locked = has_door && is_locked_door_id( terrain.str() );
    if( has_furniture ) {
        result.furniture = furniture_type->name();
        result.route_feature = result.route_feature ||
                               furniture_type->has_flag( ter_furn_flag::TFLAG_GOES_UP ) ||
                               furniture_type->has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ||
                               furniture_type->has_flag( ter_furn_flag::TFLAG_DOOR );
        if( furniture_type->has_flag( ter_furn_flag::TFLAG_DOOR ) ) {
            has_door = true;
            locked = locked || is_locked_door_id( furniture.str() );
        }
    }
    result.door = derive_door_state( has_door, locked,
                                     ter_furn_impassable( terrain_type, furniture_type ) );
    return true;
}
}

tile_observation observe_tile( const tripoint_bub_ms &position )
{
    tile_observation result;
    map &here = get_map();
    avatar &viewer = get_avatar();
    if( !here.inbounds( position ) ) {
        return result;
    }
    const visibility_variables &cache = here.get_visibility_variables_cache();
    const bool visible = here.get_visibility( here.apparent_light_at( position, cache ), cache ) ==
                         visibility_type::CLEAR;


    result.heard_marker = sounds::sound_at( position );
    if( !visible ) {
        const tripoint_abs_ms absolute = here.get_abs( position );
        if( viewer.has_memory_at( absolute ) ) {
            fill_remembered_tile( result, viewer.get_memorized_tile( absolute ) );
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

    {
        const bool has_door = here.has_flag( ter_furn_flag::TFLAG_DOOR, position );
        bool locked = false;
        if( has_door ) {
            locked = is_locked_door_id( here.ter( position ).id().str() ) ||
                     is_locked_door_id( here.furn( position ).id().str() );
        }
        result.door = derive_door_state( has_door, locked, result.impassable );
    }
    for( const auto &entry : here.field_at( position ) ) {



        const field_entry &field = entry.second;
        std::string field_name = field.name();
        if( field.get_field_intensity() > 1 ) {
            field_name = string_format( _( "%1$s, intensity %2$d of %3$d" ), field_name,
                                        field.get_field_intensity(),
                                        field.get_max_field_intensity() );
        }
        result.fields.push_back( field_name );
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
    if( vehicle_part ) {
        result.vehicle = vehicle_part->vehicle().name;



        result.vehicle_part = vehicle_part->vehicle().part(
                                  static_cast<int>( vehicle_part->part_index() ) ).name( false );
    }
    if( here.sees_some_items( position, viewer ) &&
        !( here.has_flag( ter_furn_flag::TFLAG_CONTAINER, position ) && !here.could_see_items( position, viewer ) ) &&
        !viewer.has_effect_with_flag( json_flag_BLIND ) && !viewer.worn_with_flag( flag_BLIND ) ) {
        std::map<std::string, int> names;
        for( const item &value : here.i_at( position ) ) { add_known_items( names, value ); }
        for( const auto &name : names ) {
            result.seen_items.push_back( name.second > 1 ? string_format( _( "%s [%d]" ), name.first, name.second ) : name.first );
        }
    }
    return result;
}

directional_sonar_result scan_directional_sonar( const tripoint_bub_ms &origin )
{



    static constexpr int dx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    static constexpr int dy[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
    directional_sonar_result result;
    map &here = get_map();
    for( int direction = 0; direction < 8; ++direction ) {
        for( int distance = 1; distance <= 16; ++distance ) {
            const tripoint_bub_ms target = origin + tripoint_rel_ms(
                    dx[direction] * distance, dy[direction] * distance, 0 );
            if( !here.inbounds( target ) ) {
                break;
            }
            const unrestricted_tile tile = scan_unrestricted_tile( target );
            if( result.creatures[direction] < 0 && !tile.creatures.empty() ) {
                result.creatures[direction] = distance;
            }
            if( result.items[direction] < 0 && !tile.items.empty() ) {
                result.items[direction] = distance;
            }
            if( result.creatures[direction] > 0 && result.items[direction] > 0 ) {
                break;
            }
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
    const tripoint_abs_ms absolute = here.get_abs( position );
    if( !viewer.has_memory_at( absolute ) ) {
        return result;
    }
    fill_remembered_tile( result, viewer.get_memorized_tile( absolute ) );
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


        if( !observation.furniture.empty() && observation.furniture != observation.terrain ) {
            result += ". " + observation.furniture;
        }
        if( observation.door != door_state::none ) {
            const std::string state = observation.door == door_state::locked ? _( "Locked" ) :
                                      observation.door == door_state::closed ? _( "Closed" ) :
                                      _( "Open" );
            result += ". " + string_format( _( "Door: %s" ), state );
        }
        if( observation.source == observation_source::visible && observation.impassable &&
            observation.door != door_state::closed && observation.door != door_state::locked ) {


            result += ". " + std::string( _( "Impassable" ) );
        }
        for( const auto &field : observation.fields ) { result += ". " + field; }
        for( const auto &trap : observation.known_traps ) { result += ". " + string_format( _( "Trap: %s" ), trap ); }
        for( const auto &creature : observation.seen_creatures ) { result += ". " + creature; }
        for( const auto &perception : observation.special_perceptions ) { result += ". " + perception; }
        if( !observation.vehicle.empty() ) { result += ". " + string_format( _( "Vehicle: %s" ), observation.vehicle ); }
        if( !observation.vehicle_part.empty() ) { result += ". " + string_format( _( "Vehicle part: %s" ), observation.vehicle_part ); }
        for( const auto &item : observation.seen_items ) { result += ". " + item; }
    }
    if( observation.source == observation_source::unknown ) {
        for( const auto &perception : observation.special_perceptions ) { result += ". " + perception; }
    }
    if( !observation.heard_marker.empty() ) {
        result += ". " + string_format( _( "Heard near here: %s. Position approximate." ),
                                         observation.heard_marker );
    }
    return remove_color_tags( result );
}
}
