#pragma once
#ifndef CATA_SRC_POCKET_TYPE_H
#define CATA_SRC_POCKET_TYPE_H

template <typename E> struct enum_traits;

enum class pocket_type : int {
    CONTAINER,
    MAGAZINE,
    MAGAZINE_WELL,
    MOD,
    CORPSE,
    SOFTWARE,
    E_FILE_STORAGE,
    CABLE,
    MIGRATION,
    EBOOK,
    LAST
};

template<>
struct enum_traits<pocket_type> {
    static constexpr pocket_type last = pocket_type::LAST;
};

#endif
