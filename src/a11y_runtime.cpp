#include "a11y_runtime.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "a11y_android.h"
#include "a11y_context.h"

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

// Accessed only by the game thread. Android callbacks use android's mailbox.
context_stack contexts;
std::vector<std::unique_ptr<screen_data>> screens;
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
} // namespace

bool enabled() { return android::enabled(); }
bool suspended() { return !enabled() || android::suspended(); }

void replace_game_session()
{
    contexts.invalidate_session();
    android::clear();
    for( auto &screen : screens ) {
        screen->content.clear();
        screen->snapshot = {};
        screen->revision = 0;
        screen->last_request = 0;
    }
    // Do not republish the previous character's nodes during a load.
    android::publish( "{\"inactive\":true}" );
}

void synchronize_lifecycle()
{
    const std::uint64_t current = android::lifecycle_epoch();
    if( current != seen_epoch ) {
        seen_epoch = current;
        // Force a generation boundary even when two Activity callbacks report
        // the same state. Tokens from an old UI owner must never become valid.
        contexts.set_suspended( true );
        contexts.set_suspended( suspended() );
        android::clear();
        publish_current();
    }
}

bool bound_to( const void *input_owner )
{
    synchronize_lifecycle();
    return enabled() && !screens.empty() && screens.back()->input_owner == input_owner;
}

screen_scope::screen_scope( const void *input_owner, std::string category,
                            intent_handler handler )
{
    synchronize_lifecycle();
    if( !enabled() ) {
        return;
    }
    frame = contexts.enter( "semantic-screen", category );
    auto screen = std::make_unique<screen_data>();
    screen->frame = frame;
    screen->input_owner = input_owner;
    screen->category = std::move( category );
    screen->handler = std::move( handler );
    screens.push_back( std::move( screen ) );
    android::clear();
}

screen_scope::~screen_scope()
{
    if( frame == 0 ) {
        return;
    }
    const auto found = std::find_if( screens.begin(), screens.end(),
    [this]( const std::unique_ptr<screen_data> &s ) { return s->frame == frame; } );
    if( found != screens.end() ) {
        contexts.leave( frame );
        screens.erase( found, screens.end() );
        android::clear();
        publish_current();
    }
}

void screen_scope::update( semantic_snapshot snapshot, const bool force_revision )
{
    if( frame == 0 ) {
        return;
    }
    synchronize_lifecycle();
    const auto found = std::find_if( screens.begin(), screens.end(),
    [this]( const std::unique_ptr<screen_data> &s ) { return s->frame == frame; } );
    if( found == screens.end() ) {
        return;
    }
    screen_data &screen = **found;
    snapshot.category = screen.category;
    // Reject ambiguous IDs instead of allowing a click to select the wrong
    // logical object. The adapter must provide one stable ID per visible node.
    std::vector<std::string> ids;
    ids.reserve( snapshot.nodes.size() );
    for( const semantic_node &node : snapshot.nodes ) {
        if( node.id.empty() ) {
            android::status( "unsupported" );
            return;
        }
        ids.push_back( node.id );
    }
    std::sort( ids.begin(), ids.end() );
    if( std::adjacent_find( ids.begin(), ids.end() ) != ids.end() ) {
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

std::optional<std::string> poll_input( const void *input_owner )
{
    synchronize_lifecycle();
    if( suspended() || screens.empty() || screens.back()->input_owner != input_owner ) {
        return std::nullopt;
    }
    while( auto intent = android::take() ) {
        screen_data &screen = *screens.back();
        if( !contexts.accepts( intent->token ) || intent->revision != screen.revision ||
            intent->request <= screen.last_request ) {
            android::status( "stale", &*intent );
            continue;
        }
        screen.last_request = intent->request;
        if( intent->action != "cancel" ) {
            const auto node = std::find_if( screen.snapshot.nodes.begin(), screen.snapshot.nodes.end(),
            [&intent]( const semantic_node & n ) { return n.id == intent->node; } );
            if( node == screen.snapshot.nodes.end() ||
                std::find( node->actions.begin(), node->actions.end(), intent->action ) == node->actions.end() ) {
                android::status( "stale", &*intent );
                continue;
            }
            if( intent->action != "focus" && !node->enabled ) {
                android::status( "disabled", &*intent );
                continue;
            }
        }
        if( screen.handler ) {
            if( auto action = screen.handler( *intent ) ) {
                return action;
            }
        }
    }
    return std::nullopt;
}
} // namespace a11y
