#pragma once
#ifndef CATA_SRC_A11Y_OUTCOME_H
#define CATA_SRC_A11Y_OUTCOME_H

#include <cstdint>
#include <string>

namespace a11y
{
// Result of the player's last action, recorded at the exact engine branch
// that produced it (R08). Outcomes are never inferred after the fact from
// position deltas alone: a position fallback may only report "moved" or
// "undetermined", never "blocked" (see the RAII guards in avatar_action.cpp
// and game.cpp).
enum class action_outcome_kind {
    none,
    moved,
    opened,
    blocked,
    attacked,
    stance_changed,
    vertical_moved,
    undetermined,
    // Appended at the end so the values of all existing kinds are unchanged.
    safe_mode_blocked
};

struct action_outcome {
    action_outcome_kind kind = action_outcome_kind::none;
    std::string detail;
    std::uint64_t sequence = 0;
};

// Records an outcome for the reader. No-op unless a11y::enabled(); every
// recorded outcome advances sequence by one, so callers holding an older
// sequence can tell an explicit record already happened during their call.
void record_action_outcome( action_outcome_kind kind, std::string detail );
const action_outcome &last_action_outcome();
// Clears the stored outcome (new game session: a loaded or newly created
// character must not inherit the previous character's last result).
void reset_action_outcomes();
// Reader-facing English sentence for an outcome; empty for kind none.
std::string describe_action_outcome( const action_outcome &outcome );
} // namespace a11y
#endif // CATA_SRC_A11Y_OUTCOME_H
