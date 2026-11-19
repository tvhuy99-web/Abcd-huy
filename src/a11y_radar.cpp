#include "a11y_radar.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "a11y_android.h"
#include "a11y_location.h"
#include "avatar.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "field.h"
#include "item.h"
#include "map.h"
#include "mapdata.h"
#include "omdata.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "string_formatter.h"
#include "trap.h"
#include "translations.h"
#include "type_id.h"
#include "vehicle.h"
#include "vpart_position.h"

namespace a11y
{
namespace
{

// These eight masks must agree with ReaderPreferences.RADAR_CHANNEL_*.
// Master enable is encoded by setting the native mask to zero when off.
constexpr unsigned int radar_items = 1u << 0;
constexpr unsigned int radar_monsters = 1u << 1;
constexpr unsigned int radar_places = 1u << 2;
constexpr unsigned int radar_npcs = 1u << 3;
constexpr unsigned int radar_doors = 1u << 4;
constexpr unsigned int radar_hazards = 1u << 5;
constexpr unsigned int radar_vehicles = 1u << 6;
constexpr unsigned int radar_landmarks = 1u << 7;
constexpr int range = 8;

struct radar_contact {
    unsigned int channel = 0;
    std::string key;
    std::string label;
    point offset;
    int distance = 0;
};

radar_detail::entry_tracker tracker;
radar_detail::change_tracker door_states;
radar_detail::change_tracker object_names;
std::optional<radar_detail::spatial_frame> previous_space;
std::optional<tripoint_abs_ms> previous_space_origin;

void clear_change_memory()
{
    door_states.clear();
    object_names.clear();
    previous_space.reset();
    previous_space_origin.reset();
}

std::uint64_t last_lifecycle = 0;
unsigned int last_mask = 0;
unsigned int last_radar_revision = 0;

std::string compass( const point &delta )
{
    if( delta.y < 0 ) {
        return delta.x < 0 ? _( "northwest" ) : delta.x > 0 ? _( "northeast" ) : _( "north" );
    }
    if( delta.y > 0 ) {
        return delta.x < 0 ? _( "southwest" ) : delta.x > 0 ? _( "southeast" ) : _( "south" );
    }
    if( delta.x < 0 ) {
        return _( "west" );
    }
    return delta.x > 0 ? _( "east" ) : _( "here" );
}

std::string coordinate_id( const tripoint_abs_ms &p )
{
    return std::to_string( p.x() ) + ":" + std::to_string( p.y() ) + ":" +
           std::to_string( p.z() );
}

std::string pointer_id( const void *p )
{
    // Native game-thread, in-session object identity. Unlike world position,
    // this stays fixed while a creature or vehicle moves. The cache is reset
    // when the reader or its radar is disabled / lifecycle changes.
    return std::to_string( reinterpret_cast<std::uintptr_t>( p ) );
}

void add( std::map<std::string, radar_contact> &result, const unsigned int channel,
          const std::string &id, const std::string &name, const point &offset )
{
    if( name.empty() ) {
        return;
    }
    const std::string key = std::to_string( channel ) + ":" + id;
    radar_contact contact{ channel, key, remove_color_tags( name ), offset,
                           std::max( std::abs( offset.x ), std::abs( offset.y ) ) };
    const auto found = result.find( key );
    if( found == result.end() || contact.distance < found->second.distance ) {
        result[key] = std::move( contact );
    }
}

void collect_place( std::map<std::string, radar_contact> &out,
                    const tripoint_abs_ms &world, const point &offset )
{
    const tripoint_abs_omt omt = project_to<coords::omt>( world );
    const overmap_with_local_coords owner = overmap_buffer.get_existing_om_global( omt );
    if( !owner.om ) {
        return;
    }
    const std::optional<overmap_special_id> special = owner.om->overmap_special_at( owner.local );
    const oter_id terrain = owner.om->ter( owner.local );
    if( !terrain.is_valid() || ( !special && !terrain->is_road() &&
                                 !terrain->is_highway() ) ) {
        // Ordinary forest, grass and other unbounded terrain must not be
        // mislabeled as one fully identified place.
        return;
    }
    // A placed building, or a road section with a verified local origin,
    // must produce one stable contact across its many 24x24 OMT squares.
    std::string id = special ? special->str() + ":" : "road:";
    const std::optional<local_place_coordinates> local = local_coordinates_at( world );
    if( local ) {
        id += std::to_string( world.x() - local->x + 1 ) + ":" +
              std::to_string( world.y() - local->y + 1 );
    } else if( special ) {
        // A partially known building is identified conservatively by its
        // current OMT rather than merging all same-type buildings.
        id += std::to_string( omt.x() ) + ":" + std::to_string( omt.y() );
    } else {
        // Road boundaries cannot be established from unloaded map data.
        return;
    }
    add( out, radar_places, id, terrain->get_name( om_vision_level::full ), offset );
}


// Gather only loaded tiles. A small flood fill measures *connected* space;
// walls cannot inflate the area by revealing passable rooms behind them.
radar_detail::spatial_frame collect_spatial( map &here, const tripoint_bub_ms &origin )
{
    radar_detail::spatial_frame frame;
    const std::array<point, 8> directions{ {
            point( 0, -1 ), point( 1, -1 ), point( 1, 0 ), point( 1, 1 ),
            point( 0, 1 ), point( -1, 1 ), point( -1, 0 ), point( -1, -1 )
        } };
    for( std::size_t i = 0; i < directions.size(); ++i ) {
        radar_detail::direction_probe &probe = frame.rays[i];
        for( int distance = 1; distance <= range; ++distance ) {
            const point step = directions[i] * distance;
            const tripoint_bub_ms p = origin + tripoint_rel_ms( step.x, step.y, 0 );
            if( !here.inbounds( p ) ) {
                break;
            }
            probe.distance = distance;
            if( here.impassable( p ) ) {
                probe.complete = true;
                probe.blocked = true;
                probe.blocker = coordinate_id( here.get_abs( p ) );
                break;
            }
            if( distance == range ) {
                probe.complete = true;
            }
        }
    }

    constexpr int half = 4;
    constexpr int width = half * 2 + 1;
    std::array<bool, width * width> visited{};
    std::array<point, width * width> pending{};
    visited[half * width + half] = true;
    std::size_t head = 0;
    std::size_t tail = 0;
    pending[tail++] = point( 0, 0 );
    const std::array<point, 4> directions4{ {
            point( 0, -1 ), point( 1, 0 ), point( 0, 1 ), point( -1, 0 )
        } };
    while( head < tail ) {
        const point cursor = pending[head++];
        for( const point &step : directions4 ) {
            const point next = cursor + step;
            if( std::abs( next.x ) > half || std::abs( next.y ) > half ) {
                continue;
            }
            const int index = ( next.y + half ) * width + next.x + half;
            if( visited[index] ) {
                continue;
            }
            // Do not mark an inaccessible neighbor as visited: it might be
            // approached along a different path later in the same scan.
            const tripoint_bub_ms p = origin + tripoint_rel_ms( next.x, next.y, 0 );
            if( !here.inbounds( p ) || here.impassable( p ) ) {
                continue;
            }
            visited[index] = true;
            pending[tail++] = next;
        }
    }
    frame.reachable_tiles = static_cast<int>( tail );
    return frame;
}

std::vector<std::string> describe_spatial_changes(
    const std::vector<radar_detail::spatial_change> &events )
{
    std::vector<std::string> notices;
    const std::array<point, 8> directions{ {
            point( 0, -1 ), point( 1, -1 ), point( 1, 0 ), point( 1, 1 ),
            point( 0, 1 ), point( -1, 1 ), point( -1, 0 ), point( -1, -1 )
        } };
    for( const radar_detail::spatial_change &event : events ) {
        switch( event.kind ) {
            case radar_detail::spatial_change_kind::passage_opens:
                notices.push_back( string_format( _( "Passage opens to the %1$s, %2$d tiles." ),
                                                  compass( directions[event.direction] ),
                                                  event.distance ) );
                break;
            case radar_detail::spatial_change_kind::passage_narrows:
                notices.push_back( string_format( _( "Passage narrows to the %1$s, blocked in %2$d tiles." ),
                                                  compass( directions[event.direction] ),
                                                  event.distance ) );
                break;
            case radar_detail::spatial_change_kind::area_opens:
                notices.emplace_back( _( "The space around you opens up." ) );
                break;
            case radar_detail::spatial_change_kind::area_narrows:
                notices.emplace_back( _( "The space around you narrows." ) );
                break;
        }
    }
    return notices;
}

int sort_rank( const unsigned int category )
{
    if( category == radar_hazards ) { return 0; }
    if( category == radar_monsters ) { return 1; }
    if( category == radar_npcs ) { return 2; }
    if( category == radar_doors ) { return 3; }
    if( category == radar_places ) { return 4; }
    if( category == radar_vehicles ) { return 5; }
    if( category == radar_items ) { return 6; }
    return 7;
}

void speak_new( std::vector<radar_contact> &fresh,
                const std::vector<std::string> &changes )
{
    if( fresh.empty() && changes.empty() ) {
        return;
    }
    // Multiple instances of the same name entering on one game update
    // become a single short sentence, not a stream of interruptions.
    struct grouped {
        unsigned int channel = 0;
        std::string label;
        point nearest;
        int distance = 0;
        int count = 0;
    };
    std::map<std::pair<unsigned int, std::string>, grouped> groups;
    for( const radar_contact &contact : fresh ) {
        const auto key = std::make_pair( contact.channel, contact.label );
        grouped &g = groups[key];
        if( g.count++ == 0 || contact.distance < g.distance ) {
            g.channel = contact.channel;
            g.label = contact.label;
            g.nearest = contact.offset;
            g.distance = contact.distance;
        }
    }
    std::vector<grouped> ordered;
    for( const auto &it : groups ) {
        ordered.push_back( it.second );
    }
    std::sort( ordered.begin(), ordered.end(), []( const grouped &a, const grouped &b ) {
        if( sort_rank( a.channel ) != sort_rank( b.channel ) ) {
            return sort_rank( a.channel ) < sort_rank( b.channel );
        }
        return a.distance < b.distance;
    } );
    const bool urgent = !ordered.empty() && sort_rank( ordered.front().channel ) <= 1;
    // Keep one atomic TTS utterance per snapshot. A busy crossing with many
    // objects has a bounded summary rather than overwhelming the reader.
    std::string spoken;
    constexpr std::size_t max_groups = 12;
    for( std::size_t i = 0; i < ordered.size() && i < max_groups; ++i ) {
        const grouped &g = ordered[i];
        const std::string direction = compass( g.nearest );
        if( !spoken.empty() ) {
            spoken += " ";
        }
        if( g.count == 1 ) {
            spoken += string_format( _( "%1$s to the %2$s, %3$d tiles away." ),
                                     g.label, direction, g.distance );
        } else {
            spoken += string_format( _( "%1$d new %2$s nearby, closest to the %3$s at %4$d tiles." ),
                                     g.count, g.label, direction, g.distance );
        }
    }
    if( ordered.size() > max_groups ) {
        spoken += " " + string_format( _( "%d more new radar groups were combined." ),
                                       static_cast<int>( ordered.size() - max_groups ) );
    }
    // Added state changes share the original single TTS message and urgency.
    // Do not introduce new radar channels, controls or settings.
    constexpr std::size_t max_changes = 3;
    for( std::size_t i = 0; i < changes.size() && i < max_changes; ++i ) {
        if( !spoken.empty() ) {
            spoken += " ";
        }
        spoken += changes[i];
    }
    if( changes.size() > max_changes ) {
        spoken += " " + string_format( _( "%d more environment changes." ),
                                      static_cast<int>( changes.size() - max_changes ) );
    }
    if( !spoken.empty() ) {
        a11y::android::announce( std::string( urgent ? "A11Y_RADAR_DANGER:" :
                                       "A11Y_RADAR_ALERT:" ) + spoken, urgent ? 3 : 1 );
    }
}

} // namespace

void update_automatic_radar( const tripoint_bub_ms &origin )
{
    const unsigned int mask = a11y::android::automatic_radar_mask();
    const std::uint64_t lifecycle = a11y::android::lifecycle_epoch();
    const unsigned int revision = a11y::android::automatic_radar_revision();
    // A quick Off -> On inside the action hub may occur without a world
    // snapshot in between. It must still re-arm every entry detector.
    if( revision != last_radar_revision ) {
        tracker.clear();
        clear_change_memory();
        last_radar_revision = revision;
    }
    if( mask == 0 ) {
        tracker.clear();
        clear_change_memory();
        last_mask = 0;
        last_lifecycle = lifecycle;
        return;
    }
    if( lifecycle != last_lifecycle ) {
        tracker.clear();
        clear_change_memory();
        last_lifecycle = lifecycle;
        last_mask = 0;
    }
    if( mask != last_mask ) {
        if( ( mask ^ last_mask ) & radar_doors ) {
            door_states.clear();
        }
        if( ( mask ^ last_mask ) & ( radar_items | radar_landmarks ) ) {
            object_names.clear();
        }
        if( ( mask ^ last_mask ) & radar_places ) {
            previous_space.reset();
            previous_space_origin.reset();
        }
        last_mask = mask;
    }

    map &here = get_map();
    const avatar &player = get_avatar();
    std::map<std::string, radar_contact> contacts;
    std::map<std::string, std::string> current_door_states;
    std::set<tripoint_abs_omt> examined_places;
    for( int dy = -range; dy <= range; ++dy ) {
        for( int dx = -range; dx <= range; ++dx ) {
            const tripoint_bub_ms p = origin + tripoint_rel_ms( dx, dy, 0 );
            if( !here.inbounds( p ) ) {
                continue;
            }
            const point delta( dx, dy );
            const tripoint_abs_ms world = here.get_abs( p );
            const std::string position_id = coordinate_id( world );
            if( mask & ( radar_monsters | radar_npcs ) ) {
                const Creature *creature = get_creature_tracker().creature_at( p, true );
                if( creature && creature != &player && !creature->is_fake() ) {
                    if( creature->is_npc() && ( mask & radar_npcs ) ) {
                        add( contacts, radar_npcs, pointer_id( creature ),
                             creature->disp_name(), delta );
                    } else if( creature->is_monster() && ( mask & radar_monsters ) ) {
                        add( contacts, radar_monsters, pointer_id( creature ),
                             creature->disp_name(), delta );
                    }
                }
            }
            if( mask & radar_doors ) {
                if( here.has_flag( ter_furn_flag::TFLAG_DOOR, p ) ) {
                    const std::string door_name = here.has_flag_ter(
                                                      ter_furn_flag::TFLAG_DOOR, p ) ?
                                                  here.tername( p ) : here.furnname( p );
                    add( contacts, radar_doors, "door:" + position_id,
                         door_name, delta );
                    const std::string terrain_id = here.ter( p ).id().str();
                    const std::string furniture_id = here.furn( p ).id().str();
                    const bool locked = terrain_id.find( "_locked" ) != std::string::npos ||
                                        furniture_id.find( "_locked" ) != std::string::npos;
                    const std::string state = !here.impassable( p ) ? "open" :
                                              locked ? "locked" : "closed";
                    current_door_states[std::to_string( radar_doors ) + ":door:" +
                                        position_id] = state;
                }
                if( here.has_flag( ter_furn_flag::TFLAG_GOES_UP, p ) ) {
                    add( contacts, radar_doors, "up:" + position_id, _( "Stairs up" ), delta );
                }
                if( here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, p ) ) {
                    add( contacts, radar_doors, "down:" + position_id, _( "Stairs down" ), delta );
                }
            }
            if( mask & radar_hazards ) {
                const trap &hazard = here.tr_at( p );
                if( !hazard.is_null() ) {
                    add( contacts, radar_hazards, "trap:" + position_id + ":" + hazard.name(),
                         hazard.name(), delta );
                }
                for( const auto &field : here.field_at( p ) ) {
                    const std::string label = field.second.name();
                    add( contacts, radar_hazards, "field:" + position_id + ":" + label,
                         label, delta );
                }
            }
            if( mask & radar_items ) {
                for( const item &obj : here.i_at( p ) ) {
                    const std::string id = obj.uid().is_valid() ?
                                           std::to_string( obj.uid().get_value() ) :
                                           position_id + ":" + obj.typeId().str();
                    add( contacts, radar_items, id, obj.tname(), delta );
                }
            }
            if( mask & radar_vehicles ) {
                const optional_vpart_position vehicle_at = here.veh_at( p );
                if( vehicle_at ) {
                    const vehicle &veh = vehicle_at->vehicle();
                    add( contacts, radar_vehicles, pointer_id( &veh ), veh.name, delta );
                }
            }
            if( mask & radar_landmarks ) {
                if( here.has_furn( p ) ) {
                    add( contacts, radar_landmarks, "furn:" + position_id,
                         here.furnname( p ), delta );
                }
                if( here.has_flag( ter_furn_flag::TFLAG_WINDOW, p ) ) {
                    add( contacts, radar_landmarks, "window:" + position_id,
                         here.tername( p ), delta );
                }
            }
            if( mask & radar_places ) {
                const tripoint_abs_omt omt = project_to<coords::omt>( world );
                if( examined_places.insert( omt ).second ) {
                    collect_place( contacts, world, delta );
                }
            }
        }
    }
    // Keep only currently enabled channels in the previous set. In particular
    // toggling a category off must not retain stale contacts of that type.
    std::set<std::string> now;
    for( const auto &entry : contacts ) {
        now.insert( entry.first );
    }
    const std::set<std::string> entering = tracker.entering( now );
    std::vector<radar_contact> fresh;
    for( const std::string &key : entering ) {
        fresh.push_back( contacts.at( key ) );
    }
    std::vector<std::string> changes;

