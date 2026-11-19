#include "flexbuffer_cache.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <flatbuffers/flexbuffers.h>
#include <flatbuffers/idl.h>

#include "cata_utility.h"
#include "filesystem.h"
#include "json.h"
#include "mmap_file.h"
#include "options.h"

namespace
{
std::atomic<std::uint64_t> cache_hits{ 0 };
std::atomic<std::uint64_t> cache_misses{ 0 };
std::atomic<std::uint64_t> cache_writes{ 0 };

void try_find_and_throw_json_error( TextJsonValue &jv )
{
    if( jv.test_object() ) {
        TextJsonObject jo = jv.get_object();
        for( TextJsonMember jm : jo ) {
            try_find_and_throw_json_error( jm );
        }
    } else if( jv.test_array() ) {
        TextJsonArray ja = jv.get_array();
        for( TextJsonValue jav : ja ) {
            try_find_and_throw_json_error( jav );
        }
    }
}

std::filesystem::file_time_type get_file_mtime_millis( const std::filesystem::path &path,
        std::error_code &ec )
{
    std::filesystem::file_time_type ret = std::filesystem::last_write_time( path, ec );
    if( ec ) {
        return ret;
    }

    ret = std::filesystem::file_time_type( std::chrono::milliseconds(
            std::chrono::duration_cast<std::chrono::milliseconds>( ret.time_since_epoch() ).count() ) );
    return ret;
}

std::vector<uint8_t> parse_json_to_flexbuffer_(
    const char *buffer,
    const char *source_filename_opt ) noexcept( false )
{
    flatbuffers::IDLOptions opts;
    opts.strict_json = true;
    opts.use_flexbuffers = true;
    opts.no_warnings = true;
    flatbuffers::Parser parser{ opts };
    flexbuffers::Builder fbb;

    if( !parser.ParseFlexBuffer( buffer, source_filename_opt, &fbb ) ) {
        std::istringstream is{ buffer };
        TextJsonIn jsin{ is, source_filename_opt ? source_filename_opt : "<unknown source file>" };

        if( fbb.HasDuplicateKeys() ) {


            TextJsonValue jv = jsin.get_value();
            try_find_and_throw_json_error( jv );

            is.seekg( 0, std::ios_base::beg );
        }

        size_t line = 0;
        size_t col = 0;
        size_t error_offset = 0;
        if( ( error_offset = parser.error_.find( "EOF:" ) ) == std::string::npos ) {





            int scanned_chars = 0;
            // NOLINTNEXTLINE(cert-err34-c,bugprone-unchecked-string-to-number-conversion)
            if( sscanf( parser.error_.c_str(), "%*[^:]:%*[^:]:%zu:%zu: %n", &line, &col,
                        &scanned_chars ) != 2 &&
                // NOLINTNEXTLINE(cert-err34-c,bugprone-unchecked-string-to-number-conversion)
                sscanf( parser.error_.c_str(), "%*[^:]:%zu:%zu: %n", &line, &col, &scanned_chars ) != 2 &&
                // NOLINTNEXTLINE(cert-err34-c,bugprone-unchecked-string-to-number-conversion)
                sscanf( parser.error_.c_str(), "%zu:%zu: %n", &line, &col, &scanned_chars ) != 2 ) {
                line = 0;
                col = 0;
            }
            error_offset = scanned_chars;
        } else {
            error_offset += 5;
        }

        constexpr const char *kErrorPrefix = "error: ";
        if( strncmp( parser.error_.c_str() + error_offset, kErrorPrefix, strlen( kErrorPrefix ) ) == 0 ) {
            error_offset += strlen( kErrorPrefix );
        }

        if( line != 0 ) {

            while( line > 1 ) {
                is.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
                --line;
            }
        } else {

            is.seekg( 0, std::ios_base::end );

            is.peek();
        }
        jsin.error( col ? col - 1 : 0, parser.error_.substr( error_offset ) );
    }

    return std::move( fbb ).GetBuffer();
}

}

