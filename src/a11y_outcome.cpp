#include "a11y_outcome.h"

#include <utility>

#include "a11y_android.h"
#include "a11y_runtime.h"
#include "string_formatter.h"
#include "translations.h"

namespace a11y
{
namespace
{
// Game thread only, like the rest of the reader's game-side state.
action_outcome last_outcome;
} // namespace

void record_action_outcome( action_outcome_kind kind, std::string detail )
{
    if( !enabled() ) {
        return;
    }
    action_outcome outcome;
    outcome.kind = kind;
    outcome.detail = std::move( detail );
    outcome.sequence = last_outcome.sequence + 1;
    last_outcome = std::move( outcome );
    if( kind == action_outcome_kind::blocked ||
        kind == action_outcome_kind::safe_mode_blocked ) {
        // The blocked-step cue duplicates this outcome, which the world
        // snapshot already speaks; the blocker was classified only from
        // what the character could perceive (R21.2).
        android::cue( "blocked" );
    }
}

const action_outcome &last_action_outcome()
{
    return last_outcome;
}

void reset_action_outcomes()
{
    last_outcome = action_outcome{};
}

std::string describe_action_outcome( const action_outcome &outcome )
{
    switch( outcome.kind ) {
        case action_outcome_kind::none:
            return std::string();
        case action_outcome_kind::moved:
            return _( "Moved." );
        case action_outcome_kind::opened:
            return string_format( _( "Opened the %s." ), outcome.detail );
        case action_outcome_kind::blocked:
            return string_format( _( "Blocked by %s." ), outcome.detail );
        case action_outcome_kind::attacked:
            return string_format( _( "Attacked %s." ), outcome.detail );
        case action_outcome_kind::stance_changed:
            return string_format( _( "Movement mode: %s." ), outcome.detail );
        case action_outcome_kind::vertical_moved:
            return _( "Moved to another level." );
        case action_outcome_kind::undetermined:
            return _( "The last action's result could not be determined." );
        case action_outcome_kind::safe_mode_blocked:
            return string_format( _( "Safe mode blocked the attack on %s." ), outcome.detail );
    }
    return std::string();
}
} // namespace a11y
