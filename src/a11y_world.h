#pragma once
#ifndef CATA_SRC_A11Y_WORLD_H
#define CATA_SRC_A11Y_WORLD_H
#include <optional>
#include <string>
#include <vector>
#include "a11y_semantics.h"
#include "coordinates.h"
#include "point.h"
class input_context;
namespace a11y
{
enum class world_scan_filter { all, known, noteworthy, heard };

// Read-only, unrestricted tile data shared by both assistive map scans.
// This intentionally examines the loaded map even behind walls.
struct unrestricted_tile {
    std::string terrain;
    std::string furniture;
    std::string vehicle;
    bool impassable = false;
    std::vector<std::string> fields;
    std::vector<std::string> traps;
    std::vector<std::string> creatures;
    std::vector<std::string> items;
};
unrestricted_tile scan_unrestricted_tile( const tripoint_bub_ms &position );
// explore_cursor, when set, is the explore-mode cursor position relative to
// the avatar. The scan is then centered on the cursor instead of the avatar
// and the snapshot publishes the explore controls; the cursor is purely a
// reading position and never moves the avatar or advances game time.
semantic_snapshot world_snapshot( const input_context &context, int scan_radius, int scan_depth,
                                  world_scan_filter filter,
                                  std::optional<point> camera_offset = std::nullopt,
                                  std::optional<tripoint_rel_ms> explore_cursor = std::nullopt );
semantic_snapshot activity_snapshot( const input_context &context );
// Unrestricted, read-only 16-tile accessibility scan. Never moves the avatar or advances time.
void announce_linear_scan( const point &delta );
void announce_player_coordinates();
// Announce a known overmap terrain change (forest, road, town, etc.).
// Monitors the player's absolute overmap tile, not the renderer submap shift.
void announce_area_transition();
// Speak only meaningful transitions, while a live world input scope exists.
void announce_status_changes();
void reset_status_monitor();
// Scope-limited auto-stow for the accessible Fire/weapon-cycle workflows.
// Only stores a held item when the original inventory action permits it.
void set_weapon_auto_stow( bool enabled );
bool weapon_auto_stow_enabled();
} // namespace a11y
#endif // CATA_SRC_A11Y_WORLD_H
