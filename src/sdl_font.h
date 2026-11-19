#pragma once
#ifndef CATA_SRC_SDL_FONT_H
#define CATA_SRC_SDL_FONT_H
#include "font_loader.h"

#if defined(TILES)

#include "cursesdef.h" // IWYU pragma: associated
#include "sdltiles.h" // IWYU pragma: associated

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <string>

#include "color.h"
#include "color_loader.h"
#include "debug.h"
#include "glyph_shelf_packer.h"
#include "point.h"
#include "sdl_wrappers.h"

class text_batch;

using palette_array = std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT>;


class Font
{
    public:
        Font( int w, int h, const palette_array &palette ) :
            width( w ), height( h ), palette( palette ) { }
        virtual ~Font() = default;


        virtual bool isGlyphProvided( const std::string &ch ) const = 0;






        virtual void queue_char( text_batch &batch, const SDL_Renderer_Ptr &renderer,
                                 const std::string &ch, const point &p,
                                 unsigned char color, float opacity = 1.0f ) = 0;





        virtual void queue_ascii_lines( text_batch &batch, unsigned char line_id,
                                        const point &p, unsigned char color ) const;


        void OutputChar( const SDL_Renderer_Ptr &renderer, const std::string &ch, const point &p,
                         unsigned char color, float opacity = 1.0f );




        virtual void release_gpu_resources() {}




        virtual void rebuild_for_renderer( const SDL_Renderer_Ptr &renderer ) {
            ( void )renderer;
        }


        static std::unique_ptr<Font> load_font(
            SDL_Renderer_Ptr &renderer, Uint32 pixel_format,
            const std::string &typeface, int fontsize, int fontwidth,
            int fontheight,
            const palette_array &palette,
            bool fontblending );


        int width;

        int height;

        const palette_array &palette;
};
using Font_Ptr = std::unique_ptr<Font>;



class CachedTTFFont : public Font
{
    public:
        CachedTTFFont(
            int w, int h,
            const palette_array &palette,
            std::string typeface, int fontsize, bool fontblending );
        ~CachedTTFFont() override = default;

        bool isGlyphProvided( const std::string &ch ) const override;
        void queue_char( text_batch &batch, const SDL_Renderer_Ptr &renderer,
                         const std::string &ch, const point &p,
                         unsigned char color, float opacity = 1.0f ) override;
        void release_gpu_resources() override;


        size_t atlas_glyph_count() const {
            return glyphs.size();
        }

        size_t atlas_page_count() const {
            return pages.size();
        }

        size_t immediate_glyph_count() const {
            return immediate_glyphs.size();
        }

    protected:
        struct glyph_entry {

            int page = -1;
            SDL_Rect cell = { 0, 0, 0, 0 };
        };

        struct immediate_glyph {
            SDL_Texture_Ptr texture;
            int width = 0;
        };




        SDL_Surface_Ptr render_glyph_cell( const std::string &ch, int &cell_width,
                                           const SDL_Color &color ) const;

        const immediate_glyph &immediate_glyph_for( const SDL_Renderer_Ptr &renderer,
                const std::string &ch, unsigned char color );

        const glyph_entry &glyph_for( const SDL_Renderer_Ptr &renderer, const std::string &ch );



        bool ensure_page( const SDL_Renderer_Ptr &renderer, int page, int w, int h );

        TTF_Font_Ptr font;
        std::unordered_map<std::string, glyph_entry> glyphs;
        std::vector<SDL_Texture_Ptr> pages;
        glyph_shelf_packer packer;

        std::unordered_map<std::string, immediate_glyph> immediate_glyphs;

        const bool fontblending;
};



class BitmapFont : public Font
{
    public:
        BitmapFont(
            SDL_Renderer_Ptr &renderer, Uint32 pixel_format,
            int w, int h,
            const palette_array &palette,
            const std::string &typeface_path );
        ~BitmapFont() override = default;

        bool isGlyphProvided( const std::string &ch ) const override;
        void queue_char( text_batch &batch, const SDL_Renderer_Ptr &renderer,
                         const std::string &ch, const point &p,
                         unsigned char color, float opacity = 1.0f ) override;
        void queue_ascii_lines( text_batch &batch, unsigned char line_id, const point &p,
                                unsigned char color ) const override;
        void release_gpu_resources() override;
        void rebuild_for_renderer( const SDL_Renderer_Ptr &renderer ) override;
    protected:

        void queue_index( text_batch &batch, int t, const point &p, unsigned char color,
                          float opacity ) const;



        void build_textures( const SDL_Renderer_Ptr &renderer, Uint32 pixel_format );

        std::array<SDL_Texture_Ptr, color_loader<SDL_Color>::COLOR_NAMES_COUNT> ascii;
        int tilewidth;

        point atlas_size;

        std::string typeface_path;
        Uint32 source_pixel_format = 0;
};



class FontFallbackList : public Font
{
    public:
        FontFallbackList(
            SDL_Renderer_Ptr &renderer, Uint32 pixel_format,
            int w, int h,
            const palette_array &palette,
            const std::vector<font_config> &typefaces,
            int fontsize, bool fontblending );
        ~FontFallbackList() override = default;

        bool isGlyphProvided( const std::string &ch ) const override;
        void queue_char( text_batch &batch, const SDL_Renderer_Ptr &renderer,
                         const std::string &ch, const point &p,
                         unsigned char color, float opacity = 1.0f ) override;
        void release_gpu_resources() override;
        void rebuild_for_renderer( const SDL_Renderer_Ptr &renderer ) override;
    protected:
        std::vector<std::unique_ptr<Font>> fonts;
        std::map<std::string, std::vector<std::unique_ptr<Font>>::iterator> glyph_font;
};



void arm_glyph_page_create_failures( int count );




bool text_atlas_enabled( const SDL_Renderer_Ptr &renderer );

void override_text_atlas( std::optional<bool> enabled );

#endif

#endif
