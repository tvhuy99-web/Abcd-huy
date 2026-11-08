#pragma once
#ifndef CATA_SRC_A11Y_RUNTIME_H
#define CATA_SRC_A11Y_RUNTIME_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include "a11y_semantics.h"

namespace a11y
{
using intent_handler = std::function<std::optional<std::string>( const semantic_intent & )>;

bool enabled();
bool suspended();
void synchronize_lifecycle();
void replace_game_session();
bool bound_to( const void *input_owner );
// Activates scopes that were constructed (and published a snapshot) while
// the reader was disabled or suspended, so a screen that is already open
// when the reader becomes available starts serving the reader without
// being closed and reopened (R03.3). Only scopes bound to input_owner are
// considered, and only while the reader is enabled and not suspended.
// Returns true when at least one scope was activated.
bool activate_pending_scopes( const void *input_owner );
std::optional<std::string> poll_input( const void *input_owner );
void announce_game_message( const std::string &text, bool urgent );

// Game-thread scope beside the actual input_context. The pointer is only a
// local binding identity; it is never serialized, retained by Java or used
// as the public ID of a node. Destruction invalidates this frame and overlays.
class screen_scope
{
    public:
        screen_scope( const void *input_owner, std::string category,
                      intent_handler handler );
        ~screen_scope();
        screen_scope( const screen_scope & ) = delete;
        screen_scope &operator=( const screen_scope & ) = delete;
        screen_scope( screen_scope && ) = delete;
        screen_scope &operator=( screen_scope && ) = delete;
        void update( semantic_snapshot snapshot, bool force_revision = false );

    private:
        void activate();
        // Validates and stores a snapshot in this scope's screen record,
        // publishing it when this screen is the active (top) one.
        void apply_snapshot( semantic_snapshot snapshot, bool force_revision );
        friend bool activate_pending_scopes( const void *input_owner );
        std::uint64_t frame = 0;
        const void *owner = nullptr;
        std::string screen_category;
        intent_handler intent_callback;
        // Latest snapshot published while the reader was unavailable and
        // this scope could not activate. Builders that update only once
        // (for example query popups) rely on this cache: when the reader
        // becomes available, activate_pending_scopes() applies it so the
        // already-open screen gains a reader adapter in place (R03.3).
        semantic_snapshot pending_snapshot;
        bool has_pending = false;
};
} // namespace a11y
#endif // CATA_SRC_A11Y_RUNTIME_H
