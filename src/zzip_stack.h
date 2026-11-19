#pragma once
#ifndef CATA_SRC_ZZIP_STACK_H
#define CATA_SRC_ZZIP_STACK_H

#include <stdint.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "std_hash_fs_path.h"
#include "zzip.h"













class zzip_stack
{
        zzip_stack( std::filesystem::path const &path,
                    std::filesystem::path const &dictionary_path ) noexcept;
    public:
        ~zzip_stack() noexcept;





        static std::shared_ptr<zzip_stack> load( std::filesystem::path const &path,
                std::filesystem::path const &dictionary = {} );





        bool add_file( std::filesystem::path const &zzip_relative_path, std::string_view content );




        bool copy_files_to( std::vector<std::filesystem::path> const &zzip_relative_paths,
                            zzip &to );





        bool has_file( std::filesystem::path const &zzip_relative_path ) const;





        size_t get_file_size( std::filesystem::path const &zzip_relative_path ) const;




        std::vector<std::filesystem::path> get_entries() const;





        std::vector<std::byte> get_file( std::filesystem::path const &zzip_relative_path ) const;







        size_t get_file_to( std::filesystem::path const &zzip_relative_path,
                            std::byte *dest,
                            size_t dest_len ) const;







        bool delete_files( std::unordered_set<std::filesystem::path, std_fs_path_hash> const
                           &zzip_relative_paths );








        bool compact( double bloat_factor = 1.0 );






        static std::shared_ptr<zzip_stack> create_from_folder( std::filesystem::path const &path,
                std::filesystem::path const &folder,
                std::filesystem::path const &dictionary = {} );

        static std::shared_ptr<zzip_stack> create_from_folder_with_files( std::filesystem::path const &path,
                std::filesystem::path const &folder,
                std::vector<std::filesystem::path> const &files,
                uint64_t total_file_size,
                std::filesystem::path const &dictionary = {} );





        static bool extract_to_folder( std::filesystem::path const &path,
                                       std::filesystem::path const &folder,
                                       std::filesystem::path const &dictionary = {} );

    private:
        zzip &hot() const;
        zzip &warm() const;
        zzip &cold() const;

        std::filesystem::path path_;
        std::filesystem::path dictionary_;

        enum class file_temp : int {
            unknown,
            cold,
            warm,
            hot,
        };

        mutable std::optional<zzip> hot_;
        mutable std::optional<zzip> warm_;
        mutable std::optional<zzip> cold_;
        mutable std::unordered_map<std::filesystem::path, file_temp, std_fs_path_hash> path_temp_map_;
        file_temp temp_of_file( std::filesystem::path const &zzip_relative_path ) const;

        static std::filesystem::path filename_of_temp( std::filesystem::path const &folder,
                file_temp temp );
        zzip &zzip_of_temp( file_temp temp ) const;
        std::optional<zzip> load_temp( file_temp temp ) const;
};

#endif
