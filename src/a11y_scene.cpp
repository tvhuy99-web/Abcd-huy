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

std::vector<point> representative_tiles( const std::vector<point> &tiles,
        const point &player )
{
    std::set<point> remaining( tiles.begin(), tiles.end() );
    std::vector<point> representatives;
    const auto distance = [&player]( const point &p ) {
        return std::max( std::abs( p.x - player.x ), std::abs( p.y - player.y ) );
    };
    while( !remaining.empty() ) {
        const point seed = *remaining.begin();
        remaining.erase( remaining.begin() );
        std::vector<point> queue{ seed };
        point closest = seed;
        for( std::size_t i = 0; i < queue.size(); ++i ) {
            const point current = queue[i];
            if( distance( current ) < distance( closest ) ||
                ( distance( current ) == distance( closest ) && current < closest ) ) {
                closest = current;
            }
            // Only orthogonal neighbors: diagonally touching windows are
            // distinct, and different terrain names are grouped separately.
            for( const point &step : { point::north, point::east, point::south, point::west } ) {
                const point neighbor( current.x + step.x, current.y + step.y );
                if( remaining.erase( neighbor ) != 0 ) {
                    queue.push_back( neighbor );
                }
            }
        }
        representatives.push_back( closest );
    }
    return representatives;
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
};

struct grouped_entry {
    int count = 0;
    tripoint_abs_ms first;
};

std::string sector_name( const int index )
{
    switch( index ) {
        case 0: return _( "north" );
        case 1: return _( "east" );
        case 2: return _( "south" );
        default: return _( "west" );
    }
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
    std::vector<located_entry> doors;
    std::vector<located_entry> windows;
    std::vector<located_entry> stairs;
    std::vector<located_entry> creatures;
    std::vector<located_entry> dangers;
    std::vector<located_entry> vehicles;
    std::map<std::string, grouped_entry> furniture;
    std::set<const vehicle *> seen_vehicles;
    int scanned = 0;
    bool tile_limit_hit = false;
};

