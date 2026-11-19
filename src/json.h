#pragma once
#ifndef CATA_SRC_JSON_H
#define CATA_SRC_JSON_H

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "colony.h"
#include "enum_conversions.h"
#include "json_error.h"
#include "int_id.h"
#include "memory_fast.h"
#include "string_id.h"

/* Cataclysm-DDA homegrown JSON tools
 * copyright CC-BY-SA-3.0 2013 CleverRaven
 *
 * Consists of four JSON manipulation tools:
 * TextJsonIn - for low-level parsing of an input JSON stream
 * JsonOut - for outputting JSON
 * TextJsonObject - convenience-wrapper for reading JSON objects from a TextJsonIn
 * TextJsonArray - convenience-wrapper for reading JSON arrays from a TextJsonIn
 *
 * Further documentation can be found below.
 */

template<typename E>
class enum_bitset;
class TextJsonArray;
class TextJsonIn;
class TextJsonObject;
class TextJsonValue;
class item;


template< class, class = void >
struct is_string_like : std::false_type { };
template< class T >
struct is_string_like<T, std::void_t<decltype( std::declval<T &>().substr() )>> :
std::true_type { };

template<typename T, typename Enable = void>
struct key_from_json_string {
    static constexpr bool valid = false;
};

template<>
struct key_from_json_string<std::string, void> {
    static constexpr bool valid = true;
    std::string operator()( const std::string &s ) {
        return s;
    }
};

template<typename T>
struct key_from_json_string<string_id<T>, void> {
    static constexpr bool valid = true;
    string_id<T> operator()( const std::string &s ) {
        return string_id<T>( s );
    }
};

template<typename T>
struct key_from_json_string<int_id<T>, void> {
    static constexpr bool valid = true;
    int_id<T> operator()( const std::string &s ) {
        return int_id<T>( s );
    }
};

template<typename Enum>
struct key_from_json_string<Enum, std::enable_if_t<std::is_enum_v<Enum>>> {
    static constexpr bool valid = true;
    Enum operator()( const std::string &s ) {
        return io::string_to_enum<Enum>( s );
    }
};

template<typename T>
struct key_from_json_string<T, std::void_t<decltype( std::declval<T &>().to_string_writable() )>> {
    static constexpr bool valid = true;
    T operator()( const std::string &s ) {
        return T::from_string( s );
    }
};

struct number_sci_notation {
    bool negative = false;

    uint64_t number = 0;

    int64_t exp = 0;
};

struct json_source_location {
    shared_ptr_fast<std::string> path;
    int offset = 0;
};





enum class json_error_output_colors_t : int32_t {
    unset = 0,
    no_colors = 1,
    color_tags = 2,
    ansi_escapes = 3,
};

extern json_error_output_colors_t json_error_output_colors;

class TextJsonValue
{
    private:
        TextJsonIn &jsin_;
        int pos_;

        TextJsonIn &seek() const;

    public:
        TextJsonValue( TextJsonIn &jsin, int pos ) : jsin_( jsin ), pos_( pos ) { }

        // NOLINTNEXTLINE(google-explicit-constructor)
        operator std::string() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator int() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator bool() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator double() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator TextJsonObject() const;
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator TextJsonArray() const;
        template<typename T>
        bool read( T &t, bool throw_on_error = false ) const;

        bool test_string() const;
        bool test_int() const;
        bool test_bool() const;
        bool test_float() const;
        bool test_object() const;
        bool test_array() const;
        bool test_null() const;

        std::string get_string() const;
        int get_int() const;
        bool get_bool() const;
        double get_float() const;
        TextJsonObject get_object() const;
        TextJsonArray get_array() const;

        void get_null() const;

        [[noreturn]] void string_error( const std::string &err ) const;
        [[noreturn]] void string_error( int offset, const std::string &err ) const;
        [[noreturn]] void throw_error( const std::string &err ) const;
        [[noreturn]] void throw_error( int offset, const std::string &err ) const;
        [[noreturn]] void throw_error_after( const std::string &err ) const;
};

















































































class TextJsonIn
{
    private:
        std::istream *stream;
        shared_ptr_fast<std::string> path;
        bool ate_separator = false;

        void sanity_check_stream();
        void skip_separator();
        void skip_pair_separator();
        void end_value();

