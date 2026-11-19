#pragma once
#ifndef CATA_SRC_CATA_PATH_H
#define CATA_SRC_CATA_PATH_H

#include <iosfwd>
#include <string>

#include <filesystem>

#include <fmt/format.h>

























class cata_path
{
    public:



        enum class root_path {
            base,
            config,
            data,
            memorial,
            save,
            user,
            unknown,

        };

        cata_path() : logical_root_{ root_path::unknown } {}

        template<typename T>
        cata_path( root_path logical_root, T &&relative_path ) : logical_root_{ logical_root },
            relative_path_{ as_fs_path( std::forward<T>( relative_path ) ) } { }

        cata_path( const cata_path & ) = default;
        cata_path &operator=( const cata_path & ) = default;

        cata_path( cata_path && ) = default;
        cata_path &operator=( cata_path && ) = default;




        inline std::filesystem::path get_unrelative_path() const {
            std::filesystem::path result = get_logical_root_path();
            result /= relative_path_;
            return result;
        }

        explicit inline operator std::filesystem::path() const {
            return get_unrelative_path();
        }

        inline root_path get_logical_root() const {
            return logical_root_;
        }


        std::filesystem::path get_logical_root_path() const;



        inline const std::filesystem::path &get_relative_path() const {
            return relative_path_;
        }




        template<typename T>
        cata_path operator/( T &&segment ) const {
            cata_path ret = *this;
            ret /= as_fs_path( std::forward<T>( segment ) );
            return ret;
        }

        template<typename T>
        cata_path &operator/=( T &&segment ) {
            relative_path_ /= as_fs_path( std::forward<T>( segment ) );
            return *this;
        }

        template<typename T>
        cata_path operator+( T &&segment ) const {
            cata_path ret = *this;
            ret += as_fs_path( std::forward<T>( segment ) );
            return ret;
        }

        template<typename T>
        cata_path &operator+=( T &&segment ) {
            relative_path_ += as_fs_path( std::forward<T>( segment ) );
            return *this;
        }

        inline cata_path lexically_normal() const {
            cata_path ret{ logical_root_, std::filesystem::path{} };
            ret.relative_path_ = relative_path_.lexically_normal();
            return ret;
        }

        inline bool empty() const {
            return logical_root_ == root_path::unknown && relative_path_.empty();
        }

        inline bool operator==( const cata_path &rhs ) const {
            return logical_root_ == rhs.logical_root_ && relative_path_ == rhs.relative_path_;
        }

        template< class CharT, class Traits >
        friend std::basic_ostream<CharT, Traits> &
        operator<<( std::basic_ostream<CharT, Traits> &os, const cata_path &p ) {
            os << p.get_unrelative_path();
            return os;
        }

        inline cata_path parent_path() const {
            return cata_path{ logical_root_, relative_path_.parent_path() };
        }

        inline std::string generic_u8string() const {
            return get_unrelative_path().generic_u8string();
        }

    private:
        root_path logical_root_;
        std::filesystem::path relative_path_;

        template<typename T>
        static std::enable_if_t < std::is_same_v<std::decay_t<T>, std::filesystem::path>, T && >
        as_fs_path( T &&path ) {
            return std::forward<T>( path );
        }

        template<typename T>
        static std::enable_if_t < !std::is_same_v<std::decay_t<T>, std::filesystem::path>,
               std::filesystem::path >
        as_fs_path( T &&path ) {
            return std::filesystem::u8path( std::forward<T>( path ) );
        }
};

template <>
struct fmt::formatter<cata_path> : formatter<std::string_view> {
    auto format( const cata_path &path, fmt::format_context &ctx ) const -> decltype( ctx.out() );
};

#endif
