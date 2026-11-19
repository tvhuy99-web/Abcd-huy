#pragma once
#ifndef CATA_SRC_BEHAVIOR_ORACLE_H
#define CATA_SRC_BEHAVIOR_ORACLE_H

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace behavior
{
enum class status_t : char;








class oracle_t
{
};

status_t return_running( const oracle_t *, std::string_view );

extern std::unordered_map<std::string, std::function<status_t( const oracle_t *, std::string_view )>>
        predicate_map;

extern std::unordered_map<std::string, std::function<float( const oracle_t *, std::string_view )>>
        score_predicate_map;

}
#endif
