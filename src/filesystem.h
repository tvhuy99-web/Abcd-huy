#pragma once
#ifndef CATA_SRC_FILESYSTEM_H
#define CATA_SRC_FILESYSTEM_H

#include <filesystem>
#include <fstream>
#include <string> // IWYU pragma: keep
#include <vector>

#include "cata_path.h"
#include "catacharset.h"
#include "compatibility.h"
#include "path_info.h"

bool assure_dir_exist( const std::string &path );
bool dir_exist( const std::string &path );
bool file_exist( const std::string &path );


bool remove_file( const std::string &path );
bool remove_directory( const std::string &path );

bool rename_file( const std::string &old_path, const std::string &new_path );

std::string abs_path( const std::string &path );

std::string read_entire_file( const std::string &path );


bool assure_dir_exist( const std::filesystem::path &path );
bool assure_dir_exist( const cata_path &path );
bool dir_exist( const std::filesystem::path &path );
bool file_exist( const std::filesystem::path &path );
bool file_exist( const cata_path &path );


std::string as_norm_dir( const std::string &path );
std::string as_norm_dir( const std::filesystem::path &path );



bool remove_file( const std::filesystem::path &path );
bool remove_file( const cata_path &path );
bool remove_directory( const std::filesystem::path &path );

bool rename_file( const std::filesystem::path &old_path, const std::filesystem::path &new_path );
bool rename_file( const cata_path &old_path, const cata_path &new_path );

std::filesystem::path abs_path( const std::filesystem::path &path );

std::string read_entire_file( const std::filesystem::path &path );

namespace cata_files
{
const char *eol();
}














template<typename P>
std::vector<P> get_files_from_path( const std::string &pattern,
                                    const P &root_path = P(), bool recursive_search = false,
                                    bool match_extension = false );














template<typename P>
std::vector<P> get_files_from_path_with_path_exclusion( const std::string &pattern,
        const std::string &pattern_clash,
        const P &root_path = P(), bool recursive_search = false,
        bool match_extension = false );








template<typename P>
std::vector<P> get_directories_with( const std::vector<std::string> &patterns,
                                     const P &root_path = P(), bool recursive_search = false );

template<typename P>
std::vector<P> get_directories_with( const std::string &pattern,
                                     const P &root_path = P(), bool recursive_search = false );

template<typename P>
std::vector<P> get_directories( const P &root_path = P(),
                                bool recursive_search = false );

bool copy_file( const std::string &source_path, const std::string &dest_path );
bool copy_file( const cata_path &source_path, const cata_path &dest_path );







std::string ensure_valid_file_name( const std::string &file_name );

#if defined(_WIN32)

bool is_lexically_valid( const std::filesystem::path & );
#else
constexpr bool is_lexically_valid( const std::filesystem::path & )
{
    return true;
}
#endif

#endif
