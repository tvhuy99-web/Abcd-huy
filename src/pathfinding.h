#pragma once
#ifndef CATA_SRC_PATHFINDING_H
#define CATA_SRC_PATHFINDING_H

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <unordered_set>
#include <vector>

#include "coordinates.h"
#include "mdarray.h"
#include "point.h"
#include "type_id.h"

class Character;
class map;
enum class creature_size : int;




enum class PathfindingFlag : uint8_t {
    Ground = 0,
    Slow,
    Swimmable,
    Air,
    Unsheltered,
    Obstacle,
    Bashable,
    Impassable,
    Vehicle,
    DangerousField,
    DangerousTrap,
    GoesUp,
    GoesDown,
    RampUp,
    RampDown,
    Climbable,
    Sharp,
    Door,
    InsideDoor,
    LockedDoor,
    Pit,
    DeepWater,
    Burrowable,
    HardGround,
    RestrictTiny,
    RestrictSmall,
    RestrictMedium,
    RestrictLarge,
    RestrictHuge,
    Lava,
};

class PathfindingFlags
{
    public:
        constexpr PathfindingFlags() = default;

        // NOLINTNEXTLINE(google-explicit-constructor)
        constexpr PathfindingFlags( PathfindingFlag flag ) : flags_( uint32_t{ 1 } << static_cast<uint8_t>
                    ( flag ) ) {}

        constexpr void set_union( PathfindingFlags flags ) {
            flags_ |= flags.flags_;
        }
        constexpr void set_intersect( PathfindingFlags flags ) {
            flags_ &= flags.flags_;
        }
        constexpr void set_clear( PathfindingFlags flags ) {
            flags_ &= ~flags.flags_;
        }

        constexpr void clear() {
            flags_ = 0;
        }

        constexpr bool is_set( PathfindingFlag flag ) const {
            return flags_ & ( uint32_t{ 1 } << static_cast<uint8_t>( flag ) );
        }
        constexpr bool is_set( PathfindingFlags flags ) const {
            return ( flags_ & flags.flags_ ) == flags.flags_;
        }

        constexpr bool is_any_set() const {
            return flags_;
        }

        constexpr explicit operator bool() const {
            return is_any_set();
        }

        constexpr PathfindingFlags &operator|=( PathfindingFlags flags ) {
            set_union( flags );
            return *this;
        }

        constexpr PathfindingFlags &operator&=( PathfindingFlags flags ) {
            set_intersect( flags );
            return *this;
        }

    private:
        uint32_t flags_ = 0;
};

constexpr PathfindingFlags operator|( PathfindingFlags lhs, PathfindingFlags rhs )
{
    return lhs |= rhs;
}

constexpr PathfindingFlags operator&( PathfindingFlags lhs, PathfindingFlags rhs )
{
    return lhs &= rhs;
}

constexpr PathfindingFlags operator|( PathfindingFlags lhs, PathfindingFlag rhs )
{
    return lhs |= rhs;
}

constexpr PathfindingFlags operator&( PathfindingFlags lhs, PathfindingFlag rhs )
{
    return lhs &= rhs;
}

constexpr PathfindingFlags operator |( const PathfindingFlag &a, const PathfindingFlag &b )
{
    return PathfindingFlags( a ) | PathfindingFlags( b );
}

struct pathfinding_cache {
    pathfinding_cache();

    bool dirty = false;
    std::unordered_set<point_bub_ms> dirty_points;

    cata::mdarray<PathfindingFlags, point_bub_ms> special;
};

struct pathfinding_settings {
    std::map<damage_type_id, int> bash_strength;
    int max_dist = 0;

    int max_length = 0;



    int climb_cost = 0;

    bool allow_open_doors = false;
    bool allow_unlock_doors = false;
    bool avoid_traps = false;
    bool allow_climb_stairs = true;
    bool avoid_rough_terrain = false;
    bool avoid_sharp = false;
    bool avoid_dangerous_fields = false;

    std::optional<creature_size> size = std::nullopt;

    pathfinding_settings() = default;
    pathfinding_settings( const pathfinding_settings & ) = default;

    pathfinding_settings( const std::map<damage_type_id, int> &bs, int md, int ml, int cc, bool aod,
                          bool aud, bool at, bool acs,
                          bool art, bool as, std::optional<creature_size> sz = std::nullopt )
        : bash_strength( bs ), max_dist( md ), max_length( ml ), climb_cost( cc ),
          allow_open_doors( aod ), allow_unlock_doors( aud ), avoid_traps( at ), allow_climb_stairs( acs ),
          avoid_rough_terrain( art ), avoid_sharp( as ), size( sz )  {}

    pathfinding_settings &operator=( const pathfinding_settings & ) = default;
};

struct pathfinding_target {
    const tripoint_bub_ms center;
    const int r;
    bool contains( const tripoint_bub_ms &p ) const;


    static pathfinding_target point( const tripoint_bub_ms &p ) {
        return { p, 0 };
    }


    static pathfinding_target adjacent( const tripoint_bub_ms &p ) {
        return { p, 1 };
    }


    static pathfinding_target radius( const tripoint_bub_ms &p, int radius ) {
        return { p, radius };
    }
};



bool has_grabbed_single_tile_vehicle( const Character &you, const map &here );








std::vector<tripoint_bub_ms> route_with_grab(
    map &here, const Character &you,
    const pathfinding_target &target,
    const std::function<bool( const tripoint_bub_ms & )> &avoid = {} );

#endif
