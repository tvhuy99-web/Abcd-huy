#pragma once
#ifndef CATA_SRC_TILE_TINT_H
#define CATA_SRC_TILE_TINT_H

#include <cstdint>
#include <optional>

struct light_color_rgb;


constexpr float tint_max_alpha = 80.0f;


struct tile_tint {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
};




std::optional<tile_tint> compute_tile_tint( const light_color_rgb &lc, float scalar_light );




struct tint_texture_mod {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};
tint_texture_mod tint_texture_mod_for( const tile_tint &t );
tint_texture_mod tint_texture_mod_none();


float tint_mix_channel( float src, float mod, uint8_t mod_a );

#endif
