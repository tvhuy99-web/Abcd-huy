#pragma once
#ifndef CATA_SRC_SDL_GEOMETRY_H
#define CATA_SRC_SDL_GEOMETRY_H

#if defined(TILES)
#include <memory>

#include "sdl_wrappers.h"

struct point;


class GeometryRenderer
{
    public:
        virtual ~GeometryRenderer() = default;


        virtual void rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect,
                           const SDL_Color &color ) const = 0;



        virtual void release_gpu_resources() {}



        virtual void rebuild_for_renderer( const SDL_Renderer_Ptr &renderer ) {
            ( void )renderer;
        }


        void rect( const SDL_Renderer_Ptr &renderer, const point &pos, int width, int height,
                   const SDL_Color &color ) const;
};
using GeometryRenderer_Ptr = std::unique_ptr<GeometryRenderer>;


class DefaultGeometryRenderer : public GeometryRenderer
{
    public:
        void rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect,
                   const SDL_Color &color ) const override;
};

#endif

#endif
