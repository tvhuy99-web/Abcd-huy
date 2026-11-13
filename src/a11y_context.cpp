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
    [frame_id]( const frame_entry & frame ) {
        return frame.id == frame_id;
    } );
    if( found == frames.end() ) {
        return false;
    }
    frames.erase( found, frames.end() );
    ++generation;
    return true;
}

void context_stack::invalidate_session()
{
    session = session_sequence.fetch_add( 1, std::memory_order_relaxed );
    ++generation;
}

void context_stack::set_suspended( const bool value )
{
    if( suspended != value ) {
        suspended = value;
        ++generation;
    }
}

std::optional<context_token> context_stack::active_token() const
{
    if( suspended || frames.empty() ) {
        return std::nullopt;
    }
    return context_token{ session, frames.back().id, generation };
}

bool context_stack::accepts( const context_token &token ) const
{
    return !suspended && !frames.empty() && token.session == session &&
           token.frame == frames.back().id && token.generation == generation;
}

} // namespace a11y