namespace
{
struct flexbuffer_vector_storage : flexbuffer_storage {
    std::vector<uint8_t> buffer_;

    explicit flexbuffer_vector_storage( std::vector<uint8_t> &&buffer ) : buffer_{ std::move( buffer ) } {}

    const uint8_t *data() const override {
        return buffer_.data();
    }

    size_t size() const override {
        return buffer_.size();
    }
};

struct flexbuffer_mmap_storage : flexbuffer_storage {
    std::shared_ptr<const mmap_file> mmap_handle_;

    explicit flexbuffer_mmap_storage( std::shared_ptr<const mmap_file> mmap_handle ) : mmap_handle_{ std::move( mmap_handle ) } {}

    const uint8_t *data() const override {
        return static_cast<const uint8_t *>( mmap_handle_->base() );
    }
    size_t size() const override {
        return mmap_handle_->len();
    }
};
}

parsed_flexbuffer::parsed_flexbuffer( std::shared_ptr<flexbuffer_storage> storage )
    : storage_{ std::move( storage ) }
{

}

namespace
{
struct file_flexbuffer : parsed_flexbuffer {
        file_flexbuffer(
            std::shared_ptr<flexbuffer_storage> &&storage,
            std::filesystem::path &&source_file_path,
            std::filesystem::file_time_type mtime,
            size_t offset )
            : parsed_flexbuffer( std::move( storage ) ),
              source_file_path_{ std::move( source_file_path ) },
              mtime_{ mtime },
              offset_{ static_cast<std::streampos>( offset ) } {}

        ~file_flexbuffer() override = default;

        bool is_stale() const override {
            std::error_code ec;
            std::filesystem::file_time_type mtime = get_file_mtime_millis( source_file_path_, ec );
            if( ec ) {

                return true;
            }
            return mtime != mtime_;
        }

        std::unique_ptr<std::istream> get_source_stream() const noexcept( false ) override {
            std::unique_ptr<std::istream> ifs = read_maybe_compressed_file( source_file_path_ );
            if( !ifs || !ifs->good() ) {
                return nullptr;
            }
            ifs->seekg( offset_ );
            return ifs;
        }

        std::filesystem::path get_source_path() const noexcept override {
            return source_file_path_;
        }

    private:
        std::filesystem::path source_file_path_;
        std::filesystem::file_time_type mtime_;
        std::streampos offset_;
};
}

namespace
{
struct string_flexbuffer : parsed_flexbuffer {
        string_flexbuffer( std::shared_ptr<flexbuffer_storage> &&storage, std::string &&source )
            : parsed_flexbuffer{ std::move( storage ) },
              source_{ std::move( source ) } {}

        ~string_flexbuffer() override = default;

        bool is_stale() const override {
            return false;
        }

        std::unique_ptr<std::istream> get_source_stream() const override {


            return std::make_unique<std::istringstream>( source_ );
        }

        std::filesystem::path get_source_path() const noexcept override {
            return {};
        }

    private:
        std::string source_;
};
}