    public:
        explicit TextJsonIn( std::istream &s );
        TextJsonIn( std::istream &s, const std::string &path );
        TextJsonIn( std::istream &s, const json_source_location &loc );
        TextJsonIn( const TextJsonIn & ) = delete;
        TextJsonIn &operator=( const TextJsonIn & ) = delete;

        shared_ptr_fast<std::string> get_path() const {
            return path;
        }

        bool get_ate_separator() const {
            return ate_separator;
        }
        void set_ate_separator( bool s ) {
            ate_separator = s;
        }

        int tell();
        void seek( int pos );
        char peek();
        bool good();


        void eat_whitespace();

        void uneat_whitespace();


        void skip_member();
        void skip_string();
        void skip_value();
        void skip_object();
        void skip_array();
        void skip_true();
        void skip_false();
        void skip_null();
        void skip_number();


        std::string get_string();
        int get_int();
        unsigned int get_uint();
        int64_t get_int64();
        uint64_t get_uint64();
        bool get_bool();
        double get_float();
        std::string get_member_name();
        TextJsonObject get_object();
        TextJsonArray get_array();
        TextJsonValue get_value();

        template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
        E get_enum_value() {
            const int old_offset = tell();
            try {
                return io::string_to_enum<E>( get_string() );
            } catch( const io::InvalidEnumString & ) {
                seek( old_offset );
                error( "invalid enumeration value" );
            }
        }


        void start_array();
        bool end_array();
        void start_object();
        bool end_object();


        bool test_null();
        bool test_bool();
        bool test_number();
        bool test_int() {
            return test_number();
        }
        bool test_float() {
            return test_number();
        }
        bool test_string();
        bool test_bitset();
        bool test_array();
        bool test_object();




        bool read_null( bool throw_on_error = false );
        bool read( bool &b, bool throw_on_error = false );
        bool read( char &c, bool throw_on_error = false );
        bool read( signed char &c, bool throw_on_error = false );
        bool read( unsigned char &c, bool throw_on_error = false );
        bool read( short unsigned int &s, bool throw_on_error = false );
        bool read( short int &s, bool throw_on_error = false );
        bool read( int &i, bool throw_on_error = false );
        bool read( int64_t &i, bool throw_on_error = false );
        bool read( uint64_t &i, bool throw_on_error = false );
        bool read( unsigned int &u, bool throw_on_error = false );
        bool read( float &f, bool throw_on_error = false );
        bool read( double &d, bool throw_on_error = false );
        bool read( std::string &s, bool throw_on_error = false );
        template<size_t N>
        bool read( std::bitset<N> &b, bool throw_on_error = false );

        template <typename T>
        auto read( string_id<T> &thing, bool throw_on_error = false ) -> bool {
            std::string tmp;
            if( !read( tmp, throw_on_error ) ) {
                return false;
            }
            thing = string_id<T>( tmp );
            return true;
        }


        template <typename T>
        auto read( int_id<T> &thing, bool throw_on_error = false ) -> bool {
            std::string tmp;
            if( !read( tmp, throw_on_error ) ) {
                return false;
            }
            thing = int_id<T>( tmp );
            return true;
        }


        template<typename T>
        auto read( T &v, bool throw_on_error = false ) ->
        decltype( deserialize( v, std::declval<const TextJsonValue &>() ), true ) {
            try {
                deserialize( v, this->get_value() );
                return true;
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }
        }


        template<typename T>
        auto read( T &v, bool throw_on_error = false ) -> decltype( v.deserialize(
                    std::declval<const TextJsonValue &>() ), true ) {
            try {
                v.deserialize( this->get_value() );
                return true;
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }
        }

        template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
        bool read( T &val, bool throw_on_error = false ) {
            int i;
            if( read( i, false ) ) {
                val = static_cast<T>( i );
                return true;
            }
            std::string s;
            if( read( s, throw_on_error ) ) {
                val = io::string_to_enum<T>( s );
                return true;
            }
            return false;
        }