void collect_tile( scan_results &result, map &here, const tripoint_bub_ms &p,
                   const tripoint_abs_ms &world )
{
    ++result.scanned;

    if( here.has_flag( ter_furn_flag::TFLAG_DOOR, p ) ) {
        result.doors.push_back( { remove_color_tags( here.tername( p ) ), world } );
    }
    if( here.has_flag_ter( ter_furn_flag::TFLAG_WINDOW, p ) ) {
        result.windows.push_back( { remove_color_tags( here.tername( p ) ), world } );
    }
    if( here.has_flag_ter( ter_furn_flag::TFLAG_GOES_UP, p ) ) {
        result.stairs.push_back( { _( "stairs up" ), world } );
    }
    if( here.has_flag_ter( ter_furn_flag::TFLAG_GOES_DOWN, p ) ) {
        result.stairs.push_back( { _( "stairs down" ), world } );
    }
    if( here.has_furn( p ) ) {
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
}

std::string point_speech( const tripoint_abs_ms &where, const tripoint_abs_ms &player,
                          const std::optional<local_place_coordinates> &local )
{
    if( local ) {
        return string_format( _( "X %1$d, Y %2$d" ),
                              local->x + where.x() - player.x(),
                              local->y + where.y() - player.y() );
    }
    return string_format( _( "world X %1$d, Y %2$d" ), where.x(), where.y() );
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

// Collapse adjoining squares only for the same named structure. Nearest
// square is a deterministic representative, so a three-tile window is read
// once without merging a separate window or a different window state.
std::vector<located_entry> grouped_features( const std::vector<located_entry> &features,
        const tripoint_abs_ms &player )
{
    std::map<std::string, std::vector<located_entry>> by_name;
    for( const located_entry &feature : features ) {
        by_name[feature.label].push_back( feature );
    }
    std::vector<located_entry> result;
    for( const auto &group : by_name ) {
        std::vector<point> squares;
        for( const located_entry &entry : group.second ) {
            squares.emplace_back( entry.position.x(), entry.position.y() );
        }
        for( const point &representative : scene_detail::representative_tiles(
                 squares, point( player.x(), player.y() ) ) ) {
            const auto matching = std::find_if( group.second.begin(), group.second.end(),
            [&representative]( const located_entry &entry ) {
                return entry.position.x() == representative.x &&
                       entry.position.y() == representative.y;
            } );
            if( matching != group.second.end() ) {
                result.push_back( *matching );
            }
        }
    }
    return result;
}

void append_features( std::string &report, const std::string &title,
                      const std::vector<located_entry> &features,
                      const tripoint_abs_ms &player,
                      const std::optional<local_place_coordinates> &local,
                      const std::size_t maximum, const bool group_adjacent = false )
{
    if( features.empty() ) {
        return;
    }
    std::vector<located_entry> listed = group_adjacent ?
                                        grouped_features( features, player ) : features;
    std::stable_sort( listed.begin(), listed.end(),
    [&player]( const located_entry &a, const located_entry &b ) {
        const int da = std::max( std::abs( a.position.x() - player.x() ),
                                 std::abs( a.position.y() - player.y() ) );
        const int db = std::max( std::abs( b.position.x() - player.x() ),
                                 std::abs( b.position.y() - player.y() ) );
        return da < db;
    } );
    report += " " + title + ":";
    for( std::size_t i = 0; i < listed.size() && i < maximum; ++i ) {
        const located_entry &entry = listed[i];
        report += " " + entry.label + ", " +
                  point_speech( entry.position, player, local ) + ".";
    }
    if( listed.size() > maximum ) {
        report += " " + string_format( _( "%d more locations." ),
                                       static_cast<int>( listed.size() - maximum ) );
    }
}

void append_groups( std::string &report, const std::string &title,
                    const std::map<std::string, grouped_entry> &groups,
                    const tripoint_abs_ms &player,
                    const std::optional<local_place_coordinates> &local,
                    const std::size_t maximum )
{
    if( groups.empty() ) {
        return;
    }
    std::vector<std::pair<std::string, grouped_entry>> ordered( groups.begin(), groups.end() );
    std::stable_sort( ordered.begin(), ordered.end(),
    [&player]( const auto &a, const auto &b ) {
        const auto distance = [&player]( const tripoint_abs_ms &at ) {
            return std::max( std::abs( at.x() - player.x() ),
                             std::abs( at.y() - player.y() ) );
        };
        return distance( a.second.first ) < distance( b.second.first );
    } );
    report += " " + title + ":";
    for( std::size_t i = 0; i < ordered.size() && i < maximum; ++i ) {
        report += " " + ordered[i].first + ", " +
                  point_speech( ordered[i].second.first, player, local ) + ".";
    }
    if( ordered.size() > maximum ) {
        report += " " + string_format( _( "%d more types." ),
                                       static_cast<int>( ordered.size() - maximum ) );
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
    std::vector<std::pair<std::string, std::vector<std::string>>> neighbors;
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
        // The survey must not generate unseen overmap data.
        const oter_id &neighbor = overmap_buffer.ter_existing( *next_place );
        if( !neighbor.is_valid() ) {
            continue;
        }
        const std::string place = remove_color_tags(
                                      neighbor->get_name( om_vision_level::full ) );
        const std::string direction = string_format( _( "to the %s" ), sector_name( side ) );
        const auto found = std::find_if( neighbors.begin(), neighbors.end(),
        [&place]( const auto &entry ) {
            return entry.first == place;
        } );
        if( found != neighbors.end() ) {
            found->second.push_back( direction );
        } else {
            neighbors.emplace_back( place, std::vector<std::string>{ direction } );
        }
    }
    if( neighbors.empty() ) {
        return;
    }
    report += " " + std::string( _( "Surroundings:" ) );
    for( const auto &entry : neighbors ) {
        std::string directions;
        for( std::size_t i = 0; i < entry.second.size(); ++i ) {
            if( i == 0 ) {
                directions = entry.second[i];
            } else if( i + 1 == entry.second.size() ) {
                directions = string_format( _( "%1$s and %2$s" ),
                                            directions, entry.second[i] );
            } else {
                directions += ", " + entry.second[i];
            }
        }
        report += " " + string_format( _( "%1$s: %2$s." ), directions, entry.first );
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
        return string_format( _( "Straight %1$s: %2$d tiles." ), heading, walked );
    }
    return string_format( _( "Straight %1$s: at least %2$d tiles." ), heading, walked );
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
    const std::optional<local_place_coordinates> landmark_local =
        site.identified ? local : std::nullopt;

    const oter_id &terrain = overmap_buffer.ter_existing( omt );
    const std::string name = terrain.is_valid() ?
                             remove_color_tags( terrain->get_name( om_vision_level::full ) ) :
                             _( "unknown area" );
    std::string report = string_format( _( "Place survey: %s." ), name );
    const bool bounded_site = site.identified && site.complete;
    const int loaded_width = here.getmapsize() * SEEX;
    const int loaded_height = here.getmapsize() * SEEY;
    const tripoint_abs_ms loaded_northwest = here.get_abs(
                tripoint_bub_ms( 0, 0, player.z() ) );
    const scene_detail::extent loaded_bounds = scene_detail::loaded_envelope(
                loaded_northwest.x(), loaded_northwest.y(), loaded_width, loaded_height );
    const scene_detail::extent bounds = site.identified ? site.envelope : loaded_bounds;

    if( bounded_site ) {
        report += " " + string_format( _( "Area width %1$d east-west tiles, length %2$d north-south tiles." ),
                                       bounds.width(), bounds.height() );
        if( site.floors.size() > 1 ) {
            report += " " + string_format( _( "Map levels: %d." ),
                                           static_cast<int>( site.floors.size() ) );
        }
    }
    if( local && bounded_site ) {
        report += " " + string_format(
                      _( "You are at local X %1$d, Y %2$d, %3$s of the place." ),
                      local->x, local->y, position_in_site( player, bounds ) );
    } else if( local ) {
        report += " " + string_format( _( "You are at local X %1$d, Y %2$d." ),
                                       local->x, local->y );
    } else {
        report += " " + string_format( _( "You are at world X %1$d, Y %2$d, Z %3$d." ),
                                       player.x(), player.y(), player.z() );
    }

    // Only read the location's structural features. Ground items belong to
    // the dedicated item-search commands, never a place survey.
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
                found.tile_limit_hit = true;
                break;
            }
            collect_tile( found, here, p, absolute );
        }
        if( found.tile_limit_hit ) {
            break;
        }
    }

    report += " " + std::string( _( "Paths:" ) );
    for( const point &dir : { point::north, point::east, point::south, point::west } ) {
        report += " " + straight_route( here, center, dir );
    }
    append_features( report, _( "Entrances" ), found.doors, player, landmark_local, 32, true );
    append_features( report, _( "Windows" ), found.windows, player, landmark_local, 48, true );
    append_features( report, _( "Stairs and vertical links" ), found.stairs,
                     player, landmark_local, 24, true );
    append_groups( report, _( "Furniture" ), found.furniture,
                   player, landmark_local, 24 );
    append_features( report, _( "Vehicles" ), found.vehicles,
                     player, landmark_local, 16 );
    append_features( report, _( "Creatures" ), found.creatures,
                     player, landmark_local, 32 );
    append_features( report, _( "Dangers and traces" ), found.dangers,
                     player, landmark_local, 32, true );
    if( found.tile_limit_hit ) {
        report += " " + std::string( _( "Survey limit reached." ) );
    }
    append_adjacent_places( report, site, omt );
    // No repeated orientation recap or spoken "End of survey".
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
