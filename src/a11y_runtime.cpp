#include "a11y_runtime.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "a11y_android.h"
#include "a11y_context.h"
#include "a11y_outcome.h"
#include "a11y_world.h"

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
// Every live screen_scope, in construction order, including scopes that
// could not activate yet because the reader was off or suspended. Used by
// activate_pending_scopes() to give already-open screens a reader adapter
// when the reader becomes available (R03.3). Game thread only.
std::vector<screen_scope *> live_scopes;
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
    reset_status_monitor();
    // The previous character's last action outcome must not carry over.
    reset_action_outcomes();
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
        reset_status_monitor();
        // Force a generation boundary even when two Activity callbacks report
        // the same state. Tokens from an old UI owner must never become valid.
        contexts.set_suspended( true );
        contexts.set_suspended( suspended() );
        android::clear();
        // A lifecycle change starts a new UI-owner generation, and with it
        // a new request space (R03.4): the mailbox was just cleared and the
        // generation boundary above rejects every intent minted before the
        // change, so resetting the per-screen high-water mark cannot replay
        // an old owner's command. Without the reset, a fresh Java request
        // sequence could stay below the mark of a destroyed Activity and
        // be rejected as stale forever.
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
    frame = contexts.enter( "semantic-screen", screen_category );
    auto screen = std::make_unique<screen_data>();
    screen->frame = frame;
    screen->input_owner = owner;
    screen->category = screen_category;
    screen->handler = std::move( intent_callback );
    screens.push_back( std::move( screen ) );
    android::clear();
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
            // The screen revealed by closing this (modal) scope must be
            // re-read before it can be acted on (R03.3): bump its revision
            // so every intent minted against its pre-modal publication is
            // stale, in addition to the token generation that leave()
            // already advanced. The stored content itself is republished
            // unchanged; the screen's next update() refreshes it as usual.
            ++screens.back()->revision;
        }
        publish_current();
    }
}

void screen_scope::update( semantic_snapshot snapshot, const bool force_revision )
{
    synchronize_lifecycle();
    activate();
    if( frame == 0 ) {
        // The reader is off or suspended, so this screen has no adapter
        // yet. Keep the latest snapshot: if the reader becomes available
        // while the screen is still open, activate_pending_scopes()
        // applies the cache and the screen starts serving the reader in
        // place, even when its builder never calls update() again (R03.3).
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
    // Reject ambiguous or structurally invalid snapshots instead of allowing
    // a click to select the wrong logical object (R02.4). validate_snapshot
    // covers duplicate/empty IDs plus the tree, table and form contracts;
    // the specific reason code is internal and only the generic reader
    // status is published, as before.
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
    // Construction order matters: when several scopes share one input
    // owner, the most recently constructed (innermost) scope must end up
    // on top of the screen stack, so activate in registration order.
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
    // Give an already-open screen its reader adapter first when the reader
    // became available after the screen was built (R03.3).
    activate_pending_scopes( input_owner );
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
        // Only three actions may carry a value at all (R03.1). Every other
        // action is a plain command on its node; a value smuggled into one
        // (stale editor callback, hand-crafted intent) is rejected before
        // any handler can see it, never applied speculatively.
        if( !intent->value.empty() && intent->action != "set_text" &&
            intent->action != "set_value" && intent->action != "select_choice" ) {
            android::status( "invalid_value", &*intent );
            continue;
        }
        if( intent->action == "cancel" ) {
            // Cancel is addressed to the owning screen, not to a node, and
            // only that screen's poll can consume it, so a cancel can never
            // fall through to the screen below a modal. The node id the
            // client echoes (its current focus) is not trusted: a cancel
            // naming an object the current snapshot does not publish was
            // minted against another screen and is dropped.
            if( !intent->node.empty() &&
                std::find_if( screen.snapshot.nodes.begin(), screen.snapshot.nodes.end(),
            [&intent]( const semantic_node & n ) { return n.id == intent->node; } ) ==
            screen.snapshot.nodes.end() ) {
                android::status( "stale", &*intent );
                continue;
            }
        } else {
            // Re-resolve the target against the live snapshot of the top
            // screen (R03.1): the node and the action it offers are checked
            // here, on the game thread, at receipt time. Nothing the client
            // remembered about an older snapshot is trusted.
            const auto node = std::find_if( screen.snapshot.nodes.begin(), screen.snapshot.nodes.end(),
            [&intent]( const semantic_node & n ) { return n.id == intent->node; } );
            if( node == screen.snapshot.nodes.end() ||
                std::find( node->actions.begin(), node->actions.end(), intent->action ) == node->actions.end() ) {
                // A focus intent racing a screen change is routine and the
                // client re-sends focus on every navigation; drop it
                // silently instead of announcing a stale rejection.
                if( intent->action != "focus" ) {
                    android::status( "stale", &*intent );
                }
                continue;
            }
            if( intent->action != "focus" && !node->enabled ) {
                android::status( "disabled", &*intent );
                continue;
            }
            // Form values are validated against the node's published domain
            // on the game thread (R02.4): an out-of-range number, a step
            // mismatch or an unpublished choice is rejected with a readable
            // receipt before any handler can apply it.
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
    }
    return std::nullopt;
}

void announce_game_message( const std::string &text, const bool urgent )
{
    synchronize_lifecycle();
    if( text.empty() || suspended() ) {
        return;
    }
    // ReaderSpeechQueue uses the same priority levels: information does not
    // interrupt focus speech, while urgent warnings do.
    android::announce( text, urgent ? 3 : 1 );
    if( urgent ) {
        // The danger cue duplicates this same urgent message, which the
        // engine only produces for events the player can already perceive
        // (R21.2: a cue must never leak more than the words do).
        android::cue( "danger" );
    }
}
} // namespace a11y
