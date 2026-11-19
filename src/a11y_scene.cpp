#include "a11y_scene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <stdlib.h>
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
#include "map_scale_constants.h"
#include "mapdata.h"
#include "omdata.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "output.h"
#include "point.h"
#include "string_formatter.h"
#include "trap.h"
#include "type_id.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"

namespace a11y
{
namespace scene_detail
{

std::string compass_direction( const point &delta )
{
    if( delta.x == 0 && delta.y == 0 ) {
        return _( "here" );
    }
    if( delta.y < 0 ) {
        return delta.x < 0 ? _( "northwest" ) : delta.x > 0 ? _( "northeast" ) : _( "north" );
    }
    if( delta.y > 0 ) {
        return delta.x < 0 ? _( "southwest" ) : delta.x > 0 ? _( "southeast" ) : _( "south" );
    }
    return delta.x < 0 ? _( "west" ) : _( "east" );
}

extent overmap_envelope( const int west_omt, const int north_omt,
                         const int east_omt, const int south_omt )
{
    constexpr int size = coords::map_squares_per( coords::scale::overmap_terrain );
    return { west_omt * size, north_omt * size,
             ( east_omt + 1 ) * size - 1, ( south_omt + 1 ) * size - 1 };
}

extent loaded_envelope( const int west, const int north, const int width, const int height )
{
    return { west, north, west + width - 1, north + height - 1 };
}

} // namespace scene_detail

namespace
{

constexpr int omt_size = coords::map_squares_per( coords::scale::overmap_terrain );
 // The reality bubble is normally smaller than this limit; the cap is a
// defensive guard for large modded maps. This is called ONLY on activation.
constexpr int maximum_survey_tiles = 20000;
constexpr std::size_t maximum_site_omts = 192;

struct site_region {
    std::set<tripoint_abs_omt> cells;
    std::set<int> floors;
    bool identified = false;
    bool complete = false;
    bool road = false;
    scene_detail::extent envelope;
};

bool is_survey_road( const oter_id &terrain )
{
    if( !terrain.is_valid() ) {
        return false;
    }
    const oter_travel_cost_type travel = terrain->get_travel_cost_type();
    return terrain->is_road() || terrain->is_highway() ||
           travel == oter_travel_cost_type::road ||
           travel == oter_travel_cost_type::highway ||
           travel == oter_travel_cost_type::dirt_road;
}

void finish_region( site_region &region, const tripoint_abs_omt &where )
{
    if( region.cells.empty() ) {
        return;
    }
    int west = where.x();
    int north = where.y();
    int east = where.x();
    int south = where.y();
    for( const tripoint_abs_omt &tile : region.cells ) {
        if( tile.z() != where.z() ) {
            continue;
        }
        west = std::min( west, tile.x() );
        north = std::min( north, tile.y() );
        east = std::max( east, tile.x() );
        south = std::max( south, tile.y() );
    }
    region.envelope = scene_detail::overmap_envelope( west, north, east, south );
    region.identified = true;
}

site_region find_site( const tripoint_abs_omt &where )
{
    site_region result;
    const overmap_with_local_coords owner = overmap_buffer.get_existing_om_global( where );
    if( !owner.om ) {
        return result;
    }
    const std::optional<overmap_special_id> special =
        owner.om->overmap_special_at( owner.local );
    const auto *instance = owner.om->mapgen_args( owner.local );
    if( special && instance ) {
        // The same placement has one mapgen-arguments allocation. Never
        // group neighboring houses just because their special IDs match.
        // Follow other z-levels for floor reporting, not for tile scanning.
        static const std::array<tripoint_rel_omt, 6> neighbors{ {
                tripoint_rel_omt( -1, 0, 0 ), tripoint_rel_omt( 1, 0, 0 ),
                tripoint_rel_omt( 0, -1, 0 ), tripoint_rel_omt( 0, 1, 0 ),
                tripoint_rel_omt( 0, 0, -1 ), tripoint_rel_omt( 0, 0, 1 )
            } };
        std::set<tripoint_abs_omt> visited;
        std::vector<tripoint_abs_omt> queue{ where };
        bool clipped = false;
        for( std::size_t i = 0; i < queue.size(); ++i ) {
            const tripoint_abs_omt current = queue[i];
            if( !visited.insert( current ).second ) {
                continue;
            }
            const overmap_with_local_coords part =
                overmap_buffer.get_existing_om_global( current );
            if( !part.om ) {
                clipped = true;
                continue;
            }
            // Pointers are meaningful only within the owning overmap.
            // A cross-overmap special cannot be identified by this link;
            // the scan must then be reported as a partial site.
            if( part.om != owner.om ) {
                if( part.om->overmap_special_at( part.local ) == special ) {
                    clipped = true;
                }
                continue;
            }
            if( part.om->overmap_special_at( part.local ) != special ||
                part.om->mapgen_args( part.local ) != instance ) {
                continue;
            }
            if( result.cells.size() >= maximum_site_omts ) {
                clipped = true;
                break;
            }
            result.cells.insert( current );
            result.floors.insert( current.z() );
            for( const tripoint_rel_omt &step : neighbors ) {
                queue.push_back( current + step );
            }
        }
        if( !result.cells.empty() ) {
            finish_region( result, where );
            result.complete = !clipped;
            return result;
        }
    }

    if( !is_survey_road( owner.om->ter( owner.local ) ) ) {
        return result;
    }
    const auto road_at = [z = where.z()]( const point &p ) -> std::optional<bool> {
        const overmap_with_local_coords probe = overmap_buffer.get_existing_om_global(
                    tripoint_abs_omt( p.x, p.y, z ) );
        if( !probe.om ) {
            return std::nullopt;
        }
        return is_survey_road( probe.om->ter( probe.local ) );
    };
    const location_detail::road_section segment = location_detail::find_road_section(
                point( where.x(), where.y() ), road_at, maximum_site_omts );
    if( !segment.complete || segment.tiles.empty() ) {
        return result;
    }
    for( const point &p : segment.tiles ) {
        result.cells.insert( tripoint_abs_omt( p.x, p.y, where.z() ) );
    }
    result.road = true;
    result.complete = true;
    result.floors.insert( where.z() );
    finish_region( result, where );
    return result;
}

struct located_entry {
    std::string label;
    tripoint_abs_ms position;
    bool outside_entrance = false;
    bool blocked = false;
};

struct grouped_entry {
    int count = 0;
    tripoint_abs_ms first;
};

struct sector_counts {
    int walls = 0;
    int doors = 0;
    int obstacles = 0;
    int furniture = 0;
    int open = 0;
};

int sector_of( const int x, const int y, const scene_detail::extent &bounds )
{
    const int dx = 2 * x - bounds.west - bounds.east;
    const int dy = 2 * y - bounds.north - bounds.south;
    if( std::abs( dx ) > std::abs( dy ) ) {
        return dx >= 0 ? 1 : 3; // E, W
    }
    return dy >= 0 ? 2 : 0; // S, N
}

std::string sector_name( const int index )
{
    switch( index ) {
        case 0: return _( "north" );
        case 1: return _( "east" );
        case 2: return _( "south" );
        default: return _( "west" );
    }
}

bool outside_door( map &here, const tripoint_bub_ms &p )
{
    bool indoor = false;
    bool outdoor = false;
    for( const point &step : { point::north, point::east, point::south, point::west } ) {
        const tripoint_bub_ms next = p + tripoint_rel_ms( step.x, step.y, 0 );
        if( !here.inbounds( next ) ) {
            continue;
        }
        if( here.is_outside( next ) ) {
            outdoor = true;
        } else {
            indoor = true;
        }
    }
    return indoor && outdoor;
}

template<typename T>
void count_entry( std::map<std::string, grouped_entry> &groups, const std::string &name,
                  const tripoint_abs_ms &at, const T &value )
{
    if( name.empty() ) {
        return;
    }
    grouped_entry &entry = groups[name];
    if( entry.count == 0 ) {
        entry.first = at;
    }
    entry.count += value;
}

struct scan_results {
    std::array<sector_counts, 4> sectors{};
    std::vector<located_entry> doors;
    std::vector<located_entry> windows;
    std::vector<located_entry> stairs;
    std::vector<located_entry> creatures;
    std::vector<located_entry> dangers;
    std::vector<located_entry> vehicles;
    std::map<std::string, grouped_entry> furniture;
    std::map<std::string, grouped_entry> items;
    std::set<const vehicle *> seen_vehicles;
    int walls = 0;
    int passable = 0;
    int indoor = 0;
    int outdoor = 0;
    int scanned = 0;
    int item_stacks = 0;
    bool limit_hit = false;
    bool has_inside_bounds = false;
    scene_detail::extent inside_bounds;
};

void collect_tile( scan_results &result, map &here, const tripoint_bub_ms &p,
                   const tripoint_abs_ms &world, const scene_detail::extent &bounds )
{
    const int side = sector_of( world.x(), world.y(), bounds );
    sector_counts &group = result.sectors[side];
    result.scanned++;

    const bool wall = here.has_flag_ter( ter_furn_flag::TFLAG_WALL, p );
    const bool door = here.has_flag( ter_furn_flag::TFLAG_DOOR, p );
    const bool window = here.has_flag_ter( ter_furn_flag::TFLAG_WINDOW, p );
    const bool passable = !here.impassable( p );
    const bool outside = here.is_outside( p );
    result.outdoor += outside;
    result.indoor += !outside;
    result.passable += passable;
    if( wall ) {
        group.walls++;
        result.walls++;
    } else if( !passable && !door ) {
        group.obstacles++;
    } else if( passable && !door ) {
        group.open++;
    }
    if( !outside ) {
        if( !result.has_inside_bounds ) {
            result.inside_bounds = { world.x(), world.y(), world.x(), world.y() };
            result.has_inside_bounds = true;
        } else {
            result.inside_bounds.west = std::min( result.inside_bounds.west, world.x() );
            result.inside_bounds.north = std::min( result.inside_bounds.north, world.y() );
            result.inside_bounds.east = std::max( result.inside_bounds.east, world.x() );
            result.inside_bounds.south = std::max( result.inside_bounds.south, world.y() );
        }
    }
    if( door ) {
        group.doors++;
        result.doors.push_back( { remove_color_tags( here.tername( p ) ), world,
                                  outside_door( here, p ), !passable } );
    }
    if( window ) {
        result.windows.push_back( { remove_color_tags( here.tername( p ) ), world } );
    }
    if( here.has_flag_ter( ter_furn_flag::TFLAG_GOES_UP, p ) ) {
        result.stairs.push_back( { _( "stairs up" ), world } );
    }
    if( here.has_flag_ter( ter_furn_flag::TFLAG_GOES_DOWN, p ) ) {
        result.stairs.push_back( { _( "stairs down" ), world } );
    }
    if( here.has_furn( p ) ) {
        group.furniture++;
        count_entry( result.furniture, remove_color_tags( here.furnname( p ) ), world, 1 );
    }
    if( const Creature *creature = get_creature_tracker().creature_at( p, true ) ) {
        if( creature != &get_avatar() ) {
            result.creatures.push_back( { remove_color_tags( creature->disp_name() ), world } );
        }
    }
    const trap &hazard = here.tr_at( p );
    if( !hazard.is_null() ) {
        result.dangers.push_back( { remove_color_tags( hazard.name() ), world } );
    }
    for( const auto &field_at : here.field_at( p ) ) {
        result.dangers.push_back( { remove_color_tags( field_at.second.name() ), world } );
    }
    const optional_vpart_position vehicle_part = here.veh_at( p );
    if( vehicle_part ) {
        vehicle &veh = vehicle_part->vehicle();
        if( result.seen_vehicles.insert( &veh ).second ) {
            result.vehicles.push_back( { remove_color_tags( veh.name ), world } );
        }
    }
    // Preserve counts and the first location; a shelf of 80 identical cans
    // is one meaningful summary entry rather than 80 spoken repetitions.
    if( result.item_stacks < 10000 ) {
        for( const item &obj : here.i_at( p ) ) {
            if( ++result.item_stacks > 10000 ) {
                result.limit_hit = true;
                break;
            }
            count_entry( result.items, remove_color_tags( obj.tname() ), world, 1 );
        }
    }
}

std::string point_speech( const tripoint_abs_ms &where, const tripoint_abs_ms &player,
                          const std::optional<local_place_coordinates> &local )
{
    const point offset( where.x() - player.x(), where.y() - player.y() );
    const int distance = std::max( std::abs( offset.x ), std::abs( offset.y ) );
    std::string at;
    if( local ) {
        at = string_format( _( "X%1$d, Y%2$d" ),
                            local->x + offset.x, local->y + offset.y );
    } else {
        at = string_format( _( "world X%1$d, Y%2$d" ), where.x(), where.y() );
    }
    if( distance == 0 ) {
        return string_format( _( "%s, at your position" ), at );
    }
    return string_format( _( "%1$s, %2$s, %3$d tiles by position" ), at,
                          scene_detail::compass_direction( offset ), distance );
}

std::string position_in_site( const tripoint_abs_ms &where,
                              const scene_detail::extent &bounds )
{
    const int width = std::max( 1, bounds.width() );
    const int height = std::max( 1, bounds.height() );
    const int from_left = where.x() - bounds.west;
    const int from_top = where.y() - bounds.north;
    const bool north = from_top * 3 < height;
    const bool south = from_top * 3 >= height * 2;
    const bool west = from_left * 3 < width;
    const bool east = from_left * 3 >= width * 2;
    if( north && west ) { return _( "near the northwest" ); }
    if( north && east ) { return _( "near the northeast" ); }
    if( south && west ) { return _( "near the southwest" ); }
    if( south && east ) { return _( "near the southeast" ); }
    if( north ) { return _( "toward the north" ); }
    if( south ) { return _( "toward the south" ); }
    if( west ) { return _( "toward the west" ); }
    if( east ) { return _( "toward the east" ); }
    return _( "near the center" );
}

void append_features( std::string &report, const std::string &title,
                      std::vector<located_entry> &features,
                      const tripoint_abs_ms &player,
                      const std::optional<local_place_coordinates> &local,
                      const std::size_t maximum )
{
    if( features.empty() ) {
        report += " " + string_format( _( "%s: none found." ), title );
        return;
    }
    std::stable_sort( features.begin(), features.end(),
    [&player]( const located_entry &a, const located_entry &b ) {
        const int da = std::max( std::abs( a.position.x() - player.x() ),
                                 std::abs( a.position.y() - player.y() ) );
        const int db = std::max( std::abs( b.position.x() - player.x() ),
                                 std::abs( b.position.y() - player.y() ) );
        return da < db;
    } );
    report += " " + string_format( _( "%1$s: %2$d found." ),
                                   title, static_cast<int>( features.size() ) );
    for( std::size_t i = 0; i < features.size() && i < maximum; ++i ) {
        const located_entry &entry = features[i];
        std::string detail = entry.label + ", " + point_speech( entry.position, player, local );
        if( entry.outside_entrance ) {
            detail += ", " + std::string( _( "connects inside and outside" ) );
        }
        if( entry.blocked ) {
            detail += ", " + std::string( _( "currently obstructs movement" ) );
        }
        report += " " + detail + ".";
    }
    if( features.size() > maximum ) {
        report += " " + string_format( _( "%d more positions were grouped to keep the reading manageable." ),
                                       static_cast<int>( features.size() - maximum ) );
    }
}

void append_groups( std::string &report, const std::string &title,
                    const std::map<std::string, grouped_entry> &groups,
                    const tripoint_abs_ms &player,
                    const std::optional<local_place_coordinates> &local,
                    const std::size_t maximum )
{
    if( groups.empty() ) {
        report += " " + string_format( _( "%s: none found." ), title );
        return;
    }
    std::vector<std::pair<std::string, grouped_entry>> ordered( groups.begin(), groups.end() );
    std::stable_sort( ordered.begin(), ordered.end(),
    []( const auto &a, const auto &b ) {
        return a.second.count > b.second.count;
    } );
    report += " " + string_format( _( "%1$s: %2$d types." ),
                                   title, static_cast<int>( ordered.size() ) );
    for( std::size_t i = 0; i < ordered.size() && i < maximum; ++i ) {
        const auto &entry = ordered[i];
        report += " " + string_format( _( "%1$s, %2$d positions or stacks, first at %3$s." ),
                                       entry.first, entry.second.count,
                                       point_speech( entry.second.first, player, local ) );
    }
    if( ordered.size() > maximum ) {
        report += " " + string_format( _( "%d additional types were counted but not listed individually." ),
                                       static_cast<int>( ordered.size() - maximum ) );
    }
}

void append_nearest( std::string &report, const std::string &heading,
                     const std::vector<located_entry> &features,
                     const tripoint_abs_ms &player,
                     const std::optional<local_place_coordinates> &local,
                     const bool exterior_only )
{
    const located_entry *nearest = nullptr;
    int closest = 0;
    for( const located_entry &feature : features ) {
        if( exterior_only && !feature.outside_entrance ) {
            continue;
        }
        const int distance = std::max( std::abs( feature.position.x() - player.x() ),
                                       std::abs( feature.position.y() - player.y() ) );
        if( !nearest || distance < closest ) {
            nearest = &feature;
            closest = distance;
        }
    }
    if( nearest ) {
        report += " " + string_format( _( "%1$s: %2$s, %3$s." ),
                                       heading, nearest->label,
                                       point_speech( nearest->position, player, local ) );
    }
}

void append_adjacent_places( std::string &report, const site_region &site,
                             const tripoint_abs_omt &player )
{
    if( !site.identified || !site.complete ) {
        return;
    }
    const std::array<point, 4> cardinal{ {
            point::north, point::east, point::south, point::west
        } };
    for( int side = 0; side < 4; ++side ) {
        const point step = cardinal[side];
        std::optional<tripoint_abs_omt> next_place;
        int best_distance = 0;
        for( const tripoint_abs_omt &cell : site.cells ) {
            if( cell.z() != player.z() ) {
                continue;
            }
            const tripoint_abs_omt candidate( cell.x() + step.x, cell.y() + step.y, cell.z() );
            if( site.cells.count( candidate ) ) {
                continue;
            }
            const int distance = std::abs( cell.x() - player.x() ) +
                                 std::abs( cell.y() - player.y() );
            if( !next_place || distance < best_distance ) {
                next_place = candidate;
                best_distance = distance;
            }
        }
        if( !next_place ) {
            continue;
        }
        // Never call overmap_buffer.ter(), which could generate a new
        // overmap as a side effect of a spoken-only survey.
        const oter_id &neighbor = overmap_buffer.ter_existing( *next_place );
        if( neighbor.is_valid() ) {
            report += " " + string_format( _( "Adjacent map area to the %1$s: %2$s." ),
                                           sector_name( side ),
                                           remove_color_tags( neighbor->get_name( om_vision_level::full ) ) );
        }
    }
}

std::string straight_route( map &here, const tripoint_bub_ms &origin,
                            const point &dir )
{
    constexpr int max_straight_steps = 30;
    int walked = 0;
    bool stopped = false;
    for( int i = 1; i <= max_straight_steps; ++i ) {
        const tripoint_bub_ms p = origin +
                                  tripoint_rel_ms( dir.x * i, dir.y * i, 0 );
        if( !here.inbounds( p ) ) {
            break;
        }
        if( here.impassable( p ) ) {
            stopped = true;
            break;
        }
        ++walked;
    }
    const std::string heading = scene_detail::compass_direction( dir );
    if( stopped ) {
        return string_format( _( "Straight %1$s: %2$d clear tiles before an obstruction." ),
                              heading, walked );
    }
    return string_format( _( "Straight %1$s: at least %2$d clear tiles in the loaded map." ),
                          heading, walked );
}

std::string build_survey()
{
    map &here = get_map();
    const avatar &viewer = get_avatar();
    const tripoint_abs_ms player = viewer.pos_abs();
    const tripoint_bub_ms center = viewer.pos_bub();
    const tripoint_abs_omt omt = viewer.pos_abs_omt();
    const site_region site = find_site( omt );
    const std::optional<local_place_coordinates> local = local_coordinates_at( player );

    const oter_id &terrain = overmap_buffer.ter_existing( omt );
    const std::string name = terrain.is_valid() ?
                             remove_color_tags( terrain->get_name( om_vision_level::full ) ) :
                             _( "unknown area" );
    std::string report = string_format( _( "Detailed place survey. %s." ), name );
    const bool bounded_site = site.identified && site.complete;
    // Scan the whole loaded reality bubble. A radius centered on the avatar
    // could leave out valid map squares near the bubble's opposite edge.
    const int loaded_width = here.getmapsize() * SEEX;
    const int loaded_height = here.getmapsize() * SEEY;
    const tripoint_abs_ms loaded_northwest = here.get_abs(
                tripoint_bub_ms( 0, 0, player.z() ) );
    const scene_detail::extent loaded_bounds = scene_detail::loaded_envelope(
                loaded_northwest.x(), loaded_northwest.y(), loaded_width, loaded_height );
    const scene_detail::extent bounds = site.identified ? site.envelope : loaded_bounds;
    if( bounded_site ) {
        if( site.road ) {
            report += " " + std::string( _( "This is one recognized road section; nearby buildings are separate landmarks." ) );
        }
        report += " " + string_format(
                      _( "The mapped place spans a bounding frame of %1$d east-west by %2$d north-south tiles. This is the map frame, not necessarily the outer wall dimensions." ),
                      bounds.width(), bounds.height() );
        if( site.floors.size() > 1 ) {
            report += " " + string_format( _( "%d connected map levels were identified; this survey covers your current level." ),
                                           static_cast<int>( site.floors.size() ) );
        }
    } else {
        report += " " + std::string( _( "The entire place boundary is not established. Describing the loaded area around you only; no complete building or forest dimensions are claimed." ) );
    }
    if( local && bounded_site ) {
        report += " " + string_format( _( "You are at local X%1$d, Y%2$d, %3$s of this place." ),
                                       local->x, local->y, position_in_site( player, bounds ) );
    } else if( local ) {
        report += " " + string_format( _( "You are at local X%1$d, Y%2$d. The full place boundaries have not been verified." ),
                                       local->x, local->y );
    } else {
        report += " " + string_format( _( "You are at world X%1$d, Y%2$d, Z%3$d. Local coordinates are unavailable for this place." ),
                                       player.x(), player.y(), player.z() );
    }
    report += " " + std::string( _( "Extended survey reads the loaded map beyond line of sight, including behind walls." ) );

    scan_results found;
    for( int y = 0; y < loaded_height; ++y ) {
        for( int x = 0; x < loaded_width; ++x ) {
            const tripoint_bub_ms p( x, y, player.z() );
            if( !here.inbounds( p ) ) {
                continue;
            }
            const tripoint_abs_ms absolute = here.get_abs( p );
            if( site.identified &&
                !site.cells.count( project_to<coords::omt>( absolute ) ) ) {
                continue;
            }
            if( found.scanned >= maximum_survey_tiles ) {
                found.limit_hit = true;
                break;
            }
            collect_tile( found, here, p, absolute, bounds );
        }
        if( found.limit_hit && found.scanned >= maximum_survey_tiles ) {
            break;
        }
    }
    const std::size_t floor_omts = static_cast<std::size_t>( std::count_if(
                                       site.cells.begin(), site.cells.end(),
    [&omt]( const tripoint_abs_omt &cell ) { return cell.z() == omt.z(); } ) );
    const std::size_t region_tiles = floor_omts * static_cast<std::size_t>( omt_size * omt_size );
    const bool full_loaded = bounded_site &&
                             static_cast<std::size_t>( found.scanned ) == region_tiles &&
                             !found.limit_hit;
    report += " " + string_format( _( "Map squares examined: %d." ), found.scanned );
    if( !full_loaded ) {
        report += " " + std::string( _( "Only part of the location is currently loaded or within the survey boundary. Counts below apply only to examined tiles." ) );
    }

    report += " " + std::string( _( "General structure." ) );
    report += " " + string_format( _( "%1$d wall tiles, %2$d passable tiles, %3$d roofed or inside tiles, and %4$d outside tiles." ),
                                   found.walls, found.passable, found.indoor, found.outdoor );
    if( found.has_inside_bounds ) {
        report += " " + string_format( _( "The observed covered space spans a frame of %1$d east-west by %2$d north-south tiles; this does not prove the full building perimeter." ),
                                       found.inside_bounds.width(), found.inside_bounds.height() );
    }
    for( int side = 0; side < 4; ++side ) {
        const sector_counts &c = found.sectors[side];
        report += " " + string_format(
                      _( "%1$s portion: %2$d walls, %3$d doors, %4$d other obstacles, %5$d furniture locations, %6$d open traversable tiles." ),
                      sector_name( side ), c.walls, c.doors, c.obstacles, c.furniture, c.open );
    }

    report += " " + std::string( _( "Straight directions from your position. These are direct lines, not a path around walls." ) );
    for( const point &dir : { point::north, point::east, point::south, point::west } ) {
        report += " " + straight_route( here, center, dir );
    }

    report += " " + std::string( _( "Doors, windows, stairs and routes." ) );
    append_features( report, _( "Doors" ), found.doors, player, local, 30 );
    append_features( report, _( "Windows" ), found.windows, player, local, 12 );
    append_features( report, _( "Stairs and vertical links" ), found.stairs, player, local, 20 );

    report += " " + std::string( _( "Furniture, vehicles, and noteworthy items." ) );
    append_groups( report, _( "Furniture" ), found.furniture, player, local, 18 );
    append_features( report, _( "Vehicles" ), found.vehicles, player, local, 16 );
    append_groups( report, _( "Item types" ), found.items, player, local, 22 );

    report += " " + std::string( _( "Creatures and dangers detected by extended survey." ) );
    append_features( report, _( "Creatures" ), found.creatures, player, local, 40 );
    append_features( report, _( "Traps and fields" ), found.dangers, player, local, 40 );
    if( found.limit_hit ) {
        report += " " + std::string( _( "A safety scan limit was reached. The unlisted portion was not examined." ) );
    }

    report += " " + std::string( _( "Orientation summary." ) );
    if( bounded_site ) {
        report += " " + string_format( _( "Your position is %s of the mapped place." ),
                                       position_in_site( player, bounds ) );
    } else {
        report += " " + std::string( _( "This is a local survey, not a verified whole-place outline." ) );
    }
    report += " " + string_format( _( "%1$d doors and %2$d stair links were detected in the scanned tiles." ),
                                   static_cast<int>( found.doors.size() ),
                                   static_cast<int>( found.stairs.size() ) );
    append_nearest( report, _( "Nearest exterior door in the surveyed map" ),
                    found.doors, player, local, true );
    append_nearest( report, _( "Nearest detected stairway" ),
                    found.stairs, player, local, false );
    append_nearest( report, _( "Nearest detected creature" ),
                    found.creatures, player, local, false );
    append_adjacent_places( report, site, omt );
    report += " " + std::string( _( "Distances to features are map-position distances, not guaranteed walking routes. End of survey." ) );
    return report;
}

} // namespace

void announce_detailed_place_survey()
{
    // Deliberate, read-only world query. Keep Android's existing one-request
    // TTS behavior (including its ordinary stop/restart gestures); no new UI,
    // timer, pause button, or per-turn scanning is introduced.
    a11y::android::announce( "A11Y_SCENE:" + build_survey(), 1 );
}

} // namespace a11y
