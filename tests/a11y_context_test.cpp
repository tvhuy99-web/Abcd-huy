#include <stdexcept>
#include <utility>

#include "a11y_context.h"
#include "a11y_quantity_selection.h"
#include "a11y_world_input_gate.h"
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

TEST_CASE( "a11y_fast_world_swipe_does_not_depend_on_map_revision",
           "[a11y][context][walking]" )
{
    a11y::context_stack stack;
    stack.enter( "world", "DEFAULTMODE" );
    const auto token = stack.active_token().value();

    a11y::semantic_snapshot world;
    world.token = token;
    world.category = "DEFAULTMODE";
    world.swipe_movement_allowed = true;
    world.revision = 15;
    for( const char *direction : { "UP", "DOWN", "LEFT", "RIGHT" } ) {
        a11y::semantic_node node;
        node.id = direction;
        node.actions = { "focus", "activate" };
        world.nodes.push_back( std::move( node ) );
    }

    a11y::semantic_intent swipe;
    swipe.token = token;
    swipe.action = "activate";
    swipe.request = 1;
    // The world could have published many new descriptions during the
    // previous turn. This same-generation intent is still a single step.
    swipe.revision = 2;
    REQUIRE( stack.accepts( swipe.token ) );
    for( const char *direction : { "UP", "DOWN", "LEFT", "RIGHT" } ) {
        swipe.node = direction;
        CHECK( a11y::live_world_walk_command( world, swipe ) );
    }

    swipe.revision = 0;
    CHECK_FALSE( a11y::live_world_walk_command( world, swipe ) );
    swipe.revision = 16;
    CHECK_FALSE( a11y::live_world_walk_command( world, swipe ) );
    swipe.revision = 15;
    swipe.node = "LEFTUP";
    CHECK_FALSE( a11y::live_world_walk_command( world, swipe ) );
    swipe.node = "UP";
    swipe.action = "focus";
    CHECK_FALSE( a11y::live_world_walk_command( world, swipe ) );
    swipe.action = "activate";
    swipe.value = "unexpected";
    CHECK_FALSE( a11y::live_world_walk_command( world, swipe ) );
    swipe.value.clear();

    world.nodes[0].enabled = false;
    CHECK_FALSE( a11y::live_world_walk_command( world, swipe ) );
    world.nodes[0].enabled = true;
    world.nodes[0].actions = { "focus" };
    CHECK_FALSE( a11y::live_world_walk_command( world, swipe ) );
    world.nodes[0].actions = { "focus", "activate" };

    world.swipe_movement_allowed = false;
    CHECK_FALSE( a11y::live_world_walk_command( world, swipe ) );
    world.swipe_movement_allowed = true;
    world.category = "UILIST";
    CHECK_FALSE( a11y::live_world_walk_command( world, swipe ) );

    // Matching a world command is never sufficient by itself. The actual
    // runtime first requires the current, unexpired session/frame/generation.
    world.category = "DEFAULTMODE";
    const auto overlay = stack.enter( "inventory", "INVENTORY" );
    CHECK_FALSE( stack.accepts( swipe.token ) );
    REQUIRE( stack.leave( overlay ) );
    CHECK_FALSE( stack.accepts( swipe.token ) );
    stack.set_suspended( true );
    CHECK_FALSE( stack.accepts( swipe.token ) );
}

TEST_CASE( "a11y_world_step_node_identity_is_exact",
           "[a11y][context][walking]" )
{
    CHECK( a11y::is_cardinal_step_node( "UP" ) );
    CHECK( a11y::is_cardinal_step_node( "DOWN" ) );
    CHECK( a11y::is_cardinal_step_node( "LEFT" ) );
    CHECK( a11y::is_cardinal_step_node( "RIGHT" ) );
    CHECK_FALSE( a11y::is_cardinal_step_node( "LEFTUP" ) );
    CHECK_FALSE( a11y::is_cardinal_step_node( "reader:scan:north" ) );
    CHECK_FALSE( a11y::is_cardinal_step_node( "world:here" ) );
}

TEST_CASE( "a11y_item_quantities_are_complete_one_step_choices",
           "[a11y][quantity]" )
{
    // The same strict parser is used by inventory multi-select (pickup,
    // drop, insert and trade) and the one-tile quick pickup menu.
    REQUIRE( a11y::selected_quantity( "1", 1 ) == std::optional<std::size_t>( 1 ) );
    REQUIRE( a11y::selected_quantity( "0", 1 ) == std::optional<std::size_t>( 0 ) );
    REQUIRE( a11y::selected_quantity( "12", 30 ) == std::optional<std::size_t>( 12 ) );
    REQUIRE( a11y::selected_quantity( "30", 30 ) == std::optional<std::size_t>( 30 ) );
    REQUIRE( a11y::selected_quantity( "1000000", 1000000 ) ==
             std::optional<std::size_t>( 1000000 ) );

    // Never silently clamp a stale choice to a different count, accept
    // negatives/whitespace, or overflow the native available amount.
    CHECK_FALSE( a11y::selected_quantity( "", 30 ) );
    CHECK_FALSE( a11y::selected_quantity( "31", 30 ) );
    CHECK_FALSE( a11y::selected_quantity( "-1", 30 ) );
    CHECK_FALSE( a11y::selected_quantity( "+1", 30 ) );
    CHECK_FALSE( a11y::selected_quantity( "1.5", 30 ) );
    CHECK_FALSE( a11y::selected_quantity( "1 ", 30 ) );
    CHECK_FALSE( a11y::selected_quantity( "999999999999999999999999", 30 ) );
    CHECK_FALSE( a11y::selected_quantity( "1", 0 ) );
    REQUIRE( a11y::selected_quantity( "0", 0 ) == std::optional<std::size_t>( 0 ) );
}
