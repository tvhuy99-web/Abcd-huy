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

struct context_frame {
    std::uint64_t id;
    std::string owner;
    std::string category;
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
        void reset_session();
        void set_suspended( bool suspended );
        bool is_suspended() const;
        std::optional<context_token> active_token() const;
        std::optional<context_frame> active_frame() const;
        bool accepts( const context_token &token ) const;

    private:
        std::vector<context_frame> frames;
        std::uint64_t session;
        std::uint64_t next_frame = 1;
        std::uint64_t generation = 1;
        bool suspended = false;
};

// The owning stack must outlive its scopes. Scopes are intended to sit beside
// the corresponding input_context activation on the game thread.
class context_scope
{
    public:
        context_scope( context_stack &stack, std::string owner, std::string category );
        ~context_scope();
        context_scope( const context_scope & ) = delete;
        context_scope &operator=( const context_scope & ) = delete;
        context_scope( context_scope &&other ) noexcept;
        context_scope &operator=( context_scope &&other ) = delete;

    private:
        context_stack *stack;
        std::uint64_t frame;
};

} // namespace a11y

#endif // CATA_SRC_A11Y_CONTEXT_H
