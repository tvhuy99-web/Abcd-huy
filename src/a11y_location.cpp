#include "a11y_location.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <vector>

#include "coordinates.h"
#include "omdata.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"

namespace a11y
{
namespace location_detail
{

road_section find_road_section( const point &origin, const road_probe &road_at,
                                const std::size_t max_tiles )
{
    road_section result;
    static const std::array<point, 4> adjacent{ {
            point( 0, -1 ), point( 1, 0 ), point( 0, 1 ), point( -1, 0 )
        } };
    // Cache probes and degrees. A road may extend for hundreds of OMTs, and
    // probing four neighbors repeatedly must not dominate a spoken query.
    std::map<point, std::optional<bool>> known;
    const auto road = [&]( const point &p ) -> std::optional<bool> {
        const auto found = known.find( p );
        if( found != known.end() ) {
            return found->second;
        }
        return known.emplace( p, road_at( p ) ).first->second;
    };
    const auto degree = [&]( const point &p ) -> std::optional<int> {
        int roads = 0;
        for( const point &d : adjacent ) {
            const std::optional<bool> neighbor = road( p + d );
            if( !neighbor ) {
                return std::nullopt;
            }
            if( *neighbor ) {
                ++roads;
            }
        }
        return roads;
    };

    const std::optional<bool> starting_road = road( origin );
    if( !starting_road ) {
        result.complete = false;
        return result;
    }
    if( !*starting_road ) {
        return result;
    }
    const std::optional<int> starting_degree = degree( origin );
    if( !starting_degree ) {
        result.complete = false;
        return result;
    }
    const bool intersection = *starting_degree >= 3;
    std::vector<point> pending{ origin };
    for( std::size_t i = 0; i < pending.size(); ++i ) {
        const point p = pending[i];
        if( result.tiles.count( p ) ) {
            continue;
        }
        const std::optional<bool> is_road = road( p );
        const std::optional<int> n = degree( p );
        if( !is_road || !n ) {
            result.complete = false;
            result.tiles.clear();
            return result;
        }
        if( !*is_road || ( *n >= 3 ) != intersection ) {
            continue;
        }
        if( result.tiles.size() >= max_tiles ) {
            result.complete = false;
            result.tiles.clear();
            return result;
        }
        result.tiles.insert( p );
        for( const point &d : adjacent ) {
            const point next = p + d;
            const std::optional<bool> neighbor = road( next );
            if( !neighbor ) {
                result.complete = false;
                result.tiles.clear();
                return result;
            }
            if( *neighbor && !result.tiles.count( next ) ) {
                pending.push_back( next );
            }
        }
    }
    return result;
}

point local_from_omt_corner( const point &world_ms, const point &northwest_omt )
{
    constexpr int width = coords::map_squares_per( coords::scale::overmap_terrain );
    return point( world_ms.x - northwest_omt.x * width + 1,
                  world_ms.y - northwest_omt.y * width + 1 );
}

} // namespace location_detail

namespace
{

// A placed overmap special has a unique per-placement mapgen_args allocation,
// shared by all OMTs of that placement, including its other z-levels. Checking
// the allocation AND special ID avoids merging adjacent identical houses.
std::optional<point> special_northwest( const tripoint_abs_omt &where )
{
    const overmap_with_local_coords owner = overmap_buffer.get_existing_om_global( where );
    if( !owner.om ) {
        return std::nullopt;
    }
    const std::optional<overmap_special_id> special = owner.om->overmap_special_at( owner.local );
    const auto *arguments = owner.om->mapgen_args( owner.local );
    if( !special || !arguments ) {
        return std::nullopt;
    }
    static const std::array<tripoint_rel_omt, 6> adjacent{ {
            tripoint_rel_omt( -1, 0, 0 ), tripoint_rel_omt( 1, 0, 0 ),
            tripoint_rel_omt( 0, -1, 0 ), tripoint_rel_omt( 0, 1, 0 ),
            tripoint_rel_omt( 0, 0, -1 ), tripoint_rel_omt( 0, 0, 1 )
        } };
    std::set<tripoint_om_omt> visited;
    std::vector<tripoint_om_omt> pending{ owner.local };
    int west = where.x();
    int north = where.y();
    constexpr std::size_t max_special_tiles = 2048;
    for( std::size_t i = 0; i < pending.size(); ++i ) {
        const tripoint_om_omt cell = pending[i];
        if( !overmap::inbounds( cell ) || visited.count( cell ) ||
            owner.om->mapgen_args( cell ) != arguments ||
            owner.om->overmap_special_at( cell ) != special ) {
            continue;
        }
        if( visited.size() >= max_special_tiles ) {
            return std::nullopt;
        }
        visited.insert( cell );
        const tripoint_abs_omt absolute = project_combine( owner.om->pos(), cell );
        west = std::min( west, absolute.x() );
        north = std::min( north, absolute.y() );
        for( const tripoint_rel_omt &delta : adjacent ) {
            pending.push_back( cell + delta );
        }
    }
    return point( west, north );
}

bool is_road_terrain( const oter_id &terrain )
{
    if( !terrain.is_valid() ) {
        return false;
    }
    const oter_travel_cost_type cost = terrain->get_travel_cost_type();
    return terrain->is_road() || terrain->is_highway() ||
           cost == oter_travel_cost_type::road ||
           cost == oter_travel_cost_type::highway ||
           cost == oter_travel_cost_type::dirt_road;
}

std::optional<point> road_northwest( const tripoint_abs_omt &where )
{
    // A road section ends at a real junction. A junction itself is another
    // region, so leaving an intersection does not merge an entire town.
    const auto road_at = [z = where.z()]( const point &p ) -> std::optional<bool> {
        const tripoint_abs_omt cell( p.x, p.y, z );
        const overmap_with_local_coords loc = overmap_buffer.get_existing_om_global( cell );
        if( !loc.om ) {
            return std::nullopt;
        }
        return is_road_terrain( loc.om->ter( loc.local ) );
    };
    const location_detail::road_section section = location_detail::find_road_section(
                point( where.x(), where.y() ), road_at );
    if( !section.complete || section.tiles.empty() ) {
        return std::nullopt;
    }
    int west = where.x();
    int north = where.y();
    for( const point &p : section.tiles ) {
        west = std::min( west, p.x );
        north = std::min( north, p.y );
    }
    return point( west, north );
}

} // namespace

std::optional<local_place_coordinates> local_coordinates_at( const tripoint_abs_ms &pos )
{
    const tripoint_abs_omt omt = project_to<coords::omt>( pos );
    const overmap_with_local_coords owner = overmap_buffer.get_existing_om_global( omt );
    if( !owner.om ) {
        return std::nullopt;
    }

    // Prefer a real placed structure. A multi-OMT house keeps one coordinate
    // origin even after crossing OMT boundaries or loading a saved game.
    std::optional<point> northwest = special_northwest( omt );
    if( !northwest && is_road_terrain( owner.om->ter( owner.local ) ) ) {
        northwest = road_northwest( omt );
    }
    if( !northwest ) {
        // Never fake a local coordinate system for a building with ambiguous
        // ownership, or for a forest/road whose boundary is still unknown.
        return std::nullopt;
    }
    const point local = location_detail::local_from_omt_corner(
                            point( pos.x(), pos.y() ), *northwest );
    return local_place_coordinates{ local.x, local.y };
}

} // namespace a11y
