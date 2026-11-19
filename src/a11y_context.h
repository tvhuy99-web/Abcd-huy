#pragma once
#ifndef CATA_SRC_A11Y_CONTEXT_H
#define CATA_SRC_A11Y_CONTEXT_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace a11y
{



struct context_token {
    std::uint64_t session = 0;
    std::uint64_t frame = 0;
    std::uint64_t generation = 0;
};




class context_stack
{
    public:
        context_stack();
        context_stack( const context_stack & ) = delete;
        context_stack &operator=( const context_stack & ) = delete;

        std::uint64_t enter( std::string owner, std::string category );


        bool leave( std::uint64_t frame_id );

        void invalidate_session();
        void set_suspended( bool suspended );
        std::optional<context_token> active_token() const;
        bool accepts( const context_token &token ) const;

    private:


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

}

#endif
