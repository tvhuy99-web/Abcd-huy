#include "a11y_runtime.h"

#include <algorithm>
#if defined(__ANDROID__)
#include <chrono>
#endif
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "a11y_android.h"
#include "a11y_context.h"
#include "a11y_input.h"
#include "a11y_outcome.h"
#include "overmap_ui.h"
#include "a11y_world.h"
#include "a11y_world_input_gate.h"

class input_context;

namespace a11y
{
namespace
{
struct screen_data {
    std::uint64_t frame;
    const void *input_owner;
    std::string category;
    intent_handler handler;
    semantic_snapshot snapshot;
    std::string content;
    std::uint64_t revision = 0;
    std::uint64_t last_request = 0;
};


context_stack contexts;
std::vector<std::unique_ptr<screen_data>> screens;




std::vector<screen_scope *> live_scopes;

std::unique_ptr<screen_scope> retained_world_scope;
std::uint64_t seen_epoch = 0;

void publish_current()
{
    const auto token = contexts.active_token();
    if( !token || screens.empty() || screens.back()->revision == 0 ) {
        android::publish( "{\"inactive\":true}" );
        return;
    }
    screen_data &screen = *screens.back();
    screen.snapshot.token = *token;
    screen.snapshot.revision = screen.revision;
    android::publish( snapshot_json( screen.snapshot ) );
}
}

bool enabled() { return android::enabled(); }
bool suspended() { return !enabled() || android::suspended(); }

void replace_game_session()
{
    ui::omap::cancel_accessible_drive_to_place();
    android::reset_travel_control();
    reset_status_monitor();

    reset_action_outcomes();
    retained_world_scope.reset();
    invalidate_command_snapshot_cache();
    contexts.invalidate_session();
    android::clear();
    for( auto &screen : screens ) {
        screen->content.clear();
        screen->snapshot = {};
        screen->revision = 0;
        screen->last_request = 0;
    }

    android::publish( "{\"inactive\":true}" );
}

void synchronize_lifecycle()
{
    const std::uint64_t current = android::lifecycle_epoch();
    if( current != seen_epoch ) {
        seen_epoch = current;
        reset_status_monitor();
        invalidate_command_snapshot_cache();


        contexts.set_suspended( true );
        contexts.set_suspended( suspended() );
        android::clear();







        for( auto &screen : screens ) {
            screen->last_request = 0;
        }
        publish_current();
    }
}

bool bound_to( const void *input_owner )
{
    synchronize_lifecycle();
    return enabled() && !screens.empty() && screens.back()->input_owner == input_owner;
}

void set_world_input_ready( const void *input_owner, const bool ready )
{
    synchronize_lifecycle();
    if( ready && enabled() && !suspended() && !screens.empty() &&
        screens.back()->input_owner == input_owner &&
        screens.back()->category == "DEFAULTMODE" ) {
        android::set_world_step_ready( contexts.active_token(), true );
    } else {
        android::set_world_step_ready( std::nullopt, false );
    }
}

screen_scope::screen_scope( const void *input_owner, std::string category,
                            intent_handler handler )
    : owner( input_owner ), screen_category( std::move( category ) ),
      intent_callback( std::move( handler ) )
{
    live_scopes.push_back( this );
    synchronize_lifecycle();
    activate();
}

void screen_scope::activate()
{
    if( frame != 0 || !enabled() ) {
        return;
    }
    if( screen_category != "DEFAULTMODE" ) {
        invalidate_command_snapshot_cache();
    }
    frame = contexts.enter( "semantic-screen", screen_category );
    auto screen = std::make_unique<screen_data>();
    screen->frame = frame;
    screen->input_owner = owner;
    screen->category = screen_category;
    screen->handler = std::move( intent_callback );
    screens.push_back( std::move( screen ) );
    android::clear();
}

void screen_scope::rebind( const void *input_owner, intent_handler handler )
{
    owner = input_owner;
    if( frame == 0 ) {
        intent_callback = std::move( handler );
        return;
    }
    const auto found = std::find_if( screens.begin(), screens.end(),
    [this]( const std::unique_ptr<screen_data> &s ) { return s->frame == frame; } );
    if( found != screens.end() ) {
        ( *found )->input_owner = input_owner;
        ( *found )->handler = std::move( handler );
    }
}

screen_scope::~screen_scope()
{
    live_scopes.erase( std::remove( live_scopes.begin(), live_scopes.end(), this ),
                       live_scopes.end() );
    if( frame == 0 ) {
        return;
    }
    const auto found = std::find_if( screens.begin(), screens.end(),
    [this]( const std::unique_ptr<screen_data> &s ) { return s->frame == frame; } );
    if( found != screens.end() ) {
        contexts.leave( frame );
        screens.erase( found, screens.end() );
        android::clear();
        if( !screens.empty() && screens.back()->revision != 0 ) {






            ++screens.back()->revision;
        }
        publish_current();
    }
}

screen_scope &bind_world_screen( const void *input_owner, intent_handler handler )
{
    if( !retained_world_scope ) {
        retained_world_scope = std::make_unique<screen_scope>(
                                   input_owner, "DEFAULTMODE", std::move( handler ) );
    } else {
        retained_world_scope->rebind( input_owner, std::move( handler ) );
    }
    return *retained_world_scope;
}

void unbind_world_screen()
{
    if( retained_world_scope ) {


        retained_world_scope->rebind( nullptr, {} );
    }
}

void screen_scope::update( semantic_snapshot snapshot, const bool force_revision )
{
    synchronize_lifecycle();
    activate();
    if( frame == 0 ) {





        pending_snapshot = std::move( snapshot );
        has_pending = true;
        return;
    }
    has_pending = false;
    apply_snapshot( std::move( snapshot ), force_revision );
}

void screen_scope::apply_snapshot( semantic_snapshot snapshot, const bool force_revision )
{
    const auto found = std::find_if( screens.begin(), screens.end(),
    [this]( const std::unique_ptr<screen_data> &s ) { return s->frame == frame; } );
    if( found == screens.end() ) {
        return;
    }
    screen_data &screen = **found;
    snapshot.category = screen.category;





    std::string invalid_reason;
    if( !validate_snapshot( snapshot, invalid_reason ) ) {
        android::status( "unsupported" );
        return;
    }
    const std::string content = snapshot_content_json( snapshot );
    if( force_revision || content != screen.content ) {
        screen.content = content;
        screen.snapshot = std::move( snapshot );
        ++screen.revision;
        if( screens.back().get() == &screen ) {
            publish_current();
        }
    }
}

bool activate_pending_scopes( const void *input_owner )
{
    synchronize_lifecycle();
    if( !enabled() || suspended() ) {
        return false;
    }
    bool activated = false;



    for( screen_scope *scope : live_scopes ) {
        if( scope->owner != input_owner || scope->frame != 0 || !scope->has_pending ) {
            continue;
        }
        scope->activate();
        if( scope->frame != 0 ) {
            scope->has_pending = false;
            scope->apply_snapshot( std::move( scope->pending_snapshot ), false );
            scope->pending_snapshot = semantic_snapshot{};
            activated = true;
        }
    }
    return activated;
}

std::optional<std::string> poll_input( const void *input_owner )
{


    activate_pending_scopes( input_owner );
    synchronize_lifecycle();
    if( suspended() || screens.empty() || screens.back()->input_owner != input_owner ) {
        return std::nullopt;
    }
    while( auto intent = android::take() ) {
        screen_data &screen = *screens.back();






        const bool world_walk = live_world_walk_command( screen.snapshot, *intent );






        const bool activity_autodrive =
                ( screen.category == "activity" || screen.category == "DEFAULTMODE" ) &&
                std::any_of( screen.snapshot.nodes.begin(), screen.snapshot.nodes.end(),
        []( const semantic_node &n ) { return n.id == "activity:autodrive"; } );
        const std::string &query = intent->node;
        const bool live_vehicle_query = activity_autodrive &&
                intent->revision > 0 && intent->revision <= screen.revision &&
                intent->action == "activate" && intent->value.empty() &&
                ( query.rfind( "reader:scan:vehicle:", 0 ) == 0 ||
                  query == "reader:coordinates" ||
                  query == "reader:vehicle:horn" || query == "control_vehicle" );





        const bool live_read_only_focus = intent->action == "focus" &&
                                          intent->value.empty() && intent->revision > 0 &&
                                          intent->revision <= screen.revision;
        const bool valid_token = contexts.accepts( intent->token );
        const bool valid_revision = intent->revision == screen.revision || world_walk ||
                                    live_read_only_focus || live_vehicle_query;
        const bool valid_request = intent->request > screen.last_request;
        if( !valid_token || !valid_revision || !valid_request ) {
            android::status( "stale", &*intent );
            continue;
        }
        screen.last_request = intent->request;
#if defined(__ANDROID__)
        if( world_walk && android::performance_profiling_enabled() ) {
            const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now().time_since_epoch() ).count();
            const std::uint64_t wait_ms =
                intent->received_at_ms > 0 &&
                now_ms >= static_cast<std::int64_t>( intent->received_at_ms ) ?
                static_cast<std::uint64_t>( now_ms ) - intent->received_at_ms : 0;
            android::diagnostic_event( "WORLD_INPUT_TIMING",
                    "jni_to_poll_ms=" + std::to_string( wait_ms ) +
                    " revision_lag=" + std::to_string( screen.revision - intent->revision ) );
        }
#endif




        if( !intent->value.empty() && intent->action != "set_text" &&
            intent->action != "set_value" && intent->action != "select_choice" ) {
            android::status( "invalid_value", &*intent );
            continue;
        }
        if( intent->action == "cancel" ) {


            // fall through to the screen below a modal. The node id the



            if( !intent->node.empty() &&
                std::find_if( screen.snapshot.nodes.begin(), screen.snapshot.nodes.end(),
            [&intent]( const semantic_node & n ) { return n.id == intent->node; } ) ==
            screen.snapshot.nodes.end() ) {
                android::status( "stale", &*intent );
                continue;
            }
        } else {




            const auto node = std::find_if( screen.snapshot.nodes.begin(), screen.snapshot.nodes.end(),
            [&intent]( const semantic_node & n ) { return n.id == intent->node; } );
            if( node == screen.snapshot.nodes.end() ||
                std::find( node->actions.begin(), node->actions.end(), intent->action ) == node->actions.end() ) {



                if( intent->action != "focus" ) {
                    android::status( "stale", &*intent );
                }
                continue;
            }
            if( intent->action != "focus" && intent->action != "read_detail" && !node->enabled ) {
                android::status( "disabled", &*intent );
                continue;
            }




            if( intent->action == "set_value" || intent->action == "select_choice" ) {
                std::string invalid_reason;
                if( !validate_intent_value( *node, *intent, invalid_reason ) ) {
                    android::status( "invalid_value", &*intent );
                    continue;
                }
            }
        }
        if( screen.handler ) {
            if( auto action = screen.handler( *intent ) ) {
                return action;
            }
        }





        if( intent->action == "cancel" ) {
            const input_context &context = *static_cast<const input_context *>( input_owner );
            if( auto action = registered_intent( context, *intent ) ) {
                return action;
            }
            android::status( "unsupported", &*intent );
        }
    }
    return std::nullopt;
}

void announce_game_message( const std::string_view text, const bool urgent )
{
    synchronize_lifecycle();
    if( text.empty() || suspended() ) {
        return;
    }


    android::announce( text, urgent ? 3 : 1 );
    if( urgent ) {



        android::cue( "danger" );
    }
}
}
