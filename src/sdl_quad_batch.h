#pragma once
#ifndef CATA_SRC_SDL_QUAD_BATCH_H
#define CATA_SRC_SDL_QUAD_BATCH_H

#if defined(TILES)

#include <cstddef>
#include <utility>
#include <vector>

#include "sdl_wrappers.h"

SDL_FColor to_fcolor( const SDL_Color &color );




class quad_batch
{
    public:
        void append_quad( float x, float y, float w, float h, const SDL_FColor &color );

        void append_textured_quad( const SDL_Rect &dst, const SDL_Rect &src, int tex_w, int tex_h,
                                   const SDL_FColor &color );
        void clear();
        bool empty() const;
        int vertex_count() const;
        int index_count() const;
        const SDL_Vertex *vertex_data() const;
        const int *index_data() const;
        void reserve_quads( int quads );

    private:
        void append( float x, float y, float w, float h, const SDL_FColor &color,
                     float u0, float v0, float u1, float v1 );
        std::vector<SDL_Vertex> vertices_;
        std::vector<int> indices_;
};

void render_quad_batch( const SDL_Renderer_Ptr &renderer, const quad_batch &batch,
                        SDL_Texture *texture = nullptr );






class text_batch
{
    public:

        void begin_pass( const SDL_Renderer_Ptr &renderer, bool immediate );
        bool immediate() const {
            return immediate_;
        }

        const SDL_Renderer_Ptr &renderer() const {
            return *renderer_;
        }
        void add_rect( const SDL_Rect &rect, const SDL_Color &color );
        void add_glyph( SDL_Texture *texture, int tex_w, int tex_h, const SDL_Rect &dst,
                        const SDL_Rect &src, const SDL_FColor &color );
        bool empty() const;
        void flush( const SDL_Renderer_Ptr &renderer );

    private:
        const SDL_Renderer_Ptr *renderer_ = nullptr;
        bool immediate_ = false;
        quad_batch rects_;


        std::vector<std::pair<SDL_Texture *, quad_batch>> glyphs_;
        size_t active_groups_ = 0;
};

#endif
#endif
