#pragma once
#ifndef CATA_SRC_A11Y_RADAR_H
#define CATA_SRC_A11Y_RADAR_H

#include <algorithm>
#include <array>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "coords_fwd.h"

namespace a11y
{

// Called only from a world snapshot on the native game thread. The scan is
// read-only and does no terrain generation, movement, or LOS filtering.
void update_automatic_radar( const tripoint_bub_ms &origin );

namespace radar_detail
{
// A key denotes ONE real object, not a name or a compass direction.
// Re-entering after absence is a new event; remaining nearby is silent.
class entry_tracker
{
    public:
        std::set<std::string> entering( const std::set<std::string> &now ) {
            std::set<std::string> fresh;
            for( const std::string &key : now ) {
                if( previous.count( key ) == 0 ) {
                    fresh.insert( key );
                }
            }
            previous = now;
            return fresh;
        }
        void clear() {
            previous.clear();
        }
    private:
        std::set<std::string> previous;
};
// Track meaningful state transitions only for objects present in both scans.
// New/removed objects remain the responsibility of entry_tracker.
class change_tracker
{
    public:
        std::map<std::string, std::string> changed(
            const std::map<std::string, std::string> &now ) {
            std::map<std::string, std::string> result;
            for( const auto &entry : now ) {
                const auto old = previous.find( entry.first );
                if( old != previous.end() && old->second != entry.second ) {
                    result.emplace( entry.first, entry.second );
                }
            }
            previous = now;
            return result;
        }
        void clear() {
            previous.clear();
        }
    private:
        std::map<std::string, std::string> previous;
};


// Each furniture/window map square has a type, but no native object instance ID.
// Join orthogonally touching squares of the SAME type into a spatial group.
// Do not call groups individual pieces of furniture: two touching pieces
// cannot be distinguished from one long piece in the saved map representation.
struct tile_sample {
    std::string type;
    std::string label;
    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = 0;
};

struct tile_group_summary {
    int groups = 0;
    int occupied_tiles = 0;
    int nearest_dx = 0;
    int nearest_dy = 0;
    int nearest_distance = 0;
};

inline std::map<std::string, tile_group_summary> summarize_tile_groups(
    const std::vector<tile_sample> &tiles )
{
    std::map<std::string, tile_group_summary> result;
    std::vector<bool> visited( tiles.size(), false );
    for( std::size_t i = 0; i < tiles.size(); ++i ) {
        if( visited[i] ) {
            continue;
        }
        tile_group_summary &summary = result[tiles[i].label];
        ++summary.groups;
        std::vector<std::size_t> pending{ i };
        visited[i] = true;
        for( std::size_t cursor = 0; cursor < pending.size(); ++cursor ) {
            const tile_sample &current = tiles[pending[cursor]];
            ++summary.occupied_tiles;
            const int distance = std::max( std::abs( current.dx ), std::abs( current.dy ) );
            if( summary.occupied_tiles == 1 || distance < summary.nearest_distance ) {
                summary.nearest_distance = distance;
                summary.nearest_dx = current.dx;
                summary.nearest_dy = current.dy;
            }
            for( std::size_t j = 0; j < tiles.size(); ++j ) {
                if( visited[j] || tiles[j].type != current.type ||
                    tiles[j].label != current.label ) {
                    continue;
                }
                const int adjacent = std::abs( tiles[j].x - current.x ) +
                                     std::abs( tiles[j].y - current.y );
                if( adjacent == 1 ) {
                    visited[j] = true;
                    pending.push_back( j );
                }
            }
        }
    }
    return result;
}

inline bool cardinally_adjacent( const int dx, const int dy )
{
    return std::abs( dx ) + std::abs( dy ) == 1;
}

using count_key = std::pair<unsigned int, std::string>;

// Track totals independently of first-entry announcements: a count of nine
// increasing to ten is an update, even when only one new contact crossed in.
class count_tracker
{
    public:
        std::map<count_key, int> changed( const std::map<count_key, int> &now ) {
            std::map<count_key, int> result;
            for( const auto &entry : now ) {
                const auto it = previous.find( entry.first );
                if( it == previous.end() || it->second != entry.second ) {
                    result.emplace( entry.first, entry.second );
                }
            }
            previous = now;
            return result;
        }
        void clear() {
            previous.clear();
        }
    private:
        std::map<count_key, int> previous;
};

// Geometry contains absolute blocker identities, not only distances: walking
// closer to the same wall must never sound like a newly blocked passage.
struct direction_probe {
    bool complete = false;
    bool blocked = false;
    int distance = 0;
    std::string blocker;
};

struct spatial_frame {
    // Clockwise from north: N, NE, E, SE, S, SW, W, NW.
    std::array<direction_probe, 8> rays{};
    int reachable_tiles = 0;
};

enum class spatial_change_kind { passage_opens, passage_narrows, area_opens, area_narrows };

struct spatial_change {
    spatial_change_kind kind = spatial_change_kind::passage_opens;
    int direction = -1;
    int distance = 0;
};

// Pure comparison of consecutive nearby frames.  Its announcements are scoped
// to the existing Places radar category and do not alter user preferences.
inline std::vector<spatial_change> compare_spatial( const spatial_frame &before,
        const spatial_frame &now )
{
    std::vector<spatial_change> result;
    for( const int direction : { 0, 2, 4, 6 } ) {
        const direction_probe &old = before.rays[direction];
        const direction_probe &current = now.rays[direction];
        if( !old.complete || !current.complete ) {
            continue;
        }
        if( old.blocked && current.blocked && old.blocker == current.blocker ) {
            continue;
        }
        if( old.blocked && current.distance >= old.distance + 3 &&
            current.distance >= 4 ) {
            result.push_back( { spatial_change_kind::passage_opens,
                                direction, current.distance } );
        } else if( current.blocked && old.distance >= current.distance + 3 &&
                   current.distance <= 3 ) {
            result.push_back( { spatial_change_kind::passage_narrows,
                                direction, current.distance } );
        }
    }
    // A connected, nearby walkable area, not all passable tiles through walls.
    // Require both an absolute and relative change to avoid step-by-step chatter.
    if( before.reachable_tiles >= 16 &&
        now.reachable_tiles >= before.reachable_tiles + 12 &&
        now.reachable_tiles * 3 >= before.reachable_tiles * 4 ) {
        result.push_back( { spatial_change_kind::area_opens, -1, 0 } );
    } else if( now.reachable_tiles >= 1 &&
               before.reachable_tiles >= now.reachable_tiles + 12 &&
               now.reachable_tiles * 4 <= before.reachable_tiles * 3 ) {
        result.push_back( { spatial_change_kind::area_narrows, -1, 0 } );
    }
    return result;
}
} // namespace radar_detail

} // namespace a11y
#endif
