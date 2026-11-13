#pragma once
#ifndef CATA_SRC_A11Y_CONTEXT_H
#define CATA_SRC_A11Y_CONTEXT_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace a11y
{

// Opaque identity for one input opportunity. Any stack transition invalidates
// all outstanding tokens, even when the same parent becomes active again.
struct context_token {
    std::uint64_t session = 0;
    std::uint64_t frame = 0;
    std::uint64_t generation = 0;
};

// Game-thread owned. Android callbacks must enqueue data for that thread;
// they must never access this stack or gameplay state directly.
// This is a semantic ownership primitive, not a replacement input_context.
class context_stack
{
    public:
        context_stack();
        context_stack( const context_stack & ) = delete;
        context_stack &operator=( const context_stack & ) = delete;

        std::uint64_t enter( std::string owner, std::string category );
        // Removing a parent also removes its overlays. A delayed destructor
        // for an already removed overlay cannot remove an unrelated new one.
        bool leave( std::uint64_t frame_id );
        // New world/character identity while live RAII owners unwind normally.
        void invalidate_session();
        void set_suspended( bool suspended );
        std::optional<context_token> active_token() const;
        bool accepts( const context_token &token ) const;

    private:
        // Internal frame record. Only the stack itself needs the owner and
        // category; no caller outside the stack observes frames directly.
        struct frame_entry {
            std::uint64_t id;
            std::string owner;
            std::string category;
        };
        std::vector<frame_entry> frames;
        std::uint64_t session;
        std::uint64_t next_frame = 1;
        std::uint64_t generation = 1;
        bool suspended = false;
};

} // namespace a11y

#endif // CATA_SRC_A11Y_CONTEXT_H
