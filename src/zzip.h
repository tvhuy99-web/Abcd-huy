#pragma once
#ifndef CATA_SRC_ZZIP_H
#define CATA_SRC_ZZIP_H

#include <stdint.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "flexbuffer_json.h"

class mmap_file;
struct std_fs_path_hash;
































class zzip
{
        zzip( std::shared_ptr<mmap_file> file, JsonObject footer );

    public:
        ~zzip() noexcept;

        zzip( zzip && ) noexcept ;
        zzip &operator=( zzip && ) noexcept ;








        static std::optional<zzip> load( std::filesystem::path const &path,
                                         std::filesystem::path const &dictionary = {} );








        static std::optional<zzip> load( std::shared_ptr<mmap_file> file,
                                         std::filesystem::path const &dictionary = {} );








        bool add_file( std::filesystem::path const &zzip_relative_path, std::string_view content );





        bool copy_files( std::vector<std::filesystem::path> const &zzip_relative_paths,
                         zzip const &from, bool shrink_to_fit = false );





        bool has_file( std::filesystem::path const &zzip_relative_path ) const;





        size_t get_file_size( std::filesystem::path const &zzip_relative_path ) const;





        size_t get_content_size() const;




        size_t get_zzip_size() const;




        size_t get_entry_size( std::filesystem::path const &zzip_relative_path ) const;




        std::vector<std::filesystem::path> get_entries() const;





        std::vector<std::byte> get_file( std::filesystem::path const &zzip_relative_path ) const;







        size_t get_file_to( std::filesystem::path const &zzip_relative_path,
                            std::byte *dest,
                            size_t dest_len ) const;







        bool delete_files( std::unordered_set<std::filesystem::path, std_fs_path_hash> const
                           &zzip_relative_paths );








        bool compact_to( std::filesystem::path const &dest, double bloat_factor = 1.0 );





        bool compact_to( std::shared_ptr<mmap_file> const &dest ) const;





        bool clear();






        static std::optional<zzip> create_from_folder( std::filesystem::path const &path,
                std::filesystem::path const &folder,
                std::filesystem::path const &dictionary = {} );

        static std::optional<zzip> create_from_folder_with_files( std::filesystem::path const &path,
                std::filesystem::path const &folder,
                const std::vector<std::filesystem::path> &files,
                uint64_t total_file_size,
                std::filesystem::path const &dictionary = {} );





        static bool extract_to_folder( std::filesystem::path const &path,
                                       std::filesystem::path const &folder,
                                       std::filesystem::path const &dictionary = {} );
        bool extract_to_folder( std::filesystem::path const &folder );

        struct compressed_entry;







        struct entry_layout {
            std::filesystem::path path;
            size_t offset;
            size_t len;
        };
        std::vector<entry_layout> get_layout() const;

    private:
        JsonObject copy_footer() const;
        size_t ensure_capacity_for( size_t bytes );
        size_t write_file_at( std::string_view filename, std::string_view content, size_t offset,
                              std::optional<uint64_t> force_checksum = std::nullopt );

        bool update_footer( JsonObject const &original_footer, size_t content_end,
                            const std::vector<compressed_entry> &entries, bool shrink_to_fit = false );

        bool rewrite_footer( bool shrink_to_fit = false );

        std::shared_ptr<mmap_file> file_;
        JsonObject footer_;

        void *file_base_plus( size_t offset ) const;
        size_t file_capacity_at( size_t offset ) const;

        struct context;
        std::unique_ptr<context> ctx_;
};

#endif