// NOLINTNEXTLINE(misc-use-internal-linkage): forward-declared in flexbuffer_cache.h
class flexbuffer_disk_cache
{
    public:
        static std::unique_ptr<flexbuffer_disk_cache> init_from_folder( const std::filesystem::path
                &cache_path,
                const std::filesystem::path &root_path ) {

            std::unique_ptr<flexbuffer_disk_cache> cache{ new flexbuffer_disk_cache( cache_path, root_path ) };

            std::string cache_path_string = cache_path.u8string();
            std::vector<std::string> all_cached_flexbuffers = get_files_from_path(
                        ".fb",
                        cache_path_string,
                        true,
                        true );

            for( const std::string &cached_flexbuffer : all_cached_flexbuffers ) {




                std::filesystem::path cached_flexbuffer_path = std::filesystem::u8path( cached_flexbuffer );
                const std::filesystem::path versioned = cached_flexbuffer_path.stem();
                const std::string version_str = versioned.extension().u8string();
                const std::filesystem::path sized = versioned.stem();
                const std::string size_str = sized.extension().u8string();
                const std::filesystem::path timestamped = sized.stem();
                const std::string mtime_str = timestamped.extension().u8string();
                const std::filesystem::path original_json_file_name = timestamped.stem();
                if( version_str != ".v2" || size_str.empty() || mtime_str.empty() ||
                    original_json_file_name.empty() ) {
                    remove_file( cached_flexbuffer );
                    continue;
                }


                std::filesystem::file_time_type cached_mtime = std::filesystem::file_time_type(
                            std::chrono::milliseconds( std::strtoull(

                                    mtime_str.c_str() + 1,
                                    nullptr, 10 ) ) );
                const std::uintmax_t cached_size = std::strtoull( size_str.c_str() + 1, nullptr, 10 );

                std::filesystem::path root_relative_json_path =
                    cached_flexbuffer_path.parent_path().lexically_relative(
                        cache_path ) / original_json_file_name;
                std::string root_relative_json_path_string = root_relative_json_path.u8string();


                auto old_entry = cache->cached_flexbuffers_.find( root_relative_json_path_string );
                if( old_entry != cache->cached_flexbuffers_.end() ) {
                    if( old_entry->second.mtime < cached_mtime ) {

                        remove_file( old_entry->second.flexbuffer_path.u8string() );
                        old_entry->second = disk_cache_entry{ cached_flexbuffer_path, cached_mtime, cached_size };
                    } else {

                        remove_file( cached_flexbuffer );
                    }

                } else {
                    cache->cached_flexbuffers_.emplace( root_relative_json_path_string, disk_cache_entry{ cached_flexbuffer_path, cached_mtime, cached_size } );
                }
            }
            return cache;
        }

        bool has_cached_flexbuffer_for_json( const std::filesystem::path &json_source_path ) {
            return cached_flexbuffers_.count( json_source_path.u8string() ) > 0;
        }

        std::filesystem::file_time_type cached_mtime_for_json( const std::filesystem::path
                &json_source_path ) {
            auto it = cached_flexbuffers_.find( json_source_path.u8string() );
            if( it != cached_flexbuffers_.end() ) {
                return it->second.mtime;
            }
            return {};
        }

        std::shared_ptr<flexbuffer_mmap_storage> load_flexbuffer_if_not_stale(
            const std::filesystem::path &lexically_normal_json_source_path ) {
            std::shared_ptr<flexbuffer_mmap_storage> storage;

            std::filesystem::path root_relative_source_path =
                lexically_normal_json_source_path.lexically_relative(
                    root_path_ ).lexically_normal();


            auto disk_entry = cached_flexbuffers_.find( root_relative_source_path.u8string() );
            if( disk_entry == cached_flexbuffers_.end() ) {
                return storage;
            }

            std::error_code ec;
            std::filesystem::file_time_type source_mtime = get_file_mtime_millis(
                        lexically_normal_json_source_path, ec );
            if( ec ) {
                return storage;
            }





            const std::uintmax_t source_size = std::filesystem::file_size(
                                                    lexically_normal_json_source_path, ec );
            if( ec ) {
                return storage;
            }
            if( source_mtime != disk_entry->second.mtime ||
                source_size != disk_entry->second.source_size ) {
#ifndef NO_STALE_DATA_WARN
                std::string filepath_and_name = disk_entry->first;

                bool stale_game_data = *root_relative_source_path.begin() != std::filesystem::u8path( "config" ) &&
                                       *root_relative_source_path.begin() != std::filesystem::u8path( "achievements" ) &&
                                       *root_relative_source_path.begin() != std::filesystem::u8path( "templates" );
                if( stale_game_data ) {
#if defined(__ANDROID__)





                    DebugLog( D_WARNING, D_MAIN ) << "Regenerating stale Android JSON cache: " <<
                                                  filepath_and_name;
#else
                    if( get_option<bool>( "WARN_ON_MODIFIED" ) ) {
                        debugmsg( "Stale game data detected at %s, did you overwrite old files?  When updating the game you must install to a fresh folder, overwriting old files will cause errors.",
                                  filepath_and_name );
                    } else {

                        DebugLog( D_WARNING, D_MAIN ) << "Stale game data detected (error disabled by user): " <<
                                                      filepath_and_name;
                    }
#endif
                }
#endif

                remove_file( disk_entry->second.flexbuffer_path.u8string() );
                cached_flexbuffers_.erase( disk_entry );
                return storage;
            }


            std::shared_ptr<const mmap_file> mmap_handle = mmap_file::map_file(
                        disk_entry->second.flexbuffer_path );
            if( !mmap_handle ) {
                return storage;
            }

            storage = std::make_shared<flexbuffer_mmap_storage>( mmap_handle );

            return storage;
        }

