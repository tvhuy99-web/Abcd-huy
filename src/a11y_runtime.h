#pragma once
#ifndef CATA_SRC_A11Y_RUNTIME_H
#define CATA_SRC_A11Y_RUNTIME_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "a11y_semantics.h"

namespace a11y
{
using intent_handler = std::function<std::optional<std::string>( const semantic_intent & )>;

bool enabled();
bool suspended();
void synchronize_lifecycle();
void replace_game_session();
bool bound_to( const void *input_owner );

void set_world_input_ready( const void *input_owner, bool ready );






bool activate_pending_scopes( const void *input_owner );
std::optional<std::string> poll_input( const void *input_owner );
void announce_game_message( std::string_view text, bool urgent );




class screen_scope;
screen_scope &bind_world_screen( const void *input_owner, intent_handler handler );
void unbind_world_screen();




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
        void rebind( const void *input_owner, intent_handler handler );

    private:
        void activate();


        void apply_snapshot( semantic_snapshot snapshot, bool force_revision );
        friend bool activate_pending_scopes( const void *input_owner );
        std::uint64_t frame = 0;
        const void *owner = nullptr;
        std::string screen_category;
        intent_handler intent_callback;





        semantic_snapshot pending_snapshot;
        bool has_pending = false;
};
}
#endif
