#pragma once
#ifndef CATA_SRC_CATACHARSET_H
#define CATA_SRC_CATACHARSET_H

#include <cstddef>
#include <cstdint>
#include <bitset>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

constexpr int ANY_LENGTH = 5;
constexpr int NULL_UNICODE = 0x0000;
constexpr int PERCENT_SIGN_UNICODE = 0x0025;
constexpr int UNKNOWN_UNICODE = 0xFFFD;

class utf8_wrapper;


uint32_t UTF8_getch( const char **src, int *srclen );
inline uint32_t UTF8_getch( const std::string &str )
{
    const char *utf8str = str.c_str();
    int len = str.length();
    return UTF8_getch( &utf8str, &len );
}

int cursorx_to_position( const char *line, int cursorx, int *prevpos = nullptr, int maxlen = -1 );
int utf8_width( std::string_view str, bool ignore_tags = false );
int utf8_width( const utf8_wrapper &str, bool ignore_tags = false );

std::string left_justify( const std::string &str, int width, bool ignore_tags = false );
std::string right_justify( const std::string &str, int width, bool ignore_tags = false );
std::string utf8_justify( const std::string &str, int width, bool ignore_tags = false );








int center_text_pos( const char *text, int start_pos, int end_pos );
int center_text_pos( const std::string &text, int start_pos, int end_pos );
int center_text_pos( const utf8_wrapper &text, int start_pos, int end_pos );
std::string utf32_to_utf8( uint32_t ch );
std::string utf8_truncate( const std::string &s, size_t length );

std::string base64_encode( const std::string_view &str );
std::string base64_decode( const std::string_view &str );

std::string base64_encode_bitset( const std::bitset<576> &bitset_to_encode );
void base64_decode_bitset( std::string_view packed_bitset,
                           std::bitset<576> &destination_bitset );

std::string utf32_to_utf8( std::u32string_view str );
std::u32string utf8_to_utf32( std::string_view str );



std::vector<std::string> utf8_display_split( const std::string & );
void utf8_display_split_into( const std::string &, std::vector<std::string_view> & );

























class utf8_wrapper
{
    public:
        utf8_wrapper() : _length( 0 ), _display_width( 0 ) { }
        explicit utf8_wrapper( const std::string &d );
        explicit utf8_wrapper( const char *d );

        void insert( size_t start, const utf8_wrapper &other );
        utf8_wrapper substr( size_t start, size_t length ) const;
        utf8_wrapper substr( size_t start ) const {
            return substr( start, _length - start );
        }
        void erase( size_t start, size_t length );
        void erase( size_t start ) {
            erase( start, _length - start );
        }
        void append( const utf8_wrapper &other );
        utf8_wrapper &replace_all( const utf8_wrapper &search, const utf8_wrapper &replace );














        utf8_wrapper substr_display( size_t start, size_t length ) const;
        utf8_wrapper substr_display( size_t start ) const {
            return substr_display( start, _length - start );
        }

        utf8_wrapper &operator=( const std::string &d ) {
            *this = utf8_wrapper( d );
            return *this;
        }
        const std::string &str() const {
            return _data;
        }


        uint32_t at( size_t start ) const;


        size_t size() const {
            return _length;
        }
        size_t length() const {
            return size();
        }
        bool empty() const {
            return size() == 0;
        }


        size_t display_width() const {
            return _display_width;
        }
        const char *c_str() const {
            return _data.c_str();
        }






        std::string shorten( size_t maxlength ) const;
    protected:
        std::string _data;
        size_t _length;
        size_t _display_width;



        size_t byte_start( size_t bstart, size_t start ) const;




        size_t byte_start_display( size_t bstart, size_t start ) const;

        utf8_wrapper substr_byte( size_t bytestart, size_t length, bool use_display_width ) const;
        void init_utf8_wrapper();
};









class utf8_view
{
    private:
        const char *buffer;
        std::size_t length;

        class iterator
        {
            public:
                using iterator_category = std::input_iterator_tag;
                using difference_type = std::ptrdiff_t;
                using value_type = char32_t;
                using pointer = value_type*;
                using reference = value_type&;
            private:
                const char *ptr;
                const char *next_ptr;
                int remaining;
                int next_remaining;
                char32_t unicode;

                void decode() {
                    if( remaining > 0 ) {
                        unicode = UTF8_getch( &next_ptr, &next_remaining );
                    } else {
                        next_ptr = nullptr;
                        next_remaining = 0;
                        unicode = 0;
                    }
                }
            public:
                explicit iterator( const char *ptr, int remaining ) : ptr( ptr ), remaining( remaining ) {
                    next_ptr = ptr;
                    next_remaining = remaining;
                    decode();
                }
                bool operator != ( const iterator &rhs ) const noexcept {
                    return this->ptr != rhs.ptr;
                }
                const iterator &operator++() noexcept {
                    ptr = next_ptr;
                    remaining = next_remaining;
                    decode();
                    return *this;
                }
                char32_t operator*() const noexcept {
                    return unicode;
                }
        };

    public:
        explicit utf8_view( const std::string &str ) : buffer( str.c_str() ), length( str.length() ) {}

        iterator begin() const noexcept {
            return iterator( buffer, length );
        }

        iterator end() const noexcept {
            return iterator( buffer + length, 0 );
        }
};

#endif
