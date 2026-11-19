#pragma once
#ifndef CATA_SRC_A11Y_OUTCOME_H
#define CATA_SRC_A11Y_OUTCOME_H

#include <cstdint>
#include <string>

namespace a11y
{





enum class action_outcome_kind {
    none,
    moved,
    opened,
    blocked,
    attacked,
    stance_changed,
    vertical_moved,
    undetermined,

    safe_mode_blocked,
    vertical_unchanged
};

struct action_outcome {
    action_outcome_kind kind = action_outcome_kind::none;
    std::string detail;
    std::uint64_t sequence = 0;
};




void record_action_outcome( action_outcome_kind kind, std::string detail );
const action_outcome &last_action_outcome();


void reset_action_outcomes();

std::string describe_action_outcome( const action_outcome &outcome );
}
#endif
