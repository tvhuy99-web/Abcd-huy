#pragma once
#ifndef CATA_SRC_STD_HASH_FS_PATH_H
#define CATA_SRC_STD_HASH_FS_PATH_H

#include <filesystem>
#include <utility>




struct std_fs_path_hash {
    auto operator()( const std::filesystem::path &p ) const {
        return std::filesystem::hash_value( p );
    }
};

#endif
