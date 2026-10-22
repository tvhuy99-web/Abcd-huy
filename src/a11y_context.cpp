#include "a11y_context.h"

#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <utility>

namespace a11y
{

namespace
{
std::atomic<std::uint64_t> session_sequence{ 1 };
} // namespace

context_stack::context_stack() :
    session( session_sequence.fetch_add( 1, std::memory_order_relaxed ) )
{
}

std::uint64_t context_stack::enter( std::string owner, std::string category )
{
    if( owner.empty() || category.empty() ) {
        throw std::invalid_argument( "Accessibility context needs an owner and category" );
    }
    const std::uint64_t id = next_frame++;
    frames.push_back( { id, std::move( owner ), std::move( category ) } );
    ++generation;
    return id;
}

bool context_stack::leave( const std::uint64_t frame_id )
{
    const auto found = std::find_if( frames.begin(), frames.end(),
    [frame_id]( const context_frame & frame ) {
        return frame.id == frame_id;
    } );
    if( found == frames.end() ) {
        return false;
    }
    frames.erase( found, frames.end() );
    ++generation;
    return true;
}

void context_stack::reset_session()
{
    frames.clear();
    session = session_sequence.fetch_add( 1, std::memory_order_relaxed );
    ++generation;
    // Keep suspension across a load while the app remains in the background.
}

void context_stack::set_suspended( const bool value )
{
    if( suspended != value ) {
        suspended = value;
        ++generation;
    }
}

bool context_stack::is_suspended() const
{
    return suspended;
}

std::optional<context_token> context_stack::active_token() const
{
    if( suspended || frames.empty() ) {
        return std::nullopt;
    }
    return context_token{ session, frames.back().id, generation };
}

std::optional<context_frame> context_stack::active_frame() const
{
    if( suspended || frames.empty() ) {
        return std::nullopt;
    }
    return frames.back();
}

bool context_stack::accepts( const context_token &token ) const
{
    return !suspended && !frames.empty() && token.session == session &&
           token.frame == frames.back().id && token.generation == generation;
}

context_scope::context_scope( context_stack &stack, std::string owner, std::string category ) :
    stack( &stack ), frame( stack.enter( std::move( owner ), std::move( category ) ) )
{
}

context_scope::~context_scope()
{
    if( stack != nullptr ) {
        stack->leave( frame );
    }
}

context_scope::context_scope( context_scope &&other ) noexcept :
    stack( std::exchange( other.stack, nullptr ) ), frame( other.frame )
{
}

} // namespace a11y
