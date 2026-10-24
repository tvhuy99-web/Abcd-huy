#pragma once
#ifndef CATA_SRC_A11Y_WORLD_H
#define CATA_SRC_A11Y_WORLD_H
#include <optional>
#include "a11y_semantics.h"
#include "coords_fwd.h"
#include "coordinates.h"
#include "point.h"
class input_context;
namespace a11y
{
enum class world_scan_filter { all, known, noteworthy, heard };
// explore_cursor, when set, is the explore-mode cursor position relative to
// the avatar. The scan is then centered on the cursor instead of the avatar
// and the snapshot publishes the explore controls; the cursor is purely a
// reading position and never moves the avatar or advances game time.
semantic_snapshot world_snapshot( const input_context &context, int scan_radius, int scan_depth,
                                  world_scan_filter filter,
                                  std::optional<point> camera_offset = std::nullopt,
                                  std::optional<tripoint_rel_ms> explore_cursor = std::nullopt );
semantic_snapshot activity_snapshot( const input_context &context );
// Speak only meaningful transitions, while a live world input scope exists.
void announce_status_changes();
void reset_status_monitor();
} // namespace a11y
#endif // CATA_SRC_A11Y_WORLD_H
