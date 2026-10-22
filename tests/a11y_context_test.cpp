#include <stdexcept>
#include <utility>

#include "a11y_context.h"
#include "cata_catch.h"

TEST_CASE( "a11y_only_active_overlay_accepts_input", "[a11y][context]" )
{
    a11y::context_stack stack;
    CHECK_FALSE( stack.active_token() );
    CHECK_FALSE( stack.accepts( {} ) );
    const auto world = stack.enter( "world", "DEFAULTMODE" );
    const auto world_token = stack.active_token().value();
    REQUIRE( stack.accepts( world_token ) );
    const auto menu = stack.enter( "inventory", "INVENTORY" );
    const auto menu_token = stack.active_token().value();
    CHECK_FALSE( stack.accepts( world_token ) );
    CHECK( stack.accepts( menu_token ) );
    REQUIRE( stack.leave( menu ) );
    CHECK_FALSE( stack.accepts( menu_token ) );
    // Returning to world must not replay the gesture queued before the menu.
    CHECK_FALSE( stack.accepts( world_token ) );
    CHECK( stack.accepts( stack.active_token().value() ) );
    CHECK( stack.active_frame()->id == world );
}

TEST_CASE( "a11y_out_of_order_close_cannot_leave_orphan_input", "[a11y][context]" )
{
    a11y::context_stack stack;
    stack.enter( "world", "DEFAULTMODE" );
    const auto menu = stack.enter( "inventory", "INVENTORY" );
    const auto prompt = stack.enter( "count", "STRING_INPUT" );
    const auto stale = stack.active_token().value();
    REQUIRE( stack.leave( menu ) );
    CHECK_FALSE( stack.accepts( stale ) );
    const auto replacement = stack.enter( "dialogue", "DIALOGUE" );
    const auto current = stack.active_token().value();
    CHECK_FALSE( stack.leave( prompt ) );
    CHECK( stack.active_frame()->id == replacement );
    CHECK( stack.accepts( current ) );
}

TEST_CASE( "a11y_background_and_resume_invalidate_callbacks", "[a11y][context]" )
{
    a11y::context_stack stack;
    stack.enter( "world", "DEFAULTMODE" );
    const auto before = stack.active_token().value();
    stack.set_suspended( true );
    CHECK( stack.is_suspended() );
    CHECK_FALSE( stack.active_token() );
    CHECK_FALSE( stack.active_frame() );
    CHECK_FALSE( stack.accepts( before ) );
    stack.set_suspended( false );
    CHECK_FALSE( stack.accepts( before ) );
    CHECK( stack.accepts( stack.active_token().value() ) );
}

TEST_CASE( "a11y_load_does_not_reuse_session_or_scope_identity", "[a11y][context]" )
{
    a11y::context_stack stack;
    const auto old_frame = stack.enter( "world", "DEFAULTMODE" );
    const auto old_token = stack.active_token().value();
    stack.set_suspended( true );
    stack.reset_session();
    CHECK( stack.is_suspended() );
    const auto new_frame = stack.enter( "world", "DEFAULTMODE" );
    CHECK( old_frame != new_frame );
    CHECK_FALSE( stack.leave( old_frame ) );
    stack.set_suspended( false );
    const auto token = stack.active_token().value();
    CHECK( old_token.session != token.session );
    CHECK_FALSE( stack.accepts( old_token ) );
    CHECK( stack.accepts( token ) );
}

TEST_CASE( "a11y_scope_move_and_unwind_release_only_owned_frame", "[a11y][context]" )
{
    a11y::context_stack stack;
    a11y::context_scope root( stack, "world", "DEFAULTMODE" );
    {
        a11y::context_scope first( stack, "inventory", "INVENTORY" );
        a11y::context_scope moved( std::move( first ) );
        CHECK( stack.active_frame()->category == "INVENTORY" );
    }
    CHECK( stack.active_frame()->category == "DEFAULTMODE" );
    try {
        a11y::context_scope modal( stack, "quantity", "STRING_INPUT" );
        throw std::runtime_error( "Unwind test" );
    } catch( const std::runtime_error & ) {
    }
    CHECK( stack.active_frame()->category == "DEFAULTMODE" );
}

TEST_CASE( "a11y_repeated_noop_does_not_invalidate_current_input", "[a11y][context]" )
{
    a11y::context_stack stack;
    stack.enter( "world", "DEFAULTMODE" );
    const auto token = stack.active_token().value();
    stack.set_suspended( false );
    CHECK_FALSE( stack.leave( 0 ) );
    CHECK( stack.accepts( token ) );
    CHECK_THROWS_AS( stack.enter( "", "MENU" ), std::invalid_argument );
    CHECK_THROWS_AS( stack.enter( "owner", "" ), std::invalid_argument );
    CHECK( stack.accepts( token ) );
}

TEST_CASE( "a11y_recreated_runtime_cannot_accept_another_stack_token", "[a11y][context]" )
{
    a11y::context_stack first;
    a11y::context_stack second;
    first.enter( "world", "DEFAULTMODE" );
    second.enter( "world", "DEFAULTMODE" );
    const auto token = first.active_token().value();
    CHECK_FALSE( second.accepts( token ) );
    first.reset_session();
    first.enter( "world", "DEFAULTMODE" );
    CHECK_FALSE( first.accepts( token ) );
    CHECK_FALSE( second.accepts( first.active_token().value() ) );
}