        bool save_to_disk( const std::filesystem::path &lexically_normal_json_source_path,
                           const std::vector<uint8_t> &flexbuffer_binary ) {
            std::error_code ec;
            std::filesystem::file_time_type mtime = get_file_mtime_millis( lexically_normal_json_source_path,
                                                    ec );
            if( ec ) {
                return false;
            }

            const std::uintmax_t source_size = std::filesystem::file_size(
                                                   lexically_normal_json_source_path, ec );
            if( ec ) {
                return false;
            }
            int64_t mtime_ms = std::chrono::duration_cast<std::chrono::milliseconds>
                               ( mtime.time_since_epoch() ).count();


            std::filesystem::path flexbuffer_path = ( cache_path_ /
                                                    lexically_normal_json_source_path.lexically_relative(
                                                            root_path_ ) ).remove_filename();

            assure_dir_exist( flexbuffer_path );

            std::filesystem::path flexbuffer_filename = lexically_normal_json_source_path.filename();
            flexbuffer_filename += std::filesystem::u8path( "." + std::to_string( mtime_ms ) +
                                   "." + std::to_string( source_size ) + ".v2.fb" );
            flexbuffer_path /= flexbuffer_filename;
            std::ofstream fb( flexbuffer_path, std::ofstream::binary );
            if( !fb.good() ) {
                return false;
            }

            fb.write( reinterpret_cast<const char *>( flexbuffer_binary.data() ), flexbuffer_binary.size() );
            if( !fb.good() ) {
                return false;
            }

            fb.close();


            const std::string key = lexically_normal_json_source_path.lexically_relative(
                                        root_path_ ).lexically_normal().u8string();
            auto old = cached_flexbuffers_.find( key );
            if( old != cached_flexbuffers_.end() && old->second.flexbuffer_path != flexbuffer_path ) {
                remove_file( old->second.flexbuffer_path.u8string() );
            }
            cached_flexbuffers_[key] = disk_cache_entry{ flexbuffer_path, mtime, source_size };

            return true;
        }

    private:
        explicit flexbuffer_disk_cache( std::filesystem::path cache_path,
                                        std::filesystem::path root_path ) : cache_path_{ std::move( cache_path ) },
            root_path_{ std::move( root_path ) } {}

        std::filesystem::path cache_path_;
        std::filesystem::path root_path_;

        struct disk_cache_entry {
            std::filesystem::path flexbuffer_path;
            std::filesystem::file_time_type mtime;
            std::uintmax_t source_size;
        };

        std::unordered_map<std::string, disk_cache_entry> cached_flexbuffers_;
};

flexbuffer_cache::flexbuffer_cache( const std::filesystem::path &cache_directory,
                                    const std::filesystem::path &root_directory )
{
    if( !cache_directory.empty() ) {
        disk_cache_ = flexbuffer_disk_cache::init_from_folder( cache_directory, root_directory );
    }
}

