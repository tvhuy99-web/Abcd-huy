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
std::optional<std::string> poll_input( const void *input_owner );

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
        std::uint64_t frame = 0;
};
} // namespace a11y
#endif // CATA_SRC_A11Y_RUNTIME_H
