#pragma once
#ifndef CATA_SRC_COMPATIBILITY_H
#define CATA_SRC_COMPATIBILITY_H

#include <fstream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>






constexpr bool list_is_noexcept = std::is_nothrow_move_assignable_v<std::list<int>>;

constexpr bool string_is_noexcept = std::is_nothrow_move_assignable_v<std::string>;

constexpr bool set_is_noexcept = std::is_nothrow_move_constructible_v<std::set<std::string>>;

constexpr bool map_is_noexcept = std::is_nothrow_move_constructible_v<std::map<int, int>>;

constexpr bool basic_ifstream_is_noexcept =
    std::is_nothrow_move_constructible_v<std::basic_ifstream<char>>;
constexpr bool basic_ofstream_is_noexcept =
    std::is_nothrow_move_constructible_v<std::basic_ofstream<char>>;


void reset_floating_point_mode();

#endif
