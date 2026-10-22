#pragma once
#ifndef CATA_SRC_A11Y_WORLD_H
#define CATA_SRC_A11Y_WORLD_H
#include "a11y_semantics.h"
class input_context;
namespace a11y
{
semantic_snapshot world_snapshot( const input_context &context, int scan_radius, int scan_depth );
// Speak only meaningful transitions, while a live world input scope exists.
void announce_status_changes();
void reset_status_monitor();
} // namespace a11y
#endif // CATA_SRC_A11Y_WORLD_H