    // The door channel now announces changes without falsely treating a door
    // at the same absolute position as a newly discovered object.
    const auto changed_doors = door_states.changed( current_door_states );
    for( const auto &change : changed_doors ) {
        const auto found = contacts.find( change.first );
        if( found == contacts.end() ) {
            continue;
        }
        const radar_contact &door = found->second;
        const std::string state = change.second == "open" ? _( "open" ) :
                                  change.second == "locked" ? _( "locked" ) : _( "closed" );
        changes.push_back( string_format( _( "Door to the %1$s, %2$d tiles away, is now %3$s." ),
                                          compass( door.offset ), door.distance, state ) );
    }

    // Keep the Places channel's existing discovery behavior; add only brief,
    // movement-driven changes in connected geometry. Skip teleports and level
    // changes so a new area is not mistaken for a newly opened passage.
    if( mask & radar_places ) {
        const tripoint_abs_ms current_origin = here.get_abs( origin );
        const radar_detail::spatial_frame current_space = collect_spatial( here, origin );
        if( previous_space && previous_space_origin ) {
            const int dx = std::abs( current_origin.x() - previous_space_origin->x() );
            const int dy = std::abs( current_origin.y() - previous_space_origin->y() );
            const bool adjacent_move = current_origin.z() == previous_space_origin->z() &&
                                       dx <= 1 && dy <= 1 && ( dx != 0 || dy != 0 );
            if( adjacent_move && changed_doors.empty() ) {
                const auto scene_changes = radar_detail::compare_spatial( *previous_space,
                                           current_space );
                const auto descriptions = describe_spatial_changes( scene_changes );
                changes.insert( changes.end(), descriptions.begin(), descriptions.end() );
            }
        }
        previous_space = current_space;
        previous_space_origin = current_origin;
    }

    // Item and landmark state changes use their existing channels. Newly
    // entering entities are already announced above; only stable IDs whose
    // spoken names changed can produce an additional change notice.
    std::map<std::string, std::string> current_object_names;
    for( const auto &entry : contacts ) {
        if( entry.second.channel == radar_items || entry.second.channel == radar_landmarks ) {
            current_object_names.emplace( entry.first, entry.second.label );
        }
    }
    const auto changed_objects = object_names.changed( current_object_names );
    for( const auto &change : changed_objects ) {
        const auto found = contacts.find( change.first );
        if( found == contacts.end() ) {
            continue;
        }
        const radar_contact &object = found->second;
        changes.push_back( string_format( _( "Object changed: %1$s to the %2$s, %3$d tiles away." ),
                                          object.label, compass( object.offset ),
                                          object.distance ) );
    }

    speak_new( fresh, changes );
}

} // namespace a11y
