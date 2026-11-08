#pragma once
#ifndef CATA_SRC_BLUEPRINT_CACHE_H
#define CATA_SRC_BLUEPRINT_CACHE_H

#if defined(__ANDROID__)
#include <filesystem>
#include <string>
#include <vector>

struct build_reqs;

namespace blueprint_cache
{
void begin( const std::filesystem::path &file, const std::string &version,
            const std::vector<std::string> &mods,
            const std::filesystem::path &world_options );
void record_source( const std::filesystem::path &file );
void finish_sources();
const std::vector<build_reqs> *find( const std::string &recipe_id );
void store( const std::string &recipe_id, std::vector<build_reqs> values );
void persist();
void clear();
} // namespace blueprint_cache

#endif // __ANDROID__
#endif // CATA_SRC_BLUEPRINT_CACHE_H
