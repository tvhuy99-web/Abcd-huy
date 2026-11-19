#pragma once
#ifndef CATA_SRC_SHADER_TINT_SELF_TEST_H
#define CATA_SRC_SHADER_TINT_SELF_TEST_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "point.h"
#include "tile_tint.h"





namespace shader_tint_self_test
{

struct rgba {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
};

enum class transform {
    none,
    flip_horizontal,

    rotate_90,
};

struct draw_case {
    char name = ' ';

    point dst;
    transform xform = transform::none;
    tint_texture_mod mod;
};

constexpr int source_width = 4;
constexpr int source_height = 2;
constexpr size_t texel_count = source_width * source_height;
constexpr int target_size = 32;
constexpr int channel_tolerance = 4;
constexpr size_t case_count = 5;

constexpr size_t check_count = case_count + 1;
constexpr size_t background_index = case_count;



const std::array<rgba, texel_count> &source_texels();

const std::array<draw_case, case_count> &draw_cases();
rgba clear_color();

struct coverage {
    size_t case_index = 0;
    point texel;
};


std::optional<coverage> covering_case( const point &px );



rgba expected_pixel( const point &px );

struct mismatch {
    point px;
    rgba expected;
    rgba actual;
};



std::array<std::optional<mismatch>, check_count> compare_readback(
    const std::vector<rgba> &pixels );


std::string rgb_string( const rgba &c );

}

#endif