        template<typename T, typename U>
        bool read( std::pair<T, U> &p, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array encoding pair" );
            }
            try {
                start_array();
                bool result = !end_array() &&
                              read( p.first, throw_on_error ) &&
                              !end_array() &&
                              read( p.second, throw_on_error ) &&
                              end_array();
                if( !result && throw_on_error ) {
                    error( "Array had wrong number of elements for pair" );
                }
                return result;
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }
        }


        template < typename T, typename std::enable_if_t <
                       !std::is_same_v<void, typename T::value_type> &&
                       !is_string_like<T>::value
                       > * = nullptr >
        auto read( T &v, bool throw_on_error = false ) -> decltype( v.front(), true ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    typename T::value_type element;
                    if( read( element, throw_on_error ) ) {
                        v.push_back( std::move( element ) );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }


        template <typename T, size_t N>
        bool read( std::array<T, N> &v, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                for( size_t i = 0; i < N; ++i ) {
                    if( end_array() ) {
                        if( throw_on_error ) {
                            error( "Json array is too short" );
                        }
                        return false;
                    }
                    if( !read( v[i], throw_on_error ) ) {
                        return false;
                    }
                }
                bool result = end_array();
                if( !result && throw_on_error ) {
                    error( "Array had too many elements" );
                }
                return result;
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }
        }



        template <typename T, std::enable_if_t<
                      std::is_same_v<typename T::key_type, typename T::value_type>> * = nullptr
                  >
        bool read( T &v, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    typename T::value_type element;
                    if( read( element, throw_on_error ) ) {
                        if( !v.insert( std::move( element ) ).second ) {
                            return error_or_false(
                                       throw_on_error,
                                       "Duplicate entry in set defined by json array" );
                        }
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }


        template <typename T>
        bool read( enum_bitset<T> &v, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                v = {};
                while( !end_array() ) {
                    T element;
                    if( read( element, throw_on_error ) ) {
                        if( v.test( element ) ) {
                            return error_or_false(
                                       throw_on_error,
                                       "Duplicate entry in set defined by json array" );
                        }
                        v.set( element );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }



        template <typename T, std::enable_if_t<std::is_same_v<T, item>> * = nullptr >
        bool read( cata::colony<T> &v, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    T element;
                    const int prev_pos = tell();
                    if( test_array() ) {
                        start_array();
                        int run_l;
                        if( read( element, throw_on_error ) &&
                            read( run_l, throw_on_error ) &&
                            end_array()
                          ) {

                            for( int i = 0; i < run_l - 1; i++ ) {
                                v.insert( element );
                            }

                            v.insert( std::move( element ) );
                        } else {
                            error_or_false( throw_on_error, "Expected end of array" );
                            seek( prev_pos );
                            skip_array();
                        }
                    } else {
                        if( read( element, throw_on_error ) ) {
                            v.insert( std::move( element ) );
                        } else {
                            skip_value();
                        }
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }




        template < typename T, std::enable_if_t < !std::is_same_v<T, item> > * = nullptr >
        bool read( cata::colony<T> &v, bool throw_on_error = false ) {
            if( !test_array() ) {
                return error_or_false( throw_on_error, "Expected json array" );
            }
            try {
                start_array();
                v.clear();
                while( !end_array() ) {
                    T element;
                    if( read( element, throw_on_error ) ) {
                        v.insert( std::move( element ) );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }



        template < typename T, std::enable_if_t <
                       !std::is_same_v<typename T::key_type, typename T::value_type> > * = nullptr
                   >
        bool read( T &m, bool throw_on_error = true ) {
            if( !test_object() ) {
                return error_or_false( throw_on_error, "Expected json object" );
            }
            try {
                start_object();
                m.clear();
                while( !end_object() ) {
                    using key_type = typename T::key_type;
                    key_type key = key_from_json_string<key_type>()( get_member_name() );
                    typename T::mapped_type element;
                    if( read( element, throw_on_error ) ) {
                        m[std::move( key )] = std::move( element );
                    } else {
                        skip_value();
                    }
                }
            } catch( const JsonError & ) {
                if( throw_on_error ) {
                    throw;
                }
                return false;
            }

            return true;
        }


        std::string line_number( int offset_modifier = 0 );
        [[noreturn]] void error( const std::string &message );
        [[noreturn]] void error( int offset, const std::string &message );



        [[noreturn]] void string_error( int offset, const std::string &message );



        bool error_or_false( bool throw_, const std::string &message );
        bool error_or_false( bool throw_, int offset, const std::string &message );
        void rewind( int max_lines = -1, int max_chars = -1 );
        std::string substr( size_t pos, size_t len = std::string::npos );
    private:

        number_sci_notation get_any_number();

        number_sci_notation get_any_int();
};





























class JsonOut
{
    private:
        std::ostream *stream;
        bool pretty_print;
        std::vector<bool> need_wrap;
        int indent_level = 0;
        bool need_separator = false;

    public:
        explicit JsonOut( std::ostream &stream, bool pretty_print = false, int depth = 0 );
        JsonOut( const JsonOut & ) = delete;
        JsonOut &operator=( const JsonOut & ) = delete;


        void write_indent();
        void write_separator();
        void write_member_separator();
        bool get_need_separator() const {
            return need_separator;
        }
        void set_need_separator() {
            need_separator = true;
        }
        std::ostream *get_stream() {
            return stream;
        }
        int tell();
        void seek( int pos );
        void start_pretty();
        void end_pretty();

        void start_object( bool wrap = false );
        void end_object();
        void start_array( bool wrap = false );
        void end_array();


        void write_null();

        template <typename T, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
        void write( T val ) {
            if( need_separator ) {
                write_separator();
            }
            *stream << val;
            need_separator = true;
        }


        template<typename T>
        auto write( const T &v ) -> decltype( serialize( v, *this ), void() ) {
            serialize( v, *this );
        }


        template<typename T>
        auto write( const T &v ) -> decltype( v.serialize( *this ), void() ) {
            v.serialize( *this );
        }


        template<typename T>
        auto write( const std::reference_wrapper<const T> &v ) -> decltype( v.get().serialize( *this ),
                void() ) {
            v.get().serialize( *this );
        }

        template <typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
        void write( T val ) {
            write( static_cast<std::underlying_type_t<T>>( val ) );
        }


        void write( std::string_view val );
        void write( const char *val ) {
            write( std::string( val ) );
        }


        void write( char val ) {
            write( static_cast<int>( val ) );
        }
        void write( signed char val ) {
            write( static_cast<int>( val ) );
        }
        void write( unsigned char val ) {
            write( static_cast<int>( val ) );
        }

        template<size_t N>
        void write( const std::bitset<N> &b );

        template <typename T>
        auto write( const string_id<T> &thing ) {
            write( thing.str() );
        }

        template <typename T>
        auto write( const int_id<T> &thing ) {
            write( thing.id().str() );
        }


        template <typename E, std::enable_if_t<std::is_enum_v<E>> * = nullptr>
        void write_as_string( const E value ) {
            write( io::enum_to_string<E>( value ) );
        }

        void write_as_string( const std::string_view s ) {
            write( s );
        }

        template<typename T>
        void write_as_string( const string_id<T> &s ) {
            write( s );
        }

        template<typename T>
        void write_as_string( const int_id<T> &s ) {
            write( s.id() );
        }

        template <typename T>
        auto write_as_string( const T &value ) -> decltype( std::declval<T &>().to_string_writable(),
                void() ) {
            write( value.to_string_writable() );
        }

        template<typename T, typename U>
        void write( const std::pair<T, U> &p ) {
            start_array();
            write( p.first );
            write( p.second );
            end_array();
        }

        template <typename T>
        void write_as_array( const T &container ) {
            start_array();
            for( const auto &e : container ) {
                write( e );
            }
            end_array();
        }



        template < typename T, typename std::enable_if_t <
                       !std::is_same_v<void, typename T::value_type> &&
                       !is_string_like<T>::value
                       > * = nullptr >
        auto write( const T &container ) -> decltype( container.front(), ( void )0 ) {
            write_as_array( container );
        }



        template <typename T, std::enable_if_t<
                      std::is_same_v<typename T::key_type, typename T::value_type>> * = nullptr
                  >
        void write( const T &container ) {
            write_as_array( container );
        }


        template <typename T, std::enable_if_t<std::is_same_v<T, item>> * = nullptr>
        void write( const cata::colony<T> &container ) {
            start_array();




            int run_l = 1;
            for( auto it = container.begin(); it != container.end(); ++it ) {
                const auto nxt = std::next( it );
                if( nxt == container.end() || !it->same_for_rle( *nxt ) ) {
                    if( run_l == 1 ) {
                        write( *it );
                    } else {
                        start_array();
                        write( *it );
                        write( run_l );
                        end_array();
                    }
                    run_l = 1;
                } else {
                    run_l++;
                }
            }
            end_array();
        }



        template < typename T, std::enable_if_t < !std::is_same_v<T, item> > * = nullptr >
        void write( const cata::colony<T> &container ) {
            write_as_array( container );
        }



        template < typename T, std::enable_if_t <
                       !std::is_same_v<typename T::key_type, typename T::value_type> > * = nullptr
                   >
        void write( const T &map ) {
            start_object();
            for( const auto &it : map ) {
                write_as_string( it.first );
                write_member_separator();
                write( it.second );
            }
            end_object();
        }



        void member( std::string_view name );
        void null_member( std::string_view name );
        template <typename T> void member( const std::string_view name, const T &value ) {
            member( name );
            write( value );
        }
        template <typename T> void member( const std::string_view name, const T &value,
                                           const T &value_default ) {
            if( value != value_default ) {
                member( name, value );
            }
        }
        template <typename T> void member_as_string( const std::string_view name, const T &value ) {
            member( name );
            write_as_string( value );
        }
};











































































class TextJsonObject
{
    private:
        std::map<std::string, int, std::less<>> positions;
        int start;
        int end_;
        bool final_separator;
#ifndef CATA_IN_TOOL
        mutable std::set<std::string> visited_members;
        mutable bool report_unvisited_members = true;
        mutable bool reported_unvisited_members = false;
#endif
        void mark_visited( std::string_view name ) const;
        void report_unvisited() const;

        TextJsonIn *jsin;
        int verify_position( std::string_view name,
                             bool throw_exception = true ) const;

    public:
        explicit TextJsonObject( TextJsonIn &jsin );
        TextJsonObject() :
            start( 0 ), end_( 0 ), final_separator( false ), jsin( nullptr ) {}
        TextJsonObject( const TextJsonObject & ) = default;
        TextJsonObject( TextJsonObject && ) = default;
        TextJsonObject &operator=( const TextJsonObject & ) = default;
        TextJsonObject &operator=( TextJsonObject && ) = default;
        ~TextJsonObject() {
            finish();
        }

        class const_iterator;

        friend const_iterator;

        const_iterator begin() const;
        const_iterator end() const;

        void finish();
        size_t size() const;
        bool empty() const;

        void allow_omitted_members() const;



        void copy_visited_members( const TextJsonObject &rhs ) const;
        bool has_member( const std::string &name ) const;
        std::string str() const;
        [[noreturn]] void throw_error( const std::string &err ) const;
        [[noreturn]] void throw_error_at( std::string_view name, const std::string &err ) const;

        TextJsonIn *get_raw( std::string_view name ) const;
        TextJsonValue get_member( std::string_view name ) const;
        json_source_location get_source_location() const;




        bool get_bool( std::string_view name ) const;
        bool get_bool( std::string_view name, bool fallback ) const;
        int get_int( std::string_view name ) const;
        int get_int( std::string_view name, int fallback ) const;
        double get_float( std::string_view name ) const;
        double get_float( std::string_view name, double fallback ) const;
        std::string get_string( std::string_view name ) const;
        std::string get_string( std::string_view name, const std::string &fallback ) const;

        template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
        E get_enum_value( const std::string &name, const E fallback ) const {
            if( !has_member( name ) ) {
                return fallback;
            }
            mark_visited( name );
            jsin->seek( verify_position( name ) );
            return jsin->get_enum_value<E>();
        }
        template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
        E get_enum_value( const std::string_view name ) const {
            mark_visited( name );
            jsin->seek( verify_position( name ) );
            return jsin->get_enum_value<E>();
        }



        TextJsonArray get_array( std::string_view name ) const;
        std::vector<int> get_int_array( std::string_view name ) const;
        std::vector<std::string> get_string_array( std::string_view name ) const;

        std::vector<std::string> get_as_string_array( std::string_view name ) const;

        TextJsonObject get_object( std::string_view name ) const;


        template<typename T = std::string, typename Res = std::set<T>>
        Res get_tags( std::string_view name ) const;




        bool has_null( std::string_view name ) const;
        bool has_bool( std::string_view name ) const;
        bool has_number( std::string_view name ) const;
        bool has_int( const std::string_view name ) const {
            return has_number( name );
        }
        bool has_float( const std::string_view name ) const {
            return has_number( name );
        }
        bool has_string( std::string_view name ) const;
        bool has_array( std::string_view name ) const;
        bool has_object( std::string_view name ) const;






        template <typename T>
        bool read( const std::string_view name, T &t, bool throw_on_error = true ) const {
            int pos = verify_position( name, false );
            if( !pos ) {
                return false;
            }
            mark_visited( name );
            jsin->seek( pos );
            return jsin->read( t, throw_on_error );
        }


        std::string line_number() const;
};







































































class TextJsonArray
{
    private:
        std::vector<size_t> positions;
        int start;
        size_t index;
        int end_;
        bool final_separator;
        TextJsonIn *jsin;
        void verify_index( size_t i ) const;

    public:
        explicit TextJsonArray( TextJsonIn &jsin );
        TextJsonArray( const TextJsonArray &ja );
        TextJsonArray() : start( 0 ), index( 0 ), end_( 0 ), final_separator( false ), jsin( nullptr ) {}
        ~TextJsonArray() {
            finish();
        }
        TextJsonArray &operator=( const TextJsonArray & );

        void finish();

        bool has_more() const;
        size_t size() const;
        bool empty();
        std::string str();
        [[noreturn]] void throw_error( const std::string &err ) const;
        [[noreturn]] void throw_error( int idx, const std::string &err ) const;

        [[noreturn]] void string_error( int idx, int offset, const std::string &err ) const;


        TextJsonValue next_value();
        bool next_bool();
        int next_int();
        double next_float();
        std::string next_string();
        TextJsonArray next_array();
        TextJsonObject next_object();
        void skip_value();


        bool get_bool( size_t index ) const;
        int get_int( size_t index ) const;
        double get_float( size_t index ) const;
        std::string get_string( size_t index ) const;
        TextJsonArray get_array( size_t index ) const;
        TextJsonObject get_object( size_t index ) const;


        template<typename T = std::string, typename Res = std::set<T>>
        Res get_tags( size_t index ) const;

        class const_iterator;

        const_iterator begin() const;
        const_iterator end() const;


        bool test_null() const;
        bool test_bool() const;
        bool test_number() const;
        bool test_int() const {
            return test_number();
        }
        bool test_float() const {
            return test_number();
        }
        bool test_string() const;
        bool test_bitset() const;
        bool test_array() const;
        bool test_object() const;


        bool has_null( size_t index ) const;
        bool has_bool( size_t index ) const;
        bool has_number( size_t index ) const;
        bool has_int( size_t index ) const {
            return has_number( index );
        }
        bool has_float( size_t index ) const {
            return has_number( index );
        }
        bool has_string( size_t index ) const;
        bool has_array( size_t index ) const;
        bool has_object( size_t index ) const;


        template <typename T> bool read_next( T &t ) {
            verify_index( index );
            jsin->seek( positions[index++] );
            return jsin->read( t );
        }

        template <typename T> bool read( size_t i, T &t, bool throw_on_error = false ) const {
            verify_index( i );
            jsin->seek( positions[i] );
            return jsin->read( t, throw_on_error );
        }
};

// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator std::string() const
{
    return seek().get_string();
}
// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator int() const
{
    return seek().get_int();
}
// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator bool() const
{
    return seek().get_bool();
}
// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator double() const
{
    return seek().get_float();
}
// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator TextJsonObject() const
{
    return seek().get_object();
}
// NOLINTNEXTLINE(google-explicit-constructor)
inline TextJsonValue::operator TextJsonArray() const
{
    return seek().get_array();
}
template<typename T>
inline bool TextJsonValue::read( T &t, bool throw_on_error ) const
{
    return seek().read( t, throw_on_error );
}

inline bool TextJsonValue::test_string() const
{
    return seek().test_string();
}
inline bool TextJsonValue::test_int() const
{
    return seek().test_int();
}
inline bool TextJsonValue::test_bool() const
{
    return seek().test_bool();
}
inline bool TextJsonValue::test_float() const
{
    return seek().test_float();
}
inline bool TextJsonValue::test_object() const
{
    return seek().test_object();
}
inline bool TextJsonValue::test_array() const
{
    return seek().test_array();
}
inline bool TextJsonValue::test_null() const
{
    return seek().test_null();
}

inline std::string TextJsonValue::get_string() const
{
    return seek().get_string();
}
inline int TextJsonValue::get_int() const
{
    return seek().get_int();
}
inline bool TextJsonValue::get_bool() const
{
    return seek().get_bool();
}
inline double TextJsonValue::get_float() const
{
    return seek().get_float();
}
inline TextJsonObject TextJsonValue::get_object() const
{
    return seek().get_object();
}
inline TextJsonArray TextJsonValue::get_array() const
{
    return seek().get_array();
}
inline void TextJsonValue::get_null() const
{
    seek().skip_null();
}

class TextJsonArray::const_iterator
{
    private:
        TextJsonArray array_;
        size_t index_;

    public:
        const_iterator( const TextJsonArray &array, size_t index ) : array_( array ), index_( index ) { }

        const_iterator &operator++() {
            index_++;
            return *this;
        }
        TextJsonValue operator*() const {
            array_.verify_index( index_ );
            return TextJsonValue( *array_.jsin, array_.positions[index_] );
        }

        friend bool operator==( const const_iterator &lhs, const const_iterator &rhs ) {
            return lhs.index_ == rhs.index_;
        }
        friend bool operator!=( const const_iterator &lhs, const const_iterator &rhs ) {
            return !operator==( lhs, rhs );
        }
};

inline TextJsonArray::const_iterator TextJsonArray::begin() const
{
    return const_iterator( *this, 0 );
}

inline TextJsonArray::const_iterator TextJsonArray::end() const
{
    return const_iterator( *this, size() );
}










class TextJsonMember : public TextJsonValue
{
    private:
        const std::string &name_;

    public:
        TextJsonMember( const std::string &name, const TextJsonValue &value ) : TextJsonValue( value ),
            name_( name ) { }

        const std::string &name() const {
            return name_;
        }





        bool is_comment() const {
            return name_ == "//";
        }
};

class TextJsonObject::const_iterator
{
    private:
        const TextJsonObject &object_;
        decltype( TextJsonObject::positions )::const_iterator iter_;

    public:
        const_iterator( const TextJsonObject &object, const decltype( iter_ ) &iter ) : object_( object ),
            iter_( iter ) { }

        const_iterator &operator++() {
            iter_++;
            return *this;
        }
        TextJsonMember operator*() const {
            object_.mark_visited( iter_->first );
            return TextJsonMember( iter_->first, TextJsonValue( *object_.jsin, iter_->second ) );
        }

        friend bool operator==( const const_iterator &lhs, const const_iterator &rhs ) {
            return lhs.iter_ == rhs.iter_;
        }
        friend bool operator!=( const const_iterator &lhs, const const_iterator &rhs ) {
            return !operator==( lhs, rhs );
        }
};

inline TextJsonObject::const_iterator TextJsonObject::begin() const
{
    return const_iterator( *this, positions.begin() );
}

inline TextJsonObject::const_iterator TextJsonObject::end() const
{
    return const_iterator( *this, positions.end() );
}

template <typename T, typename Res>
Res TextJsonArray::get_tags( const size_t index ) const
{
    Res res;

    verify_index( index );
    jsin->seek( positions[ index ] );


    if( jsin->test_string() ) {
        res.insert( T( jsin->get_string() ) );
        return res;
    }

    for( const std::string line : jsin->get_array() ) {
        if( !res.insert( T( line ) ).second ) {
            jsin->error( "duplicate item in set defined by json array" );
        }
    }

    return res;
}

template <typename T, typename Res>
Res TextJsonObject::get_tags( const std::string_view name ) const
{
    Res res;
    int pos = verify_position( name, false );
    if( !pos ) {
        return res;
    }
    mark_visited( name );
    jsin->seek( pos );


    if( jsin->test_string() ) {
        res.insert( T( jsin->get_string() ) );
        return res;
    }


    for( const std::string line : jsin->get_array() ) {
        if( !res.insert( T( line ) ).second ) {
            jsin->error( "duplicate item in set defined by json array" );
        }
    }

    return res;
}





void add_array_to_set( std::set<std::string> &, const TextJsonObject &json,
                       std::string_view name );

std::ostream &operator<<( std::ostream &stream, const JsonError &err );

template<typename T>
void serialize( const std::optional<T> &obj, JsonOut &jsout )
{
    if( obj ) {
        jsout.write( *obj );
    } else {
        jsout.write_null();
    }
}

template<typename T>
void deserialize( std::optional<T> &obj, const TextJsonValue &jsin )
{
    if( jsin.test_null() ) {
        obj.reset();

        jsin.get_null();
    } else {
        obj.emplace();
        jsin.read( *obj, true );
    }
}

#include "flexbuffer_json.h"

#endif
