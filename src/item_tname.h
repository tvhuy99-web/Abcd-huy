#pragma once
#ifndef CATA_SRC_ITEM_TNAME_H
#define CATA_SRC_ITEM_TNAME_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "enum_bitset.h"

class item;
template <typename T> struct enum_traits;

namespace tname
{



enum class segments : std::size_t {
    FAULTS = 0,
    DIRT,
    OVERHEAT,
    FAVORITE_PRE,
    DURABILITY,
    WHEEL_DIAMETER,
    BURN,
    WEAPON_MODS,
    CUSTOM_ITEM_PREFIX,
    TYPE,
    CATEGORY,
    CUSTOM_ITEM_SUFFIX,
    FAULTS_SUFFIX,
    MODS,
    CRAFT,
    WHITEBLACKLIST,
    CHARGES,
    FOOD_TRAITS,
    FOOD_STATUS,
    FOOD_IRRADIATED,
    TEMPERATURE,
    LOCATION_HINT,
    LOCATION_PRECISE_CLOSEST_CITY,
    CLOTHING_SIZE,
    ETHEREAL,
    FILTHY,
    BROKEN,
    CBM_STATUS,
    UPS,
    TRAITS,
    TAGS,
    VARS,
    WETNESS,
    ACTIVE,
    SEALED,
    FAVORITE_POST,
    RELIC,
    LINK,
    TECHNIQUES,
    CONTENTS,
    EMEMORY,
    ACTIVITY_OCCUPANCY,

    last_segment,


    VARIANT,
    COMPONENTS,
    CORPSE,


    CONTENTS_FULL,
    CONTENTS_ABREV,
    CONTENTS_COUNT,


    FOOD_PERISHABLE,

    last,
};

using segment_bitset = enum_bitset<tname::segments>;

#ifndef CATA_IN_TOOL

std::string print_segment( tname::segments segment, item const &it, unsigned int quantity,
                           segment_bitset const &segments );

using tname_set = std::vector<tname::segments>;
tname_set const &get_tname_set();

#endif
}

template<>
struct enum_traits<tname::segments> {
    static constexpr tname::segments last = tname::segments::last;
};

namespace tname
{
constexpr uint64_t default_tname_bits =
    std::numeric_limits<uint64_t>::max() &
    ~( 1ULL << static_cast<size_t>( tname::segments::CATEGORY ) );
constexpr uint64_t tname_prefix_bits =
    1ULL << static_cast<size_t>( tname::segments::FAVORITE_PRE ) |
    1ULL << static_cast<size_t>( tname::segments::DURABILITY ) |
    1ULL << static_cast<size_t>( tname::segments::BURN );
constexpr uint64_t tname_unsortable_bits =
    tname_prefix_bits |
    ( 1ULL << static_cast<size_t>( tname::segments::CONTENTS_COUNT ) );
constexpr uint64_t tname_contents_bits =
    1ULL << static_cast<size_t>( tname::segments::CONTENTS ) |
    1ULL << static_cast<size_t>( tname::segments::CONTENTS_FULL ) |
    1ULL << static_cast<size_t>( tname::segments::CONTENTS_ABREV ) |
    1ULL << static_cast<size_t>( tname::segments::CONTENTS_COUNT );
constexpr uint64_t tname_conditional_bits =
    1ULL << static_cast<size_t>( tname::segments::COMPONENTS ) |
    1ULL << static_cast<size_t>( tname::segments::TAGS ) |
    1ULL << static_cast<size_t>( tname::segments::VARS );
constexpr uint64_t base_item_name_bits =
    1ULL << static_cast<size_t>( tname::segments::CUSTOM_ITEM_PREFIX ) |
    1ULL << static_cast<size_t>( tname::segments::TYPE ) |
    1ULL << static_cast<size_t>( tname::segments::CUSTOM_ITEM_SUFFIX );
constexpr uint64_t variant_bits =
    1ULL << static_cast<size_t>( tname::segments::VARIANT );
constexpr segment_bitset default_tname( default_tname_bits );
constexpr segment_bitset unprefixed_tname( default_tname_bits & ~tname_prefix_bits );
constexpr segment_bitset tname_sort_key( default_tname_bits & ~tname_unsortable_bits );
constexpr segment_bitset tname_contents( tname_contents_bits );
constexpr segment_bitset tname_conditional( tname_conditional_bits );




constexpr segment_bitset base_item_name( base_item_name_bits );



constexpr segment_bitset item_identity_name( base_item_name_bits | variant_bits );

}

#endif