flexbuffer_cache::~flexbuffer_cache() = default;

flexbuffer_cache_stats flexbuffer_cache::statistics()
{
    return { cache_hits.load( std::memory_order_relaxed ),
             cache_misses.load( std::memory_order_relaxed ),
             cache_writes.load( std::memory_order_relaxed ) };
}

std::shared_ptr<parsed_flexbuffer> flexbuffer_cache::parse( std::filesystem::path json_source_path,
        size_t offset )
{
    std::string json_source_path_string = json_source_path.generic_u8string();
    std::shared_ptr<const mmap_file> json_source = mmap_file::map_file( json_source_path );
    if( !json_source ) {
        throw std::runtime_error( "Failed to mmap " + json_source_path_string );
    }

    const char *json_text = reinterpret_cast<const char *>( json_source->base() ) + offset;

    std::vector<uint8_t> fb = parse_json_to_flexbuffer_( json_text, json_source_path_string.c_str() );

    auto storage = std::make_shared<flexbuffer_vector_storage>( std::move( fb ) );

    std::error_code ec;

    std::filesystem::file_time_type mtime = get_file_mtime_millis( json_source_path, ec );
    ( void )ec;

    return std::make_shared<file_flexbuffer>(
               std::move( storage ),
               std::move( json_source_path ),
               mtime,
               offset );
}

std::shared_ptr<parsed_flexbuffer> flexbuffer_cache::parse_and_cache(
    std::filesystem::path lexically_normal_json_source_path, size_t offset )
{


    if( disk_cache_ ) {
        std::shared_ptr<flexbuffer_mmap_storage> cached_storage = disk_cache_->load_flexbuffer_if_not_stale(
                    lexically_normal_json_source_path );
        if( cached_storage ) {
            cache_hits.fetch_add( 1, std::memory_order_relaxed );
            std::error_code ec;
            std::filesystem::file_time_type mtime = get_file_mtime_millis( lexically_normal_json_source_path,
                                                    ec );
            ( void )ec;

            return std::make_shared<file_flexbuffer>( std::move( cached_storage ),
                    std::move( lexically_normal_json_source_path ), mtime, offset );
        }
    }

    if( disk_cache_ ) {
        cache_misses.fetch_add( 1, std::memory_order_relaxed );
    }
    std::string json_source_path_string = lexically_normal_json_source_path.generic_u8string();
    std::optional<std::string> json_file_contents = read_whole_file(
                lexically_normal_json_source_path );
    if( !json_file_contents.has_value() || json_file_contents->empty() ) {
        throw std::runtime_error( "Failed to read " + json_source_path_string );
    }
    std::string &json_source = *json_file_contents;

    const char *json_text = reinterpret_cast<const char *>( json_source.c_str() ) + offset;
    std::vector<uint8_t> fb = parse_json_to_flexbuffer_( json_text, json_source_path_string.c_str() );

    if( disk_cache_ && disk_cache_->save_to_disk( lexically_normal_json_source_path, fb ) ) {
        cache_writes.fetch_add( 1, std::memory_order_relaxed );
    }

    auto storage = std::make_shared<flexbuffer_vector_storage>( std::move( fb ) );

    std::error_code ec;
    std::filesystem::file_time_type mtime = std::filesystem::last_write_time(
            lexically_normal_json_source_path, ec );
    if( ec ) {

    }

    return std::make_shared<file_flexbuffer>( std::move( storage ),
            std::move( lexically_normal_json_source_path ),
            mtime, offset );
}

std::shared_ptr<parsed_flexbuffer> flexbuffer_cache::parse_buffer( std::string buffer )
{
    std::vector<uint8_t> fb = parse_json_to_flexbuffer_( buffer.c_str(), nullptr );
    auto storage = std::make_shared<flexbuffer_vector_storage>( std::move( fb ) );
    return std::make_shared<string_flexbuffer>( std::move( storage ), std::move( buffer ) );
}
